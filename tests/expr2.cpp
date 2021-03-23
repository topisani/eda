#include <catch2/catch_all.hpp>
#include <eda/expr2.hpp>

namespace topisani::eda::expr2 {

  template<typename T>
  auto eval(const Literal<T>& lit) -> T {
    return static_cast<T>(lit.value);
  }

  template<AnOperandRef Lhs, AnOperandRef Rhs>
  decltype(auto) eval(const Expression<PlusOp, Lhs, Rhs>& expr)
  {
    return eval(get_op<0>(expr)) + eval(get_op<1>(expr));
  }

  template<AnOperandRef Lhs, AnOperandRef Rhs>
  decltype(auto) eval(const Expression<MinusOp, Lhs, Rhs>& expr)
  {
    return eval(get_op<0>(expr)) - eval(get_op<1>(expr));
  }

  TEST_CASE ("Expr2") {
    SECTION ("Literal") {
      auto lit = literal(1);
      static_assert(std::is_same_v<decltype(lit), Literal<int&&>>);
      REQUIRE(lit.value == 1);
      REQUIRE(eval(lit) == 1);
    }
    SECTION ("Literal of reference") {
      int i = 1;
      auto lit = literal(i);
      static_assert(std::is_same_v<decltype(lit), Literal<int&>>);
      REQUIRE(&lit.value == &i);
      REQUIRE(&eval(lit) == &i);
    }
    SECTION ("PlusOp") {
      auto expr = literal(2) + literal(2);
      static_assert(std::is_same_v<decltype(expr), Expression<PlusOp, Literal<int&&>&&, Literal<int&&>&&>>);
      REQUIRE(get_op<0>(expr).value == 2);
      REQUIRE(eval(expr) == 4);
    }
    SECTION ("PlusOp automatic literal conversion") {
      auto expr = literal(2) + 2;
      static_assert(std::is_same_v<decltype(expr), Expression<PlusOp, Literal<int&&>&&, Literal<int&&>>>);
      REQUIRE(get_op<0>(expr).value == 2);
      REQUIRE(eval(expr) == 4);
    }

    SECTION ("fix") {
      int i = 10;
      auto expr = fix(literal(2) + i);
      static_assert(std::is_same_v<decltype(expr), Expression<PlusOp, Literal<int>, Literal<const int&>>>);
      REQUIRE(&get_op<1>(expr).value == &i);
      REQUIRE(eval(expr) == 12);
    }
  }

} // namespace topisani::eda::expr2
