# Copy_Tool

A multithreaded command-line file copy utility written in modern C++20. It uses a **producer–consumer** design: one thread reads the source file in chunks while another writes those chunks to the destination, connected by a bounded, thread-safe queue.

## Features

- Concurrent read and write using `std::jthread`
- Bounded buffer (`std::queue` + `std::mutex` + two `std::condition_variable`s) so memory use stays limited
- Configurable chunk size
- Safe overwrite behaviour: refuses to replace an existing destination unless `-f` is given
- Guards against copying a file onto itself
- Exceptions from either thread are captured and reported cleanly in `main`
- Binary-safe copying (files are opened with `std::ios::binary`)

## How It Works

```
 ┌────────┐   chunks    ┌──────────────────┐   chunks    ┌────────┐
 │ Reader │ ──────────▶ │ Bounded queue    │ ──────────▶ │ Writer │
 │ thread │  not_full   │ (mutex + 2 CVs)  │  not_empty  │ thread │
 └────────┘             └──────────────────┘             └────────┘
```

- **Reader** (`src/reader.cpp`): reads the source file in `chunk_size` byte blocks and pushes them onto the queue. It waits on `not_full` when the queue is full. A scope guard always sets `done_reading` and wakes the writer, even if the reader throws.
- **Writer** (`src/writer.cpp`): waits on `not_empty`, pops chunks, and writes them to the destination. A scope guard sets `stop_read` and wakes the reader when the writer finishes or fails, so the reader never blocks forever.
- **Main** (`main.cpp`): parses arguments, validates paths, launches both threads, and rethrows the first captured exception (if any) to print a readable error.

## Project Structure

```
Copy_Tool/
├── CMakeLists.txt        # Build configuration
├── vcpkg.json            # Dependency manifest
├── main.cpp              # Argument parsing, validation, thread orchestration
├── src/
│   ├── reader.cpp        # Producer: reads source file into the queue
│   └── writer.cpp        # Consumer: writes queued chunks to destination
└── test/
    ├── src_file.txt          # Sample input
    └── destination_file.txt  # Sample output for comparison
```

## Requirements

- A C++20 compiler (GCC 11+, Clang 14+, or MSVC 2019 16.11+; `<format>` and `std::jthread` support required)
- CMake (the project file requests version 4.3; lower it in `CMakeLists.txt` if your CMake is older)
- Boost with the `program_options` component (managed via vcpkg)

## Build

### With vcpkg (recommended)

```bash
git clone <repo-url>
cd Copy_Tool

cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

### With a system-installed Boost

```bash
# Debian/Ubuntu example
sudo apt install libboost-program-options-dev

cmake -B build -S .
cmake --build build
```

The executable is produced as `build/Copy_Tool` (or `build/Debug/Copy_Tool.exe` on Windows with MSVC).

## Usage

```
Copy_Tool [options] <src> <dst>
```

| Option | Description | Default |
|---|---|---|
| `-h`, `--help` | Display the help message | – |
| `-c`, `--chunk_size <n>` | Size in bytes of each chunk read/written at a time | `1024` |
| `-f`, `--force` | Overwrite the destination file if it already exists | off |

### Examples

```bash
# Basic copy
./Copy_Tool test/src_file.txt test/destination_file.txt

# Overwrite an existing destination
./Copy_Tool -f test/src_file.txt test/destination_file.txt

# Use 64 KiB chunks for a large file
./Copy_Tool -c 65536 big_input.bin big_output.bin
```

On success the tool prints `Copying Completed` and exits with code `0`. On any error it prints a message and exits with code `1`.

## Error Handling

| Situation | Behaviour |
|---|---|
| Missing or invalid arguments | Prints `Argument Error: ...`, exits with `1` |
| Negative chunk size | Prints an error, exits with `1` |
| Destination exists without `-f` | Prints `Destination exists (use -f to overwrite)`, exits with `1` |
| Source and destination are the same file | Prints an error, exits with `1` |
| Read or write failure (open, I/O, flush) | Reported as `Copy failed: ...`, exits with `1` |

## Known Limitations / Ideas for Improvement

- A chunk size of `0` is not rejected (the check only catches negative values); it should be treated as invalid.
- If the source is not a regular file, a message is printed but execution continues; the reader then fails with an error. Returning early would be cleaner.
- The queue limit is `chunk_size * 16` *chunks*, not bytes, so a large chunk size allows a very large buffer. A fixed maximum queue depth would be more predictable.
- No automated tests yet; the `test/` folder contains only sample files. Adding a checksum comparison or GoogleTest/Catch2 suite would be a natural next step.
- Does not preserve file permissions or timestamps.

## Author

Abhishek Jalkhare
