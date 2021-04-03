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
    }

  private : //
            std::array<float, Channels>
              data_ = {0};
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

    constexpr bool operator==(const AudioFrame&) const noexcept = default;
  };

  template<std::size_t N>
  AudioFrame(std::array<float, N> arr) -> AudioFrame<N>;

  AudioFrame(auto... floats) -> AudioFrame<sizeof...(floats)>;

  template<std::ptrdiff_t Begin, std::ptrdiff_t End, std::size_t Channels>
  requires(Begin >= 0 && ((End >= Begin && End <= Channels))) //
    constexpr auto splice(const AudioFrame<Channels>& in)
  {
    AudioFrame<End - Begin> res;
    std::copy(in.begin() + Begin, in.begin() + End, res.begin());
    return res;
  }

  template<std::ptrdiff_t Begin, std::ptrdiff_t End, std::size_t Channels>
  requires(Begin >= 0 && End < 0 && (Channels + End + 1) >= Begin) //
    constexpr auto splice(const AudioFrame<Channels>& in)          //
  {
    return splice<Begin, Channels + End + 1, Channels>(in);
  }

  template<std::size_t S1, std::size_t S2>
  constexpr auto concat(const AudioFrame<S1>& x1, const AudioFrame<S2>& x2) -> AudioFrame<S1 + S2>
  {
    AudioFrame<S1 + S2> res;
    auto b2 = std::copy(x1.begin(), x1.end(), res.begin());
    std::copy(x2.begin(), x2.end(), b2);
    return res;
  }

  template<std::size_t S1, std::size_t... Sizes>
  constexpr auto concat(const AudioFrame<S1>& x, const AudioFrame<Sizes>&... xs) -> AudioFrame<S1 + (Sizes + ...)>
  {
    return concat(x, concat(xs...));
  }

  template<typename T, std::size_t InChannels, std::size_t OutChannels>
  concept AnAudioProcessor = requires(T t, AudioBuffer<InChannels> in, AudioBuffer<OutChannels> out)
  {
    t.process(in, out);
  };

  // BLOCK /////////////////////////////////////////////

  template<typename Derived, std::size_t InChannels, std::size_t OutChannels>
  struct BlockBase {
    static constexpr std::size_t in_channels = InChannels;
    static constexpr std::size_t out_channels = OutChannels;

    constexpr auto operator()(auto&&... inputs) requires(sizeof...(inputs) <= InChannels);
  };

  template<typename T>
  concept ABlock =                                            //
    util::decays_to<decltype(T::in_channels), std::size_t> && //
    util::decays_to<decltype(T::in_channels), std::size_t> &&
    std::is_base_of_v<BlockBase<T, T::in_channels, T::out_channels>, T>;

  template<typename T>
  concept ABlockRef = ABlock<std::remove_cvref_t<T>>;

  // EVALUATOR /////////////////////////////////////////

  template<typename T>
  struct evaluator;

  template<typename T>
  struct processor;

  constexpr auto make_processor(ABlockRef auto&& b)
  {
    return processor<std::decay_t<decltype(b)>>(b);
  }

  constexpr auto make_evaluator(ABlockRef auto&& b)
  {
    return evaluator<std::decay_t<decltype(b)>>(b);
  }

  template<ABlock Block>
  constexpr auto eval(Block block, AudioFrame<Block::in_channels> in)
  {
    return make_evaluator(block).eval(in);
  }

  // LITERAL ///////////////////////////////////////////

  struct Literal : BlockBase<Literal, 0, 1> {
    float value;
  };

  constexpr decltype(auto) wrap_literal(ABlockRef auto&& input)
  {
    return FWD(input);
  }

  constexpr Literal wrap_literal(float f)
  {
    return Literal{{}, f};
  }

  template<typename T>
  using wrapped_t = std::remove_cvref_t<decltype(wrap_literal(std::declval<T>()))>;

  template<>
  struct evaluator<Literal> {
    constexpr evaluator(Literal l) : literal_(l) {}

    constexpr AudioFrame<1> eval(AudioFrame<0>)
    {
      return {{literal_.value}};
    }

    Literal literal_;
  };

  namespace literals {
    constexpr Literal operator "" _eda(long double f) {
      return wrap_literal(f);
    }
    constexpr Literal operator "" _eda(unsigned long long f) {
      return wrap_literal(f);
    }
  }

  // CURRYING //////////////////////////////////////////

  template<ABlock Block, ABlock Input>
  requires(Input::out_channels <= Block::in_channels) //
    struct Curry : BlockBase<Curry<Block, Input>,
                             Input::in_channels + Block::in_channels - Input::out_channels,
                             Block::out_channels> {
    constexpr Curry(Block b, Input input) : block(b), input(input) {}

    Block block;
    Input input;
  };

  template<ABlock Block, ABlock Input>
  struct evaluator<Curry<Block, Input>> {
    constexpr evaluator(const Curry<Block, Input>& block) : block_(block.block), input_(block.input) {}

    constexpr AudioFrame<Curry<Block, Input>::out_channels> eval(AudioFrame<Curry<Block, Input>::in_channels> in)
    {
      return block_.eval(concat(input_.eval(splice<0, Input::in_channels>(in)), splice<Input::in_channels, -1>(in)));
    }

  private:
    evaluator<Block> block_;
    evaluator<Input> input_;
  };

  template<typename D, std::size_t I, std::size_t O>
  constexpr auto BlockBase<D, I, O>::operator()(auto&&... inputs) requires(sizeof...(inputs) <= I)
  {
    auto joined = (wrap_literal(FWD(inputs)), ...);
    return Curry<D, decltype(joined)>(static_cast<D&>(*this), std::move(joined));
  }

  // IDENT /////////////////////////////////////////////

  struct Ident : BlockBase<Ident, 1, 1> {};
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

  // CUT ///////////////////////////////////////////////

  struct Cut : BlockBase<Cut, 1, 0> {};

  constexpr Cut cut;

  template<>
  struct evaluator<Cut> {
    constexpr evaluator(Cut) {}
    static AudioFrame<0> eval(AudioFrame<1>)
    {
      return {};
    }
  };

  // PARALLEL //////////////////////////////////////////

  template<ABlock Lhs, ABlock Rhs>
  struct Parallel
    : BlockBase<Parallel<Lhs, Rhs>, Lhs::in_channels + Rhs::in_channels, Lhs::out_channels + Rhs::out_channels> {
    constexpr Parallel(Lhs l, Rhs r) : lhs(l), rhs(r) {}
    Lhs lhs;
    Rhs rhs;
  };

  template<typename Lhs, typename Rhs>
  Parallel(Lhs, Rhs) -> Parallel<std::decay_t<Lhs>, std::decay_t<Rhs>>;

  template<typename Lhs, typename Rhs>
  constexpr auto operator,(Lhs&& lhs, Rhs&& rhs) //
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
      return concat(l, r);
    }

  private:
    evaluator<Lhs> lhs_;
    evaluator<Rhs> rhs_;
  };

  // SERIAL ////////////////////////////////////////////

  template<ABlock Lhs, ABlock Rhs>
  requires(Lhs::out_channels == Rhs::in_channels) //
    struct Serial : BlockBase<Serial<Lhs, Rhs>, Lhs::in_channels, Rhs::out_channels> {
    constexpr Serial(Lhs l, Rhs r) : lhs(l), rhs(r) {}
    Lhs lhs;
    Rhs rhs;
  };

  template<typename Lhs, typename Rhs>
  Serial(Lhs, Rhs) -> Serial<std::decay_t<Lhs>, std::decay_t<Rhs>>;

  template<typename Lhs, typename Rhs>
  constexpr auto operator|(Lhs&& lhs, Rhs&& rhs) //
    requires(ABlockRef<Lhs> || ABlockRef<Rhs>)
  {
    // static_assert(std::decay_t<Lhs>::out_channels == std::decay_t<Rhs>::in_channels,
    //               "Serial operator requires Out channels of left hand side to equal in channels of right hand side");
    //               //
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

  // Split /////////////////////////////////////////////

  template<ABlock Lhs, ABlock Rhs>
  requires(Rhs::in_channels % Lhs::out_channels == 0) //
    struct Split : BlockBase<Split<Lhs, Rhs>, Lhs::in_channels, Rhs::out_channels> {
    constexpr Split(Lhs l, Rhs r) : lhs(l), rhs(r) {}
    Lhs lhs;
    Rhs rhs;
  };

  template<typename Lhs, typename Rhs>
  Split(Lhs, Rhs) -> Split<std::decay_t<Lhs>, std::decay_t<Rhs>>;

  template<typename Lhs, typename Rhs>
  constexpr auto operator<<(Lhs&& lhs, Rhs&& rhs) //
    requires(ABlockRef<Lhs> || ABlockRef<Rhs>)
  {
    return Split(wrap_literal(FWD(lhs)), wrap_literal(FWD(rhs)));
  }

  template<ABlock Lhs, ABlock Rhs>
  struct evaluator<Split<Lhs, Rhs>> {
    constexpr evaluator(const Split<Lhs, Rhs>& block) : lhs_(block.lhs), rhs_(block.rhs) {}

    constexpr AudioFrame<Split<Lhs, Rhs>::out_channels> eval(AudioFrame<Split<Lhs, Rhs>::in_channels> in)
    {
      auto l = lhs_.eval(in);
      AudioFrame<Rhs::in_channels> rhs_in;
      for (std::size_t i = 0; i < rhs_in.channels(); i++) {
        rhs_in[i] = l[i % l.channels()];
      }
      return rhs_.eval(rhs_in);
    }

  private:
    evaluator<Lhs> lhs_;
    evaluator<Rhs> rhs_;
  };

  // MERGE /////////////////////////////////////////////

  template<ABlock Lhs, ABlock Rhs>
  requires(Lhs::out_channels % Rhs::in_channels == 0) //
    struct Merge : BlockBase<Merge<Lhs, Rhs>, Lhs::in_channels, Rhs::out_channels> {
    constexpr Merge(Lhs l, Rhs r) : lhs(l), rhs(r) {}
    Lhs lhs;
    Rhs rhs;
  };

  template<typename Lhs, typename Rhs>
  Merge(Lhs, Rhs) -> Merge<std::decay_t<Lhs>, std::decay_t<Rhs>>;

  template<typename Lhs, typename Rhs>
  constexpr auto operator>>(Lhs&& lhs, Rhs&& rhs) //
    requires(ABlockRef<Lhs> || ABlockRef<Rhs>)
  {
    return Merge(wrap_literal(FWD(lhs)), wrap_literal(FWD(rhs)));
  }

  template<ABlock Lhs, ABlock Rhs>
  struct evaluator<Merge<Lhs, Rhs>> {
    constexpr evaluator(const Merge<Lhs, Rhs>& block) : lhs_(block.lhs), rhs_(block.rhs) {}

    constexpr AudioFrame<Merge<Lhs, Rhs>::out_channels> eval(AudioFrame<Merge<Lhs, Rhs>::in_channels> in)
    {
      auto lhs_out = lhs_.eval(in);
      AudioFrame<Rhs::in_channels> rhs_in;
      for (std::size_t i = 0; i < lhs_out.channels(); i++) {
        rhs_in[i % rhs_in.channels()] += lhs_out[i];
      }
      return rhs_.eval(rhs_in);
    }

  private:
    evaluator<Lhs> lhs_;
    evaluator<Rhs> rhs_;
  };

  // ARITHMETIC ////////////////////////////////////////

#define EDA_DEFINE_BLOCK_BINOP(Name, Operator)                                                                         \
  template<ABlock Lhs, ABlock Rhs>                                                                                     \
  requires(Lhs::out_channels == 1) && (Rhs::out_channels == 1) struct Name                                             \
    : BlockBase<Name<Lhs, Rhs>, Lhs::in_channels + Rhs::in_channels, 1> {                                              \
    constexpr Name(Lhs l, Rhs r) : lhs(l), rhs(r) {}                                                                   \
    Lhs lhs;                                                                                                           \
    Rhs rhs;                                                                                                           \
  };                                                                                                                   \
                                                                                                                       \
  template<typename Lhs, typename Rhs>                                                                                 \
  Name(Lhs, Rhs) -> Name<std::decay_t<Lhs>, std::decay_t<Rhs>>;                                                        \
                                                                                                                       \
  template<typename Lhs, typename Rhs>                                                                                 \
  constexpr auto operator Operator(Lhs&& lhs, Rhs&& rhs) requires(ABlockRef<Lhs> || ABlockRef<Rhs>)                    \
  {                                                                                                                    \
    return Name(wrap_literal(FWD(lhs)), wrap_literal(FWD(rhs)));                                                       \
  }                                                                                                                    \
                                                                                                                       \
  template<ABlock Lhs, ABlock Rhs>                                                                                     \
  struct evaluator<Name<Lhs, Rhs>> {                                                                                   \
    constexpr evaluator(const Name<Lhs, Rhs>& block) : lhs_(block.lhs), rhs_(block.rhs) {}                             \
    constexpr AudioFrame<1> eval(AudioFrame<Name<Lhs, Rhs>::in_channels> in)                                           \
    {                                                                                                                  \
      float l = lhs_.eval(splice<0, Lhs::in_channels>(in));                                                            \
      float r = rhs_.eval(splice<Lhs::in_channels, -1>(in));                                                           \
      return {l Operator r};                                                                                           \
    }                                                                                                                  \
                                                                                                                       \
  private:                                                                                                             \
    evaluator<Lhs> lhs_;                                                                                               \
    evaluator<Rhs> rhs_;                                                                                               \
  };

  EDA_DEFINE_BLOCK_BINOP(Plus, +)
  EDA_DEFINE_BLOCK_BINOP(Minus, -)
  EDA_DEFINE_BLOCK_BINOP(Times, *)
  EDA_DEFINE_BLOCK_BINOP(Division, /)

  // RECURSIVE /////////////////////////////////////////

  template<typename D, ABlock Lhs, ABlock Rhs, std::size_t In, std::size_t Out>
  struct BinOp : BlockBase<D, In, Out> {
    constexpr BinOp(Lhs l, Rhs r) : lhs(l), rhs(r) {}
    Lhs lhs;
    Rhs rhs;
  };

  template<ABlock Lhs, ABlock Rhs>
  requires(Rhs::in_channels <= Lhs::out_channels) && (Rhs::out_channels <= Lhs::in_channels) //
    struct Recursive : BinOp<Recursive<Lhs, Rhs>, Lhs, Rhs, Lhs::in_channels - Rhs::out_channels, Lhs::out_channels> {};

  template<typename Lhs, typename Rhs>
  constexpr auto operator%(Lhs&& lhs, Rhs&& rhs) requires(ABlockRef<Lhs> || ABlockRef<Rhs>)
  {
    return Recursive<wrapped_t<Lhs>, wrapped_t<Rhs>>{{wrap_literal(FWD(lhs)), wrap_literal(FWD(rhs))}};
  }

  template<ABlock Lhs, ABlock Rhs>
  struct evaluator<Recursive<Lhs, Rhs>> {
    constexpr evaluator(const Recursive<Lhs, Rhs>& block) : lhs_(block.lhs), rhs_(block.rhs) {}
    constexpr AudioFrame<Recursive<Lhs, Rhs>::out_channels> eval(AudioFrame<Recursive<Lhs, Rhs>::in_channels> in)
    {
      auto l_out = lhs_.eval(concat(memory_, in));
      memory_ = rhs_.eval(splice<0, Rhs::in_channels>(l_out));
      return l_out;
    }

  private:
    AudioFrame<Rhs::out_channels> memory_;
    evaluator<Lhs> lhs_;
    evaluator<Rhs> rhs_;
  };

  // MEM ///////////////////////////////////////////////
   
  struct Mem : BlockBase<Mem, 1, 1> {};
  constexpr Mem mem;

  template<>
  struct evaluator<Mem> {
    constexpr evaluator(const Mem&) {}
    constexpr AudioFrame<Mem::out_channels> eval(AudioFrame<Mem::in_channels> in)
    {
      auto res = memory_;
      memory_ = in;
      return res;
    }

    AudioFrame<1> memory_;
  };

} // namespace topisani::eda
