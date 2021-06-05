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
  
  template<typename Test, template<typename...> class Ref>
  struct is_instance : std::false_type {};

  template<template<typename...> class Ref, typename... Args>
  struct is_instance<Ref<Args...>, Ref>: std::true_type {};

  /// Check whether T is a instance of the template Template
  /// Does not work when Template has non-type template parameters
  template<typename T, template<typename...> typename Template>
  concept instance_of = is_instance<T, Template>::value;

  template<typename T>
  concept ATuple = instance_of<T, std::tuple>;

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
