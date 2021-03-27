#include "eda/block.hpp"

#include <catch2/catch_all.hpp>

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

  TEST_CASE ("_ + _") {
    auto process = _ + _;
    REQUIRE(make_evaluator(process).eval({{2, 4}}) == 6.f);
  }

  TEST_CASE ("_ + 0.5") {
    auto process = _ + 0.5;
    REQUIRE(make_evaluator(process).eval({2}) == 2.5f);
  }

  TEST_CASE ("_ , _") {
    auto process = (_, _);
    REQUIRE(make_evaluator(process).eval({1, 2}) == AudioFrame(1, 2));
  }

  TEST_CASE ("_ , 1") {
    auto process = (_, 2);
    REQUIRE(make_evaluator(process).eval({1}) == AudioFrame(1, 2));
  }

  TEST_CASE ("_ | (_ + 1) | (_ , 10)") {
    auto process = (_ | (_ + 1) | (_, 10));
    REQUIRE(make_evaluator(process).eval({1}) == AudioFrame(2, 10));
  }

} // namespace topisani::eda
