set(CMAKE_CXX_STANDARD 20)

set(sources 
  main.cpp
  block.cpp
  benchmarks.cpp
)

add_executable(tests ${sources})
target_link_libraries(tests PRIVATE topisani::eda)
target_link_libraries(tests PRIVATE Catch2::Catch2)
