cmake_minimum_required(VERSION 3.20)
project(rpc-demo C CXX)

# 1. Fetch Nanopb
include(FetchContent)
FetchContent_Declare(
  nanopb
  GIT_REPOSITORY https://github.com/nanopb/nanopb.git
  GIT_TAG nanopb-0.4.9.1
)
FetchContent_MakeAvailable(nanopb)

set(dir_pw_third_party_nanopb "${nanopb_SOURCE_DIR}" CACHE PATH "" FORCE)

set(PIGWEED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/pigweed")

# 2. Include Pigweed CMake helper
include(${PIGWEED_DIR}/pw_build/pigweed.cmake)

# 3. Include backend variables
include(${PIGWEED_DIR}/pw_log/backend.cmake)
include(${PIGWEED_DIR}/pw_assert/backend.cmake)
include(${PIGWEED_DIR}/pw_assert_basic/backend.cmake)
include(${PIGWEED_DIR}/pw_sys_io/backend.cmake)
include(${PIGWEED_DIR}/pw_sync/backend.cmake)
include(${PIGWEED_DIR}/pw_thread/backend.cmake)
include(${PIGWEED_DIR}/pw_chrono/backend.cmake)

# 4. Set Backends for host execution
pw_set_backend(pw_log pw_log_basic)
pw_set_backend(pw_assert.check pw_assert_basic.check_backend)
pw_set_backend(pw_assert.assert pw_assert.print_and_abort_assert_backend)
pw_set_backend(pw_assert_basic.handler pw_assert_basic.basic_handler)
pw_set_backend(pw_sys_io pw_sys_io_stdio)
pw_set_backend(pw_sync.mutex pw_sync_stl.mutex_backend)
pw_set_backend(pw_sync.interrupt_spin_lock pw_sync_stl.interrupt_spin_lock)
pw_set_backend(pw_chrono.system_clock pw_chrono_stl.system_clock)
pw_set_backend(pw_thread.sleep pw_thread_stl.sleep)
pw_set_backend(pw_thread.id pw_thread_stl.id)
pw_set_backend(pw_thread.thread pw_thread_stl.thread)
pw_set_backend(pw_thread.yield pw_thread_stl.yield)

# Disable experimental editions globally for Pigweed proto compiler
set(pw_protobuf_compiler_GENERATE_PROTOS_ARGS "--no-experimental-editions" CACHE STRING "" FORCE)

# Disable pw_rpc global mutex to avoid deadlock in synchronous loopback
add_compile_definitions(PW_RPC_USE_GLOBAL_MUTEX=0)

# 5. Add Pigweed
add_subdirectory(${PIGWEED_DIR} pigweed)

# 6. Find protoc
find_program(PROTOC protoc REQUIRED)

# 7. Generate python proto modules needed by Pigweed plugins
set(PYTHON_PACKAGES_DIR "${CMAKE_CURRENT_BINARY_DIR}/python_packages")
file(MAKE_DIRECTORY ${PYTHON_PACKAGES_DIR}/pw_protobuf_codegen_protos)
file(MAKE_DIRECTORY ${PYTHON_PACKAGES_DIR}/pw_protobuf_protos)

add_custom_command(
  OUTPUT ${PYTHON_PACKAGES_DIR}/pw_protobuf_codegen_protos/codegen_options_pb2.py
  COMMAND ${PROTOC}
          --python_out=${PYTHON_PACKAGES_DIR}
          -I${PIGWEED_DIR}/pw_protobuf
          ${PIGWEED_DIR}/pw_protobuf/pw_protobuf_codegen_protos/codegen_options.proto
  DEPENDS ${PIGWEED_DIR}/pw_protobuf/pw_protobuf_codegen_protos/codegen_options.proto
)

add_custom_command(
  OUTPUT
    ${PYTHON_PACKAGES_DIR}/pw_protobuf_protos/common_pb2.py
    ${PYTHON_PACKAGES_DIR}/pw_protobuf_protos/status_pb2.py
    ${PYTHON_PACKAGES_DIR}/pw_protobuf_protos/field_options_pb2.py
  COMMAND ${PROTOC}
          --python_out=${PYTHON_PACKAGES_DIR}
          -I${PIGWEED_DIR}/pw_protobuf
          ${PIGWEED_DIR}/pw_protobuf/pw_protobuf_protos/common.proto
          ${PIGWEED_DIR}/pw_protobuf/pw_protobuf_protos/status.proto
          ${PIGWEED_DIR}/pw_protobuf/pw_protobuf_protos/field_options.proto
  DEPENDS
    ${PIGWEED_DIR}/pw_protobuf/pw_protobuf_protos/common.proto
    ${PIGWEED_DIR}/pw_protobuf/pw_protobuf_protos/status.proto
    ${PIGWEED_DIR}/pw_protobuf/pw_protobuf_protos/field_options.proto
)

add_custom_command(
  OUTPUT ${PYTHON_PACKAGES_DIR}/pw_rpc/internal/packet_pb2.py
  COMMAND ${PROTOC}
          --python_out=${PYTHON_PACKAGES_DIR}
          -I${PIGWEED_DIR}
          ${PIGWEED_DIR}/pw_rpc/internal/packet.proto
  DEPENDS ${PIGWEED_DIR}/pw_rpc/internal/packet.proto
)

add_custom_target(generate_python_protos
  DEPENDS
    ${PYTHON_PACKAGES_DIR}/pw_protobuf_codegen_protos/codegen_options_pb2.py
    ${PYTHON_PACKAGES_DIR}/pw_protobuf_protos/common_pb2.py
    ${PYTHON_PACKAGES_DIR}/pw_protobuf_protos/status_pb2.py
    ${PYTHON_PACKAGES_DIR}/pw_protobuf_protos/field_options_pb2.py
    ${PYTHON_PACKAGES_DIR}/pw_rpc/internal/packet_pb2.py
)

# Add dependency to Pigweed internal proto targets
if(TARGET pw_rpc.protos._generate.pwpb)
  add_dependencies(pw_rpc.protos._generate.pwpb generate_python_protos)
endif()
if(TARGET pw_rpc.protos._generate.nanopb)
  add_dependencies(pw_rpc.protos._generate.nanopb generate_python_protos)
endif()
if(TARGET pw_rpc.protos._generate.nanopb_rpc)
  add_dependencies(pw_rpc.protos._generate.nanopb_rpc generate_python_protos)
endif()

# 8. Manual Proto Generation Setup for User Proto
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${GENERATED_DIR})

# Construct PYTHONPATH for Pigweed RPC plugin
set(PW_PYTHON_PATH
  ${PYTHON_PACKAGES_DIR}
  ${PIGWEED_DIR}/pw_rpc/py
  ${PIGWEED_DIR}/pw_status/py
  ${PIGWEED_DIR}/pw_protobuf/py
  ${PIGWEED_DIR}/pw_protobuf_compiler/py
  ${PIGWEED_DIR}/pw_stream/py
  ${PIGWEED_DIR}/pw_log/py
)
string(REPLACE ";" ":" PW_PYTHON_PATH_STR "${PW_PYTHON_PATH}")

# Command to generate nanopb .pb.h/.pb.c
add_custom_command(
  OUTPUT
    ${GENERATED_DIR}/ping.pb.h
    ${GENERATED_DIR}/ping.pb.c
  COMMAND ${CMAKE_COMMAND} -E env "PYTHONPATH=${nanopb_SOURCE_DIR}/generator"
          ${PROTOC}
          --plugin=protoc-gen-nanopb=${nanopb_SOURCE_DIR}/generator/protoc-gen-nanopb
          --nanopb_out=${GENERATED_DIR}
          -I.
          ping.proto
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ping.proto ping.options
)

# Command to generate pw_rpc .rpc.pb.h
add_custom_command(
  OUTPUT
    ${GENERATED_DIR}/ping.rpc.pb.h
  COMMAND ${CMAKE_COMMAND} -E env "PYTHONPATH=${PW_PYTHON_PATH_STR}"
          ${PROTOC}
          --plugin=protoc-gen-custom=${PIGWEED_DIR}/pw_rpc/py/pw_rpc/plugin_nanopb.py
          --custom_out=${GENERATED_DIR}
          -I.
          ping.proto
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ping.proto ${GENERATED_DIR}/ping.pb.h
)

# Create library for generated protos
add_library(ping_protos STATIC
  ${GENERATED_DIR}/ping.pb.c
  ${GENERATED_DIR}/ping.pb.h
  ${GENERATED_DIR}/ping.rpc.pb.h
)
target_include_directories(ping_protos PUBLIC ${GENERATED_DIR})
target_link_libraries(ping_protos
  PUBLIC
    protobuf-nanopb-static
    pw_rpc.nanopb.client_api
    pw_rpc.nanopb.server_api
    pw_rpc.server
)

# 9. Define Demo Executable
add_executable(rpc_demo main.cc)
target_link_libraries(rpc_demo
  PRIVATE
    ping_protos
    pw_rpc.client
    pw_rpc.server
    pw_log
)
add_dependencies(rpc_demo generate_python_protos)
