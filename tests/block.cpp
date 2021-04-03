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

  TEST_CASE ("mem") {
    auto e = make_evaluator(mem);
    REQUIRE(e.eval({1}) == AudioFrame(0));
    REQUIRE(e.eval({2}) == AudioFrame(1));
    REQUIRE(e.eval({3}) == AudioFrame(2));
  }

} // namespace topisani::eda
