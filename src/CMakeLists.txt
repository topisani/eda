set(CMAKE_CXX_STANDARD 20)

set(sources "")

add_library(eda INTERFACE)
add_library(topisani::eda ALIAS eda)
target_include_directories(eda INTERFACE "${EDA_SOURCE_DIR}/include")

target_link_libraries(eda INTERFACE mdspan)
