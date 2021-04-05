#include "eda/block.hpp"

#include <catch2/catch_all.hpp>

#define BASIC_TEST_EXPR(TestMacro, Expr, In, Out)                                                                      \
  TestMacro("Expr '" #Expr "' given " #In " returns " #Out)                                                            \
  {                                                                                                                    \
    auto process = (Expr);                                                                                             \
    REQUIRE(make_evaluator(process).eval(AudioFrame In) == AudioFrame Out);                                            \
  }
#define TEST_EXPR(Expr, In, Out) BASIC_TEST_EXPR(TEST_CASE, Expr, In, Out)
#define EXPR_SECTION(Expr, In, Out) BASIC_TEST_EXPR(SECTION, Expr, In, Out)

namespace topisani::eda {

  using namespace literals;

  void test_process(auto&& block, auto&& input, auto&& output)
  {
    auto processor = make_processor(block);
    processor.process(input, output);
  }

  TEST_CASE ("Ident") {
    std::array<float, 5> input = {1, 2, 3, 4, 5};
    std::array<float, 5> output = {};

    auto process = _;

    test_process(process, input, output);
    REQUIRE(std::ranges::equal(input, output));
  }

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

  TEST_EXPR(cut, (10), ());
  TEST_EXPR((_, cut), (1, 2), (1));

  TEST_CASE ("Recursive: (_, _) % (cut, _)") {
    auto p = (_, _) % (cut, _);
    auto e = make_evaluator(p);
    REQUIRE(e.eval({1}) == AudioFrame(0, 1));
    REQUIRE(e.eval({2}) == AudioFrame(1, 2));
    REQUIRE(e.eval({3}) == AudioFrame(2, 3));
  }

  TEST_CASE ("Currying") {
    auto f1 = (_ + 1);
    auto f2 = (_ - _);
    auto f4 = (_, _, _, _);
    auto f = f4(f1, f2);
    REQUIRE(eval(f, {1, 2, 3, 4, 5}) == AudioFrame(2, -1, 4, 5));
  }

  TEST_EXPR((1_eda, 2), (), (1, 2))
  TEST_EXPR((1, 2_eda), (), (1, 2))

  TEST_CASE ("mem<1>") {
    auto e = make_evaluator(mem<1>);
    REQUIRE(e.eval({1}) == AudioFrame(0));
    REQUIRE(e.eval({2}) == AudioFrame(1));
    REQUIRE(e.eval({3}) == AudioFrame(2));
  }

  TEST_CASE ("mem<5>") {
    auto e = make_evaluator(mem<5>);
    REQUIRE(e.eval({1}) == AudioFrame(0));
    REQUIRE(e.eval({2}) == AudioFrame(0));
    REQUIRE(e.eval({3}) == AudioFrame(0));
    REQUIRE(e.eval({4}) == AudioFrame(0));
    REQUIRE(e.eval({5}) == AudioFrame(0));
    REQUIRE(e.eval({6}) == AudioFrame(1));
    REQUIRE(e.eval({7}) == AudioFrame(2));
    REQUIRE(e.eval({8}) == AudioFrame(3));
    REQUIRE(e.eval({9}) == AudioFrame(4));
    REQUIRE(e.eval({10}) == AudioFrame(5));
    REQUIRE(e.eval({11}) == AudioFrame(6));
  }

  TEST_CASE ("delay") {
    auto e = make_evaluator(delay);
    REQUIRE(e.eval({5, 1}) == AudioFrame(0));
    REQUIRE(e.eval({5, 2}) == AudioFrame(0));
    REQUIRE(e.eval({5, 3}) == AudioFrame(0));
    REQUIRE(e.eval({5, 4}) == AudioFrame(0));
    REQUIRE(e.eval({5, 5}) == AudioFrame(0));
    REQUIRE(e.eval({5, 6}) == AudioFrame(1));
    REQUIRE(e.eval({5, 7}) == AudioFrame(2));
    REQUIRE(e.eval({5, 8}) == AudioFrame(3));
    REQUIRE(e.eval({5, 9}) == AudioFrame(4));
    REQUIRE(e.eval({5, 10}) == AudioFrame(5));
    REQUIRE(e.eval({5, 11}) == AudioFrame(6));
    // Change delay while running
    REQUIRE(e.eval({2, 12}) == AudioFrame(10));
    REQUIRE(e.eval({2, 13}) == AudioFrame(11));
    REQUIRE(e.eval({2, 14}) == AudioFrame(12));
    REQUIRE(e.eval({3, 15}) == AudioFrame(12));
    REQUIRE(e.eval({4, 16}) == AudioFrame(12));
    REQUIRE(e.eval({5, 17}) == AudioFrame(12));
    REQUIRE(e.eval({6, 18}) == AudioFrame(0));
    REQUIRE(e.eval({6, 19}) == AudioFrame(13));
    REQUIRE(e.eval({8, 20}) == AudioFrame(0));
    REQUIRE(e.eval({8, 21}) == AudioFrame(0));
    REQUIRE(e.eval({8, 22}) == AudioFrame(14));
    REQUIRE(e.eval({8, 23}) == AudioFrame(15));
    REQUIRE(e.eval({8, 24}) == AudioFrame(16));
    REQUIRE(e.eval({8, 25}) == AudioFrame(17));
    REQUIRE(e.eval({8, 26}) == AudioFrame(18));
    REQUIRE(e.eval({8, 27}) == AudioFrame(19));
    REQUIRE(e.eval({8, 28}) == AudioFrame(20));
    REQUIRE(e.eval({8, 29}) == AudioFrame(21));
    REQUIRE(e.eval({8, 30}) == AudioFrame(22));
  }

} // namespace topisani::eda
