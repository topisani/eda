#pragma once

#include <array>
#include <cstddef>
#include <span>

namespace topisani::eda {

  template<std::size_t Channels>
  struct Frame {
    constexpr Frame() = default;
    constexpr Frame(std::array<float, Channels> data) : data_(data) {}
    constexpr Frame(auto... floats) requires(sizeof...(floats) == Channels &&
                                             (std::convertible_to<decltype(floats), float> && ...))
      : data_{static_cast<float>(floats)...}
    {}

    static constexpr std::size_t size()
    {
      return Channels;
    }
    static constexpr std::size_t channels()
    {
      return Channels;
    }

    [[nodiscard]] constexpr auto begin() const noexcept
    {
      return data_.begin();
    }
    [[nodiscard]] constexpr auto end() const noexcept
    {
      return data_.end();
    }

    constexpr auto begin()
    {
      return data_.begin();
    }
    constexpr auto end()
    {
      return data_.end();
    }

    constexpr float& operator[](std::size_t Idx)
    {
      return data_[Idx];
    }

    constexpr float* data()
    {
      return data_.data();
    }

    operator float&() //
      requires(size() == 1)
    {
      return data_[0];
    }

    constexpr bool operator==(const Frame&) const noexcept = default;
    constexpr bool operator==(float f) const noexcept //
      requires(size() == 1)
    {
      return data_[0] == f;
    }

  private : //
            std::array<float, Channels>
              data_ = {0};
  };

  template<>
  struct Frame<0> {
    constexpr Frame() = default;
    static constexpr std::size_t size()

    // PARALLEL //////////////////////////////////////////
    {
      return 0;
    }
    static constexpr std::size_t channels()
    {
      return 0;
    }

    static constexpr float* begin()
    {
      return nullptr;
    }
    static constexpr float* end()
    {
      return nullptr;
    }

    static constexpr float* data()
    {
      return nullptr;
    }

    constexpr bool operator==(const Frame&) const noexcept = default;
  };

  template<std::size_t N>
  Frame(std::array<float, N> arr) -> Frame<N>;

  Frame(auto... floats) -> Frame<sizeof...(floats)>;

  template<std::ptrdiff_t Begin, std::ptrdiff_t End, std::size_t Channels>
  requires(Begin >= 0 && ((End >= Begin && End <= Channels))) //
    constexpr auto splice(const Frame<Channels>& in)
  {
    Frame<End - Begin> res;
    std::copy(in.begin() + Begin, in.begin() + End, res.begin());
    return res;
  }

  template<std::ptrdiff_t Begin, std::ptrdiff_t End, std::size_t Channels>
  requires(Begin >= 0 && End < 0 && (Channels + End + 1) >= Begin) //
    constexpr auto splice(const Frame<Channels>& in)               //
  {
    return splice<Begin, Channels + End + 1, Channels>(in);
  }

  template<std::size_t S1, std::size_t S2>
  constexpr auto concat(const Frame<S1>& x1, const Frame<S2>& x2) -> Frame<S1 + S2>
  {
    Frame<S1 + S2> res;
    auto b2 = std::copy(x1.begin(), x1.end(), res.begin());
    std::copy(x2.begin(), x2.end(), b2);
    return res;
  }

  template<std::size_t S1, std::size_t... Sizes>
  constexpr auto concat(const Frame<S1>& x, const Frame<Sizes>&... xs) -> Frame<S1 + (Sizes + ...)>
  {
    return concat(x, concat(xs...));
  }


  /// The basic data type used for audio samples and other real numbers.
  /// Hardcoded to float here for simplicity, but all types could have it
  /// as a template parameter.
  using real = float;

  /// A non-owning buffer of audio.
  template<std::size_t Channels>
  struct AudioBuffer {
    /// Constructor that does not require explicit conversion to span first
    /// Takes a `contiguous_range` of `real`, and constructs the internal span
    /// with it
    template<std::ranges::contiguous_range Range>
    requires(std::is_same_v<std::ranges::range_value_t<Range>, real>) //
      AudioBuffer(Range&& data)
    noexcept // NOLINT
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
  template<typename T, std::size_t InChannels, std::size_t OutChannels>
  concept AnAudioProcessor = requires(T t, AudioBuffer<InChannels> in, AudioBuffer<OutChannels> out)
  {
    t.process(in, out);
  };

} // namespace topisani::eda
