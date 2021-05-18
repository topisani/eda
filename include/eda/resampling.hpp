#pragma once

#include <eda/block.hpp>

namespace eda {
  /// Halfband FIR filter
  /// Koefficients from: https://www.dsprelated.com/showcode/270.php
  constexpr FIRFilter halfband_firwin = fir(std::array<float, 33>({
    0.000000f,  -0.001888f, 0.000000f, 0.003862f, 0.000000f,  -0.008242f, 0.000000f, 0.015947f, 0.000000f,
    -0.028677f, 0.000000f,  0.050719f, 0.000000f, -0.098016f, 0.000000f,  0.315942f, 0.500706f, 0.315942f,
    0.000000f,  -0.098016f, 0.000000f, 0.050719f, 0.000000f,  -0.028677f, 0.000000f, 0.015947f, 0.000000f,
    -0.008242f, 0.000000f,  0.003862f, 0.000000f, -0.001888f, 0.000000f,
  }));

  /// Halfband FIR filter
  /// Koefficients from: https://www.dsprelated.com/showcode/270.php
  constexpr FIRFilter halfband_remez = fir(std::array<float, 33>({
    0.000000, -0.010233, 0.000000,  0.010668, 0.000000, -0.016324, 0.000000, 0.024377, 0.000000,  -0.036482, 0.000000,
    0.056990, 0.000000,  -0.101993, 0.000000, 0.316926, 0.500009,  0.316926, 0.000000, -0.101993, 0.000000,  0.056990,
    0.000000, -0.036482, 0.000000,  0.024377, 0.000000, -0.016324, 0.000000, 0.010668, 0.000000,  -0.010233, 0.000000,
  }));

  /// Halfband FIR filter
  /// Koefficients from: https://www.dsprelated.com/showcode/270.php
  constexpr FIRFilter halfband = halfband_remez;
} // namespace eda
