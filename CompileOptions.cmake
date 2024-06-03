# I'm just making this a separate file, so both the framework and
# projects can use it.

function(setCompileOptions target)
  target_compile_features(${target} PRIVATE cxx_std_20)

  # Keep assert() in the release build
  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${target} PRIVATE -UNDEBUG)
  endif()

  # Sanitizer (doesn't really work, though)
  # if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  #   if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  #     target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-sanitize=leak)
  #     target_link_options(${target} PRIVATE -fsanitize=address,undefined -fno-sanitize=leak)
  #   endif()
  # endif()

  # Set warning options
  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${target} PRIVATE
      -Wall -Wextra -Wpedantic -Werror -Wformat=2 -Wformat-overflow=2 -Wformat-signedness -Wswitch-default
      -Wuninitialized -Wduplicated-branches -Wfloat-equal -Wshadow -Wcast-qual -Wcast-align -Wsign-conversion
      -Wlogical-op -Wmissing-declarations -Wno-psabi
    )
  endif()

  if (MSVC)
    target_compile_options(${target} PRIVATE "/EHsc" "-D_WIN32_WINNT=0x0601" "/Zc:__cplusplus")
  endif (MSVC)

endfunction()
