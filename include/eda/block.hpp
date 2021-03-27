#pragma once
#include <algorithm>
#include <ranges>
#include <span>

#include "eda/expr2.hpp"

namespace topisani::eda {

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
      AudioBuffer(Range&& data) noexcept                              // NOLINT
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

  template<std::size_t Channels>
  struct AudioFrame {
    constexpr AudioFrame() = default;
    constexpr AudioFrame(std::array<float, Channels> data) : data_(data) {}
    constexpr AudioFrame(auto... floats) requires(sizeof...(floats) == Channels &&
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

    constexpr bool operator==(const AudioFrame&) const noexcept = default;
    constexpr bool operator==(float f) const noexcept //
      requires(size() == 1)
    {
      return data_[0] == f;
    };

  private:
    std::array<float, Channels> data_;
  };

  template<>
  struct AudioFrame<0> {
    constexpr AudioFrame() = default;
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

    bool operator==(const AudioFrame&) const noexcept = default;
  };

  template<std::size_t N>
  AudioFrame(std::array<float, N> arr) -> AudioFrame<N>;

  AudioFrame(auto... floats) -> AudioFrame<sizeof...(floats)>;

  template<std::ptrdiff_t Begin, std::ptrdiff_t End, std::size_t Channels>
  requires(Begin >= 0 && ((End >= Begin && End <= Channels))) //
    auto splice(const AudioFrame<Channels>& in)
  {
    AudioFrame<End - Begin> res;
    std::copy(in.begin() + Begin, in.begin() + End, res.begin());
    return res;
  }

  template<std::ptrdiff_t Begin, std::ptrdiff_t End, std::size_t Channels>
  requires(Begin >= 0 && End < 0 && (Channels + End + 1) >= Begin) //
    auto splice(const AudioFrame<Channels>& in)                    //
  {
    return splice<Begin, Channels + End + 1, Channels>(in);
  }

  template<typename T, std::size_t InChannels, std::size_t OutChannels>
  concept AnAudioProcessor = requires(T t, AudioBuffer<InChannels> in, AudioBuffer<OutChannels> out)
  {
    t.process(in, out);
  };

  // BLOCK /////////////////////////////////////////////

  template<std::size_t InChannels, std::size_t OutChannels>
  struct BlockBase {
    static constexpr std::size_t in_channels = InChannels;
    static constexpr std::size_t out_channels = OutChannels;
  };

  template<typename T>
  concept ABlock =                                           //
    util::decays_to<decltype(T::in_channels), std::size_t>&& //
      util::decays_to<decltype(T::in_channels), std::size_t>&&
        std::is_base_of_v<BlockBase<T::in_channels, T::out_channels>, T>;

  template<typename T>
  concept ABlockRef = ABlock<std::remove_cvref_t<T>>;

  // EVALUATOR /////////////////////////////////////////

  template<typename T>
  struct evaluator;

  template<typename T>
  struct processor;

  auto make_processor(ABlockRef auto&& b)
  {
    return processor<std::decay_t<decltype(b)>>(b);
  }

  auto make_evaluator(ABlockRef auto&& b)
  {
    return evaluator<std::decay_t<decltype(b)>>(b);
  }

  // LITERAL ///////////////////////////////////////////

  struct Literal : BlockBase<0, 1> {
    float value;
  };

  decltype(auto) wrap_literal(ABlockRef auto&& input)
  {
    return FWD(input);
  }

  inline Literal wrap_literal(float f)
  {
    return Literal{{}, f};
  }

  template<>
  struct evaluator<Literal> {
    constexpr evaluator(Literal l) : literal_(l) {}

    AudioFrame<1> eval(AudioFrame<0>)
    {
      return {{literal_.value}};
    }

    Literal literal_;
  };

  // IDENT /////////////////////////////////////////////

  struct Ident : BlockBase<1, 1> {};
  static_assert(ABlock<Ident>);

  constexpr Ident ident;
  namespace literals {
    constexpr Ident _ = ident;
  }

  template<>
  struct processor<Ident> {
    constexpr processor(Ident){};
    static void process(AudioBuffer<1> input, AudioBuffer<1> output)
    {
      std::ranges::copy(input, output.begin());
    }
  };

  template<>
  struct evaluator<Ident> {
    constexpr evaluator(Ident){};
    static AudioFrame<1> eval(AudioFrame<1> in)
    {
      return in;
    }
  };

  // PARALLEL //////////////////////////////////////////

  template<ABlock Lhs, ABlock Rhs>
  struct Parallel : BlockBase<Lhs::in_channels + Rhs::in_channels, Lhs::out_channels + Rhs::out_channels> {
    Parallel(Lhs l, Rhs r) : lhs(l), rhs(r) {}
    Lhs lhs;
    Rhs rhs;
  };

  template<typename Lhs, typename Rhs>
  Parallel(Lhs, Rhs) -> Parallel<std::decay_t<Lhs>, std::decay_t<Rhs>>;

  template<typename Lhs, typename Rhs>
  auto operator,(Lhs&& lhs, Rhs&& rhs) //
    requires(ABlockRef<Lhs> || ABlockRef<Rhs>)
  {
    return Parallel(wrap_literal(FWD(lhs)), wrap_literal(FWD(rhs)));
  }

  template<ABlock Lhs, ABlock Rhs>
  struct evaluator<Parallel<Lhs, Rhs>> {
    constexpr evaluator(const Parallel<Lhs, Rhs>& block) : lhs_(block.lhs), rhs_(block.rhs) {}

    constexpr AudioFrame<Parallel<Lhs, Rhs>::out_channels> eval(AudioFrame<Parallel<Lhs, Rhs>::in_channels> in)
    {
      auto l = lhs_.eval(splice<0, Lhs::in_channels>(in));
      auto r = rhs_.eval(splice<Lhs::in_channels, -1>(in));
      AudioFrame<Parallel<Lhs, Rhs>::out_channels> res;
      auto rbegin = std::copy(l.begin(), l.end(), res.begin());
      std::copy(r.begin(), r.end(), rbegin);
      return res;
    }

  private:
    evaluator<Lhs> lhs_;
    evaluator<Rhs> rhs_;
  };

  // SERIAL ////////////////////////////////////////////

  template<ABlock Lhs, ABlock Rhs>
  requires(Lhs::out_channels == Rhs::in_channels) //
    struct Serial : BlockBase<Lhs::in_channels, Rhs::out_channels> {
    Serial(Lhs l, Rhs r) : lhs(l), rhs(r) {}
    Lhs lhs;
    Rhs rhs;
  };

  template<typename Lhs, typename Rhs>
  Serial(Lhs, Rhs) -> Serial<std::decay_t<Lhs>, std::decay_t<Rhs>>;

  template<typename Lhs, typename Rhs>
  auto operator|(Lhs&& lhs, Rhs&& rhs) //
    requires(ABlockRef<Lhs> || ABlockRef<Rhs>)
  {
    return Serial(wrap_literal(FWD(lhs)), wrap_literal(FWD(rhs)));
  }

  template<ABlock Lhs, ABlock Rhs>
  struct evaluator<Serial<Lhs, Rhs>> {
    constexpr evaluator(const Serial<Lhs, Rhs>& block) : lhs_(block.lhs), rhs_(block.rhs) {}

    constexpr AudioFrame<Serial<Lhs, Rhs>::out_channels> eval(AudioFrame<Serial<Lhs, Rhs>::in_channels> in)
    {
      auto l = lhs_.eval(in);
      return rhs_.eval(l);
    }

  private:
    evaluator<Lhs> lhs_;
    evaluator<Rhs> rhs_;
  };

  // PLUS //////////////////////////////////////////////

  template<ABlock Lhs, ABlock Rhs>
    requires(Lhs::out_channels == 1) && (Rhs::out_channels == 1) //
    struct Plus : BlockBase<Lhs::in_channels + Rhs::in_channels, 1> {
    Plus(Lhs l, Rhs r) : lhs(l), rhs(r) {}
    Lhs lhs;
    Rhs rhs;
  };

  template<typename Lhs, typename Rhs>
  Plus(Lhs, Rhs) -> Plus<std::decay_t<Lhs>, std::decay_t<Rhs>>;

  template<typename Lhs, typename Rhs>
  auto operator+(Lhs&& lhs, Rhs&& rhs) //
    requires(ABlockRef<Lhs> || ABlockRef<Rhs>)
  {
    return Plus(wrap_literal(FWD(lhs)), wrap_literal(FWD(rhs)));
  }

  template<ABlock Lhs, ABlock Rhs>
  struct evaluator<Plus<Lhs, Rhs>> {
    constexpr evaluator(const Plus<Lhs, Rhs>& block) : lhs_(block.lhs), rhs_(block.rhs) {}
    constexpr AudioFrame<1> eval(AudioFrame<Plus<Lhs, Rhs>::in_channels> in)
    {
      float l = lhs_.eval(splice<0, Lhs::in_channels>(in));
      float r = rhs_.eval(splice<Lhs::in_channels, -1>(in));
      return AudioFrame<1>(l + r);
    }

  private:
    evaluator<Lhs> lhs_;
    evaluator<Rhs> rhs_;
  };

} // namespace topisani::eda

namespace topisani::eda::expr2 {
  // Any block can be an operand
  template<ABlockRef B>
  struct enable_operand<B> : std::true_type {};
} // namespace topisani::eda::expr2
