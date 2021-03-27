#pragma once

#include <concepts>
#include <type_traits>

#include "eda/internal/util.hpp"

/// An Expression template design based on https://hal.archives-ouvertes.fr/hal-01351060/document
namespace topisani::eda::expr2 {

  template<typename T>
  concept AnOperator = true;

  template<typename T>
  struct enable_operand : std::false_type {};
  template<typename T>
  constexpr bool enable_operand_v = enable_operand<T>::value;

  template<typename T>
  concept AnOperand = std::is_same_v<T, std::remove_cvref_t<T>>&& enable_operand_v<T>;

  template<typename T>
  concept AnOperandRef = AnOperand<std::remove_cvref_t<T>>;

  template<AnOperandRef T>
  struct fixed {
    using type = T;
  };

  template<AnOperandRef Op>
  using fixed_t = typename fixed<Op>::type;

  template<AnOperandRef Op>
  constexpr bool is_fixed_v = std::is_same_v<Op, fixed_t<Op>>;

  // Store lvalue references as const lvalue references if the operand is already fixed,
  // otherwise fix and copy
  template<AnOperand Op>
  struct fixed<Op&> {
  private:
    using fixed_type = fixed_t<Op>;

  public:
    using type = std::conditional_t<std::is_same_v<Op, fixed_type>, const Op&, fixed_type>;
  };

  // Copy rvalue references
  template<typename Op>
  struct fixed<Op&&> {
    using type = fixed_t<std::remove_cvref_t<Op>>;
  };

  template<AnOperator Op, AnOperandRef... Ops>
  struct Expression {
    using operator_t = Op;
    using operands_t = std::tuple<Ops...>;
    std::tuple<Ops...> operands;

    Expression(auto&&... args) requires std::is_constructible_v<std::tuple<Ops...>, decltype(args)...>
      : operands(FWD(args)...)
    {}

    template<AnOperandRef... Ops2>
    Expression(Expression<Op, Ops2...>&&
                 rhs) requires std::is_constructible_v<std::tuple<Ops...>, decltype(std::move(rhs.operands))>
      : operands(std::move(rhs.operands))
    {}

    template<AnOperandRef... Ops2>
    Expression(
      const Expression<Op, Ops2...>& rhs) requires std::is_constructible_v<std::tuple<Ops...>, decltype(rhs.operands)>
      : operands(rhs.operands)
    {}
  };

  template<std::size_t I, AnOperator Op, AnOperandRef... Ops>
  decltype(auto) get_op(Expression<Op, Ops...>& expr) requires(I < sizeof...(Ops))
  {
    return std::get<I>(expr.operands);
  }

  template<std::size_t I, AnOperator Op, AnOperandRef... Ops>
  decltype(auto) get_op(const Expression<Op, Ops...>& expr) requires(I < sizeof...(Ops))
  {
    return std::get<I>(expr.operands);
  }

  template<std::size_t I, AnOperator Op, AnOperandRef... Ops>
  decltype(auto) get_op(Expression<Op, Ops...>&& expr) requires(I < sizeof...(Ops))
  {
    return std::move(std::get<I>(expr.operands));
  }

  template<AnOperator Op, AnOperandRef... Ops>
  struct enable_operand<Expression<Op, Ops...>> : std::true_type {};

  template<AnOperator Op, AnOperandRef... Ops>
  struct fixed<Expression<Op, Ops...>> {
    using type = Expression<Op, fixed_t<Ops>...>;
  };

  template<typename T>
  struct Literal {
    T value;
    Literal(T value) : value(FWD(value)) {}

    template<typename U>
    Literal(Literal<U>&& u) requires std::is_constructible_v<T, U&&> : value(std::move(u.value))
    {}

    template<typename U>
    Literal(const Literal<U>& u) requires std::is_constructible_v<T, const U&> : value(std::move(u.value))
    {}
  };

  template<typename T>
  struct fixed<Literal<T>> {
    using type = Literal<std::remove_cvref_t<T>>;
  };

  template<typename T>
  struct fixed<Literal<T&>> {
    using type = Literal<const T&>;
  };

  template<typename T>
  struct enable_operand<Literal<T>> : std::true_type {};

  auto literal(auto&& v)
  {
    return Literal<decltype(v)>(FWD(v));
  }

  decltype(auto) operand(auto&& v)
  {
    if constexpr (AnOperandRef<decltype(v)>) {
      return FWD(v);
    } else {
      return literal(FWD(v));
    }
  }

  template<AnOperator Op>
  auto make_expr(auto&&... ops)
  {
    return Expression<Op, decltype(operand(FWD(ops)))...>(operand(FWD(ops))...);
  }

  decltype(auto) fix(AnOperandRef auto&& operand)
  {
    return fixed_t<decltype(operand)>(FWD(operand));
  }

  // OPERATORS ///////////////////////////////////

#define EDA_DEF_BINOP(Type, OP)                                                                                        \
  struct Type {};                                                                                                      \
  template<typename Lhs, typename Rhs>                                                                                 \
  auto operator OP(Lhs&& lhs, Rhs&& rhs) requires(AnOperandRef<Lhs> || AnOperandRef<Rhs>)                              \
  {                                                                                                                    \
    return make_expr<Type>(FWD(lhs), FWD(rhs));                                                                        \
  }
#define EDA_DEF_UNOP(Type, OP)                                                                                         \
  struct Type {};                                                                                                      \
  template<AnOperandRef Lhs>                                                                                           \
  auto operator OP(Lhs&& lhs)                                                                                          \
  {                                                                                                                    \
    return make_expr<Type>(FWD(lhs));                                                                                  \
  }

  EDA_DEF_BINOP(PlusOp, +)
  EDA_DEF_BINOP(MinusOp, -)
  EDA_DEF_BINOP(TimesOp, *)
  EDA_DEF_BINOP(DivisionOp, /)
  EDA_DEF_BINOP(ModuloOp, %)
  EDA_DEF_BINOP(AndOp, &)
  EDA_DEF_BINOP(OrOp, |)
  EDA_DEF_BINOP(XorOp, ^)
  EDA_DEF_BINOP(ShiftLeftOp, <<)
  EDA_DEF_BINOP(ShiftRightOp, >>)

  EDA_DEF_BINOP(AndAndOp, &&)
  EDA_DEF_BINOP(OrOrOp, ||)

  EDA_DEF_BINOP(PlusAssignOp, +=)
  EDA_DEF_BINOP(MinusAssignOp, -=)
  EDA_DEF_BINOP(TimesAssignOp, *=)
  EDA_DEF_BINOP(DivisionAssignOp, /=)
  EDA_DEF_BINOP(ModuloAssignOp, %=)
  EDA_DEF_BINOP(AndAssignOp, &=)
  EDA_DEF_BINOP(OrAssignOp, |=)
  EDA_DEF_BINOP(XorAssignOp, ^=)
  EDA_DEF_BINOP(ShiftLeftAssignOp, <<=)
  EDA_DEF_BINOP(ShiftRightAssignOp, >>=)

  EDA_DEF_BINOP(EqOp, ==)
  EDA_DEF_BINOP(NeqOp, !=)
  EDA_DEF_BINOP(LessOp, <)
  EDA_DEF_BINOP(GreaterOp, >)
  EDA_DEF_BINOP(LeqOp, <=)
  EDA_DEF_BINOP(GeqOp, >=)
  EDA_DEF_BINOP(SpaceshipOp, <=>)

  EDA_DEF_UNOP(UnPlusOp, +)
  EDA_DEF_UNOP(UnMinusOp, -)
  EDA_DEF_UNOP(NegOp, !)
  EDA_DEF_UNOP(TildeOp, ~)
  EDA_DEF_UNOP(StarOp, *)
  EDA_DEF_UNOP(PrefixIncOp, ++)
  EDA_DEF_UNOP(PrefixDecOp, --)
#undef EDA_DEF_BINOP
#undef EDA_DEF_UNOP

  struct PostfixIncOp {};
  template<AnOperandRef Lhs>
  auto operator++(Lhs&& lhs, int) // NOLINT
  {
    return make_expr<PostfixIncOp>(FWD(lhs));
  }
  struct PostfixDecOp {};
  template<AnOperandRef Lhs>
  auto operator--(Lhs&& lhs, int) // NOLINT
  {
    return make_expr<PostfixDecOp>(FWD(lhs));
  }

  // For syntax reasons this is done without the macro
  struct CommaOp {};
  template<typename Lhs, typename Rhs>
  auto operator,(Lhs&& lhs, Rhs&& rhs) requires(AnOperandRef<Lhs> || AnOperandRef<Rhs>)
  {
    return make_expr<CommaOp>(FWD(lhs), FWD(rhs));
  }

} // namespace topisani::eda::expr2
