# RingBuffer

## Overview

`RingBuffer` is a thread-safe circular buffer implementation in C++. It provides efficient FIFO (First-In-First-Out) operations with methods for pushing and pulling elements while ensuring synchronization using mutexes and condition variables. The advantage over STL containter is, it 

The project includes a test application (`ringbuffer_test.cpp`) that demonstrates its thread safety by spawning `N` worker threads (defined by command-line argument `argv[1]`). These workers push integer values into the buffer, while the main thread pulls elements out. The test verifies correctness by comparing the sum of pushed and pulled elements.

## Features
- Thread-safe push and pull operations
- Supports both static (`std::array`) and dynamic (`std::unique_ptr`) storage types (just pick one of `StorageType::Static` or `StorageType::Dynamic` as the class template argument)
- Uses condition variables to manage synchronization between producer and consumer threads
- In opposite to typical STL containers it doesn't do any further heap allocations during push and pull operations by simply moving/wraping read and write pointers around the storage
- Fixed-size buffer, meaning it can become full if not drained at a sufficient speed

## Build and Run Instructions

### Prerequisites
- C++17 or later
- CMake
- A compatible compiler (GCC, Clang, MSVC, etc.)

### Build Instructions
1. Clone the repository:
   ```sh
   git clone https://github.com/sp1mdo/RingBuffer.git
   cd RingBuffer/
   ```
2. Create a build directory and navigate to it:
   ```sh
   mkdir build && cd build
   ```
3. Run CMake to configure the project:
   ```sh
   cmake ..
   ```
4. Compile the project:
   ```sh
   make
   ```

### Run the Test Application
After building, run the test program with the number of worker threads as an argument:
```sh
./ringbuffer_test <N>
```
For example, to run with 100 worker threads:
```sh
./ringbuffer_test 100
```

The application verifies thread safety by ensuring the sum of all pushed elements matches the sum of all pulled elements.

## Contributions
Contributions are welcome! Feel free to submit pull requests or report issues.

