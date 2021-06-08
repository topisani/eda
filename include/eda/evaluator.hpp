#pragma once

#include <numeric>

#include "eda/block.hpp"
#include "eda/frame.hpp"

namespace eda {

  // EVALUATOR /////////////////////////////////////////

  template<AnyBlock T>
  struct evaluator;

  template<typename T>
  struct block_for;

  template<typename T>
  struct block_for<evaluator<T>> {
    using type = T;
  };

  template<typename T>
  using block_for_t = typename block_for<T>::type;

  template<AnyBlock Block>
  constexpr auto eval(Block block, Frame<Block::in_channels> in)
  {
    return make_evaluator(block).eval(in);
  }
  
  // COMPOSITION EVALUATOR /////////////////////////////

  template<AnyBlock T>
  struct EvaluatorBase {};

  namespace detail {
    template<typename T>
    struct add_evaluator {};

    template<typename... Ts>
    struct add_evaluator<std::tuple<Ts...>> {
      using type = std::tuple<evaluator<Ts>...>;
    };

    template<typename T>
    using add_evaluator_t = typename add_evaluator<T>::type;
  } // namespace detail

  template<AComposition T>
  struct EvaluatorBase<T> {
    constexpr EvaluatorBase(const T& t) : operands(t.operands) {}
    detail::add_evaluator_t<operands_t<T>> operands;
  };

  template<typename T>
  concept AnEvaluator = 
    std::derived_from<T, EvaluatorBase<block_for_t<T>>>
    && std::is_constructible_v<T, block_for_t<T> const&>
    && requires (T t, Frame<ins<block_for_t<T>>> in) {
      { t.eval(in) } -> std::convertible_to<Frame<outs<block_for_t<T>>>>;
    };

  template<AnyBlockRef T>
  constexpr auto make_evaluator(T&& b)
  requires AnEvaluator<evaluator<std::remove_cvref_t<T>>>
  {
    return evaluator<std::remove_cvref_t<T>>(b);
  }

  // DYN EVALUATOR ///////////////////////////////////// $\label{code:dyn_eval}$

  template<std::size_t Ins, std::size_t Outs>
  struct DynEvaluator {
    DynEvaluator() = default;
  
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
  struct evaluator<Partial<Block, Inputs...>> : EvaluatorBase<Partial<Block, Inputs...>> {
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
        auto arg_res = arg_block.eval(slice<0, arg_ins>(in));
        return concat(arg_res, eval_impl<Idx + 1>(slice<arg_ins, -1>(in)));
      }
    }

    evaluator<Block> block_;
    std::tuple<evaluator<Inputs>...> inputs_;
  };

  // IDENT /////////////////////////////////////////////

  template<std::size_t N>
  struct evaluator<Ident<N>> : EvaluatorBase<Ident<N>> {
    constexpr evaluator(Ident<N>){};
    static Frame<N> eval(Frame<N> in)
    {
      return in;
    }
  };

  // CUT ///////////////////////////////////////////////

  template<std::size_t N>
  struct evaluator<Cut<N>> : EvaluatorBase<Cut<N>> {
    constexpr evaluator(Cut<N>) {}
    static Frame<0> eval(Frame<N>)
    {
      return {};
    }
  };

  // SEQUENTIAL ////////////////////////////////////////

  template<AnyBlock Lhs, AnyBlock Rhs>
  struct evaluator<Sequential<Lhs, Rhs>> : EvaluatorBase<Sequential<Lhs, Rhs>> {
    constexpr evaluator(const Sequential<Lhs, Rhs>& block) : EvaluatorBase<Sequential<Lhs, Rhs>>(block) {}

    constexpr Frame<outs<Sequential<Lhs, Rhs>>> eval(Frame<ins<Sequential<Lhs, Rhs>>> in)
    {
      auto l = std::get<0>(this->operands).eval(in);
      return std::get<1>(this->operands).eval(l);
    }
  };

  // PARALLEL ////////////////////////////////////////// $\label{code:comp_eval}$

  template<AnyBlock Lhs, AnyBlock Rhs>
  struct evaluator<Parallel<Lhs, Rhs>> : EvaluatorBase<Parallel<Lhs, Rhs>> {
    constexpr evaluator(const Parallel<Lhs, Rhs>& block) : EvaluatorBase<Parallel<Lhs, Rhs>>(block) {}

    constexpr Frame<outs<Parallel<Lhs, Rhs>>> eval(Frame<ins<Parallel<Lhs, Rhs>>> in)
    {
      auto l = std::get<0>(this->operands).eval(slice<0, ins<Lhs>>(in));
      auto r = std::get<1>(this->operands).eval(slice<ins<Lhs>, -1>(in));
      return concat(l, r);
    }
  };

  // RECURSIVE /////////////////////////////////////////

  template<AnyBlock Lhs, AnyBlock Rhs>
  struct evaluator<Recursive<Lhs, Rhs>> : EvaluatorBase<Recursive<Lhs, Rhs>> {
    constexpr evaluator(const Recursive<Lhs, Rhs>& block) : EvaluatorBase<Recursive<Lhs, Rhs>>(block) {}
    constexpr Frame<outs<Recursive<Lhs, Rhs>>> eval(Frame<ins<Recursive<Lhs, Rhs>>> in)
    {
      auto l_out = std::get<0>(this->operands).eval(concat(memory_, in));
      memory_ = std::get<1>(this->operands).eval(slice<0, ins<Rhs>>(l_out));
      return l_out;
    }

  private:
    Frame<outs<Rhs>> memory_;
  };

  // Split /////////////////////////////////////////////

  template<AnyBlock Lhs, AnyBlock Rhs>
  struct evaluator<Split<Lhs, Rhs>> : EvaluatorBase<Split<Lhs, Rhs>> {
    constexpr evaluator(const Split<Lhs, Rhs>& block) : EvaluatorBase<Split<Lhs, Rhs>>(block) {}

    constexpr Frame<outs<Split<Lhs, Rhs>>> eval(Frame<ins<Split<Lhs, Rhs>>> in)
    {
      auto l = std::get<0>(this->operands).eval(in);
      Frame<ins<Rhs>> rhs_in;
      for (std::size_t i = 0; i < rhs_in.channels(); i++) {
        rhs_in[i] = l[i % l.channels()];
      }
      return std::get<1>(this->operands).eval(rhs_in);
    }
  };

  // MERGE /////////////////////////////////////////////

  template<AnyBlock Lhs, AnyBlock Rhs>
  struct evaluator<Merge<Lhs, Rhs>> : EvaluatorBase<Merge<Lhs, Rhs>> {
    constexpr evaluator(const Merge<Lhs, Rhs>& block) : EvaluatorBase<Merge<Lhs, Rhs>>(block) {}

    constexpr Frame<outs<Merge<Lhs, Rhs>>> eval(Frame<ins<Merge<Lhs, Rhs>>> in)
    {
      auto lhs_out = std::get<0>(this->operands).eval(in);
      Frame<ins<Rhs>> rhs_in;
      for (std::size_t i = 0; i < lhs_out.channels(); i++) {
        rhs_in[i % rhs_in.channels()] += lhs_out[i];
      }
      return std::get<1>(this->operands).eval(rhs_in);
    }
  };

  // ARITHMETIC ////////////////////////////////////////

  template<>
  struct evaluator<Plus> : EvaluatorBase<Plus> {
    constexpr evaluator(Plus){};
    constexpr static Frame<1> eval(Frame<2> in)
    {
      return in[0] + in[1];
    }
  };

  template<>
  struct evaluator<Minus> : EvaluatorBase<Minus> {
    constexpr evaluator(Minus){};
    constexpr static Frame<1> eval(Frame<2> in)
    {
      return in[0] - in[1];
    }
  };

  template<>
  struct evaluator<Times> : EvaluatorBase<Times> {
    constexpr evaluator(Times){};
    constexpr static Frame<1> eval(Frame<2> in)
    {
      return in[0] * in[1];
    }
  };
  template<>
  struct evaluator<Divide> : EvaluatorBase<Divide> {
    constexpr evaluator(Divide){};
    constexpr static Frame<1> eval(Frame<2> in)
    {
      return in[0] / in[1];
    }
  };

  // MEM ///////////////////////////////////////////////

  template<>
  struct evaluator<Mem<1>> : EvaluatorBase<Mem<1>> {
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
  struct evaluator<Mem<0>> : EvaluatorBase<Mem<0>> {
    constexpr evaluator(const Mem<0>&) {}
    constexpr Frame<1> eval(Frame<1> in)
    {
      return in;
    }
  };

  template<std::size_t Samples>
  struct evaluator<Mem<Samples>> : EvaluatorBase<Mem<Samples>> {
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
  struct evaluator<Delay> : EvaluatorBase<Delay> {
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
      ++index_;
      index_ %= static_cast<std::ptrdiff_t>(memory_.size());
      return res;
    }

  private:
    std::vector<float> memory_;
    std::ptrdiff_t index_ = 0;
  };

  // REF ///////////////////////////////////////////////

  template<>
  struct evaluator<Ref> : EvaluatorBase<Ref> {
    constexpr evaluator(const Ref& r) noexcept : ref_(r) {}
    [[nodiscard]] Frame<1> eval(Frame<0>) const
    {
      return *ref_.ptr;
    }

  private:
    Ref ref_;
  };

  // FUNCTION ////////////////////////////////////////// $\label{code:extra_eval}$

  /// Adapt a function to a block
  template<std::size_t In, std::size_t Out, typename F>
  struct evaluator<FunBlock<In, Out, F>> : EvaluatorBase<FunBlock<In, Out, F>> {
    constexpr evaluator(const FunBlock<In, Out, F>& f) noexcept : fb_(f) {}
    [[nodiscard]] Frame<Out> eval(Frame<In> in) const
    {
      return fb_.func_(in);
    }

  private:
    FunBlock<In, Out, F> fb_;
  };

  // STATEFUL_FUNC /////////////////////////////////////

  template<std::size_t In, std::size_t Out, typename Func, typename... States>
  struct evaluator<StatefulFunc<In, Out, Func, States...>> : EvaluatorBase<StatefulFunc<In, Out, Func, States...>> {
    constexpr evaluator(const StatefulFunc<In, Out, Func, States...>& f) noexcept : func_(f.func), states_(f.states) {}

    Frame<Out> eval(Frame<In> in)
    {
      return std::apply([&](States&... s) { return func_(in, s...); }, states_);
    }

  private:
    Func func_;
    std::tuple<States...> states_;
  };

  // FIR ///////////////////////////////////////////////

  template<std::size_t N>
  struct evaluator<FIRFilter<N>> : EvaluatorBase<FIRFilter<N>> {
    constexpr evaluator(const FIRFilter<N>& fir) noexcept
    {
      std::ranges::copy(fir.kernel, kernel.begin());
      std::ranges::copy(fir.kernel, kernel.begin() + N);
    }

    constexpr Frame<1> eval(Frame<1> in)
    {
      if (t == N) t = 0;
      t++;
      z[N - t] = in;
      // By repeating the kernel, there is always a contiguous range to
      // run the inner product on
      auto start = kernel.begin() + t;
      return std::inner_product(start, start + N, z.begin(), 0.f);
    }

  private:
    std::size_t t = 0;
    std::array<float, N> z = {0};
    /// Kernel is repeated
    std::array<float, 2 * N> kernel;
  };

} // namespace eda
