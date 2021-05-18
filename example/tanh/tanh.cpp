#include <eda/eda.hpp>
#include <eda/resampling.hpp>

#include "../lv2.hpp"

struct Tanh final : LV2Plugin {
  constexpr static auto uri = "http://topisani.co/lv2/eda/tanh";
  Tanh()
    : process(eda::make_evaluator([this] {
        using namespace eda;
        using namespace eda::syntax;
        const auto sat =_ * ref(gain) | eda::tanh | _ / ref(gain) ;
        return halfband; // resample<2>(halfband | sat | halfband);
      }()))
  {}

  void connect_port(uint32_t port, float* data) override
  {
    switch (port) {
      case 0: gain_port = data; break;
      case 1: input_port = data; break;
      case 2: output_port = data; break;
    }
  };
  void run(uint32_t n_samples) override
  {
    gain = *gain_port;
    for (int i = 0; i < n_samples; i++) {
      output_port[i] = process(input_port[i]);
    }
  }
  float gain = 1.0;
  eda::DynEvaluator<1, 1> process;

  float* gain_port = nullptr;
  float* input_port = nullptr;
  float* output_port = nullptr;
};

extern "C" {
LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
  static auto descriptor = make_descriptor<Tanh>(Tanh::uri);
  switch (index) {
    case 0: return &descriptor;
    default: return nullptr;
  }
}
}
