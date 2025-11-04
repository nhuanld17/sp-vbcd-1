# Hướng Dẫn Viết Chương Trình Phát Hiện Deadlock - Cursor AI IDE

## I. MỤC TIÊU DỰ ÁN
Xây dựng chương trình C phát hiện **Deadlock** trong hệ thống **Unix/Ubuntu Linux** với khả năng:
- Giám sát các tiến trình và tài nguyên đang chạy
- Xây dựng Resource Allocation Graph (RAG)
- Áp dụng các thuật toán phát hiện chu trình (DFS)
- Cung cấp báo cáo chi tiết về các tiến trình bị deadlock

---

## II. YÊINFORMATICYÊU CẤU TRÚC DỰ ÁN

### Các Module Chính:
```
deadlock_detector/
├── src/
│   ├── main.c                      # Entry point chương trình
│   ├── process_monitor.c           # Thu thập thông tin từ /proc
│   ├── process_monitor.h
│   ├── resource_graph.c            # Xây dựng Resource Allocation Graph
│   ├── resource_graph.h
│   ├── deadlock_detection.c        # Các thuật toán phát hiện deadlock
│   ├── deadlock_detection.h
│   ├── cycle_detection.c           # Phát hiện chu trình (DFS)
│   ├── cycle_detection.h
│   ├── output_handler.c            # In kết quả báo cáo
│   ├── output_handler.h
│   └── utility.c, utility.h        # Hàm trợ giúp chung
├── test/
│   ├── test_graph.c                # Test tạo đồ thị
│   ├── test_cycle.c                # Test phát hiện chu trình
│   └── test_system.c               # Test hệ thống tích hợp
├── Makefile                        # Build script
├── README.md                       # Tài liệu hướng dẫn
└── .gitignore
```

---

## III. QUY TẮC VIẾT CODE CHUNG

### 1. **Quy Tắc Đặt Tên (Naming Conventions)**
- **Hàm**: `snake_case` (ví dụ: `get_process_list`, `detect_deadlock`)
- **Hằng số**: `UPPER_CASE` (ví dụ: `MAX_PROCESSES`, `ERROR_FILE_NOT_FOUND`)
- **Typedef struct**: `PascalCase` (ví dụ: `ResourceGraph`, `ProcessNode`)
- **Biến toàn cục**: `g_snake_case` (ví dụ: `g_process_count`)
- **Static biến**: `s_snake_case` trong file

### 2. **Quy Tắc Comment Code**
- **Function header comment**:
```c
/*
 * function_name: Brief description of what this function does
 * @param1_name: Description of first parameter
 * @param2_name: Description of second parameter
 * @return: Description of return value
 * Description: Detailed explanation if needed, including algorithms used,
 *              time complexity, special cases to handle
 * Error handling: How errors are reported
 */
```

- **Inline comments** cho logic phức tạp:
```c
// Khởi tạo mảng với giá trị ban đầu
for (int i = 0; i < size; i++) {
    array[i] = 0;
}

// DFS traverse để phát hiện chu trình
// Nếu gặp nút màu xám (GRAY), chứng tỏ có back edge -> deadlock
if (state->color[neighbor] == GRAY) {
    return 1;  // Chu trình được phát hiện
}
```

### 3. **Error Handling & Return Codes**
```c
// Định nghĩa error codes chung
#define SUCCESS 0
#define ERROR_FILE_NOT_FOUND -1
#define ERROR_PERMISSION_DENIED -2
#define ERROR_OUT_OF_MEMORY -3
#define ERROR_INVALID_ARGUMENT -4
#define ERROR_SYSTEM_CALL_FAILED -5

// Sử dụng safe allocation
void* safe_malloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Error: Out of memory\n");
        exit(ERROR_OUT_OF_MEMORY);
    }
    return ptr;
}

// Xử lý khi gọi system function
FILE* f = fopen(path, "r");
if (!f) {
    if (errno == ENOENT) {
        error_log("File not found: %s", path);
        return ERROR_FILE_NOT_FOUND;
    }
    // ... xử lý các lỗi khác
}
```

### 4. **Memory Management**
- Luôn cấp phát memory với check lỗi
- Giải phóng memory khi không dùng
- Tránh memory leak bằng cleanup functions
```c
void cleanup_graph(ResourceGraph* graph) {
    if (!graph) return;
    
    free(graph->processes);
    free(graph->resources);
    // ... free other resources
    free(graph);
}
```

### 5. **Logging & Debug**
```c
#define DEBUG 1

#if DEBUG
#define debug_log(fmt, ...) \
    fprintf(stderr, "[DEBUG %s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define debug_log(fmt, ...)
#endif

#define error_log(fmt, ...) \
    fprintf(stderr, "[ERROR %s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define info_log(fmt, ...) \
    fprintf(stdout, "[INFO] " fmt "\n", ##__VA_ARGS__)
```

### 6. **Quy Tắc Indentation & Formatting**
- Sử dụng **4 spaces** (không tabs)
- Max line length: **100 characters**
- Đặt dấu `{` trên cùng dòng với function
```c
int detect_deadlock(ResourceGraph* graph) {
    // Code here
    return result;
}
```

---

## IV. CHI TIẾT CÁC MODULE

### A. **process_monitor.h/c** - Module Giám Sát Tiến Trình
**Chức năng**: Thu thập thông tin về tiến trình, tài nguyên, file locks từ `/proc`

**Header định nghĩa (process_monitor.h)**:
```c
#ifndef PROCESS_MONITOR_H
#define PROCESS_MONITOR_H

#include <sys/types.h>

typedef struct {
    pid_t pid;
    char name[256];
    char state;  // R, S, D, Z, T, W, X
    int ppid;
    int* fds;  // File descriptors
    int num_fds;
} ProcessInfo;

typedef struct {
    int pid;
    int* held_resources;
    int num_held;
    int* waiting_resources;
    int num_waiting;
} ProcessResourceInfo;

// Hàm thực thi
pid_t* get_all_processes(int* count);
int get_process_info(pid_t pid, ProcessInfo* info);
int get_process_resources(pid_t pid, ProcessResourceInfo* res_info);
char* read_proc_file(pid_t pid, const char* filename);
void free_process_info(ProcessInfo* info);
void free_process_list(pid_t* pids);

#endif
```

**Yêu cầu implement**:
- Đọc danh sách tiến trình từ `/proc`
- Parse file `/proc/[PID]/status` lấy thông tin: Name, State, VmRSS
- Parse file `/proc/[PID]/fd` để xem file descriptors đang mở
- Parse file `/proc/[PID]/locks` để lấy thông tin file locks
- Xử lý errors khi process kết thúc giữa chừng
- Cache thông tin để tránh I/O lặp lại

### B. **resource_graph.h/c** - Module Xây Dựng RAG
**Chức năng**: Tạo và quản lý Resource Allocation Graph

**Header định nghĩa (resource_graph.h)**:
```c
#ifndef RESOURCE_GRAPH_H
#define RESOURCE_GRAPH_H

#include "process_monitor.h"

typedef struct GraphNode {
    int id;
    int type;  // 0=process, 1=resource
    int pid_or_rid;
    struct GraphNode* next;
} GraphNode;

typedef struct {
    GraphNode** adjacency_list;
    int* color;  // 0=WHITE, 1=GRAY, 2=BLACK
    int num_vertices;
    int max_vertices;
    int* vertex_type;  // 0=process, 1=resource
    int* vertex_id;
} ResourceGraph;

ResourceGraph* create_graph(int max_vertices);
int add_request_edge(ResourceGraph* g, int pid, int rid);
int add_allocation_edge(ResourceGraph* g, int rid, int pid);
int convert_to_wfg(ResourceGraph* rag, ResourceGraph** wfg_out);
void print_graph(ResourceGraph* graph);
void free_graph(ResourceGraph* graph);

#endif
```

**Yêu cầu implement**:
- Sử dụng adjacency list hoặc adjacency matrix
- Hỗ trợ thêm request edge (process -> resource)
- Hỗ trợ thêm allocation edge (resource -> process)
- Có thể chuyển đổi RAG thành Wait-For Graph
- Print graph dạng readable để debug

### C. **cycle_detection.h/c** - Module Phát Hiện Chu Trình
**Chức năng**: Implement DFS để phát hiện cycle trong đồ thị

**Header định nghĩa (cycle_detection.h)**:
```c
#ifndef CYCLE_DETECTION_H
#define CYCLE_DETECTION_H

#include "resource_graph.h"

typedef struct {
    int* cycle_path;
    int cycle_length;
    int cycle_start_node;
    int cycle_end_node;
} CycleInfo;

// DFS-based cycle detection
int has_cycle(ResourceGraph* graph, CycleInfo** cycle_list, int* num_cycles);
int dfs_visit(ResourceGraph* graph, int vertex, int* color, 
              int* parent, CycleInfo* cycle_info, int* found);
void extract_cycle_path(int* parent, int start, int end, 
                        CycleInfo* cycle_info);
void print_cycle(CycleInfo* cycle, ResourceGraph* graph);
void free_cycle_info(CycleInfo* cycle);

#endif
```

**Yêu cầu implement**:
- Implement DFS với 3 colors: WHITE (0), GRAY (1), BLACK (2)
- Detect back edge khi gặp GRAY vertex
- Extract full cycle path từ back edge
- Trả về tất cả các cycles nếu có nhiều
- Time complexity: O(V + E)

### D. **deadlock_detection.h/c** - Module Phát Hiện Deadlock
**Chức năng**: Kết hợp RAG + cycle detection để phát hiện deadlock

**Header định nghĩa (deadlock_detection.h)**:
```c
#ifndef DEADLOCK_DETECTION_H
#define DEADLOCK_DETECTION_H

#include "cycle_detection.h"

typedef struct {
    int* deadlocked_pids;
    int num_deadlocked;
    CycleInfo* cycle_chain;
    char** explanation;
} DeadlockReport;

int detect_deadlock_in_system(ProcessResourceInfo* procs, 
                              int num_procs, 
                              DeadlockReport* report);
DeadlockReport* create_deadlock_report();
void analyze_cycle_for_deadlock(CycleInfo* cycle, 
                                ProcessResourceInfo* procs,
                                DeadlockReport* report);
void free_deadlock_report(DeadlockReport* report);

#endif
```

**Yêu cầu implement**:
- Thu thập process/resource info
- Xây dựng RAG từ thông tin
- Chạy cycle detection
- Phân tích cycle để xác định process nào bị deadlock
- Lọc out false positives

### E. **output_handler.h/c** - Module Xuất Kết Quả
**Chức năng**: Format và display kết quả phát hiện deadlock

**Header định nghĩa (output_handler.h)**:
```c
#ifndef OUTPUT_HANDLER_H
#define OUTPUT_HANDLER_H

#include "deadlock_detection.h"

typedef enum {
    FORMAT_TEXT,
    FORMAT_JSON,
    FORMAT_VERBOSE
} OutputFormat;

void display_deadlock_report(DeadlockReport* report, OutputFormat fmt);
void print_summary(DeadlockReport* report);
void print_detailed_wait_chain(DeadlockReport* report);
void print_recommendations(DeadlockReport* report);
void export_to_file(DeadlockReport* report, const char* filename, 
                    OutputFormat fmt);

#endif
```

**Yêu cầu implement**:
- In báo cáo dạng text, JSON, verbose
- Hiển thị danh sách tiến trình bị deadlock
- Hiển thị wait chain/cycle chain
- Gợi ý cách giải quyết
- Xuất ra file nếu cần

### F. **utility.h/c** - Module Hàm Trợ Giúp
**Chức năng**: Các hàm chung, safe allocation, string handling

**Header định nghĩa (utility.h)**:
```c
#ifndef UTILITY_H
#define UTILITY_H

#include <stddef.h>

// Memory management
void* safe_malloc(size_t size);
void* safe_realloc(void* ptr, size_t size);
void safe_free(void** ptr);

// String utilities
char* str_trim(char* str);
int str_starts_with(const char* str, const char* prefix);
char** str_split(const char* str, char delim, int* count);
void free_str_array(char** arr, int count);

// File utilities
int file_exists(const char* path);
char* read_entire_file(const char* path);

// Error handling
void error_exit(const char* msg, int code);
void print_error(const char* msg);

#endif
```

---

## V. MAIN.C - ENTRY POINT

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "process_monitor.h"
#include "resource_graph.h"
#include "deadlock_detection.h"
#include "output_handler.h"

#define VERSION "1.0.0"
#define DEFAULT_INTERVAL 5  // seconds

typedef struct {
    int verbose;
    int continuous_monitor;
    int interval;
    char output_format[32];
    char output_file[256];
} CommandLineArgs;

void print_usage(const char* program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("Options:\n");
    printf("  -h, --help              Show this help message\n");
    printf("  -v, --verbose           Verbose output\n");
    printf("  -c, --continuous        Continuous monitoring\n");
    printf("  -i, --interval SEC      Monitoring interval (default: %d)\n", 
           DEFAULT_INTERVAL);
    printf("  -f, --format FORMAT     Output format: text, json, verbose\n");
    printf("  -o, --output FILE       Output to file\n");
    printf("  --version               Show version\n");
}

int parse_arguments(int argc, char** argv, CommandLineArgs* args) {
    args->verbose = 0;
    args->continuous_monitor = 0;
    args->interval = DEFAULT_INTERVAL;
    strcpy(args->output_format, "text");
    strcpy(args->output_file, "");
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            return -1;  // Signal to print usage
        }
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            args->verbose = 1;
        }
        else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--continuous") == 0) {
            args->continuous_monitor = 1;
        }
        else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interval") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -i requires an argument\n");
                return -2;
            }
            args->interval = atoi(argv[++i]);
            if (args->interval <= 0) {
                fprintf(stderr, "Error: interval must be positive\n");
                return -2;
            }
        }
        else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--format") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -f requires an argument\n");
                return -2;
            }
            strncpy(args->output_format, argv[++i], sizeof(args->output_format) - 1);
        }
        else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -o requires an argument\n");
                return -2;
            }
            strncpy(args->output_file, argv[++i], sizeof(args->output_file) - 1);
        }
        else if (strcmp(argv[i], "--version") == 0) {
            printf("Deadlock Detector v%s\n", VERSION);
            return -3;
        }
        else {
            fprintf(stderr, "Error: Unknown option %s\n", argv[i]);
            return -2;
        }
    }
    
    return 0;
}

int main(int argc, char** argv) {
    CommandLineArgs args;
    int parse_result = parse_arguments(argc, argv, &args);
    
    if (parse_result == -1) {
        print_usage(argv[0]);
        return 0;
    } else if (parse_result == -3) {
        return 0;
    } else if (parse_result < 0) {
        return 1;
    }
    
    if (args.verbose) {
        info_log("Deadlock Detection System Started");
        info_log("Format: %s, Continuous: %s, Interval: %d seconds",
                args.output_format, 
                args.continuous_monitor ? "yes" : "no",
                args.interval);
    }
    
    // Main detection loop
    do {
        // 1. Thu thập thông tin tiến trình
        int num_procs;
        pid_t* pids = get_all_processes(&num_procs);
        
        if (args.verbose) {
            info_log("Collected %d processes", num_procs);
        }
        
        // 2. Xây dựng process resource info
        ProcessResourceInfo* procs = safe_malloc(
            sizeof(ProcessResourceInfo) * num_procs);
        for (int i = 0; i < num_procs; i++) {
            get_process_resources(pids[i], &procs[i]);
        }
        
        // 3. Phát hiện deadlock
        DeadlockReport* report = create_deadlock_report();
        int deadlock_status = detect_deadlock_in_system(procs, num_procs, report);
        
        if (deadlock_status) {
            info_log("DEADLOCK DETECTED!");
            OutputFormat fmt;
            if (strcmp(args.output_format, "json") == 0) {
                fmt = FORMAT_JSON;
            } else if (strcmp(args.output_format, "verbose") == 0) {
                fmt = FORMAT_VERBOSE;
            } else {
                fmt = FORMAT_TEXT;
            }
            
            if (strlen(args.output_file) > 0) {
                export_to_file(report, args.output_file, fmt);
            } else {
                display_deadlock_report(report, fmt);
            }
        } else {
            info_log("No deadlock detected");
        }
        
        // Cleanup
        free_deadlock_report(report);
        free(procs);
        free_process_list(pids);
        
        if (args.continuous_monitor) {
            sleep(args.interval);
        }
        
    } while (args.continuous_monitor);
    
    return 0;
}
```

---

## VI. MAKEFILE

```makefile
CC := gcc
CFLAGS := -Wall -Wextra -O2 -std=c99 -D_POSIX_C_SOURCE=200809L
LDFLAGS := -lpthread

SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin
TEST_DIR := test

# Source files
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
OBJS := $(filter-out $(OBJ_DIR)/main.o,$(OBJS))  # Exclude main.o from library

# Test files
TEST_SRCS := $(wildcard $(TEST_DIR)/*.c)
TESTS := $(patsubst $(TEST_DIR)/%.c,$(BIN_DIR)/%,$(TEST_SRCS))

# Target
TARGET := $(BIN_DIR)/deadlock_detector

all: $(TARGET)

# Create directories
$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

# Main executable
$(TARGET): $(OBJ_DIR) $(BIN_DIR) $(OBJ_DIR)/main.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJ_DIR)/main.o $(OBJS) $(LDFLAGS)
	@echo "Build successful: $(TARGET)"

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Test targets
test: $(TESTS)
	@echo "Running tests..."
	@for test in $(TESTS); do \
		echo "Running $$test..."; \
		./$$test; \
	done

$(BIN_DIR)/%: $(TEST_DIR)/%.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $< $(OBJS) $(LDFLAGS)

# Cleaning
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "Clean completed"

# Phony targets
.PHONY: all test clean

# Help
help:
	@echo "Available targets:"
	@echo "  all    - Build the main executable"
	@echo "  test   - Run unit tests"
	@echo "  clean  - Remove build artifacts"
	@echo "  help   - Show this help message"
```

---

## VII. QUY TẮC TỐI ƯU CHO CURSOR IDE

### **Rule 1: Code Generation Efficiency**
- **Tách biệt các module rõ ràng**: Cursor sẽ generate từng module một cách độc lập
- **Header files đầy đủ**: Luôn provide complete function signatures
- **Comment detailedtrước code**: Viết comment mô tả chính xác trước khi request generate code

### **Rule 2: Cycle Detection - Thuật Toán Chính**
Cursor sẽ ưu tiên implement **DFS-based cycle detection**:
```
1. Initialize all vertices as WHITE (unvisited)
2. For each unvisited vertex:
   a. Call DFS_VISIT(vertex)
   b. In DFS_VISIT:
      - Mark vertex as GRAY (being processed)
      - For each adjacent vertex:
        * If WHITE: recursively call DFS_VISIT
        * If GRAY: Found cycle! Extract and return
      - Mark vertex as BLACK (completed)
3. Return all cycles found
```

### **Rule 3: /proc Filesystem Parsing**
- `/proc/[PID]/status`: Lấy Name, State, Parent PID
- `/proc/[PID]/fd/`: Đọc tất cả open file descriptors
- `/proc/[PID]/locks`: Lấy file locks nếu có
- **Always check if process exists trước khi access**

### **Rule 4: Error Handling Consistency**
Mọi function return:
- `0` hoặc positive number = SUCCESS
- Negative numbers = ERRORS (define constants)
- Luôn check return values

### **Rule 5: Testing Requirements**
Cursor sẽ generate test cases cho:
1. **No deadlock scenario**: Các process không có circular dependency
2. **Simple deadlock**: 2 processes, 2 resources, rõ ràng
3. **Complex deadlock**: N processes, M resources, chu trình phức tạp

---

## VIII. CHỈ DẪN CHO CURSOR GENERATE CODE

### **Workflow Gợi Ý (đưa cho Cursor)**:

1. **Generate Module 1: process_monitor**
   - Implement: `get_all_processes()`, `get_process_info()`, `read_proc_file()`
   - File: `src/process_monitor.c` + `src/process_monitor.h`

2. **Generate Module 2: resource_graph**
   - Implement: Adjacency list representation, `add_request_edge()`, `add_allocation_edge()`
   - File: `src/resource_graph.c` + `src/resource_graph.h`

3. **Generate Module 3: cycle_detection**
   - Implement: DFS-based cycle detection algorithm
   - File: `src/cycle_detection.c` + `src/cycle_detection.h`

4. **Generate Module 4: deadlock_detection**
   - Implement: Integration module that ties everything together
   - File: `src/deadlock_detection.c` + `src/deadlock_detection.h`

5. **Generate Module 5: output_handler**
   - Implement: Result formatting and display
   - File: `src/output_handler.c` + `src/output_handler.h`

6. **Generate Module 6: utility**
   - Implement: Helper functions
   - File: `src/utility.c` + `src/utility.h`

7. **Generate main.c**: Entry point with CLI argument parsing

8. **Generate test files**: Unit tests for each module

9. **Generate Makefile**: Build configuration

---

## IX. PERFORMANCE OPTIMIZATION HINTS

1. **Caching**: Cache `/proc` reads để avoid repeated I/O
2. **Graph representation**: Use adjacency list thay vì matrix cho sparse graphs
3. **DFS optimization**: Sử dụng stack thay recursive nếu graph rất lớn
4. **Memory pool**: Pre-allocate structures cho frequent allocation/deallocation

---

## X. DEBUG & VALIDATION

### Compile & Run:
```bash
make clean && make
./bin/deadlock_detector -v              # Verbose mode
./bin/deadlock_detector -c -i 10        # Continuous, every 10 sec
./bin/deadlock_detector -f json         # JSON output
make test                               # Run unit tests
```

### Debugging:
```bash
gdb ./bin/deadlock_detector
valgrind --leak-check=full ./bin/deadlock_detector
strace -e open,read -o trace.txt ./bin/deadlock_detector
```

---

## XI. PRIORITIZED REQUIREMENTS FOR CURSOR

**Critical (Must-Have)**:
1. ✅ Process monitoring from `/proc`
2. ✅ Resource Allocation Graph construction
3. ✅ DFS-based cycle detection (O(V+E))
4. ✅ Deadlock process identification
5. ✅ Error handling & memory safety

**Important (Should-Have)**:
1. ⚠️ Multiple cycles handling
2. ⚠️ Multiple instance resources support
3. ⚠️ Continuous monitoring mode
4. ⚠️ JSON output format

**Nice-to-Have**:
1. 📌 Graph visualization
2. 📌 Advanced deadlock resolution suggestions
3. 📌 Performance metrics

---

## XII. ESTIMATION & TIMELINE

| Module | Lines | Complexity | Est. Time |
|--------|-------|-----------|-----------|
| process_monitor | 200-300 | Medium | 2-3h |
| resource_graph | 150-200 | Medium | 1.5-2h |
| cycle_detection | 100-150 | High | 2-3h |
| deadlock_detection | 150-200 | High | 2-3h |
| output_handler | 100-150 | Low | 1-1.5h |
| utility | 100-150 | Low | 1-1.5h |
| main + tests | 150-250 | Medium | 2-3h |
| **Total** | **950-1400** | **-** | **12-18h** |

---

## XIII. FINAL NOTES FOR CURSOR

### Context You Need to Know:
- This is for a **Unix/Linux system programming course project**
- Target: **Ubuntu Linux** with `/proc` filesystem
- Focus: **Educational, correctness > performance**
- Scope: **Single-threaded detection engine** (thread-safe where needed)

### Code Quality Standards:
- **No compiler warnings**: Use `-Wall -Wextra`
- **Memory safe**: No leaks, proper bounds checking
- **Well documented**: Every function has header comment
- **Error resilient**: Handle edge cases gracefully
- **Follows POSIX standards**: Use standard C99 + POSIX functions

### Success Criteria:
✅ Successfully detects deadlocks in test scenarios
✅ Compiles without warnings
✅ No memory leaks
✅ Handles process termination gracefully
✅ Generates readable output/reports
✅ Well-structured, modular code
✅ Comprehensive documentation & comments
