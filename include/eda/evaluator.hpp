#pragma once

#include "eda/block.hpp"
#include "eda/frame.hpp"

namespace eda {

  // EVALUATOR /////////////////////////////////////////

  template<typename T>
  struct evaluator;

  template<typename T>
  struct block_for;

  template<typename T>
  struct block_for<evaluator<T>> {
    using type = T;
  };

  template<typename T>
  using block_for_t = typename block_for<T>::type;

  constexpr auto make_evaluator(AnyBlockRef auto&& b)
  {
    return evaluator<std::decay_t<decltype(b)>>(b);
  }

  template<AnyBlock Block>
  constexpr auto eval(Block block, Frame<Block::in_channels> in)
  {
    return make_evaluator(block).eval(in);
  }

  template<std::size_t Ins, std::size_t Outs>
  struct DynEvaluator {
    template<ABlock<Ins, Outs> Block>
    DynEvaluator(evaluator<Block> evaluator) : func_([evaluator](Frame<Ins> in) mutable { return evaluator.eval(in); })
    {}

    Frame<Outs> eval(Frame<Ins> in)
    {
      return func_(in);
    }

    Frame<Outs> operator()(Frame<Ins> in)
    {
      return func_(in);
    }

  private:
    std::function<Frame<Outs>(Frame<Ins>)> func_;
  };

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

  template<AnyBlock Block, AnyBlock... Inputs>
  struct evaluator<Partial<Block, Inputs...>> {
    constexpr evaluator(const Partial<Block, Inputs...>& block) : block_(block.block), inputs_(block.inputs) {}

    constexpr Frame<outs<Partial<Block, Inputs...>>> eval(Frame<ins<Partial<Block, Inputs...>>> in)
    {
      return block_.eval(eval_impl<>(in));
    }

  private:
    template<std::size_t Idx = 0>
    auto eval_impl(auto in)
    {
      if constexpr (Idx == sizeof...(Inputs)) {
        return in;
      } else {
        auto& arg_block = std::get<Idx>(inputs_);
        constexpr auto arg_ins = ins<block_for_t<std::remove_cvref_t<decltype(arg_block)>>>;
        auto arg_res = arg_block.eval(splice<0, arg_ins>(in));
        return concat(arg_res, eval_impl<Idx + 1>(splice<arg_ins, -1>(in)));
      }
    }

    evaluator<Block> block_;
    std::tuple<evaluator<Inputs>...> inputs_;
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

  template<AnyBlock Lhs, AnyBlock Rhs>
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

  template<AnyBlock Lhs, AnyBlock Rhs>
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

  template<AnyBlock Lhs, AnyBlock Rhs>
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

  template<AnyBlock Lhs, AnyBlock Rhs>
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

  template<AnyBlock Lhs, AnyBlock Rhs>
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
    constexpr evaluator(Plus){};
    constexpr static Frame<1> eval(Frame<2> in)
    {
      return in[0] + in[1];
    }
  };

  template<>
  struct evaluator<Minus> {
    constexpr evaluator(Minus){};
    constexpr static Frame<1> eval(Frame<2> in)
    {
      return in[0] - in[1];
    }
  };

  template<>
  struct evaluator<Times> {
    constexpr evaluator(Times){};
    constexpr static Frame<1> eval(Frame<2> in)
    {
      return in[0] * in[1];
    }
  };
  template<>
  struct evaluator<Divide> {
    constexpr evaluator(Divide){};
    constexpr static Frame<1> eval(Frame<2> in)
    {
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

  // REF ///////////////////////////////////////////////

  template<>
  struct evaluator<Ref> {
    evaluator(const Ref& r) noexcept : ref_(r) {}
    [[nodiscard]] Frame<1> eval(Frame<0>) const
    {
      return *ref_.ptr;
    }

  private:
    Ref ref_;
  };

} // namespace eda
