#include <concepts>
#include <ranges>
#include <span>

#include <catch2/catch_all.hpp>

namespace topisani::eda {

  /// The basic data type used for audio samples and other real numbers.
  /// Hardcoded to float here for simplicity, but all types could have it
  /// as a template parameter.
  using real = float;

  /// A non-owning buffer of audio.
  struct AudioBuffer {

    /// Constructor that does not require explicit conversion to span first
    /// Takes a `contiguous_range` of `real`, and constructs the internal span
    /// with it
    template<std::ranges::contiguous_range Range>
    requires(std::is_same_v<std::ranges::range_value_t<Range>, real>) //
      AudioBuffer(Range&& data) noexcept
      : data_(std::forward<Range>(data))
    {}

    /// Get the begin iterator
    auto begin() noexcept
    {
      return data_.begin();
    }

    /// Get the end iterator
    auto end() noexcept
    {
      return data_.end();
    }

  private:
    std::span<real> data_;
  };

  TEST_CASE ("AudioBuffer") {
    std::vector<real> data = {0.f, 0.f, 0.f, 0.f};

    AudioBuffer buf = data;
  }

} // namespace topisani::eda
