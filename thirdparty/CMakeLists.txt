include(FetchContent)

# jmixin
FetchContent_Declare(
  jmixin
  GIT_REPOSITORY https://github.com/jeffersonfr/jmixin.git
)
FetchContent_MakeAvailable(jmixin)

# jinject
FetchContent_Declare(
  jinject
  GIT_REPOSITORY https://github.com/jeffersonfr/jinject.git
  GIT_TAG main
)
FetchContent_MakeAvailable(jinject)

# jcanvas
FetchContent_Declare(
  jcanvas
  GIT_REPOSITORY https://github.com/jeffersonfr/jcanvas.git
  GIT_TAG main
)
FetchContent_MakeAvailable(jinject)

# fmt
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
)
FetchContent_MakeAvailable(fmt)

# expected
FetchContent_Declare(
  expected
  GIT_REPOSITORY https://github.com/TartanLlama/expected.git
)
FetchContent_MakeAvailable(expected)

FetchContent_GetProperties(expected)
if(expected_POPULATED)
  message("SOURCE_DIR: ${expected_SOURCE_DIR}")
  message("BINARY_DIR: ${expected_BINARY_DIR}")
endif()
