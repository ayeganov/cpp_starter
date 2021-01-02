# Generic test that uses conan libs
add_executable(solve main.cpp)
target_link_libraries(
  solve
  PRIVATE project_options
          project_warnings
          CONAN_PKG::docopt.cpp
          CONAN_PKG::fmt
          CONAN_PKG::spdlog)
