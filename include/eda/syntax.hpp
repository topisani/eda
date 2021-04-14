#pragma once

#include "eda/block.hpp"

namespace topisani::eda::syntax {

  // LITERAL ///////////////////////////////////////////
   
  constexpr Literal operator"" _eda(long double f)
  {
    return wrap_literal(static_cast<float>(f));
  }
  constexpr Literal operator"" _eda(unsigned long long f)
  {
    return wrap_literal(static_cast<float>(f));
  }

  // IDENT /////////////////////////////////////////////

  constexpr Ident _ = ident;

  // PARALLEL //////////////////////////////////////////

  template<typename Lhs, typename Rhs>
  constexpr auto operator,(Lhs&& lhs, Rhs&& rhs) //
    requires(ABlockRef<Lhs> || ABlockRef<Rhs>)
  {
    return parallel(wrap_literal(FWD(lhs)), wrap_literal(FWD(rhs)));
  }

  // SEQUENTIAL ////////////////////////////////////////

  template<typename Lhs, typename Rhs>
  constexpr auto operator|(Lhs&& lhs, Rhs&& rhs) //
    requires(ABlockRef<Lhs> || ABlockRef<Rhs>)
  {
    return sequential(wrap_literal(FWD(lhs)), wrap_literal(FWD(rhs)));
  }

  // Split /////////////////////////////////////////////

  template<typename Lhs, typename Rhs>
  constexpr auto operator<<(Lhs&& lhs, Rhs&& rhs) //
    requires(ABlockRef<Lhs> || ABlockRef<Rhs>)
  {
    return split(wrap_literal(FWD(lhs)), wrap_literal(FWD(rhs)));
  }

  // MERGE /////////////////////////////////////////////

  template<typename Lhs, typename Rhs>
  constexpr auto operator>>(Lhs&& lhs, Rhs&& rhs) //
    requires(ABlockRef<Lhs> || ABlockRef<Rhs>)
  {
    return merge(wrap_literal(FWD(lhs)), wrap_literal(FWD(rhs)));
  }

  // ARITHMETIC ////////////////////////////////////////

  template<typename Lhs, typename Rhs>
  constexpr auto operator+(Lhs&& lhs, Rhs&& rhs) requires(ABlockRef<Lhs> || ABlockRef<Rhs>)
  {
    return plus(lhs, rhs);
  }

  template<typename Lhs, typename Rhs>
  constexpr auto operator-(Lhs&& lhs, Rhs&& rhs) requires(ABlockRef<Lhs> || ABlockRef<Rhs>)
  {
    return minus(lhs, rhs);
  }

  template<typename Lhs, typename Rhs>
  constexpr auto operator*(Lhs&& lhs, Rhs&& rhs) requires(ABlockRef<Lhs> || ABlockRef<Rhs>)
  {
    return times(lhs, rhs);
  }

  template<typename Lhs, typename Rhs>
  constexpr auto operator/(Lhs&& lhs, Rhs&& rhs) requires(ABlockRef<Lhs> || ABlockRef<Rhs>)
  {
    return divide(lhs, rhs);
  }

  // RECURSIVE /////////////////////////////////////////

  template<typename Lhs, typename Rhs>
  constexpr auto operator%(Lhs&& lhs, Rhs&& rhs) requires(ABlockRef<Lhs> || ABlockRef<Rhs>)
  {
    return recursive(wrap_literal(FWD(lhs)), wrap_literal(FWD(rhs)));
  }

} // namespace topisani::eda::syntax
