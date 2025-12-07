cmake_minimum_required(VERSION 3.20)
project(grpc-server)

# 1. Setup Dependencies
find_package(gRPC CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)

# 2. Define the Proto file (UPDATED PATH)
set(PROTO_FILE "protos/dsm_proto/dsm.proto") 

# 3. Create the Executable
add_executable(${PROJECT_NAME} 
    src/main.cpp
    src/service/dsm_service.cpp   # <--- ADD THIS
    src/server/server.cpp           # <--- ADD THIS
    ${PROTO_FILE}
)

# 4. Generate the Code
get_target_property(GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin LOCATION)

# Step A: Generate standard Protobuf messages (.pb.cc / .pb.h)
protobuf_generate(
    TARGET ${PROJECT_NAME}
    LANGUAGE cpp
)

# Step B: Generate gRPC services (.grpc.pb.cc / .grpc.pb.h)
protobuf_generate(
    TARGET ${PROJECT_NAME}
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN "protoc-gen-grpc=${GRPC_CPP_PLUGIN}"
)

# 5. Link Libraries and Includes
target_include_directories(${PROJECT_NAME} PUBLIC 
    ${CMAKE_CURRENT_BINARY_DIR}         # For the auto-generated proto files
    ${CMAKE_CURRENT_SOURCE_DIR}/include # For your own headers (config, server, service)
)
target_link_libraries(${PROJECT_NAME} PRIVATE gRPC::grpc++ protobuf::libprotobuf absl::base absl::synchronization absl::strings)