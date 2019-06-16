cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)
project(gif LANGUAGES C)
add_library(gif "src/gif_reader.c" "src/gif_writer.c")
add_dependencies(gif lzw darray)
target_include_directories(gif PUBLIC "src/")
target_compile_options(gif
  PRIVATE "-std=c89"
  PRIVATE "-pedantic-errors"
  PRIVATE "-Wall"
  PRIVATE "-Wconversion"
  )
target_link_libraries(gif lzw darray)
