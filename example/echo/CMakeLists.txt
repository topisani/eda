set(CMAKE_CXX_STANDARD 20)

set(sources "echo.cpp")

macro(add_lv2_fx name)
  add_library(${name} MODULE
    ${ARGN})
  set_target_properties(${name} PROPERTIES
    PREFIX "" SUFFIX ".fx"
    LIBRARY_OUTPUT_NAME "${name}"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lv2/${name}.lv2"
    C_VISIBILITY_PRESET "hidden"
    CXX_VISIBILITY_PRESET "hidden")
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/manifest.ttl" 
    "${PROJECT_BINARY_DIR}/lv2/${name}.lv2/manifest.ttl"
    COPYONLY)
  target_link_libraries(${name} PUBLIC topisani::eda)
  install(DIRECTORY "${PROJECT_BINARY_DIR}/lv2" DESTINATION "lib")
endmacro()

add_lv2_fx(echo "${sources}")

target_link_libraries(echo PUBLIC eda)
