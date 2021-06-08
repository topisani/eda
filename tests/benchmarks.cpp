#include <chrono>
#include <iostream>
#include <random>
#include <ranges>

#include <catch2/catch_all.hpp>

#include "eda/evaluator.hpp"
#include "eda/syntax.hpp"

struct Filter {
  float eval(float in)
  {
    z = z * a + (1 - a) * in;
    return z;
  }

  float a = 0.9;

private:
  float z = 0;
};

struct Delay {
  void set_delay(int delay)
  {
    delay_ = delay;
    if (auto old_size = memory_.size(); old_size < delay) {
      memory_.resize(delay);
      auto src = memory_.begin() + old_size - 1;
      auto dst = memory_.begin() + delay - 1;
      auto n = old_size - index_;
      for (int i = 0; i < n; i++, src--, dst--) {
        *dst = *src;
        *src = 0;
      }
    }
  }

  float eval(float in)
  {
    float res = memory_[(memory_.size() + index_ - delay_) % memory_.size()];
    memory_[index_] = in;
    ++index_;
    index_ %= static_cast<std::ptrdiff_t>(memory_.size());
    return res;
  }

private:
  int delay_ = 0;
  std::size_t index_ = 0;
  std::vector<float> memory_;
};

struct Echo {
  float eval(float in)
  {
    prev_ = delay_.eval(filter_.eval(prev_) * feedback + in);
    return prev_;
  }
  float feedback;
  float prev_ = 0;
  Filter filter_;
  Delay delay_;
};

struct EchoFX {
  float eval(float in)
  {
    echo.delay_.set_delay(time_samples);
    echo.filter_.a = filter_a;
    echo.feedback = feedback;
    return echo.eval(in) * dry_wet_mix + in * (1 - dry_wet_mix);
  }
  int time_samples = 11025;
  float filter_a = 0.9;
  float feedback = 1.0;
  float dry_wet_mix = 0.5;

private:
  Echo echo;
};

void fill_random(std::ranges::range auto& data)
{
  std::random_device rd;
  std::mt19937 e2(rd());
  std::uniform_real_distribution<float> dist(-1, 1);
  std::ranges::generate(data, [&] { return dist(e2); });
}

void benchmark_fx(std::string name, auto fx)
{
  std::array<float, 1024> in = {};
  fill_random(in);
  std::array<float, 1024> out = {};

  int iterations = 1000;
  using clock = std::chrono::high_resolution_clock;
  clock::duration total_time = clock::duration::zero();
  for (int i = 0; i < iterations; i++) {
    const auto iter_start = clock::now();
    for (int j = 0; j < 1024; j++) {
      out[j] = fx.eval(in[j]);
    }
    const auto iter_end = clock::now();
    total_time += iter_end - iter_start;
    // std::cout << i << ": " << (iter_end - iter_start).count() << "ns\n";
  }
  std::cout << name << ", " << iterations << " iterations, average: " << (total_time.count() / iterations) << "ns\n";
}

TEST_CASE ("Echo benchmark Handwritten C++") {
  benchmark_fx("Handwritten", EchoFX());
  float time_samples = 11025;
  float filter_a = 0.9;
  float feedback = 1.0;
  float dry_wet_mix = 0.5;
  benchmark_fx("EDA", eda::make_evaluator([&] {
                 using namespace eda;
                 using namespace eda::syntax;
                 ABlock<2, 1> auto const filter = (_ << (_, _), _) | (((_ * _, (1 - _) * _) | plus) % _);
                 ABlock<1, 1> auto const echo =
                   (plus | delay(ref(time_samples))) % (filter(ref(filter_a)) * ref(feedback));
                 ABlock<1, 1> auto const process = _ << (echo * ref(dry_wet_mix)) + (_ * (1 - ref(dry_wet_mix)));
                 return process;
               }()));
}
