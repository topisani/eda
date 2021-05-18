#include "eda/block.hpp"
#include "eda/syntax.hpp"
#include "eda/evaluator.hpp"

#include <catch2/catch_all.hpp>

#define BASIC_TEST_EXPR(TestMacro, Expr, In, Out)                                                                      \
  TestMacro("Expr '" #Expr "' given " #In " returns " #Out)                                                            \
  {                                                                                                                    \
    auto process = (Expr);                                                                                             \
    REQUIRE(make_evaluator(process).eval(Frame In) == Frame Out);                                            \
  }
#define TEST_EXPR(Expr, In, Out) BASIC_TEST_EXPR(TEST_CASE, Expr, In, Out)
#define EXPR_SECTION(Expr, In, Out) BASIC_TEST_EXPR(SECTION, Expr, In, Out)

namespace eda {

  using namespace syntax;

  TEST_EXPR(_ - _, (1, 2), (-1));
  TEST_EXPR((_ + 0.5), //
            (2),
            (2.5f));

  TEST_EXPR((_, _), //
            (1, 2),
            (1, 2));

  TEST_EXPR(((_, _), (_, _)), //
            (1, 2, 3, 4),
            (1, 2, 3, 4));

  TEST_EXPR((_, 2), //
            (1),
            (1, 2));

  TEST_EXPR((_ | (_ + 1) | (_, 10)), //
            (1),
            (2, 10));

  TEST_EXPR((_ << (_, _)), //
            (1),
            (1, 1));

  TEST_EXPR(((_, _) << (_, _, _, _)), //
            (1, 2),
            (1, 2, 1, 2));

  TEST_EXPR(((_, _) << (_, _, _, _, _, _)), //
            (1, 2),
            (1, 2, 1, 2, 1, 2));

  TEST_EXPR(((_, _) >> _), //
            (1, 2),
            (3))

  TEST_EXPR($, (10), ());
  TEST_EXPR((_, $), (1, 2), (1));

  TEST_CASE ("Recursive: (_, _) % ($, _)") {
    auto p = (_, _) % ($, _);
    auto e = make_evaluator(p);
    REQUIRE(e.eval({1}) == Frame(0, 1));
    REQUIRE(e.eval({2}) == Frame(1, 2));
    REQUIRE(e.eval({3}) == Frame(2, 3));
  }

  TEST_CASE ("Currying") {
    REQUIRE(eval((_, _, _, _)(1, 2, 3), {4}) == Frame(1, 2, 3, 4));
    auto f1 = (_ + 1);
    auto f2 = (_ - _);
    auto f4 = (_, _, _, _);
    auto f = f4(f1, f2);
    //static_assert(ABlock<decltype(f), 1, 2>);
    REQUIRE(eval(f, {1, 2, 3, 4, 5}) == Frame(2, -1, 4, 5));
  }

  TEST_EXPR((1_eda, 2), (), (1, 2))
  TEST_EXPR((1, 2_eda), (), (1, 2))

  TEST_CASE ("mem<1>") {
    auto e = make_evaluator(mem<1>);
    REQUIRE(e.eval({1}) == Frame(0));
    REQUIRE(e.eval({2}) == Frame(1));
    REQUIRE(e.eval({3}) == Frame(2));
    auto b = make_evaluator(~_);
    REQUIRE(b.eval({1}) == Frame(0));
    REQUIRE(b.eval({2}) == Frame(1));
    REQUIRE(b.eval({3}) == Frame(2));
  }

  TEST_CASE ("mem<5>") {
    auto e = make_evaluator(mem<5>);
    REQUIRE(e.eval({1}) == Frame(0));
    REQUIRE(e.eval({2}) == Frame(0));
    REQUIRE(e.eval({3}) == Frame(0));
    REQUIRE(e.eval({4}) == Frame(0));
    REQUIRE(e.eval({5}) == Frame(0));
    REQUIRE(e.eval({6}) == Frame(1));
    REQUIRE(e.eval({7}) == Frame(2));
    REQUIRE(e.eval({8}) == Frame(3));
    REQUIRE(e.eval({9}) == Frame(4));
    REQUIRE(e.eval({10}) == Frame(5));
    REQUIRE(e.eval({11}) == Frame(6));
  }

  TEST_CASE ("delay") {
    auto e = make_evaluator(delay);
    REQUIRE(e.eval({5, 1}) == Frame(0));
    REQUIRE(e.eval({5, 2}) == Frame(0));
    REQUIRE(e.eval({5, 3}) == Frame(0));
    REQUIRE(e.eval({5, 4}) == Frame(0));
    REQUIRE(e.eval({5, 5}) == Frame(0));
    REQUIRE(e.eval({5, 6}) == Frame(1));
    REQUIRE(e.eval({5, 7}) == Frame(2));
    REQUIRE(e.eval({5, 8}) == Frame(3));
    REQUIRE(e.eval({5, 9}) == Frame(4));
    REQUIRE(e.eval({5, 10}) == Frame(5));
    REQUIRE(e.eval({5, 11}) == Frame(6));
    // Change delay while running
    REQUIRE(e.eval({2, 12}) == Frame(10));
    REQUIRE(e.eval({2, 13}) == Frame(11));
    REQUIRE(e.eval({2, 14}) == Frame(12));
    REQUIRE(e.eval({3, 15}) == Frame(12));
    REQUIRE(e.eval({4, 16}) == Frame(12));
    REQUIRE(e.eval({5, 17}) == Frame(12));
    REQUIRE(e.eval({6, 18}) == Frame(0));
    REQUIRE(e.eval({6, 19}) == Frame(13));
    REQUIRE(e.eval({8, 20}) == Frame(0));
    REQUIRE(e.eval({8, 21}) == Frame(0));
    REQUIRE(e.eval({8, 22}) == Frame(14));
    REQUIRE(e.eval({8, 23}) == Frame(15));
    REQUIRE(e.eval({8, 24}) == Frame(16));
    REQUIRE(e.eval({8, 25}) == Frame(17));
    REQUIRE(e.eval({8, 26}) == Frame(18));
    REQUIRE(e.eval({8, 27}) == Frame(19));
    REQUIRE(e.eval({8, 28}) == Frame(20));
    REQUIRE(e.eval({8, 29}) == Frame(21));
    REQUIRE(e.eval({8, 30}) == Frame(22));
  }

  TEST_CASE("FunBlock") {
    const auto f = fun<1, 2>([] (Frame<1> in) {
      return Frame(in[0], in[0] * 2);
    });
    REQUIRE(eval(f, {10}) == Frame(10, 20));
  }

  TEST_CASE("Resample") {
    // const auto f = resample<2>(mem<1>);
  }

} // namespace eda
