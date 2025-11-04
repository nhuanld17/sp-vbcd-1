# Cursor IDE Integration Rules & Prompts

Tài liệu này cung cấp các rules và prompts tối ưu để sử dụng với Cursor IDE.

---

## PHẦN 1: RULES CHO CURSOR IDE

### Tạo File `.cursor/rules` trong Project

```
# =============================================================================
# DEADLOCK DETECTION SYSTEM - CURSOR IDE RULES
# =============================================================================
# Các quy tắc này hướng dẫn Cursor tạo code hiệu quả cho dự án phát hiện 
# Deadlock trên hệ thống Unix/Linux

# =============================================================================
# RULE 1: CODE GENERATION PRINCIPLES
# =============================================================================

Rule: CodeGenerationStyle
- Luôn generate **modular, well-organized code**
- Mỗi module .c phải có corresponding .h file
- Header file phải có `#ifndef` include guards
- Implement từ interface (.h) trước, sau đó chi tiết trong .c
- Không bao giờ có circular dependencies giữa headers

Rule: NamingConventions
- Functions: snake_case (get_process_list, detect_deadlock)
- Constants: UPPER_CASE (MAX_PROCESSES, ERROR_FILE_NOT_FOUND)
- Structs: PascalCase (ResourceGraph, ProcessNode)
- Global vars: g_snake_case (g_process_count)
- Static vars: s_snake_case trong file
- File names: snake_case.c/.h

Rule: CommentingStandard
BEFORE each function, viết:
/*
 * function_name - brief description
 * @param1: description
 * @param2: description
 * @return: what it returns
 * Description: detailed explanation, algorithms, complexity
 * Error handling: how errors are reported
 */

Rule: ErrorHandling
- LUÔN define error codes chung (SUCCESS=0, ERROR_*=-1,-2,...)
- Mỗi function return >= 0 cho success, < 0 cho errors
- Check tất cả pointer returns từ malloc
- Use safe_malloc() wrapper từ utility.c
- Luôn log errors với error_log() macro

Rule: MemoryManagement
- Mỗi malloc() phải có corresponding free() hoặc cleanup_*() function
- Không để dangling pointers
- Sử dụng valgrind-safe coding patterns
- Tránh buffer overflows (use snprintf thay sprintf)
- Initialize tất cả variables trước khi dùng

# =============================================================================
# RULE 2: MODULE-SPECIFIC RULES
# =============================================================================

Rule: ProcessMonitor
- MUST read từ /proc filesystem (không sử dụng system calls khác)
- Implement: get_all_processes(), get_process_info(), read_proc_file()
- Parse /proc/[PID]/status, /proc/[PID]/fd, /proc/[PID]/locks
- Handle process termination gracefully (ENOENT)
- Cache reads để tránh repeated I/O
- Complexity: O(n) where n = number of processes

Rule: ResourceGraph
- Use adjacency list, không adjacency matrix (vì sparse)
- Implement: add_request_edge(), add_allocation_edge()
- Support both single-instance và multiple-instance resources
- Convert RAG to WFG nếu cần
- Memory: O(V+E) where V=vertices, E=edges

Rule: CycleDetection
- MUST implement DFS-based cycle detection
- Use 3-color marking: WHITE(0), GRAY(1), BLACK(2)
- Detect back edges khi gặp GRAY vertex
- Extract complete cycle path, không chỉ node
- Return ALL cycles found, không stop at first
- Time Complexity: MUST be O(V+E)
- Space Complexity: O(V)

Rule: DeadlockDetection
- Gọi cycle detection
- Filter cycles để xác định processes bị deadlock
- Xử lý both single-instance (definite deadlock) và 
  multiple-instance (potential deadlock)
- Recommend kiến thức về Coffman conditions

Rule: OutputHandler
- Support 3 formats: TEXT (default), JSON, VERBOSE
- Include process names, PIDs, wait chain
- Show resource information (ID, type, holders)
- Provide actionable recommendations
- Make output parseable (cho integration with other tools)

Rule: UtilityModule
- safe_malloc(), safe_realloc(), safe_free()
- String utilities: trim, split, starts_with
- File utilities: file_exists, read_file
- Error logging: error_log, debug_log, info_log
- These MUST be used khắp codebase

# =============================================================================
# RULE 3: QUALITY ASSURANCE
# =============================================================================

Rule: CompilerWarnings
- MUST compile với: gcc -Wall -Wextra -std=c99 -pedantic
- KHÔNG được có compiler warnings
- Sử dụng -Werror nếu cần strict mode

Rule: MemorySafety
- MUST pass: valgrind --leak-check=full
- KHÔNG được memory leaks
- KHÔNG được out-of-bounds access
- KHÔNG được use-after-free

Rule: Robustness
- Handle: process không tồn tại (ENOENT)
- Handle: permission denied (EACCES)
- Handle: out of memory
- Handle: graph với >10000 vertices
- Handle: circular references trong resource hierarchy

Rule: Testing
- Generate unit tests cho mỗi module
- Test cases: no deadlock, simple deadlock, complex deadlock
- Test edge cases: empty graph, single process, disconnected graph

Rule: Documentation
- Header files: chứa đầy đủ function descriptions
- Source files: explain complex algorithms
- Main.c: có usage examples và CLI help
- README: hướng dẫn build, run, debug

# =============================================================================
# RULE 4: PERFORMANCE RULES
# =============================================================================

Rule: GraphRepresentation
- Sử dụng adjacency list (không matrix) cho sparse graphs
- Allocate incrementally, không pre-allocate quá lớn
- Reuse graph structures nếu continuous monitoring

Rule: AlgorithmOptimization
- DFS: tối ưu để tránh revisiting nodes
- Process discovery: cache results
- File reading: minimize /proc accesses
- Only recompute khi có changes

Rule: ResourceUsage
- Max memory: < 10MB cho 1000 processes
- Acceptable time: < 500ms cho 1000 processes
- No infinite loops hoặc busy waiting

# =============================================================================
# RULE 5: CODE GENERATION WORKFLOW
# =============================================================================

Rule: GenerationOrder
1. FIRST: Header files (.h) cho tất cả modules
2. THEN: Utility module (utility.c/.h)
3. THEN: Process monitor module
4. THEN: Resource graph module
5. THEN: Cycle detection module
6. THEN: Deadlock detection module
7. THEN: Output handler module
8. THEN: main.c integration
9. FINALLY: Unit tests

Rule: GenerationSize
- Mỗi file: 100-300 lines (manageable size)
- Không viết files quá dài (>500 lines)
- Split logic thành multiple functions

Rule: IncompleteCodePattern
Nếu code không complete, ALWAYS:
- Thêm TODO comments
- Provide skeleton functions
- Giải thích cần implement gì

# =============================================================================
# RULE 6: SPECIAL HANDLING
# =============================================================================

Rule: /procFilesystemHandling
- ALWAYS check file tồn tại trước khi read
- ALWAYS handle ENOENT (process terminated)
- ALWAYS handle EACCES (permission)
- Use open()/read() hoặc fopen()/fgets()
- Parse line-by-line, không assume format

Rule: GraphCycleHandling
- Một cycle có thể liên quan đến 2-N processes
- Một process có thể trong multiple cycles
- Extract cycle as ordered list: P1→R1→P2→R2→P1
- Store both processes và resources trong cycle

Rule: EdgeCaseHandling
- Xử lý process list rỗng
- Xử lý process với 0 resources
- Xử lý graph với 1 vertex (không cycle)
- Xử lý multi-digit PIDs (> 9999)

# =============================================================================
# RULE 7: DEBUGGING AIDS
# =============================================================================

Rule: DebugOutput
- ALWAYS provide debug_log() macro implementation
- Generate với ifdef DEBUG blocks
- Include timing info: printf timing when -v flag
- Include graph visualization output

Rule: ErrorMessages
- Descriptive: "Failed to open /proc/1234/status: Permission denied"
- Include errno details
- Suggest solutions khi có thể
- Log to stderr, info_log to stdout

# =============================================================================
# RULE 8: BUILD SYSTEM
# =============================================================================

Rule: MakefileGeneration
- CC := gcc
- CFLAGS := -Wall -Wextra -O2 -std=c99 -D_POSIX_C_SOURCE=200809L
- LDFLAGS := -lpthread
- Targets: all, clean, test
- Output: bin/deadlock_detector
- Objects: obj/*.o

# =============================================================================
# RULE 9: PROJECT STRUCTURE
# =============================================================================

Rule: DirectoryLayout
deadlock_detector/
├── src/
│   ├── *.c files (8-10 files)
│   ├── *.h files (same)
│   └── config.h (constants)
├── test/
│   ├── test_*.c (3-5 files)
│   └── test_data.c
├── obj/ (generated)
├── bin/ (generated)
├── Makefile
├── README.md
├── instruction.md
└── .gitignore

# =============================================================================
# END OF RULES
# =============================================================================
```

---

## PHẦN 2: PROMPTS CHO CURSOR

### Prompt 1: Generate Header Files (All at once)

```
Based on the deadlock detection system architecture, generate the following 
header files for the project:

1. src/config.h - macros and constants
2. src/utility.h - utility functions
3. src/process_monitor.h - process info collection
4. src/resource_graph.h - resource allocation graph
5. src/cycle_detection.h - cycle detection algorithm
6. src/deadlock_detection.h - deadlock detection engine
7. src/output_handler.h - report generation

Requirements:
- Include proper #ifndef guards
- Add comprehensive function descriptions
- Define all necessary data structures
- Include error codes
- Add function prototypes with full parameters
- No implementation, just declarations
- Make sure headers are self-contained (minimal dependencies)

Follow naming conventions:
- Functions: snake_case
- Constants: UPPER_CASE
- Structs: PascalCase
```

### Prompt 2: Generate Utility Module

```
Generate the utility module (utility.c and verify consistency with utility.h):

Functions to implement:
1. safe_malloc() - malloc wrapper with error checking
2. safe_realloc() - realloc wrapper
3. safe_free() - safe free wrapper
4. str_trim() - trim whitespace from string
5. str_starts_with() - check if string starts with prefix
6. str_split() - split string by delimiter
7. free_str_array() - free array of strings
8. file_exists() - check if file exists
9. read_entire_file() - read file into string
10. error_exit() - print error and exit
11. Implement debug_log, error_log, info_log macros

Requirements:
- Proper error handling
- Memory safety (no leaks)
- POSIX compliance
- Include comprehensive inline comments
- Handle NULL pointers gracefully
- Time complexity: O(n) for string operations
```

### Prompt 3: Generate Process Monitor Module

```
Generate the process_monitor.c implementation based on process_monitor.h:

Key functions:
1. get_all_processes() - list all running processes from /proc
2. get_process_info() - read /proc/[PID]/status, parse Name/State/PPID
3. get_process_resources() - determine held and waiting resources
4. read_proc_file() - read arbitrary /proc file with error handling
5. get_open_files() - read /proc/[PID]/fd
6. get_file_locks() - read /proc/[PID]/locks
7. free_process_info() - cleanup function

Requirements:
- Read from /proc filesystem directly
- Handle ENOENT (process terminated) gracefully
- Handle EACCES (permission denied) gracefully
- Parse format correctly (/proc/[PID]/status format)
- Implement caching to avoid repeated reads
- Time Complexity: O(n) where n = number of processes
- Memory: allocated with safe_malloc()
- Include error handling and validation

Focus:
- Accuracy in parsing /proc files
- Robustness when processes terminate
- Efficient file I/O
```

### Prompt 4: Generate Resource Graph Module

```
Generate the resource_graph.c implementation:

Key functions:
1. create_graph() - allocate and initialize resource graph
2. add_request_edge() - add P→R edge (process waits for resource)
3. add_allocation_edge() - add R→P edge (resource assigned to process)
4. add_process_vertex() - add process node to graph
5. add_resource_vertex() - add resource node to graph
6. get_vertex_id() - convert PID/RID to vertex ID
7. find_vertex_by_id() - lookup vertex by PID/RID
8. print_graph() - debug output of graph structure
9. free_graph() - cleanup graph memory

Requirements:
- Use adjacency list representation (not matrix)
- Support both single and multiple instance resources
- Efficient edge lookup: O(1) or O(log E)
- Memory efficient for sparse graphs
- Can convert to Wait-For Graph (WFG) if needed
- Vertex capacity should be pre-allocated
- Edges dynamically allocated

Data Structure:
- Adjacency list using linked lists or arrays
- Each vertex stores: id, type (process/resource), instance count
- Track both outgoing and incoming edges for each vertex

Focus:
- Efficient graph construction
- Proper memory management
- Clear vertex/edge semantics
```

### Prompt 5: Generate Cycle Detection Module

```
Generate the cycle_detection.c implementation for DFS-based cycle detection:

Algorithm: Depth-First Search with color marking
- WHITE (0): unvisited
- GRAY (1): currently being processed (in recursion stack)
- BLACK (2): finished processing

Key functions:
1. has_cycle() - main entry point, returns 1 if cycle exists
2. dfs_visit() - recursive DFS traversal
3. find_all_cycles() - find all cycles in graph
4. extract_cycle_path() - reconstruct cycle vertices from parent array
5. detect_back_edge() - identify back edge (marks deadlock cycle)
6. print_cycle() - debug output for a cycle
7. free_cycle_info() - cleanup cycle structures

Requirements:
- Time Complexity: MUST be O(V+E)
- Space Complexity: O(V)
- Find ALL cycles, not just first one
- Extract complete cycle paths as ordered vertex lists
- Handle multiple connected components
- Return cycles as CycleInfo[] array

Algorithm specifics:
1. Initialize color[v] = WHITE for all vertices
2. For each unvisited vertex, call DFS_VISIT
3. In DFS_VISIT(v):
   - Set color[v] = GRAY
   - For each neighbor u:
     * If color[u] == WHITE: recursively DFS_VISIT(u)
     * If color[u] == GRAY: back edge found! Extract cycle
   - Set color[v] = BLACK
4. Store cycle path for later analysis

Focus:
- Correctness of back edge detection
- Efficient cycle extraction
- Handling large graphs (up to 10000 vertices)
```

### Prompt 6: Generate Deadlock Detection Module

```
Generate deadlock_detection.c - integration of all components:

Key functions:
1. detect_deadlock_in_system() - main detection entry point
2. build_rag_from_processes() - create RAG from ProcessResourceInfo
3. analyze_cycles_for_deadlock() - determine which processes are deadlocked
4. filter_actual_deadlocks() - distinguish definite vs potential deadlock
5. create_deadlock_report() - generate DeadlockReport structure
6. identify_deadlocked_processes() - extract PIDs from cycles
7. generate_recommendations() - suggest solutions
8. free_deadlock_report() - cleanup

Workflow:
1. Call get_all_processes() via process_monitor
2. For each process, get ProcessResourceInfo
3. Build RAG using resource_graph module
4. Run cycle detection via cycle_detection module
5. Analyze each cycle to identify deadlocked processes
6. Create structured report

Requirements:
- Integrate process_monitor, resource_graph, cycle_detection
- Handle single-instance: cycle = definite deadlock
- Handle multi-instance: cycle = potential deadlock (need deeper analysis)
- Extract deadlocked PIDs from cycle paths
- Include deadlock chain/cycle info in report
- Provide meaningful recommendations

Rules:
- Single-instance resource: ANY cycle → DEADLOCK
- Multi-instance resource: cycle → possible deadlock (may resolve)
- Report should include: PID list, cycle chain, wait chain

Focus:
- Correct deadlock determination
- Meaningful cycle-to-process mapping
- Report quality and clarity
```

### Prompt 7: Generate Output Handler Module

```
Generate output_handler.c - result formatting and display:

Key functions:
1. display_deadlock_report() - main output function
2. print_summary() - print deadlock summary
3. print_detailed_wait_chain() - print cycle as chain
4. print_process_info() - print individual process details
5. print_resource_info() - print resource info
6. print_recommendations() - print resolution suggestions
7. export_to_file() - write report to file
8. format_as_text() - format report as plain text
9. format_as_json() - format report as JSON
10. format_as_verbose() - format detailed verbose output

Output Formats:

TEXT (default):
========================================
DEADLOCK DETECTED!
========================================
Deadlocked Processes: PID1, PID2, ...
Cycle Chain: P1 → R1 → P2 → R2 → P1
Recommendations: [list of actions]

JSON:
{
  "deadlock_detected": true,
  "timestamp": "...",
  "deadlocked_processes": [...],
  "cycle": [...],
  "recommendations": [...]
}

VERBOSE:
[Long form with full process details, resource info, wait chains, etc.]

Requirements:
- Generate readable, professional output
- Support 3 output formats
- Can write to stdout or file
- JSON output must be valid
- Include timestamps
- Make recommendations actionable
- Pretty-print for human readability

Focus:
- Output quality
- Format compliance
- Clear communication of deadlock info
```

### Prompt 8: Generate main.c

```
Generate main.c - command-line interface and orchestration:

Requirements:
1. Command-line argument parsing:
   -h, --help: Show usage
   -v, --verbose: Verbose output
   -c, --continuous: Continuous monitoring mode
   -i, --interval SEC: Monitoring interval (default 5)
   -f, --format FORMAT: text, json, verbose
   -o, --output FILE: Output file
   --version: Show version

2. Program flow:
   - Parse arguments
   - Initialize systems
   - Detect deadlock once or continuously
   - Display results
   - Cleanup and exit

3. Features:
   - One-shot detection (default)
   - Continuous monitoring with interval
   - Multiple output formats
   - File output support
   - Proper signal handling (SIGINT for Ctrl+C)

4. Error handling:
   - Invalid arguments
   - File open errors
   - System errors
   - Graceful degradation

Requirements:
- Include usage help message
- Support version string
- Clean program exit
- Resource cleanup (free all memory before exit)
- Work with both text and file output
- Handle continuous mode gracefully

Focus:
- User experience
- Argument validation
- Clean integration of modules
```

### Prompt 9: Generate Unit Tests

```
Generate comprehensive unit tests:

test_graph.c:
- Test graph creation/initialization
- Test adding vertices and edges
- Test edge lookup
- Test graph cleanup (memory safety)
- Test large graphs (1000+ vertices)

test_cycle.c:
- Test graph with no cycles
- Test simple cycle (2 nodes)
- Test complex cycles
- Test multiple disjoint cycles
- Test performance with large graphs

test_system.c:
- Integration test: process monitor + graph + cycle detection
- Test with mock process data
- Test deadlock detection end-to-end
- Test output generation

Requirements:
- Each test: print PASS or FAIL
- Use assert() macros
- Include memory checks (no leaks)
- Mock data for /proc when needed
- Measure execution time
- Exit with 0 if all pass, 1 if fail

Focus:
- Correctness verification
- Edge case coverage
- Memory safety verification
```

### Prompt 10: Generate Makefile

```
Generate Makefile for building the project:

Requirements:
- Compiler: gcc
- Flags: -Wall -Wextra -O2 -std=c99 -D_POSIX_C_SOURCE=200809L
- Libraries: -lpthread
- Objects: obj/*.o
- Binary: bin/deadlock_detector
- Tests: bin/test_*

Targets:
- all: build main executable
- test: build and run all tests
- clean: remove build artifacts
- help: show available targets

Features:
- Parallel build support (-j)
- Automatic directory creation
- Dependency tracking
- Verbose mode option
- Install target (optional)

Focus:
- Build speed
- Cleanliness
- Ease of use
```

---

## PHẦN 3: INTEGRATION CHECKLIST

Trước khi bắt đầu sử dụng Cursor:

- [ ] Tạo project directory structure
- [ ] Copy instruction.md vào project
- [ ] Setup .cursor/rules file (từ PHẦN 1)
- [ ] Read README-guide.md để hiểu architecture
- [ ] Prepare Prompt 1-10 để sử dụng

Workflow:

1. [ ] Generate headers (Prompt 1)
2. [ ] Generate utility (Prompt 2)
3. [ ] Generate process_monitor (Prompt 3)
4. [ ] Generate resource_graph (Prompt 4)
5. [ ] Generate cycle_detection (Prompt 5)
6. [ ] Generate deadlock_detection (Prompt 6)
7. [ ] Generate output_handler (Prompt 7)
8. [ ] Generate main.c (Prompt 8)
9. [ ] Generate tests (Prompt 9)
10. [ ] Generate Makefile (Prompt 10)
11. [ ] Build: `make clean && make`
12. [ ] Test: `make test`
13. [ ] Debug: resolve any issues
14. [ ] Final check: `make clean && make` (no warnings)

---

## PHẦN 4: EXPECTED OUTPUT

Sau khi chạy tất cả prompts, bạn sẽ có:

```
deadlock_detector/
├── src/
│   ├── config.h (150 lines)
│   ├── utility.h (60 lines)
│   ├── utility.c (200 lines)
│   ├── process_monitor.h (50 lines)
│   ├── process_monitor.c (250 lines)
│   ├── resource_graph.h (70 lines)
│   ├── resource_graph.c (300 lines)
│   ├── cycle_detection.h (60 lines)
│   ├── cycle_detection.c (280 lines)
│   ├── deadlock_detection.h (50 lines)
│   ├── deadlock_detection.c (250 lines)
│   ├── output_handler.h (50 lines)
│   └── output_handler.c (300 lines)
├── test/
│   ├── test_graph.c (150 lines)
│   ├── test_cycle.c (200 lines)
│   └── test_system.c (150 lines)
├── src/main.c (300 lines)
├── Makefile (100 lines)
└── [compiled binaries]

Total: ~3500+ lines of production code
Total: ~500+ lines of tests
Build time: < 2 seconds
Binary size: < 100KB
```

---

## PHẦN 5: QUALITY VERIFICATION

Sau khi Cursor generate xong:

```bash
# Compile check
gcc -Wall -Wextra -std=c99 -pedantic src/*.c test/*.c -o test_app

# Memory check
valgrind --leak-check=full ./bin/deadlock_detector -v

# Warning check
make clean && make 2>&1 | grep -i warning
# Expected: (no output = no warnings)

# Test run
make test
# Expected: All tests passed ✓

# Functional test
./bin/deadlock_detector -v
# Expected: Successful execution, proper output
```

---

**End of Cursor IDE Integration Guide**
