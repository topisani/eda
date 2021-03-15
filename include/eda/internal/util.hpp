#pragma once
#include <utility>

#define FWD(X) std::forward<decltype(X)>(X)
