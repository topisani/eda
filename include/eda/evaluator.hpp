#pragma once

#include "eda/frame.hpp"
#include "eda/block.hpp"

namespace topisani::eda {

  // EVALUATOR /////////////////////////////////////////

  template<typename T>
  struct evaluator;

  constexpr auto make_evaluator(ABlockRef auto&& b)
  {
    return evaluator<std::decay_t<decltype(b)>>(b);
  }

  template<ABlock Block>
  constexpr auto eval(Block block, Frame<Block::in_channels> in)
  {
    return make_evaluator(block).eval(in);
  }

  // EVALUATOR IMPLEMENTATIONS /////////////////////////

  // LITERAL ///////////////////////////////////////////

  template<>
  struct evaluator<Literal> {
    constexpr evaluator(Literal l) : literal_(l) {}

    constexpr Frame<1> eval(Frame<0>)
    {
      return {{literal_.value}};
    }

    Literal literal_;
  };

  // CURRYING //////////////////////////////////////////

  template<ABlock Block, ABlock Input>
  struct evaluator<Partial<Block, Input>> {
    constexpr evaluator(const Partial<Block, Input>& block) : block_(block.block), input_(block.input) {}

    constexpr Frame<outs<Partial<Block, Input>>> eval(Frame<ins<Partial<Block, Input>>> in)
    {
      return block_.eval(concat(input_.eval(splice<0, ins<Input>>(in)), splice<ins<Input>, -1>(in)));
    }

  private:
    evaluator<Block> block_;
    evaluator<Input> input_;
  };

  // IDENT /////////////////////////////////////////////

  template<>
  struct evaluator<Ident> {
    constexpr evaluator(Ident){};
    static Frame<1> eval(Frame<1> in)
    {
      return in;
    }
  };

  // CUT ///////////////////////////////////////////////

  template<>
  struct evaluator<Cut> {
    constexpr evaluator(Cut) {}
    static Frame<0> eval(Frame<1>)
    {
      return {};
    }
  };

  // PARALLEL //////////////////////////////////////////

  template<ABlock Lhs, ABlock Rhs>
  struct evaluator<Parallel<Lhs, Rhs>> {
    constexpr evaluator(const Parallel<Lhs, Rhs>& block) : lhs_(block.lhs), rhs_(block.rhs) {}

    constexpr Frame<outs<Parallel<Lhs, Rhs>>> eval(Frame<ins<Parallel<Lhs, Rhs>>> in)
    {
      auto l = lhs_.eval(splice<0, ins<Lhs>>(in));
      auto r = rhs_.eval(splice<ins<Lhs>, -1>(in));
      return concat(l, r);
    }

  private:
    evaluator<Lhs> lhs_;
    evaluator<Rhs> rhs_;
  };

  // SERIAL ////////////////////////////////////////////

  template<ABlock Lhs, ABlock Rhs>
  struct evaluator<Sequential<Lhs, Rhs>> {
    constexpr evaluator(const Sequential<Lhs, Rhs>& block) : lhs_(block.lhs), rhs_(block.rhs) {}

    constexpr Frame<outs<Sequential<Lhs, Rhs>>> eval(Frame<ins<Sequential<Lhs, Rhs>>> in)
    {
      auto l = lhs_.eval(in);
      return rhs_.eval(l);
    }

  private:
    evaluator<Lhs> lhs_;
    evaluator<Rhs> rhs_;
  };

  // RECURSIVE /////////////////////////////////////////

  template<ABlock Lhs, ABlock Rhs>
  struct evaluator<Recursive<Lhs, Rhs>> {
    constexpr evaluator(const Recursive<Lhs, Rhs>& block) : lhs_(block.lhs), rhs_(block.rhs) {}
    constexpr Frame<outs<Recursive<Lhs, Rhs>>> eval(Frame<ins<Recursive<Lhs, Rhs>>> in)
    {
      auto l_out = lhs_.eval(concat(memory_, in));
      memory_ = rhs_.eval(splice<0, ins<Rhs>>(l_out));
      return l_out;
    }

  private:
    Frame<outs<Rhs>> memory_;
    evaluator<Lhs> lhs_;
    evaluator<Rhs> rhs_;
  };

  // Split /////////////////////////////////////////////

  template<ABlock Lhs, ABlock Rhs>
  struct evaluator<Split<Lhs, Rhs>> {
    constexpr evaluator(const Split<Lhs, Rhs>& block) : lhs_(block.lhs), rhs_(block.rhs) {}

    constexpr Frame<outs<Split<Lhs, Rhs>>> eval(Frame<ins<Split<Lhs, Rhs>>> in)
    {
      auto l = lhs_.eval(in);
      Frame<ins<Rhs>> rhs_in;
      for (std::size_t i = 0; i < rhs_in.channels(); i++) {
        rhs_in[i] = l[i % l.channels()];
      }
      return rhs_.eval(rhs_in);
    }

  private:
    evaluator<Lhs> lhs_;
    evaluator<Rhs> rhs_;
  };

  // MERGE /////////////////////////////////////////////

  template<ABlock Lhs, ABlock Rhs>
  struct evaluator<Merge<Lhs, Rhs>> {
    constexpr evaluator(const Merge<Lhs, Rhs>& block) : lhs_(block.lhs), rhs_(block.rhs) {}

    constexpr Frame<outs<Merge<Lhs, Rhs>>> eval(Frame<ins<Merge<Lhs, Rhs>>> in)
    {
      auto lhs_out = lhs_.eval(in);
      Frame<ins<Rhs>> rhs_in;
      for (std::size_t i = 0; i < lhs_out.channels(); i++) {
        rhs_in[i % rhs_in.channels()] += lhs_out[i];
      }
      return rhs_.eval(rhs_in);
    }

  private:
    evaluator<Lhs> lhs_;
    evaluator<Rhs> rhs_;
  };

  // ARITHMETIC ////////////////////////////////////////

  template<>
  struct evaluator<Plus> {
    constexpr evaluator(Plus) {};
    constexpr static Frame<1> eval(Frame<2> in) {
      return in[0] + in[1];
    }
  };

  template<>
  struct evaluator<Minus> {
    constexpr evaluator(Minus) {};
    constexpr static Frame<1> eval(Frame<2> in) {
      return in[0] - in[1];
    }
  };

  template<>
  struct evaluator<Times> {
    constexpr evaluator(Times) {};
    constexpr static Frame<1> eval(Frame<2> in) {
      return in[0] * in[1];
    }
  };
  template<>
  struct evaluator<Divide> {
    constexpr evaluator(Divide) {};
    constexpr static Frame<1> eval(Frame<2> in) {
      return in[0] / in[1];
    }
  };

  // MEM ///////////////////////////////////////////////

  template<>
  struct evaluator<Mem<1>> {
    constexpr evaluator(const Mem<1>&) {}
    constexpr Frame<1> eval(Frame<1> in)
    {
      auto res = memory_;
      memory_ = in;
      return res;
    }

    Frame<1> memory_;
  };

  template<>
  struct evaluator<Mem<0>> {
    constexpr evaluator(const Mem<0>&) {}
    constexpr Frame<1> eval(Frame<1> in)
    {
      return in;
    }
  };

  template<std::size_t Samples>
  struct evaluator<Mem<Samples>> {
    constexpr evaluator(const Mem<Samples>&) {}
    constexpr Frame<1> eval(Frame<1> in)
    {
      float res = memory_[index_];
      memory_[index_] = in;
      index_++;
      index_ %= Samples;
      return res;
    }

    std::array<float, Samples> memory_ = {};
    std::ptrdiff_t index_ = 0;
  };

  // DELAY /////////////////////////////////////////////

  /// Evaluator for variable sized delay.
  /// 
  /// Memory is implemented as a `std::vector`, and never shrinks
  template<>
  struct evaluator<Delay> {
    evaluator(const Delay&) {}
    Frame<outs<Delay>> eval(Frame<ins<Delay>> in)
    {
      auto delay = static_cast<int>(in[0]);
      if (auto old_size = memory_.size(); old_size < delay) {
        memory_.resize(delay);
        auto src = memory_.begin() + old_size - 1;
        auto dst = memory_.begin() + delay - 1;
        auto n = old_size - index_;
        for (int i = 0; i < n; i++, src--, dst--) {
          *dst = *src;
          *src = 0;
        }
      }
      float res = memory_[(memory_.size() + index_ - delay) % memory_.size()];
      memory_[index_] = in[1];
      index_++;
      index_ %= static_cast<std::ptrdiff_t>(memory_.size());
      return res;
    }

  private:
    std::vector<float> memory_;
    std::ptrdiff_t index_ = 0;
  };
} // namespace topisani::eda
