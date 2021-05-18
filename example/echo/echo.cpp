#include <eda/eda.hpp>

#include "../lv2.hpp"

struct Echo final : LV2Plugin {
  constexpr static auto uri = "http://topisani.co/lv2/eda/echo";
  Echo()
    : process(eda::make_evaluator([this] {
        using namespace eda;
        using namespace eda::syntax;
        ABlock<2, 1> auto const filter = (_ << (_, _), _) | (((_ * _, (1 - _) * _) | plus) % _);
        ABlock<1, 1> auto const echo = (plus | delay(ref(time_samples))) % (filter(ref(filter_a)) * ref(feedback));
        ABlock<1, 1> auto const process = _ << (echo * ref(dry_wet_mix)) + (_ * (1 - ref(dry_wet_mix)));
        return process;
      }()))
  {}

  void connect_port(uint32_t port, float* data) override
  {
    switch (port) {
      case 0: time_samples_port = data; break;
      case 1: filter_a_port = data; break;
      case 2: feedback_port = data; break;
      case 3: dry_wet_mix_port = data; break;
      case 4: input_port = data; break;
      case 5: output_port = data; break;
    }
  };
  void run(uint32_t n_samples) override
  {
    time_samples = *time_samples_port;
    filter_a = *filter_a_port;
    feedback = *feedback_port;
    dry_wet_mix = *dry_wet_mix_port;
    for (int i = 0; i < n_samples; i++) {
      output_port[i] = process(input_port[i]);
    }
  }
  float time_samples = 11025;
  float filter_a = 0.9;
  float feedback = 1.0;
  float dry_wet_mix = 0.5;
  eda::DynEvaluator<1, 1> process;

  float* time_samples_port = nullptr;
  float* filter_a_port = nullptr;
  float* feedback_port = nullptr;
  float* dry_wet_mix_port = nullptr;
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

