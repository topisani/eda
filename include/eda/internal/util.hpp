#pragma once
#include <utility>

#define FWD(X) std::forward<decltype(X)>(X)

namespace topisani::eda::util {
  template<typename T>
  struct is_tuple : std::false_type {};
  template<typename... Ts>
  struct is_tuple<std::tuple<Ts...>> : std::true_type {};

  template<typename T>
  concept ATuple = is_tuple<T>::value;

  template<typename T, ATuple Tup>
  struct is_constructible_through_apply;

  template<typename T, typename... Ts>
  struct is_constructible_through_apply<T, std::tuple<Ts...>> : std::is_constructible<T, Ts...> {};

  template<typename T, typename Tuple>
  concept ConstructibleThroughApply = is_constructible_through_apply<T, Tuple>::value;

  template<typename T, typename U>
  concept decays_to = std::is_same_v<std::decay_t<T>, U>;

} // namespace topisani::eda::util
