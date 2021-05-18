#pragma once
#include <utility>

#define FWD(X) std::forward<decltype(X)>(X)

namespace eda::util {

  namespace detail {
    template<typename T>
    struct function_ptr_impl;
    template<typename Ret, typename... Args>
    struct function_ptr_impl<Ret(Args...)> {
      using type = Ret (*)(Args...);
    };
    template<typename Ret, typename... Args>
    struct function_ptr_impl<Ret(Args...) noexcept> {
      using type = Ret (*)(Args...) noexcept;
    };
  } // namespace detail

  template<typename Func>
  using function_ptr = typename detail::function_ptr_impl<Func>::type;

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


  namespace detail {
    template<typename T, typename Func>
    struct is_callable : std::false_type {};
    
    template<typename T, typename Ret, typename... Args>
    struct is_callable<T, Ret(Args...)> : std::is_invocable_r<Ret, T, Args...> {};
  }

  template<typename T, typename Func>
  concept Callable = detail::is_callable<T, Func>::value;

} // namespace topisani::eda::util
