cmake_minimum_required(VERSION 3.15)
project(DSMCore)

set(CMAKE_CXX_STANDARD 17)

# --- FIX: Load gRPC FIRST, then Protobuf ---
# gRPC internally calls find_package(Protobuf), so loading it first prevents conflicts.
find_package(gRPC CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)

# ---------------------------------------------
# Proto Generation Logic
# ---------------------------------------------
set(PROTO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/protos")
set(PROTO_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${PROTO_BINARY_DIR})

set(PROTO_FILES "${PROTO_SRC_DIR}/dsm.proto")

# Helper to generate names
get_filename_component(dsm_proto_name "${PROTO_FILES}" NAME_WE)
set(dsm_proto_srcs "${PROTO_BINARY_DIR}/${dsm_proto_name}.pb.cc")
set(dsm_proto_hdrs "${PROTO_BINARY_DIR}/${dsm_proto_name}.pb.h")
set(dsm_grpc_srcs "${PROTO_BINARY_DIR}/${dsm_proto_name}.grpc.pb.cc")
set(dsm_grpc_hdrs "${PROTO_BINARY_DIR}/${dsm_proto_name}.grpc.pb.h")

# We use $<TARGET_FILE:protobuf::protoc> to ensure we get the correct executable
add_custom_command(
        OUTPUT "${dsm_proto_srcs}" "${dsm_proto_hdrs}" "${dsm_grpc_srcs}" "${dsm_grpc_hdrs}"
        COMMAND $<TARGET_FILE:protobuf::protoc>
        ARGS --grpc_out "${PROTO_BINARY_DIR}"
        --cpp_out "${PROTO_BINARY_DIR}"
        -I "${PROTO_SRC_DIR}"
        --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
        "${PROTO_FILES}"
        DEPENDS "${PROTO_FILES}"
)

# ---------------------------------------------
# Build Executable
# ---------------------------------------------
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${PROTO_BINARY_DIR}
)

add_executable(dsm_node
        main.cpp
        src/dsm/DsmCore.cpp
        src/net/GrpcDsmNetwork.cpp
        src/net/DsmServiceImpl.cpp
        src/sync/LockManager.cpp
        ${dsm_proto_srcs}
        ${dsm_grpc_srcs}
)

target_link_libraries(dsm_node
        protobuf::libprotobuf
        gRPC::grpc++
)



add_executable(demo_gui
        demo_gui.cpp
        src/dsm/DsmCore.cpp
        src/net/GrpcDsmNetwork.cpp
        src/net/DsmServiceImpl.cpp
        src/sync/LockManager.cpp
        ${dsm_proto_srcs}
        ${dsm_grpc_srcs}
)

target_include_directories(demo_gui PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${PROTO_BINARY_DIR}
)

target_link_libraries(demo_gui PRIVATE
        protobuf::libprotobuf
        gRPC::grpc++
)

add_executable(dsm_headless
        dsm_headless.cpp
        src/dsm/DsmCore.cpp
        src/net/GrpcDsmNetwork.cpp
        src/net/DsmServiceImpl.cpp
        src/sync/LockManager.cpp
        ${dsm_proto_srcs}
        ${dsm_grpc_srcs}
)

target_include_directories(dsm_headless PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${PROTO_BINARY_DIR}
)

target_link_libraries(dsm_headless PRIVATE
        protobuf::libprotobuf
        gRPC::grpc++
)