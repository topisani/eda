#include <catch2/catch_all.hpp>
#include <eda/expression.hpp>

namespace topisani::eda::expr {

  TEST_CASE ("Expr") {
    SECTION ("PlusOp") {
      PlusOp<Const<1>, Const<2>> op;
      REQUIRE(op.eval() == 3);
    }

    SECTION ("operator+") {
      auto op = Const<1>() + Const<2>();

      REQUIRE(op.eval() == 3);
    }

    SECTION ("Nested operations") {
      auto op = Const<1>() + Const<2>() + Const<3>();
      REQUIRE(op.eval() == 6);
    }

    SECTION ("Value<T>") {
      REQUIRE(expr(1).eval() == 1);

      REQUIRE(eval(1 + expr(2) + expr(3)) == 6);
    }
  }

} // namespace topisani::eda::expr
