cmake_minimum_required(VERSION 3.10)
project(motor_stand C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(BUILD_GUI "Build GTK-based GUI (requires gtk+-3.0 pkg-config)" ON)

find_package(PkgConfig)

if (PkgConfig_FOUND)
  pkg_check_modules(GTK3 QUIET gtk+-3.0)
endif()

# Common source groups
set(SRC_COMMON
    src/numerical_basics.c
    src/method_of_least_squares.c
    src/load_cell_cli.c
    src/load_cell.h
)

if(UNIX)
  add_executable(load_cell_serv src/load_cell_serv.c ${SRC_COMMON})
  target_include_directories(load_cell_serv PRIVATE src)

  if(UNIX AND NOT APPLE)
    # Try to find pigpio on Linux
    find_library(PIGPIO_LIB pigpio)
    if(PIGPIO_LIB)
      target_link_libraries(load_cell_serv PRIVATE ${PIGPIO_LIB})
      target_compile_definitions(load_cell_serv PRIVATE HAVE_PIGPIO=1)
    else()
      # compile the compatibility stubs when pigpio isn't found
      target_sources(load_cell_serv PRIVATE src/gpio_compat.c)
    endif()
    find_library(RT_LIB rt)
    if(RT_LIB)
      target_link_libraries(load_cell_serv PRIVATE ${RT_LIB})
    endif()
  else()
    # non-Linux UNIX (e.g. macOS) compile compat stubs
    target_sources(load_cell_serv PRIVATE src/gpio_compat.c)
  endif()

  target_compile_options(load_cell_serv PRIVATE -pthread)
endif()

if(UNIX)
  add_executable(load_cell_logger src/load_cell_logger.c ${SRC_COMMON})
  target_include_directories(load_cell_logger PRIVATE src)
  target_compile_options(load_cell_logger PRIVATE -pthread)

  add_executable(load_cell_actor src/load_cell_actor.c ${SRC_COMMON})
  target_include_directories(load_cell_actor PRIVATE src)
  target_compile_options(load_cell_actor PRIVATE -pthread)
endif()

add_executable(output_rewrite src/output_rewrite.c)
target_include_directories(output_rewrite PRIVATE src)

if(BUILD_GUI AND GTK3_FOUND)
  if(UNIX)
    add_executable(load_cell_visual src/teststandui.c src/load_cell_cli.c)
  else()
    add_executable(load_cell_visual src/teststandui.c src/load_cell_cli_stub.c)
  endif()
  target_include_directories(load_cell_visual PRIVATE src ${GTK3_INCLUDE_DIRS})
  target_compile_options(load_cell_visual PRIVATE ${GTK3_CFLAGS_OTHER})
  target_link_libraries(load_cell_visual PRIVATE ${GTK3_LIBRARIES} m)
else()
  message(STATUS "GTK3 not found via pkg-config; GUI target will not be built unless GTK3 is available.")
endif()

message(STATUS "Configured project '${PROJECT_NAME}'. Use 'cmake -S . -B build' then 'cmake --build build' to build.")
