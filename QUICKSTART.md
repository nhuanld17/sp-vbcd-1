# Quick Start Guide - Deadlock Detection System

Hướng dẫn nhanh để bắt đầu với dự án phát hiện Deadlock.

---

## 🚀 30 Giây Setup

```bash
# 1. Create project structure
mkdir -p deadlock_detector/{src,test,obj,bin}
cd deadlock_detector

# 2. Create .cursor directory for rules
mkdir -p .cursor

# 3. Copy instruction files
cp /path/to/instruction.md .
cp /path/to/cursor-rules.md .cursor/rules
cp /path/to/README-guide.md README.md
```

---

## 📝 Using with Cursor IDE

### Step 1: Open Project
- Open `deadlock_detector/` folder in Cursor IDE

### Step 2: Create .cursor/rules
- New file: `.cursor/rules`
- Copy content from `cursor-rules.md` PHẦN 1 (Rules for Cursor)

### Step 3: Use Prompts in Order

**Prompt Sequence** (copy each into Cursor chat):

#### 1️⃣ Generate Header Files
```
Based on the deadlock detection system architecture described in 
instruction.md, generate the following header files:

1. src/config.h - macros and constants (MAX_PROCESSES, ERROR_* codes)
2. src/utility.h - utility functions (malloc, string, file ops)
3. src/process_monitor.h - collect process info from /proc
4. src/resource_graph.h - build Resource Allocation Graph
5. src/cycle_detection.h - DFS-based cycle detection
6. src/deadlock_detection.h - main deadlock detection logic
7. src/output_handler.h - report generation and display

Requirements:
- Proper #ifndef guards on all headers
- All structs and constants defined
- Complete function prototypes
- Comprehensive comments
- No circular dependencies
- Follow naming: functions=snake_case, constants=UPPER_CASE, structs=PascalCase
```

#### 2️⃣ Generate Utility Module
```
Generate utility.c implementation to match utility.h:

Implement these functions:
- safe_malloc(), safe_realloc(), safe_free() with error checking
- str_trim(), str_starts_with(), str_split()
- free_str_array(), file_exists(), read_entire_file()
- error_exit() function
- error_log(), debug_log(), info_log() macros

Quality requirements:
- Must compile without warnings with -Wall -Wextra
- No memory leaks
- Proper error handling
- Follow existing code patterns
```

#### 3️⃣ Generate Process Monitor
```
Generate process_monitor.c to read process info from /proc:

Implement:
- get_all_processes() → list all PIDs from /proc
- get_process_info() → parse /proc/[PID]/status
- get_process_resources() → determine held/waiting resources
- read_proc_file() → safely read any /proc file
- free_process_info() → cleanup function

Must handle:
- ENOENT when process terminates
- EACCES permission denied
- Parse /proc/[PID]/status format correctly
- Cache reads to avoid repeated I/O
```

#### 4️⃣ Generate Resource Graph
```
Generate resource_graph.c to build Resource Allocation Graph:

Implement:
- create_graph() → initialize graph structure
- add_process_vertex() → add process node
- add_resource_vertex() → add resource node
- add_request_edge() → P→R edge (process waits)
- add_allocation_edge() → R→P edge (resource assigned)
- print_graph() → debug visualization
- free_graph() → cleanup

Use adjacency list (not matrix) for efficiency.
Support single and multiple instance resources.
```

#### 5️⃣ Generate Cycle Detection
```
Generate cycle_detection.c - DFS-based cycle detection:

Implement DFS with 3-color marking:
- WHITE (0) = unvisited
- GRAY (1) = in recursion stack
- BLACK (2) = finished

Functions:
- has_cycle() → main entry point
- dfs_visit() → recursive DFS traversal  
- find_all_cycles() → find ALL cycles
- extract_cycle_path() → reconstruct cycle vertices
- print_cycle() → debug output

MUST have O(V+E) time complexity.
Find all cycles, not just first.
Return complete cycle paths.
```

#### 6️⃣ Generate Deadlock Detection
```
Generate deadlock_detection.c - main detection logic:

Integrate all components:
1. Collect process data
2. Build RAG
3. Run cycle detection
4. Analyze cycles for deadlocks
5. Generate report

Implement:
- detect_deadlock_in_system()
- build_rag_from_processes()
- analyze_cycles_for_deadlock()
- identify_deadlocked_processes()
- create_deadlock_report()
- free_deadlock_report()

Handle single-instance (definite deadlock) vs 
multi-instance (potential deadlock).
```

#### 7️⃣ Generate Output Handler
```
Generate output_handler.c - format and display results:

Support 3 output formats:

TEXT (default):
- Clean summary of deadlock
- List of PIDs
- Cycle chain
- Recommendations

JSON:
- Valid JSON structure
- Complete deadlock info
- Timestamps
- Machine-parseable

VERBOSE:
- Detailed process info
- Resource info
- Full wait chains
- Explanations

Functions:
- display_deadlock_report()
- format_as_text/json/verbose()
- export_to_file()
```

#### 8️⃣ Generate main.c
```
Generate main.c - CLI and orchestration:

Command-line options:
-h, --help
-v, --verbose
-c, --continuous (keep running)
-i, --interval SEC (default 5)
-f, --format FORMAT (text, json, verbose)
-o, --output FILE
--version

Main flow:
1. Parse arguments
2. Initialize systems
3. Run detection (once or continuous)
4. Display results
5. Cleanup and exit

Must handle:
- Invalid arguments
- Signal handling (SIGINT)
- Resource cleanup
```

#### 9️⃣ Generate Tests
```
Generate unit tests in test/ directory:

test_graph.c:
- Test graph creation/deletion
- Test adding edges
- Test large graphs
- Memory checks

test_cycle.c:
- No cycles
- Simple cycle
- Complex cycles  
- Multiple cycles

test_system.c:
- End-to-end integration
- Mock process data
- Output formatting

All tests should:
- Print PASS/FAIL
- Check memory (valgrind-safe)
- Return 0 (success) or 1 (failure)
```

#### 🔟 Generate Makefile
```
Generate Makefile:

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lpthread

Targets:
all → compile main executable
test → build and run tests
clean → remove build artifacts
help → show targets

Output:
bin/deadlock_detector (main executable)
bin/test_* (test executables)
obj/*.o (object files)
```

---

## ✅ Verification Checklist

After Cursor generates all code:

```bash
# 1. Build project
cd deadlock_detector
make clean && make

# Expected: Compilation succeeds with NO warnings

# 2. Run tests
make test

# Expected: All tests PASSED

# 3. Check for memory leaks
valgrind --leak-check=full ./bin/deadlock_detector -v

# Expected: "no leaks are possible"

# 4. Run program
./bin/deadlock_detector -v

# Expected: Program runs, shows process info or "No deadlock"

# 5. Test JSON output
./bin/deadlock_detector -f json

# Expected: Valid JSON output

# 6. Test continuous mode (Ctrl+C to stop)
./bin/deadlock_detector -c -i 2

# Expected: Runs for 2 seconds, shows output, can be interrupted
```

---

## 🐛 Troubleshooting

### Issue: Compilation warnings
**Solution**: Check Cursor generated code for:
- Unused variables → initialize or remove
- Implicit declarations → include proper headers
- Type mismatches → explicit casts
- Format string errors → check printf args

### Issue: Memory leaks in valgrind
**Solution**: Check that:
- All malloc() calls have free() in cleanup functions
- No dangling pointers
- Proper cleanup in main() before exit

### Issue: /proc files not readable
**Solution**: 
- Some /proc files need root access
- Cursor code should handle EACCES errors gracefully
- Skip processes that can't be read

### Issue: Cycle not detected
**Solution**:
- Verify graph is built correctly (debug output)
- Check DFS implementation (3-color marking)
- Verify adjacency list has all edges
- Check if cycle detection finds back edges

### Issue: Test failures
**Solution**:
- Run individual tests with verbose output
- Use GDB to debug: `gdb ./bin/test_graph`
- Check test data initialization
- Verify assertions are correct

---

## 📊 Expected Structure After Generation

```
deadlock_detector/
├── src/
│   ├── config.h                 (Constants & macros)
│   ├── utility.h/c              (Helper functions)
│   ├── process_monitor.h/c      (/proc reading)
│   ├── resource_graph.h/c       (RAG construction)
│   ├── cycle_detection.h/c      (DFS algorithm)
│   ├── deadlock_detection.h/c   (Main logic)
│   ├── output_handler.h/c       (Report generation)
│   └── main.c                   (CLI + orchestration)
├── test/
│   ├── test_graph.c             (Graph tests)
│   ├── test_cycle.c             (Cycle tests)
│   └── test_system.c            (Integration tests)
├── Makefile
├── README.md
├── instruction.md
└── .cursor/
    └── rules

Total code: ~3500 lines
Executables: 
  - bin/deadlock_detector (main)
  - bin/test_graph, bin/test_cycle, bin/test_system
```

---

## 📈 Code Quality Metrics (Expected)

| Metric | Expected | Check |
|--------|----------|-------|
| Compiler Warnings | 0 | `make 2>&1 \| grep warning` |
| Memory Leaks | 0 | `valgrind --leak-check=full` |
| Code Coverage | >80% | Run all tests |
| Execution Time (1000 procs) | <500ms | `time ./bin/deadlock_detector` |
| Memory Usage | <10MB | `ps aux \| grep deadlock` |
| Cycles Detected | 100% | Compare with manual verification |

---

## 🎯 Key Implementation Points (for Cursor)

### 1. Process Monitor
- Read from `/proc/[PID]/status` for process names
- Read `/proc/[PID]/fd` for open files
- Handle ENOENT gracefully when processes terminate

### 2. Resource Graph
- Use adjacency list (linked lists) NOT matrix
- Support adding edges: P→R (request), R→P (allocation)
- Efficient for sparse graphs (most graphs are sparse)

### 3. Cycle Detection
- Implement DFS with 3 colors: WHITE, GRAY, BLACK
- Back edge (to GRAY vertex) = cycle found
- Extract and store complete cycle paths

### 4. Deadlock Detection
- Single-instance resource + cycle = DEADLOCK (100% sure)
- Multi-instance resource + cycle = potential deadlock (may resolve)
- Report should include: PIDs, cycle chain, wait chain

### 5. Output Handler
- TEXT format: human-readable summary
- JSON format: machine-parseable, can integrate with other tools
- VERBOSE: detailed debugging information

---

## 🚀 Performance Tips (for Cursor)

1. **Avoid repeated /proc reads**: Cache results
2. **Graph representation**: Adjacency list for sparse graphs
3. **DFS optimization**: Single pass, no revisiting
4. **Memory allocation**: Pre-allocate reasonable sizes
5. **I/O optimization**: Batch /proc reads when possible

---

## 📚 Key Algorithms (Summary for Cursor)

### DFS Cycle Detection
```
Color[v] = WHITE (unvisited)
Color[v] = GRAY (in process)
Color[v] = BLACK (finished)

For each unvisited vertex v:
  DFS(v):
    Color[v] = GRAY
    For each neighbor u:
      If Color[u] == WHITE: DFS(u)
      If Color[u] == GRAY: CYCLE FOUND!
    Color[v] = BLACK

Time: O(V+E), Space: O(V)
```

### Deadlock Detection
```
1. For each process P:
   - Read which resources it holds
   - Read which resources it waits for

2. Build graph:
   - Vertices: processes + resources
   - Edges: P→R (waiting), R→P (allocated)

3. Find cycles using DFS

4. For each cycle:
   - Extract involved processes
   - Check if single-instance (definite) or multi-instance (potential)
   - Report as deadlock

5. Output results
```

---

## 💾 Files to Use with Cursor

1. **instruction.md** - Full detailed instructions
2. **cursor-rules.md** - Rules and prompts
3. **README-guide.md** - Architecture documentation
4. **This file** - Quick reference

---

## ⏱️ Estimated Timeline

| Task | Time | Status |
|------|------|--------|
| Generate headers | 5 min | ⏳ |
| Generate utility | 10 min | ⏳ |
| Generate process_monitor | 15 min | ⏳ |
| Generate resource_graph | 15 min | ⏳ |
| Generate cycle_detection | 20 min | ⏳ |
| Generate deadlock_detection | 20 min | ⏳ |
| Generate output_handler | 15 min | ⏳ |
| Generate main.c | 15 min | ⏳ |
| Generate tests | 15 min | ⏳ |
| Generate Makefile | 5 min | ⏳ |
| **Testing & debugging** | **30-60 min** | ⏳ |
| **TOTAL** | **2.5-3.5 hours** | ⏳ |

---

## ✨ Success Criteria

Your final implementation should:

✅ Compile without warnings  
✅ Pass all unit tests  
✅ No memory leaks (valgrind clean)  
✅ Detect deadlocks correctly  
✅ Handle edge cases gracefully  
✅ Well-documented code  
✅ Modular architecture  
✅ Efficient performance  

---

## 🎓 Learning Outcomes

By completing this project, you will understand:

- **Operating Systems**: Deadlock, synchronization, resource management
- **C Programming**: Pointers, structures, memory management, file I/O
- **Algorithms**: Graph representation, DFS cycle detection, algorithm analysis
- **Linux/Unix**: `/proc` filesystem, system programming, debugging tools
- **Software Design**: Modular architecture, clean interfaces, error handling

---

**Start with the 30-second setup above, then follow the prompts in order!**

Good luck! 🚀
