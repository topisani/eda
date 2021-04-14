#pragma once

#include <algorithm>
#include <ranges>
#include <span>

#include "eda/internal/util.hpp"

namespace topisani::eda {

  // BLOCK /////////////////////////////////////////////

  /// CRTP Base class of all blocks.
  ///
  /// Provides `in_channels` and `out_channels` constants, as well as
  /// the call operator for currying.
  template<typename Derived, std::size_t InChannels, std::size_t OutChannels>
  struct BlockBase {
    static constexpr std::size_t in_channels = InChannels;
    static constexpr std::size_t out_channels = OutChannels;

    constexpr auto operator()(auto&&... inputs) const //
      requires(sizeof...(inputs) <= InChannels);
  };

  /// The basic concept of a block
  template<typename T>
  concept ABlock = std::is_base_of_v<BlockBase<T, T::in_channels, T::out_channels>, T>;

  /// A type that may be a reference to `ABlock`
  template<typename T>
  concept ABlockRef = ABlock<std::remove_cvref_t<T>>;

  /// Number of input channels for block
  template<ABlockRef T>
  constexpr auto ins = std::remove_cvref_t<T>::in_channels;

  /// Number of output channels for block
  template<ABlockRef T>
  constexpr auto outs = std::remove_cvref_t<T>::out_channels;

  /// Helper class for modeling Binary operators
  template<typename D, ABlock Lhs, ABlock Rhs, std::size_t In, std::size_t Out>
  struct BinOp : BlockBase<D, In, Out> {
    constexpr BinOp(Lhs l, Rhs r) : lhs(l), rhs(r) {}
    Lhs lhs;
    Rhs rhs;
  };

  // LITERAL ///////////////////////////////////////////

  /// A block that models a literal/constant value
  struct Literal : BlockBase<Literal, 0, 1> {
    float value;
  };

  /// Wrap a value in `Literal` if it is not already a block
  constexpr decltype(auto) wrap_literal(ABlockRef auto&& input)
  {
    return FWD(input);
  }

  /// Wrap a value in `Literal` if it is not already a block
  constexpr Literal wrap_literal(float f)
  {
    return Literal{{}, f};
  }

  /// The result of calling `wrap_literal`
  template<typename T>
  using wrapped_t = std::remove_cvref_t<decltype(wrap_literal(std::declval<T>()))>;

  // CURRYING //////////////////////////////////////////

  /// Block used to capture parameters when calling blocks as functions
  template<ABlock Block, ABlock Input>
  requires(outs<Input> <= ins<Block>) //
    struct Partial : BlockBase<Partial<Block, Input>, ins<Input> + ins<Block> - outs<Input>, outs<Block>> {
    constexpr Partial(Block b, Input input) : block(b), input(input) {}

    Block block;
    Input input;
  };

  /// Call operator of BlockBase. Pass parameters as input signals
  ///
  /// Allows currying/partial application of parameters. Parameters will be
  /// sent as inputs from left to right.
  template<typename D, std::size_t I, std::size_t O>
  constexpr auto BlockBase<D, I, O>::operator()(auto&&... inputs) const //
    requires(sizeof...(inputs) <= I)
  {
    auto joined = parallel(wrap_literal(FWD(inputs))...);
    return Partial<D, decltype(joined)>(static_cast<const D&>(*this), std::move(joined));
  }

  // IDENT /////////////////////////////////////////////

  /// Identity block. Returns its input unchanged
  struct Ident : BlockBase<Ident, 1, 1> {};
  /// The identity block.
  constexpr Ident ident;

  // CUT ///////////////////////////////////////////////

  /// Cut block. Ends a signal path and discards its input
  struct Cut : BlockBase<Cut, 1, 0> {};
  /// The cut block.
  constexpr Cut cut;

  // PARALLEL //////////////////////////////////////////

  /// Parallel composition block.
  ///
  /// Given input `(x0, x1)` and blocks `l, r`, outputs `(l(x0), r(x1))`
  template<ABlock Lhs, ABlock Rhs>
  struct Parallel : BinOp<Parallel<Lhs, Rhs>, Lhs, Rhs, ins<Lhs> + ins<Rhs>, outs<Lhs> + outs<Rhs>> {};

  /// The parallel composition of two blocks.
  template<ABlockRef Lhs, ABlockRef Rhs>
  constexpr auto parallel(Lhs&& lhs, Rhs&& rhs)
  {
    return Parallel<std::remove_cvref_t<Lhs>, std::remove_cvref_t<Rhs>>{{FWD(lhs), FWD(rhs)}};
  }

  // SEQUENTIAL ////////////////////////////////////////

  /// Sequential composition block.
  ///
  /// Given input `x0` and blocks `l, r`, outputs `r(l(x0))`
  template<ABlock Lhs, ABlock Rhs>
  requires(outs<Lhs> == ins<Rhs>) //
    struct Sequential : BinOp<Sequential<Lhs, Rhs>, Lhs, Rhs, ins<Lhs>, outs<Rhs>> {};

  /// The sequential composition of two blocks.
  template<ABlockRef Lhs, ABlockRef Rhs>
  constexpr auto sequential(Lhs&& lhs, Rhs&& rhs)
  {
    return Sequential<std::remove_cvref_t<Lhs>, std::remove_cvref_t<Rhs>>{{FWD(lhs), FWD(rhs)}};
  }

  // RECURSIVE /////////////////////////////////////////

  /// Recursive composition block.
  ///
  /// Given input `x` and blocks `l, r`, outputs `l(r(x'), x)`, where `x'` is the output
  /// of the previous evaluation.
  template<ABlock Lhs, ABlock Rhs>
  requires(ins<Rhs> <= outs<Lhs>) && (outs<Rhs> <= ins<Lhs>) //
    struct Recursive : BinOp<Recursive<Lhs, Rhs>, Lhs, Rhs, ins<Lhs> - outs<Rhs>, outs<Lhs>> {};

  /// The recursive composition of two blocks
  template<ABlockRef Lhs, ABlockRef Rhs>
  constexpr auto recursive(Lhs&& lhs, Rhs&& rhs)
  {
    return Recursive<std::remove_cvref_t<Lhs>, std::remove_cvref_t<Rhs>>{{FWD(lhs), FWD(rhs)}};
  }

  // Split /////////////////////////////////////////////

  /// Split composition block.
  ///
  /// Given two blocks `l, r`, applies `l`, and passes the output to `r`, repeating the signals
  /// to fit the arity of `r`.
  template<ABlock Lhs, ABlock Rhs>
  requires(ins<Rhs> % outs<Lhs> == 0) //
    struct Split : BinOp<Split<Lhs, Rhs>, Lhs, Rhs, ins<Lhs>, outs<Rhs>> {};

  /// The split composition of two blocks.
  template<ABlockRef Lhs, ABlockRef Rhs>
  constexpr auto split(Lhs&& lhs, Rhs&& rhs)
  {
    return Split<std::remove_cvref_t<Lhs>, std::remove_cvref_t<Rhs>>{{FWD(lhs), FWD(rhs)}};
  }

  // MERGE /////////////////////////////////////////////

  /// Merge composition block.
  ///
  /// Given two blocks `l, r`, applies `l`, and passes the output to `r`, such that each
  /// input to `r` is the sum of all `l(x)[i mod ins<r>]`.
  template<ABlock Lhs, ABlock Rhs>
  requires(outs<Lhs> % ins<Rhs> == 0) //
    struct Merge : BinOp<Merge<Lhs, Rhs>, Lhs, Rhs, ins<Lhs>, outs<Rhs>> {};

  /// The merge composition of two blocks.
  template<ABlockRef Lhs, ABlockRef Rhs>
  constexpr auto merge(Lhs&& lhs, Rhs&& rhs)
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

  // DELAY /////////////////////////////////////////////

  /// Variable size memory block.
  ///
  /// Given input signals `(d, x)`, outputs `x` delayed by `d` samples
  struct Delay : BlockBase<Delay, 2, 1> {};
  constexpr Delay delay;

} // namespace topisani::eda
