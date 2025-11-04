# Deadlock Detection System for Unix/Linux

Chương trình phát hiện **Deadlock** trong hệ thống **Unix/Ubuntu Linux** sử dụng ngôn ngữ **C**.

## 📋 Mục Đích

Xây dựng một hệ thống phát hiện deadlock chuyên nghiệp có khả năng:

- **Thu thập thông tin** về các tiến trình đang chạy và tài nguyên từ `/proc` filesystem
- **Xây dựng Resource Allocation Graph (RAG)** để biểu diễn quan hệ giữa tiến trình và tài nguyên
- **Phát hiện chu trình** (cycle) trong đồ thị sử dụng thuật toán DFS với độ phức tạp O(V+E)
- **Xác định các tiến trình bị deadlock** từ các chu trình phát hiện được
- **Sinh báo cáo chi tiết** về deadlock situation, wait chains, và gợi ý giải quyết

---

## 🏗️ Kiến Trúc Hệ Thống

### Sơ Đồ Kiến Trúc:
```
┌─────────────────────────────────────────────────┐
│           User Interface / Main                  │
│         (CLI Arguments, Continuous Mode)        │
└──────────────────────┬──────────────────────────┘
                       │
        ┌──────────────┼──────────────┐
        │              │              │
        ▼              ▼              ▼
    ┌────────┐  ┌────────────┐  ┌─────────┐
    │Process │  │ Resource   │  │Deadlock │
    │Monitor │  │   Graph    │  │Detection│
    │Module  │  │  Module    │  │ Engine  │
    └────────┘  └────────────┘  └─────────┘
        │              │              │
        └──────────────┼──────────────┘
                       │
                       ▼
            ┌──────────────────────┐
            │  Cycle Detection     │
            │   (DFS Algorithm)    │
            └──────────┬───────────┘
                       │
                       ▼
            ┌──────────────────────┐
            │  Output Handler      │
            │ (Report Generation)  │
            └──────────────────────┘
```

### Các Module Chính:

| Module | Chức Năng | Input | Output |
|--------|----------|-------|--------|
| **Process Monitor** | Thu thập thông tin tiến trình từ `/proc` | PID list | ProcessResourceInfo[] |
| **Resource Graph** | Xây dựng RAG từ process/resource data | ProcessResourceInfo[] | ResourceGraph (adjacency list) |
| **Cycle Detection** | Phát hiện chu trình bằng DFS | ResourceGraph | CycleInfo[] |
| **Deadlock Detection** | Xác định deadlock từ cycles | CycleInfo[] | Deadlocked PIDs |
| **Output Handler** | Format và xuất kết quả | DeadlockReport | Text/JSON/Verbose output |

---

## 🚀 Quick Start

### Build:
```bash
cd deadlock_detector
make clean && make
```

### Run:
```bash
# Chế độ đơn lần
./bin/deadlock_detector

# Verbose output
./bin/deadlock_detector -v

# Continuous monitoring, check mỗi 10 giây
./bin/deadlock_detector -c -i 10

# JSON output
./bin/deadlock_detector -f json

# Output to file
./bin/deadlock_detector -o results.txt

# Chạy tests
make test
```

### Command-line Options:
```
-h, --help              Hiển thị help
-v, --verbose           Verbose output (debug info)
-c, --continuous        Continuous monitoring mode
-i, --interval SEC      Interval giữa checks (mặc định: 5 giây)
-f, --format FORMAT     Output format: text, json, verbose
-o, --output FILE       Output đến file
--version               Hiển thị phiên bản
```

---

## 📊 Thuật Toán Chính

### 1. **Resource Allocation Graph (RAG)**

**Định nghĩa**: 
- Biểu đồ có hướng với 2 loại đỉnh: Process (P) và Resource (R)
- Cạnh Request: P → R (tiến trình P chờ tài nguyên R)
- Cạnh Allocation: R → P (tài nguyên R được cấp phát cho P)

**Quy tắc phát hiện deadlock**:
- Nếu **tài nguyên có 1 instance (single instance)**: Chu trình trong RAG → **Chắc chắn có deadlock**
- Nếu **tài nguyên có nhiều instances (multiple instances)**: Chu trình → **Khả năng deadlock** (cần kiểm tra thêm)

### 2. **Wait-For Graph (WFG)**

**Định nghĩa**:
- Biến thể đơn giản của RAG chỉ giữ lại tiến trình (bỏ đi nút tài nguyên)
- Cạnh P1 → P2 có nghĩa: P1 đang chờ P2 giải phóng tài nguyên

**Ưu điểm**: 
- Nhỏ hơn RAG, dễ duyệt
- Kiểm tra chu trình trực tiếp

### 3. **DFS-Based Cycle Detection**

**Thuật toán**:
```
1. Initialize tất cả vertices = WHITE (unvisited)
2. For mỗi unvisited vertex:
   a. Gọi DFS_VISIT(vertex)
   b. Trong DFS_VISIT(v):
      - Mark v = GRAY (đang xử lý)
      - For mỗi neighbor u của v:
        * Nếu u = WHITE: Recursively DFS_VISIT(u)
        * Nếu u = GRAY: → Found cycle! (Back edge)
      - Mark v = BLACK (xong)
3. Trả về tất cả cycles tìm được
```

**Độ phức tạp**: 
- Time: O(V + E)
- Space: O(V)

**Ưu điểm**:
- Đơn giản, hiệu quả
- Tìm được tất cả cycles
- Phù hợp cho đồ thị sparse

---

## 📁 Cấu Trúc File

```
deadlock_detector/
├── src/
│   ├── main.c                           # Entry point
│   ├── process_monitor.c/.h             # /proc filesystem reading
│   ├── resource_graph.c/.h              # RAG construction
│   ├── cycle_detection.c/.h             # DFS cycle detection
│   ├── deadlock_detection.c/.h          # Deadlock detection engine
│   ├── output_handler.c/.h              # Result formatting
│   ├── utility.c/.h                     # Helper functions
│   └── config.h                         # Macros & constants
│
├── test/
│   ├── test_graph.c                     # Test RAG construction
│   ├── test_cycle.c                     # Test cycle detection
│   ├── test_system.c                    # Integration tests
│   └── test_data.c                      # Mock data for testing
│
├── Makefile                             # Build configuration
├── README.md                            # This file
├── instruction.md                       # Detailed instructions for Cursor
└── .gitignore
```

---

## 🔍 Data Structures

### ProcessResourceInfo
```c
typedef struct {
    int pid;                    // Process ID
    int* held_resources;        // Resource IDs đang giữ
    int num_held;               // Số lượng resources held
    int* waiting_resources;     // Resource IDs đang chờ
    int num_waiting;            // Số lượng resources waiting
} ProcessResourceInfo;
```

### ResourceGraph
```c
typedef struct {
    GraphNode** adjacency_list;  // Adjacency list representation
    int* color;                  // 0=WHITE, 1=GRAY, 2=BLACK
    int num_vertices;            // Số đỉnh
    int max_vertices;            // Max capacity
    int* vertex_type;            // 0=process, 1=resource
    int* vertex_id;              // PID or Resource ID
} ResourceGraph;
```

### CycleInfo
```c
typedef struct {
    int* cycle_path;             // Dãy vertices trong cycle
    int cycle_length;            // Số đỉnh trong cycle
    int cycle_start_node;        // Node bắt đầu cycle
    int cycle_end_node;          // Node kết thúc cycle
} CycleInfo;
```

### DeadlockReport
```c
typedef struct {
    int* deadlocked_pids;        // PIDs bị deadlock
    int num_deadlocked;          // Số tiến trình bị deadlock
    CycleInfo* cycle_chain;      // Chu trình gây deadlock
    char** explanation;          // Giải thích chi tiết
} DeadlockReport;
```

---

## 📥 Input Sources

### `/proc` Filesystem:

| File | Thông Tin | Ví Dụ |
|------|----------|-------|
| `/proc/[PID]/status` | Process metadata (Name, State, Parent PID, Memory usage) | `Name: bash`, `State: S` |
| `/proc/[PID]/fd/` | File descriptors đang mở | Symlinks to open files |
| `/proc/[PID]/locks` | File locks | `1: FLOCK ADVISORY WRITE` |
| `/proc/[PID]/task/` | Threads trong process | Subdirectories cho each thread |

### Khả Năng Thu Thập:
- ✅ Process list & metadata
- ✅ File locks (fcntl locks)
- ✅ Pipe/socket connections
- ⚠️ Mutex/semaphore (limited access)
- ❌ Kernel locks (privileged only)

---

## 📊 Output Formats

### Text Format:
```
========================================
DEADLOCK DETECTED!
========================================

Deadlocked Processes:
  PID: 1234, Name: process_a
  PID: 1235, Name: process_b

Cycle Chain:
  process_a → (waiting for Resource_X) → process_b → (waiting for Resource_Y) → process_a

Recommendations:
  1. Terminate one of the deadlocked processes
  2. Review resource acquisition order
  3. Implement timeout mechanism
```

### JSON Format:
```json
{
  "deadlock_detected": true,
  "timestamp": "2024-11-04T08:59:00Z",
  "deadlocked_processes": [
    {"pid": 1234, "name": "process_a"},
    {"pid": 1235, "name": "process_b"}
  ],
  "cycle": [1234, 1235, 1234],
  "recommendations": [...]
}
```

---

## 🧪 Test Cases

### Test 1: No Deadlock
```
Process: P1 locks R1 → releases R1
Process: P2 locks R2 → releases R2
Result: No cycle in graph → No deadlock ✓
```

### Test 2: Simple Deadlock
```
P1: locks R1, waits for R2
P2: locks R2, waits for R1
Graph: P1 → R2 → P2 → R1 → P1 (cycle!)
Result: Deadlock detected ✓
```

### Test 3: Complex Deadlock
```
P1: locks R1, R2 → waits R3
P2: locks R3 → waits R1
P3: locks R2 → waits R3
Graph: P1 → R3 → P2 → R1 → P1 (cycle!)
Result: Deadlock detected ✓
```

---

## 🔧 Build & Compile

### Requirements:
- **GCC** (hoặc Clang)
- **Linux/Unix system** with `/proc` filesystem
- **Standard C library** (glibc)
- **POSIX threads** (libpthread)

### Compile Options:
```makefile
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lpthread
```

### Warning Handling:
```bash
# Compile without warnings
make clean && make 2>&1 | grep -i warning

# Strict compilation
gcc -pedantic -Wall -Wextra -Werror ...
```

---

## 🐛 Debugging & Testing

### GDB:
```bash
gdb ./bin/deadlock_detector
(gdb) break main
(gdb) run -v
(gdb) print pids
(gdb) continue
```

### Valgrind (Memory Check):
```bash
valgrind --leak-check=full --show-leak-kinds=all \
  ./bin/deadlock_detector -v
```

### Strace (System Call Tracing):
```bash
strace -e open,read -o trace.txt ./bin/deadlock_detector
cat trace.txt
```

### Run Tests:
```bash
make test
# Output: All tests passed ✓
```

---

## 📈 Performance Characteristics

| Scenario | Time | Space | Notes |
|----------|------|-------|-------|
| 100 processes, simple | <100ms | ~10KB | Fast, minimal |
| 1000 processes, complex | ~500ms | ~100KB | Still reasonable |
| 10000 processes | ~2-5s | ~1MB | May need optimization |

### Optimization Tips:
1. **Caching**: Cache `/proc` reads
2. **Incremental Detection**: Update graph incrementally
3. **Parallel DFS**: Use multiple threads for large graphs
4. **Memory Pooling**: Pre-allocate structures

---

## 🚨 Edge Cases Handled

- ✅ Process terminates during detection
- ✅ Permission denied reading `/proc/[PID]`
- ✅ File locks released between reads
- ✅ System with no deadlock
- ✅ Multiple cycles in same graph
- ✅ Out of memory situations
- ✅ Invalid command-line arguments

---

## 📚 How to Use with Cursor IDE

### Step 1: Prepare
Copy `instruction.md` to your Cursor IDE project

### Step 2: Setup Project Structure
```bash
mkdir -p deadlock_detector/{src,test,obj,bin}
cd deadlock_detector
```

### Step 3: Import Instructions to Cursor
In Cursor IDE:
1. Open `.cursor/rules` or create it
2. Paste content from `instruction.md`
3. Add additional context in prompt

### Step 4: Generate Code Module by Module
**Prompt example for Cursor**:
```
Based on the instruction.md file provided, generate the process_monitor.c 
and process_monitor.h modules. 

Key requirements:
- Use /proc filesystem to read process information
- Implement get_all_processes(), get_process_info(), get_process_resources()
- Follow naming conventions and error handling patterns
- Include comprehensive comments
- Make sure memory is properly managed
```

---

## 📖 Learning Resources

### Key Concepts:
1. **Deadlock**: Situation where processes wait indefinitely for resources
2. **Resource Allocation Graph**: Visual representation of resource dependencies
3. **Cycle Detection**: Algorithm to find circular dependencies
4. **Graph Traversal**: DFS, BFS techniques

### Recommended Reading:
- "Operating System Concepts" by Silberschatz (Chapter on Deadlocks)
- Linux `/proc` filesystem documentation: `man 5 proc`
- POSIX threads: `man pthreads`
- C Programming: "The C Programming Language" by Kernighan & Ritchie

---

## 🎯 Success Criteria

Your implementation should:
- ✅ Compile without warnings (`-Wall -Wextra`)
- ✅ Handle all error conditions gracefully
- ✅ No memory leaks (pass `valgrind`)
- ✅ Detect deadlocks correctly in test cases
- ✅ Run efficiently on systems with many processes
- ✅ Well-documented code with clear comments
- ✅ Modular architecture (easy to maintain/extend)

---

## 👨‍💻 Author Notes

This is an **educational project** for learning:
- Operating Systems concepts (synchronization, deadlock)
- C Systems Programming
- Graph algorithms
- Linux/Unix programming

Focus on **correctness first**, then optimize if needed.

---

## 📞 Support & Troubleshooting

### Common Issues:

**Q: "Permission denied" when reading `/proc/[PID]`**
A: Some processes are owned by other users. Catch and skip these errors.

**Q: Valgrind shows memory leaks**
A: Ensure all allocated memory is freed in cleanup functions.

**Q: Graph not building correctly**
A: Check that you're correctly parsing the `/proc` files.

**Q: DFS not detecting cycles**
A: Verify that:
1. Color array is properly initialized
2. Back edges are correctly identified
3. All vertices are visited

---

## 📄 License

This project is for educational purposes.

---

**Last Updated**: November 4, 2025
**Status**: ✅ Ready for Cursor IDE integration
