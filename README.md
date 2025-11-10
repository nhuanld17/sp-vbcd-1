# Deadlock Detector - Process-Level Deadlock Detection System

A comprehensive C-based deadlock detection system for Linux and WSL2 that monitors system processes and detects deadlocks involving pipes and file locks.

## 📋 Table of Contents

1. [Project Overview](#project-overview)
2. [Requirements](#requirements)
3. [Installation on Linux](#installation-on-linux)
4. [Installation on WSL2 Ubuntu](#installation-on-wsl2-ubuntu)
5. [Usage](#usage)
6. [Testing Deadlock Detection](#testing-deadlock-detection)
7. [Demo Script](#demo-script)
8. [Output Formats](#output-formats)
9. [Troubleshooting](#troubleshooting)
10. [Project Structure](#project-structure)
11. [Features and Limitations](#features-and-limitations)

---

## 🎯 Project Overview

The Deadlock Detector is a system-level tool that monitors Linux processes and detects deadlock conditions in real-time. It scans the `/proc` filesystem to build a Resource Allocation Graph (RAG), uses Depth-First Search (DFS) with 3-color marking for cycle detection, and reports processes involved in deadlocks.

### Key Features

- **Process-Level Deadlock Detection**: Detects deadlocks between multiple processes
- **Pipe Deadlock Detection**: Identifies circular dependencies in pipe I/O operations
- **File Lock Deadlock Detection**: Detects deadlocks involving `flock()` file locks
- **Real-Time Monitoring**: Continuous monitoring mode with configurable intervals
- **Multiple Output Formats**: Text, JSON, and verbose output formats
- **Comprehensive Testing**: 116 unit tests, all passing
- **Memory Safe**: Valgrind-clean with no memory leaks

### Supported Deadlock Types

1. **Pipe Deadlocks**: When processes block waiting for each other through pipes
   - Process A writes to pipe1, waits on pipe2
   - Process B writes to pipe2, waits on pipe1

2. **File Lock Deadlocks**: When processes deadlock on file locks
   - Process A holds lock1, waits for lock2
   - Process B holds lock2, waits for lock1

### Not Supported

- **Thread-Level Deadlocks**: Only process-level deadlocks are detected
- **Semaphore Deadlocks**: Not currently supported
- **Mutex Deadlocks**: Not currently supported

---

## 📦 Requirements

### Operating System
- **Linux** (Ubuntu 18.04+, Debian, Fedora, etc.)
- **WSL2 Ubuntu** (Windows Subsystem for Linux 2)

### Tools and Dependencies
- `gcc` (GNU Compiler Collection) - version 7.0 or higher
- `make` - Build automation tool
- `valgrind` - Memory leak detection (optional, for testing)

### System Requirements
- Access to `/proc` filesystem (standard on Linux)
- Sufficient permissions to read process information (usually requires root or same user)

---

## 🐧 Installation on Linux

### Step 1: Install Dependencies

```bash
# Update package list
sudo apt-get update

# Install build tools
sudo apt-get install -y build-essential gcc make

# Install valgrind (optional, for memory leak testing)
sudo apt-get install -y valgrind
```

### Step 2: Clone or Navigate to Project Directory

```bash
cd /path/to/deadlock_detector
```

### Step 3: Build the Project

```bash
# Clean any previous builds
make clean

# Build the project
make
```

**Expected Output:**
```
========================================
Build successful: bin/deadlock_detector
========================================
```

### Step 4: Run Tests

```bash
# Run all unit tests
make test
```

**Expected Output:**
```
========================================
Running All Tests
========================================
[PASS] test_graph
[PASS] test_cycle
[PASS] test_system
========================================
All tests passed!
========================================
```

### Step 5: Verify Installation

```bash
# Check if executable was created
ls -lh bin/deadlock_detector

# Run with help to verify
./bin/deadlock_detector --help
```

---

## 🪟 Installation on WSL2 Ubuntu

### Step 1: Install WSL2 (if not already installed)

On Windows, open PowerShell as Administrator:

```powershell
# Enable WSL2
wsl --install

# Set WSL2 as default
wsl --set-default-version 2
```

### Step 2: Launch WSL2 Ubuntu

```bash
# Open Ubuntu from Start Menu or run:
wsl
```

### Step 3: Install Dependencies in WSL2

```bash
# Update package list
sudo apt-get update

# Install build tools
sudo apt-get install -y build-essential gcc make

# Install valgrind (optional)
sudo apt-get install -y valgrind
```

### Step 4: Navigate to Project Directory

```bash
# If project is in Windows filesystem (e.g., D:\)
cd /mnt/d/DOCUMENT/System_Programing/deadlock_detector

# Or if project is in WSL filesystem
cd ~/deadlock_detector
```

### Step 5: Build the Project

```bash
# Clean and build
make clean && make
```

### Step 6: Run Tests

```bash
make test
```

**Note**: WSL2 has full access to `/proc` filesystem, so all features work identically to native Linux.

---

## 🚀 Usage

### Basic Usage

Run a single deadlock detection scan:

```bash
./bin/deadlock_detector
```

**Output:**
```
========================================
Deadlock Detection System
========================================
Scanning system processes...
No deadlock detected.
```

### Command-Line Options

| Option | Short | Description | Default |
|--------|-------|-------------|---------|
| `--help` | `-h` | Show help message | - |
| `--verbose` | `-v` | Enable verbose output | Off |
| `--continuous` | `-c` | Continuous monitoring mode | Off |
| `--interval` | `-i SEC` | Monitoring interval in seconds | 5 |
| `--format` | `-f FORMAT` | Output format: text, json, verbose | text |
| `--output` | `-o FILE` | Write output to file | stdout |
| `--version` | - | Show version information | - |

### Usage Examples

#### 1. Verbose Mode (One-Time Scan)

```bash
./bin/deadlock_detector -v
```

**Output:**
```
[INFO] Starting deadlock detection...
[INFO] Scanning /proc filesystem...
[INFO] Found 150 processes
[INFO] Building Resource Allocation Graph...
[INFO] Running cycle detection...
[INFO] No deadlock detected.
```

#### 2. Continuous Monitoring

```bash
# Monitor every 10 seconds
./bin/deadlock_detector -c -i 10
```

**Output:**
```
[INFO] Starting continuous monitoring (interval: 10 seconds)
[INFO] Press Ctrl+C to stop
[INFO] Scan 1: No deadlock detected.
[INFO] Scan 2: No deadlock detected.
...
```

#### 3. JSON Output

```bash
# Output results in JSON format
./bin/deadlock_detector -f json
```

**Output:**
```json
{
  "deadlock_detected": false,
  "timestamp": 1703123456,
  "processes_scanned": 150,
  "resources_found": 45
}
```

#### 4. Save Output to File

```bash
# Save verbose output to file
./bin/deadlock_detector -v -o deadlock_report.txt

# Save JSON output to file
./bin/deadlock_detector -f json -o results.json
```

#### 5. Continuous Monitoring with JSON Output

```bash
./bin/deadlock_detector -c -i 5 -f json -o monitor.json
```

#### 6. Show Version

```bash
./bin/deadlock_detector --version
```

**Output:**
```
Deadlock Detector v1.0.0
Copyright (C) 2024
```

---

## 🧪 Testing Deadlock Detection

### Testing Pipe Deadlock Detection

#### Step 1: Compile the Demo Program

```bash
# Compile pipe_deadlock.c
gcc -o pipe_deadlock pipe_deadlock.c
```

#### Step 2: Run the Demo in Background

```bash
# Run pipe deadlock demo in background
./pipe_deadlock &
```

**Expected Output:**
```
========================================
  REAL PIPE DEADLOCK SIMULATION
========================================
Process A: READ pipe2 (block), then WRITE pipe1
Process B: READ pipe1 (block), then WRITE pipe2
Both will block forever waiting for each other.
Press Ctrl+C to stop.
========================================

[Process A - PID 12345] Started
[Process B - PID 12346] Started
[Process B - PID 12346] Trying to READ from pipe1... (WILL BLOCK FOREVER)
[Process A - PID 12345] Trying to READ from pipe2... (WILL BLOCK FOREVER)
```

#### Step 3: Run Deadlock Detector

```bash
# In another terminal, run the detector
./bin/deadlock_detector -v
```

**Expected Output:**
```
========================================
DEADLOCK DETECTED!
========================================

Deadlocked Processes:
PID: 12345, Name: pipe_deadlock
PID: 12346, Name: pipe_deadlock

Cycle Chain:
Process 12345 → (waiting on pipe) → Process 12346 → (waiting on pipe) → Process 12345

Explanation:
Process 12345 is blocked waiting for pipe resource held by process 12346.
Process 12346 is blocked waiting for pipe resource held by process 12345.
This creates a circular dependency (deadlock).

Recommendations:
1. Terminate one of the processes in the cycle
2. Review resource acquisition order
3. Implement timeout mechanisms
```

#### Step 4: Cleanup

```bash
# Kill the demo processes
pkill -f pipe_deadlock

# Or kill by PID
kill 12345 12346
```

### Testing File Lock Deadlock Detection

#### Step 1: Compile the Demo Program

```bash
# Compile file_deadlock.c
gcc -o file_deadlock file_deadlock.c
```

#### Step 2: Run the Demo in Background

```bash
# Run file lock deadlock demo
./file_deadlock &
```

**Expected Output:**
```
========================================
  FILE LOCK DEADLOCK SIMULATION
========================================
Process A: locks FILE1, waits for FILE2
Process B: locks FILE2, waits for FILE1
Press Ctrl+C to stop.
========================================

[Process A - PID 12347] Started
[Process A - PID 12347] Locking /tmp/deadlock_file1.lock...
[Process A - PID 12347] ✓ Locked /tmp/deadlock_file1.lock
[Process B - PID 12348] Started
[Process B - PID 12348] Locking /tmp/deadlock_file2.lock...
[Process B - PID 12348] ✓ Locked /tmp/deadlock_file2.lock
[Process A - PID 12347] Waiting for lock on /tmp/deadlock_file2.lock... (WILL BLOCK)
[Process B - PID 12348] Waiting for lock on /tmp/deadlock_file1.lock... (WILL BLOCK)
```

#### Step 3: Run Deadlock Detector

```bash
# In another terminal
./bin/deadlock_detector -v
```

**Expected Output:**
```
========================================
DEADLOCK DETECTED!
========================================

Deadlocked Processes:
PID: 12347, Name: file_deadlock
PID: 12348, Name: file_deadlock

Cycle Chain:
Process 12347 → (waiting for /tmp/deadlock_file2.lock) → Process 12348 → (waiting for /tmp/deadlock_file1.lock) → Process 12347

Explanation:
Process 12347 holds /tmp/deadlock_file1.lock and waits for /tmp/deadlock_file2.lock.
Process 12348 holds /tmp/deadlock_file2.lock and waits for /tmp/deadlock_file1.lock.
This creates a circular dependency (deadlock).
```

#### Step 4: Cleanup

```bash
# Kill the demo processes
pkill -f file_deadlock

# Clean up lock files
rm -f /tmp/deadlock_file*.lock
```

---

## 📜 Demo Script

Here's a complete bash script that demonstrates the deadlock detection workflow:

```bash
#!/bin/bash
# deadlock_demo.sh - Comprehensive deadlock detection demo

set -e

echo "========================================="
echo "  DEADLOCK DETECTOR DEMO"
echo "========================================="
echo ""

# Step 1: Build the project
echo "[1/5] Building project..."
make clean > /dev/null 2>&1
make > /dev/null 2>&1
echo "✓ Build successful"
echo ""

# Step 2: Run unit tests
echo "[2/5] Running unit tests..."
make test
echo ""

# Step 3: Test pipe deadlock detection
echo "[3/5] Testing pipe deadlock detection..."
gcc -o pipe_deadlock pipe_deadlock.c
./pipe_deadlock > /dev/null 2>&1 &
PIPE_PID=$!
sleep 2
echo "  Pipe deadlock processes started (PIDs: $PIPE_PID and child)"
echo "  Running detector..."
./bin/deadlock_detector -v | grep -E "(DEADLOCK|Deadlocked|Cycle)" || echo "  No deadlock detected (may take a moment)"
sleep 1
pkill -f pipe_deadlock > /dev/null 2>&1
echo "✓ Pipe deadlock test completed"
echo ""

# Step 4: Test file lock deadlock detection
echo "[4/5] Testing file lock deadlock detection..."
gcc -o file_deadlock file_deadlock.c
./file_deadlock > /dev/null 2>&1 &
FILE_PID=$!
sleep 3
echo "  File lock deadlock processes started (PIDs: $FILE_PID and child)"
echo "  Running detector..."
./bin/deadlock_detector -v | grep -E "(DEADLOCK|Deadlocked|Cycle)" || echo "  No deadlock detected (may take a moment)"
sleep 1
pkill -f file_deadlock > /dev/null 2>&1
rm -f /tmp/deadlock_file*.lock
echo "✓ File lock deadlock test completed"
echo ""

# Step 5: Memory leak check
echo "[5/5] Checking for memory leaks..."
valgrind --leak-check=full --error-exitcode=1 ./bin/deadlock_detector > /dev/null 2>&1
echo "✓ No memory leaks detected"
echo ""

echo "========================================="
echo "  DEMO COMPLETED SUCCESSFULLY"
echo "========================================="
```

**Save and run the script:**

```bash
# Make script executable
chmod +x deadlock_demo.sh

# Run the demo
./deadlock_demo.sh
```

---

## 📄 Output Formats

### Text Format (Default)

```bash
./bin/deadlock_detector
```

**Output:**
```
========================================
Deadlock Detection System
========================================
Scanning system processes...
No deadlock detected.
```

### Verbose Format

```bash
./bin/deadlock_detector -v
```

**Output:**
```
[INFO] Starting deadlock detection...
[INFO] Scanning /proc filesystem...
[INFO] Found 150 processes
[INFO] Building Resource Allocation Graph...
[INFO] Graph contains 150 processes, 45 resources, 89 edges
[INFO] Running cycle detection algorithm...
[INFO] No cycles detected.
[INFO] No deadlock detected.
```

### JSON Format

```bash
./bin/deadlock_detector -f json
```

**Output:**
```json
{
  "deadlock_detected": false,
  "timestamp": 1703123456,
  "processes_scanned": 150,
  "resources_found": 45,
  "cycles_found": 0
}
```

### Deadlock Detected Output (Text)

```
========================================
DEADLOCK DETECTED!
========================================

Deadlocked Processes:
PID: 12345, Name: process_a
PID: 12346, Name: process_b

Cycle Chain:
Process 12345 → (waiting Resource_X) → Process 12346 → (waiting Resource_Y) → Process 12345

Explanation:
Process 12345 is blocked waiting for resource held by process 12346.
Process 12346 is blocked waiting for resource held by process 12345.
This creates a circular dependency (deadlock).

Recommendations:
1. Terminate one of the processes in the cycle
2. Review resource acquisition order
3. Implement timeout mechanisms
4. Use deadlock prevention algorithms
```

### Deadlock Detected Output (JSON)

```json
{
  "deadlock_detected": true,
  "timestamp": 1703123456,
  "deadlocked_processes": [12345, 12346],
  "cycles": [
    {
      "cycle_id": 1,
      "vertices": [12345, 12346],
      "explanation": "Process 12345 waits for process 12346, process 12346 waits for process 12345"
    }
  ],
  "recommendations": [
    "Terminate one of the processes in the cycle",
    "Review resource acquisition order",
    "Implement timeout mechanisms"
  ]
}
```

---

## 🔧 Troubleshooting

### Common Issues and Solutions

#### 1. Build Errors

**Problem:** `make: gcc: command not found`

**Solution:**
```bash
# Install build-essential
sudo apt-get update
sudo apt-get install -y build-essential
```

#### 2. Permission Denied

**Problem:** `Permission denied` when running detector

**Solution:**
```bash
# Make executable
chmod +x bin/deadlock_detector

# Or run with appropriate permissions
./bin/deadlock_detector
```

#### 3. Cannot Read /proc

**Problem:** `Error: Failed to read /proc filesystem`

**Solution:**
- Ensure you're on Linux or WSL2 (not WSL1)
- Check that `/proc` is mounted: `mount | grep proc`
- Run with appropriate permissions (may need root for some processes)

#### 4. Valgrind Errors

**Problem:** Valgrind reports memory leaks

**Solution:**
```bash
# Check for leaks
valgrind --leak-check=full ./bin/deadlock_detector -v

# If leaks are found, check the source code and fix memory management
```

#### 5. Tests Fail

**Problem:** `make test` fails

**Solution:**
```bash
# Clean and rebuild
make clean
make

# Run tests individually
./bin/test_graph
./bin/test_cycle
./bin/test_system
```

#### 6. Demo Programs Don't Create Deadlock

**Problem:** Deadlock not detected in demo

**Solution:**
- Ensure processes have time to deadlock (wait 2-3 seconds)
- Check that processes are actually running: `ps aux | grep deadlock`
- Verify processes are in 'D' state (uninterruptible sleep): `ps aux | grep -E "D.*deadlock"`

#### 7. WSL2 Issues

**Problem:** WSL2 doesn't show processes correctly

**Solution:**
```bash
# Update WSL2
wsl --update

# Restart WSL2
wsl --shutdown
# Then reopen Ubuntu
```

#### 8. No Deadlock Detected (False Negative)

**Problem:** Detector doesn't detect obvious deadlock

**Solution:**
- Check process state: `ps aux | grep <pid>`
- Check wchan: `cat /proc/<pid>/wchan`
- Verify processes are actually blocked: `strace -p <pid>`
- Run with verbose mode: `./bin/deadlock_detector -v`

---

## 📁 Project Structure

```
deadlock_detector/
├── src/                          # Source code
│   ├── main.c                   # Main program entry point
│   ├── process_monitor.c/.h     # Process monitoring from /proc
│   ├── resource_graph.c/.h      # Resource Allocation Graph (RAG)
│   ├── cycle_detection.c/.h     # DFS cycle detection algorithm
│   ├── deadlock_detection.c/.h  # Deadlock detection engine
│   ├── output_handler.c/.h      # Output formatting (text, JSON)
│   ├── utility.c/.h             # Utility functions
│   └── config.h                 # Configuration constants
│
├── test/                         # Unit tests
│   ├── test_graph.c             # Graph data structure tests
│   ├── test_cycle.c             # Cycle detection tests
│   └── test_system.c            # System integration tests
│
├── bin/                          # Compiled binaries
│   ├── deadlock_detector        # Main executable
│   ├── test_graph               # Graph test executable
│   ├── test_cycle               # Cycle test executable
│   └── test_system              # System test executable
│
├── obj/                          # Object files (generated)
│   └── *.o, *.d                 # Compiled object files
│
├── pipe_deadlock.c              # Pipe deadlock demo program
├── file_deadlock.c              # File lock deadlock demo program
├── process_deadlock.c           # Process deadlock demo program
│
├── Makefile                     # Build configuration
├── README.md                    # This file
└── .gitignore                   # Git ignore file
```

### Key Files

- **`src/main.c`**: Command-line interface, argument parsing, signal handling
- **`src/process_monitor.c`**: Reads `/proc` filesystem, collects process information
- **`src/deadlock_detection.c`**: Main deadlock detection logic, RAG building
- **`src/cycle_detection.c`**: DFS algorithm for cycle detection
- **`Makefile`**: Build system with test targets

---

## ✨ Features and Limitations

### ✅ What Works

1. **Process-Level Deadlock Detection**
   - Detects deadlocks between multiple processes
   - Scans all running processes on the system
   - Builds Resource Allocation Graph (RAG)

2. **Pipe Deadlock Detection**
   - Detects circular dependencies in pipe I/O
   - Identifies processes blocked on pipe read/write
   - Matches pipe inodes between processes

3. **File Lock Deadlock Detection**
   - Detects deadlocks involving `flock()` locks
   - Parses `/proc/locks` for system-wide locks
   - Matches file paths and inodes

4. **Real-Time Monitoring**
   - Continuous monitoring mode
   - Configurable monitoring intervals
   - Graceful shutdown on SIGINT

5. **Multiple Output Formats**
   - Text format (human-readable)
   - JSON format (machine-readable)
   - Verbose format (detailed debugging)

6. **Comprehensive Testing**
   - 116 unit tests
   - Graph structure tests
   - Cycle detection tests
   - System integration tests

7. **Memory Safety**
   - Valgrind-clean (no memory leaks)
   - Proper memory management
   - Safe error handling

### ❌ What Doesn't Work

1. **Thread-Level Deadlocks**
   - Only process-level deadlocks are detected
   - Thread deadlocks within a single process are not supported
   - Reason: Requires thread-level monitoring which is more complex

2. **Semaphore Deadlocks**
   - POSIX semaphores are not currently supported
   - System V semaphores are not supported
   - Reason: Requires tracking semaphore operations

3. **Mutex Deadlocks**
   - POSIX mutexes are not supported
   - pthread mutexes are not supported
   - Reason: Mutexes are typically thread-level, not process-level

4. **Network Socket Deadlocks**
   - TCP/UDP socket deadlocks are not detected
   - Reason: Socket operations are more complex to track

5. **Database Lock Deadlocks**
   - Database-level locks are not supported
   - Reason: Requires database-specific monitoring

### 🔮 Future Improvements

- Thread-level deadlock detection
- Semaphore and mutex deadlock detection
- Network socket deadlock detection
- Graphical visualization of deadlock cycles
- Real-time alerting and notifications
- Performance optimizations for large systems

---

## 📚 Additional Resources

### Documentation

- **Linux `/proc` filesystem**: `man 5 proc`
- **DFS Cycle Detection**: [Wikipedia - Cycle Detection](https://en.wikipedia.org/wiki/Cycle_detection)
- **Deadlock Concepts**: Operating System Concepts (Silberschatz et al.)

### Related Concepts

- **Resource Allocation Graph (RAG)**: Graph representation of resource allocation
- **Deadlock Prevention**: Techniques to prevent deadlocks
- **Deadlock Avoidance**: Banker's algorithm and similar methods
- **Deadlock Recovery**: Process termination and resource preemption

---

## 🤝 Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new features
5. Ensure all tests pass
6. Submit a pull request

---

## 📝 License

This project is provided as-is for educational purposes.

---

## 🐛 Reporting Bugs

If you encounter a bug, please:

1. Check the [Troubleshooting](#troubleshooting) section
2. Run with verbose mode: `./bin/deadlock_detector -v`
3. Check process states: `ps aux | grep <pid>`
4. Report the issue with:
   - Operating system and version
   - Steps to reproduce
   - Expected vs actual behavior
   - Relevant error messages

---

## 📧 Contact

For questions, suggestions, or bug reports, please open an issue on the repository.

---

## 🙏 Acknowledgments

- Linux kernel developers for the `/proc` filesystem
- Operating systems community for deadlock detection algorithms
- All contributors and testers

---

**Happy Deadlock Detecting! 🎉**
