# Distributed Shared Memory (DSM) Middleware

This project implements a Distributed Shared Memory middleware using C++ and gRPC. Currently, the **Network Layer** is fully implemented, allowing nodes to communicate via the defined network interface.

## Prerequisites

Before building, ensure you have the following installed:

- **C++ Compiler** (MSVC recommended for Windows)
- **CMake** (Version 3.10 or higher)
- **gRPC** and **Protobuf** installed (via `vcpkg` is recommended)

## Building the Project

This project uses CMake. Follow these steps to build the application from the source.

1.  **Create a build directory:**
    From the project root terminal:

    ```bash
    mkdir build
    cd build
    ```

2.  **Configure the project:**
    Run CMake to generate the build files. If you are using `vcpkg`, ensure it is linked correctly (usually automatic if environment variables are set, otherwise specify the toolchain file).

    ```bash
    cmake ..
    ```

3.  **Compile the code:**
    Build the executable (Debug configuration is default on Windows):
    ```bash
    cmake --build .
    ```
    _Note: To build in Release mode, use: `cmake --build . --config Release`_

## Running the Application

After a successful build, the executable will be located in the `build/Debug` (or `build/Release`) folder.

1.  **Navigate to the executable:**

    ```bash
    cd Debug
    ```

2.  **Run the Server:**
    ```bash
    .\grpc-server.exe
    ```

### Expected Output

Upon running, the terminal should display logs indicating the network interface has started:

```text
[INFO] Network Interface initialized.
[INFO] Server listening on 0.0.0.0:50051 (or configured port)
...
```
