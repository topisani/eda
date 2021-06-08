#include <eda/eda.hpp>

#include "../lv2.hpp"

struct Echo final : LV2Plugin {
  constexpr static auto uri = "http://topisani.co/lv2/eda/echo";
  Echo() = default;

  void connect_port(uint32_t port, float* data) override
  {
    switch (port) {
      case 0: time_samples = data; break;
      case 1: filter_a = data; break;
      case 2: feedback = data; break;
      case 3: dry_wet_mix = data; break;
      case 4: input_port = data; break;
      case 5: output_port = data; break;
    }
  };

  void run(uint32_t n_samples) override
  {
    for (int i = 0; i < n_samples; i++) {
      output_port[i] = eval(input_port[i]);
    }
  }

  void activate() override
  {
    eval = eda::make_evaluator([this] {
      using namespace eda;
      using namespace eda::syntax;
      ABlock<2, 1> auto const filter = (_ << (_, _), _) | (((_ * _, (1 - _) * _) | plus) % _);
      ABlock<1, 1> auto const echo = (plus | delay(ref(*time_samples))) % (filter(ref(*filter_a)) * ref(*feedback));
      ABlock<1, 1> auto const process = _ << (echo * ref(*dry_wet_mix)) + (_ * (1 - ref(*dry_wet_mix)));
      return process;
    }());
  }

private:
  eda::DynEvaluator<1, 1> eval;

  float* time_samples = nullptr;
  float* filter_a = nullptr;
  float* feedback = nullptr;
  float* dry_wet_mix = nullptr;

  float* input_port = nullptr;
  float* output_port = nullptr;
};

extern "C" {
LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
  static auto descriptor = make_descriptor<Echo>(Echo::uri);
  switch (index) {
    case 0: return &descriptor;
    default: return nullptr;
  }
}
}
