#pragma once

#include <algorithm>
#include <cmath>
#include <ranges>
#include <span>

#include "eda/frame.hpp"
#include "eda/internal/util.hpp"

namespace eda {

  // BLOCK /////////////////////////////////////////////

  /// CRTP Base class of all blocks.
  ///
  /// Provides `in_channels` and `out_channels` constants, as well as
  /// the call operator for currying.
  template<typename Derived, std::size_t InChannels, std::size_t OutChannels>
  struct BlockBase {
    static constexpr std::size_t in_channels = InChannels;
    static constexpr std::size_t out_channels = OutChannels;

    constexpr auto operator()(auto&&... inputs) const noexcept //
      requires(sizeof...(inputs) <= InChannels);
  };

  /// The basic concept of a block
  template<typename T>
  concept AnyBlock = std::is_base_of_v<BlockBase<T, T::in_channels, T::out_channels>, T> && std::copyable<T>;

  /// The basic concept of a block
  template<typename T, std::size_t I, std::size_t O>
  concept ABlock = AnyBlock<T> &&(T::in_channels == I) && (T::out_channels == O);

  /// A type that may be a reference to `AnyBlock`
  template<typename T>
  concept AnyBlockRef = AnyBlock<std::remove_cvref_t<T>>;

  /// Number of input channels for block
  template<AnyBlockRef T>
  constexpr auto ins = std::remove_cvref_t<T>::in_channels;

  /// Number of output channels for block
  template<AnyBlockRef T>
  constexpr auto outs = std::remove_cvref_t<T>::out_channels;

  /// Helper class for modeling Binary operators
  template<typename D, AnyBlock Lhs, AnyBlock Rhs, std::size_t In, std::size_t Out>
  struct BinOp : BlockBase<D, In, Out> {
    constexpr BinOp(Lhs l, Rhs r) noexcept : lhs(l), rhs(r) {}
    Lhs lhs;
    Rhs rhs;
  };

  // LITERAL ///////////////////////////////////////////

  /// A block that models a literal/constant value
  struct Literal : BlockBase<Literal, 0, 1> {
    float value;
  };

  /// Wrap a value in `Literal` if it is not already a block
  constexpr decltype(auto) as_block(AnyBlockRef auto&& input) noexcept
  {
    return FWD(input);
  }

  /// Wrap a value in `Literal` if it is not already a block
  constexpr Literal as_block(float f) noexcept
  {
    return Literal{{}, f};
  }

  /// The result of calling `as_block`
  template<typename T>
  using as_block_t = std::remove_cvref_t<decltype(as_block(std::declval<T>()))>;

  // REF ///////////////////////////////////////////////

  struct Ref : BlockBase<Ref, 0, 1> {
    float* ptr = nullptr;
  };

  constexpr Ref ref(float& f) noexcept
  {
    return Ref{{}, &f};
  }

  /// Convert a pointer to float to a ref block
  constexpr Ref as_block(float* fp) noexcept
  {
    return ref(*fp);
  }

  // CURRYING //////////////////////////////////////////

  /// Block used to capture parameters when calling blocks as functions
  template<AnyBlock Block, AnyBlock... Inputs>
  requires((outs<Inputs> <= ins<Block>) &&...) //
    struct Partial
    : BlockBase<Partial<Block, Inputs...>, ins<Block> + (ins<Inputs> + ...) - (outs<Inputs> + ...), outs<Block>> {
    constexpr Partial(Block b, Inputs... input) noexcept : block(b), inputs(input...) {}

    Block block;
    std::tuple<Inputs...> inputs;
  };

  /// Call operator of BlockBase. Pass parameters as input signals
  ///
  /// Allows currying/partial application of parameters. Parameters will be
  /// sent as inputs from left to right.
  template<typename D, std::size_t I, std::size_t O>
  constexpr auto BlockBase<D, I, O>::operator()(auto&&... inputs) const noexcept //
    requires(sizeof...(inputs) <= I)
  {
    return Partial<D, as_block_t<decltype(inputs)>...>(static_cast<const D&>(*this), as_block(FWD(inputs))...);
  }

  // IDENT /////////////////////////////////////////////

  /// Identity block. Returns its input unchanged
  template<std::size_t N = 1>
  struct Ident : BlockBase<Ident<N>, N, N> {};
  /// The identity block.
  template<std::size_t N = 1>
  constexpr Ident<N> ident;

  // CUT ///////////////////////////////////////////////

  /// Cut block. Ends a signal path and discards its input
  template<std::size_t N = 1>
  struct Cut : BlockBase<Cut<N>, N, 0> {};
  /// The cut block.
  template<std::size_t N = 1>
  constexpr Cut<N> cut;

  // PARALLEL //////////////////////////////////////////

  /// Parallel composition block.
  ///
  /// Given input `(x0, x1)` and blocks `l, r`, outputs `(l(x0), r(x1))`
  template<AnyBlock Lhs, AnyBlock Rhs>
  struct Parallel : BinOp<Parallel<Lhs, Rhs>, Lhs, Rhs, ins<Lhs> + ins<Rhs>, outs<Lhs> + outs<Rhs>> {};

  /// The parallel composition of two blocks.
  template<AnyBlockRef Lhs, AnyBlockRef Rhs>
  constexpr auto par(Lhs&& lhs, Rhs&& rhs) noexcept
  {
    return Parallel<std::remove_cvref_t<Lhs>, std::remove_cvref_t<Rhs>>{{FWD(lhs), FWD(rhs)}};
  }
  
  template<AnyBlockRef A, AnyBlockRef B, AnyBlockRef... Blocks>
  constexpr auto par(A&& a, B&& b, Blocks&&... blocks) noexcept
  {
    return par(a, par(b, blocks...));
  }

  // SEQUENTIAL ////////////////////////////////////////

  /// Sequential composition block.
  ///
  /// Given input `x0` and blocks `l, r`, outputs `r(l(x0))`
  template<AnyBlock Lhs, AnyBlock Rhs>
  requires(outs<Lhs> == ins<Rhs>) //
    struct Sequential : BinOp<Sequential<Lhs, Rhs>, Lhs, Rhs, ins<Lhs>, outs<Rhs>> {};

  /// The sequential composition of two blocks.
  template<AnyBlockRef Lhs, AnyBlockRef Rhs>
  constexpr auto seq(Lhs&& lhs, Rhs&& rhs) noexcept
  {
    return Sequential<std::remove_cvref_t<Lhs>, std::remove_cvref_t<Rhs>>{{FWD(lhs), FWD(rhs)}};
  }

  template<AnyBlockRef A, AnyBlockRef B, AnyBlockRef... Blocks>
  constexpr auto seq(A&& a, B&& b, Blocks&&... blocks) noexcept
  {
    return seq(a, seq(b, blocks...));
  }

  // REPEAT ////////////////////////////////////////////

  /// Apply the binary operator `composition` to `block` repeated `N` times
  template<std::size_t N>
  constexpr auto repeat(AnyBlock auto const& block, auto&& composition) requires requires
  {
    composition(block, block);
  }
  {
    if constexpr (N == 0) {
      return ident<0>;
    } else if constexpr (N == 1) {
      return block;
    } else {
      return composition(block, repeat<N - 1>(block, composition));
    }
  }

  /// Repeat `block` `N` times through the sequential operator
  template<std::size_t N>
  constexpr auto repeat_seq(AnyBlock auto const& block)
  {
    return repeat<N>(block, [](auto&& a, auto&& b) { return seq(a, b); });
  }

  /// Repeat `block` `N` times through the parallel operator
  template<std::size_t N>
  constexpr auto repeat_par(AnyBlock auto const& block)
  {
    return repeat<N>(block, [](auto&& a, auto&& b) { return par(a, b); });
  }

  // RECURSIVE /////////////////////////////////////////

  /// Recursive composition block.
  ///
  /// Given input `x` and blocks `l, r`, outputs `l(r(x'), x)`, where `x'` is the output
  /// of the previous evaluation.
  template<AnyBlock Lhs, AnyBlock Rhs>
  requires(ins<Rhs> <= outs<Lhs>) && (outs<Rhs> <= ins<Lhs>) //
    struct Recursive : BinOp<Recursive<Lhs, Rhs>, Lhs, Rhs, ins<Lhs> - outs<Rhs>, outs<Lhs>> {};

  /// The recursive composition of two blocks
  template<AnyBlockRef Lhs, AnyBlockRef Rhs>
  constexpr auto rec(Lhs&& lhs, Rhs&& rhs) noexcept
  {
    return Recursive<std::remove_cvref_t<Lhs>, std::remove_cvref_t<Rhs>>{{FWD(lhs), FWD(rhs)}};
  }

  // Split /////////////////////////////////////////////

  /// Split composition block.
  ///
  /// Given two blocks `l, r`, applies `l`, and passes the output to `r`, repeating the signals
  /// to fit the arity of `r`.
  template<AnyBlock Lhs, AnyBlock Rhs>
  requires(ins<Rhs> % outs<Lhs> == 0) //
    struct Split : BinOp<Split<Lhs, Rhs>, Lhs, Rhs, ins<Lhs>, outs<Rhs>> {};

  /// The split composition of two blocks.
  template<AnyBlockRef Lhs, AnyBlockRef Rhs>
  constexpr auto split(Lhs&& lhs, Rhs&& rhs) noexcept
  {
    return Split<std::remove_cvref_t<Lhs>, std::remove_cvref_t<Rhs>>{{FWD(lhs), FWD(rhs)}};
  }

  // MERGE /////////////////////////////////////////////

  /// Merge composition block.
  ///
  /// Given two blocks `l, r`, applies `l`, and passes the output to `r`, such that each
  /// input to `r` is the sum of all `l(x)[i mod ins<r>]`.
  template<AnyBlock Lhs, AnyBlock Rhs>
  requires(outs<Lhs> % ins<Rhs> == 0) //
    struct Merge : BinOp<Merge<Lhs, Rhs>, Lhs, Rhs, ins<Lhs>, outs<Rhs>> {};

  /// The merge composition of two blocks.
  template<AnyBlockRef Lhs, AnyBlockRef Rhs>
  constexpr auto merge(Lhs&& lhs, Rhs&& rhs) noexcept
  {
    return Merge<std::remove_cvref_t<Lhs>, std::remove_cvref_t<Rhs>>{{FWD(lhs), FWD(rhs)}};
  }

  // ARITHMETIC ////////////////////////////////////////

  /// A simple block that calculates the sum of its two inputs
  struct Plus : BlockBase<Plus, 2, 1> {};
  constexpr Plus plus;
  /// A simple block that calculates the difference between its two inputs
  struct Minus : BlockBase<Minus, 2, 1> {};
  constexpr Minus minus;
  /// A simple block that calculates the multiple of its two inputs
  struct Times : BlockBase<Times, 2, 1> {};
  constexpr Times times;
  /// A simple block that calculates the ratio between its two inputs
  struct Divide : BlockBase<Divide, 2, 1> {};
  constexpr Divide divide;

  // MEM ///////////////////////////////////////////////

  /// Fixed size memory block.
  ///
  /// Outputs its input delayed by n samples
  template<std::size_t Samples = 1>
  struct Mem : BlockBase<Mem<Samples>, 1, 1> {};

  template<std::size_t Samples = 1>
  constexpr Mem<Samples> mem;

  /// Delay output of block by 1 sample
  template<AnyBlock Block>
  auto operator~(Block const& b)
  {
    return seq(b, repeat_par<outs<Block>>(mem<1>));
  }

  // DELAY /////////////////////////////////////////////

  /// Variable size memory block.
  ///
  /// Given input signals `(d, x)`, outputs `x` delayed by `d` samples
  struct Delay : BlockBase<Delay, 2, 1> {};
  constexpr Delay delay;

  // FUNCTION //////////////////////////////////////////

  /// Adapt a function to a block
  template<std::size_t In, std::size_t Out, util::Callable<Frame<Out>(Frame<In>)> F>
  requires std::copyable<F>
  struct FunBlock : BlockBase<FunBlock<In, Out, F>, In, Out> {
    F func_;
  };

  template<std::size_t In, std::size_t Out>
  constexpr auto fun(util::Callable<Frame<Out>(Frame<In>)> auto&& f)
  {
    return FunBlock<In, Out, std::decay_t<decltype(f)>>{.func_ = f};
  }

  constexpr auto sin = fun<1, 1>(&::sinf);
  constexpr auto cos = fun<1, 1>(&::cosf);
  constexpr auto tan = fun<1, 1>(&::tanf);
  constexpr auto tanh = fun<1, 1>(&::tanhf);
  constexpr auto mod = fun<2, 1>([](auto in) { return std::fmod(in[0], in[1]); });

  // STATEFUL_FUNC /////////////////////////////////////

  template<std::size_t In, std::size_t Out, typename Func, typename... States>
  requires(util::Callable<Func, Frame<Out>(Frame<In>, States&...)>&& std::copyable<Func>) &&
    (std::copyable<States> && ...) //
    struct StatefulFunc : BlockBase<StatefulFunc<In, Out, Func, States...>, In, Out> {
    Func func;
    std::tuple<States...> states;
  };

  template<std::size_t In, std::size_t Out, typename... States>
  constexpr auto fun(util::Callable<Frame<Out>(Frame<In>, std::remove_cvref_t<States>&...)> auto&& f,
                     States&&... states)
  {
    return StatefulFunc<In, Out, std::decay_t<decltype(f)>, std::remove_cvref_t<States>...>{.func = f,
                                                                                            .states = {FWD(states)...}};
  }

  // FIR ///////////////////////////////////////////////

  template<std::size_t N>
  struct FIRFilter : BlockBase<FIRFilter<N>, 1, 1> {
    std::array<float, N> kernel;
  };

  template<std::size_t N>
  constexpr auto fir(std::array<float, N> kernel) noexcept
  {
    return FIRFilter<N>{.kernel = kernel};
  }

} // namespace eda
