#pragma once

#include "eda/block.hpp"

namespace eda::syntax {

  // LITERAL ///////////////////////////////////////////

  constexpr Literal operator"" _eda(long double f) noexcept
  {
    return as_block(static_cast<float>(f));
  }
  constexpr Literal operator"" _eda(unsigned long long f) noexcept
  {
    return as_block(static_cast<float>(f));
  }

  // IDENT /////////////////////////////////////////////

  constexpr Ident<1> _ = ident<1>;
  constexpr Cut<1> $ = cut<1>;

  // PARALLEL //////////////////////////////////////////

  template<typename Lhs, typename Rhs>
  constexpr auto operator,(Lhs&& lhs, Rhs&& rhs) noexcept //
    requires(AnyBlockRef<Lhs> || AnyBlockRef<Rhs>)
  {
    return par(as_block(FWD(lhs)), as_block(FWD(rhs)));
  }

  // SEQUENTIAL ////////////////////////////////////////

  template<typename Lhs, typename Rhs>
  constexpr auto operator|(Lhs&& lhs, Rhs&& rhs) noexcept //
    requires(AnyBlockRef<Lhs> || AnyBlockRef<Rhs>)
  {
    return seq(as_block(FWD(lhs)), as_block(FWD(rhs)));
  }

  // Split /////////////////////////////////////////////

  template<typename Lhs, typename Rhs>
  constexpr auto operator<<(Lhs&& lhs, Rhs&& rhs) noexcept //
    requires(AnyBlockRef<Lhs> || AnyBlockRef<Rhs>)
  {
    return split(as_block(FWD(lhs)), as_block(FWD(rhs)));
  }

  // MERGE /////////////////////////////////////////////

  template<typename Lhs, typename Rhs>
  constexpr auto operator>>(Lhs&& lhs, Rhs&& rhs) noexcept //
    requires(AnyBlockRef<Lhs> || AnyBlockRef<Rhs>)
  {
    return merge(as_block(FWD(lhs)), as_block(FWD(rhs)));
  }

  // ARITHMETIC ////////////////////////////////////////

  template<typename Lhs, typename Rhs>
  constexpr auto operator+(Lhs&& lhs, Rhs&& rhs) noexcept requires(AnyBlockRef<Lhs> || AnyBlockRef<Rhs>)
  {
    return plus(lhs, rhs);
  }

  template<typename Lhs, typename Rhs>
  constexpr auto operator-(Lhs&& lhs, Rhs&& rhs) noexcept requires(AnyBlockRef<Lhs> || AnyBlockRef<Rhs>)
  {
    return minus(lhs, rhs);
  }

  template<typename Lhs, typename Rhs>
  constexpr auto operator*(Lhs&& lhs, Rhs&& rhs) noexcept requires(AnyBlockRef<Lhs> || AnyBlockRef<Rhs>)
  {
    return times(lhs, rhs);
  }

  template<typename Lhs, typename Rhs>
  constexpr auto operator/(Lhs&& lhs, Rhs&& rhs) noexcept requires(AnyBlockRef<Lhs> || AnyBlockRef<Rhs>)
  {
    return divide(lhs, rhs);
  }

  // RECURSIVE /////////////////////////////////////////

  template<typename Lhs, typename Rhs>
  constexpr auto operator%(Lhs&& lhs, Rhs&& rhs) noexcept requires(AnyBlockRef<Lhs> || AnyBlockRef<Rhs>)
  {
    return rec(as_block(FWD(lhs)), as_block(FWD(rhs)));
  }

} // namespace eda::syntax
