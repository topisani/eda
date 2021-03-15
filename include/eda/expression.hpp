#pragma once

#include <eda/internal/util.hpp>

/// A Simple/Naive implementation of expression templates
namespace topisani::eda::expr {

  /// Concept for an expression.
  /// Requires a member function named `eval` to be available
  template<typename T>
  concept AnExpr = requires(T t)
  {
    t.eval();
  };

  template<AnExpr Expr>
  using eval_result_t = decltype(std::declval<Expr>().eval());

  /// Evaluate an expression
  auto eval(AnExpr auto&& expr)
  {
    return FWD(expr).eval();
  }

  /// A Basic value leaf expression
  template<typename T>
  struct ValueExpr {
    T value;
    constexpr auto eval()
    {
      return value;
    }
  };

  /// Concept of type that is either an expression or can be
  /// converted to one through `value()` or `expr()`
  template<typename T>
  concept ValOrExpr = true;

  /// Ensure expression type by converting to `Value<T>` if needed.
  ///
  /// Can be used by operators to allow conversions
  constexpr auto expr(ValOrExpr auto&& e)
  {
    if constexpr (AnExpr<decltype(e)>) {
      return FWD(e);
    } else {
      return ValueExpr<decltype(e)>{FWD(e)};
    }
  }

  /// The type returned by `expr(T)`
  template<ValOrExpr T>
  using expr_t = decltype(expr(std::declval<T>()));

  /// A compile-time value leaf expression
  template<auto Value>
  struct Const {
    constexpr static auto eval()
    {
      return Value;
    }
  };

  /// Lazily evaluated plus operator expression
  ///
  /// Captures its subexpressions as an aggregate
  template<AnExpr Lhs, AnExpr Rhs>
  struct PlusOp {
    Lhs lhs;
    Rhs rhs;
    constexpr auto eval()
    {
      return lhs.eval() + rhs.eval();
    }
  };

  /// Operator to construct a `PlusOp`.
  /// Will convert its parameters to `ValueExpr` through `expr` if needed
  template<ValOrExpr Lhs, ValOrExpr Rhs>
  constexpr auto operator+(Lhs&& lhs, Rhs&& rhs) -> PlusOp<expr_t<Lhs>, expr_t<Rhs>>
  {
    return {expr(FWD(lhs)), expr(FWD(rhs))};
  }
} // namespace topisani::eda::expr
