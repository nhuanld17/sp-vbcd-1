# Tóm Tắt - Hệ Thống Phát Hiện Deadlock cho Cursor IDE

Bạn đã nhận được bộ hoàn chỉnh để phát triển chương trình phát hiện Deadlock trên Unix/Linux.

---

## 📦 Các File Tạo Được

1. **instruction.md** (15KB)
   - Hướng dẫn chi tiết 100+ trang
   - Tất cả quy tắc viết code
   - Mô tả từng module
   - API chi tiết
   - Code examples
   - **→ Dùng để import vào Cursor làm system prompt**

2. **cursor-rules.md** (10KB)
   - Quy tắc cụ thể cho Cursor IDE
   - 10 prompts sẵn sàng để sử dụng
   - Integration checklist
   - **→ Tạo `.cursor/rules` file với nội dung này**

3. **README-guide.md** (8KB)
   - Tổng quan kiến trúc hệ thống
   - Mô tả từng module
   - Data structures
   - Algorithms
   - Testing guide
   - **→ Tài liệu tham khảo chính**

4. **QUICKSTART.md** (6KB)
   - Hướng dẫn 30 giây
   - 10 prompts sẵn sàng copy-paste
   - Verification checklist
   - Troubleshooting
   - **→ Sử dụng để bắt đầu nhanh**

5. **File này** - Tóm tắt tổng quan

---

## 🎯 Cách Sử Dụng (Step by Step)

### Step 1: Chuẩn Bị Project (2 phút)
```bash
# Tạo structure
mkdir -p deadlock_detector/{src,test,obj,bin}
cd deadlock_detector

# Tạo .cursor directory
mkdir -p .cursor
```

### Step 2: Tạo Cursor Rules (1 phút)
```bash
# Tạo file .cursor/rules
# Copy phần "RULE 1-9" từ cursor-rules.md
```

### Step 3: Import Instruction vào Cursor (1 phút)
- Mở Cursor IDE
- New file: `.cursor/instructions` (optional)
- Paste toàn bộ content từ `instruction.md`

### Step 4: Generate Code theo Thứ Tự (2-3 giờ)
Dùng 10 prompts trong `cursor-rules.md` PHẦN 2 hoặc `QUICKSTART.md`:

```
Prompt 1  → src/*.h (header files)
Prompt 2  → src/utility.c
Prompt 3  → src/process_monitor.c
Prompt 4  → src/resource_graph.c
Prompt 5  → src/cycle_detection.c
Prompt 6  → src/deadlock_detection.c
Prompt 7  → src/output_handler.c
Prompt 8  → src/main.c
Prompt 9  → test/*.c (unit tests)
Prompt 10 → Makefile
```

### Step 5: Build & Verify (30 phút)
```bash
make clean && make
make test
valgrind --leak-check=full ./bin/deadlock_detector -v
```

---

## 🚀 Quick Reference: 10 Prompts

### 1️⃣ Headers
```
Generate all 7 header files (config.h, utility.h, process_monitor.h, 
resource_graph.h, cycle_detection.h, deadlock_detection.h, output_handler.h)
with complete function prototypes and data structures.
```

### 2️⃣ Utility
```
Generate utility.c with: safe_malloc/free, string functions, file I/O,
error logging macros, memory management helpers.
```

### 3️⃣ Process Monitor
```
Generate process_monitor.c to read /proc filesystem:
- get_all_processes()
- get_process_info()
- get_process_resources()
- Handle ENOENT and EACCES errors gracefully
```

### 4️⃣ Resource Graph
```
Generate resource_graph.c to build RAG:
- create_graph()
- add_request_edge() / add_allocation_edge()
- Use adjacency list representation
- Support single and multiple instance resources
```

### 5️⃣ Cycle Detection
```
Generate cycle_detection.c with DFS algorithm:
- Color marking: WHITE, GRAY, BLACK
- detect back edges for cycles
- Extract full cycle paths
- O(V+E) complexity
```

### 6️⃣ Deadlock Detection
```
Generate deadlock_detection.c to orchestrate detection:
- Integrate all modules
- Analyze cycles to identify deadlocked processes
- Handle single vs multiple instance resources
- Generate DeadlockReport
```

### 7️⃣ Output Handler
```
Generate output_handler.c supporting 3 formats:
- TEXT: human-readable summary
- JSON: machine-parseable
- VERBOSE: detailed debugging info
```

### 8️⃣ Main
```
Generate main.c with CLI:
- Arguments: -h -v -c -i -f -o --version
- One-shot or continuous monitoring
- Proper resource cleanup
- Signal handling
```

### 9️⃣ Tests
```
Generate comprehensive unit tests:
- test_graph.c: graph operations
- test_cycle.c: cycle detection
- test_system.c: integration tests
- All tests with memory safety checks
```

### 🔟 Makefile
```
Generate Makefile with:
- all: compile main executable
- test: build and run tests
- clean: remove artifacts
- gcc with -Wall -Wextra -std=c99 -O2
```

---

## 📊 Kiến Trúc Hệ Thống (Ngắn gọn)

```
Input: Các tiến trình đang chạy
  ↓
Process Monitor: Đọc từ /proc
  ↓
Resource Graph: Xây dựng RAG (Process + Resource nodes/edges)
  ↓
Cycle Detection: DFS để tìm chu trình
  ↓
Deadlock Analysis: Xác định tiến trình bị deadlock
  ↓
Output Handler: Sinh báo cáo (Text/JSON/Verbose)
  ↓
Output: Danh sách tiến trình bị deadlock + gợi ý
```

**Algorithms:**
- **Graph:** Adjacency list (efficient for sparse)
- **Cycle Detection:** DFS with 3-color marking O(V+E)
- **Deadlock Rules:** Single-instance cycle=definite, multi-instance cycle=potential

---

## ✅ Verification Checklist

```bash
# 1. Build
make clean && make
# Expected: "Build successful" - NO warnings

# 2. Tests
make test
# Expected: "All tests passed"

# 3. Memory
valgrind --leak-check=full ./bin/deadlock_detector -v
# Expected: "no leaks are possible"

# 4. Execution
./bin/deadlock_detector -v
# Expected: Program runs without errors

# 5. Output Formats
./bin/deadlock_detector -f json
./bin/deadlock_detector -f verbose
# Expected: Valid output in each format
```

---

## 🎯 Success Criteria

✅ Compiles without warnings  
✅ All tests pass  
✅ No memory leaks  
✅ Detects deadlocks correctly  
✅ Handles edge cases  
✅ Well-documented code  
✅ Modular architecture  
✅ ~3500 lines of production code  

---

## 📚 Key Files & Their Purpose

| File | Purpose | When to Use |
|------|---------|------------|
| instruction.md | **Main instruction** | Import to Cursor at start |
| cursor-rules.md | **Rules + 10 prompts** | Copy prompts one by one |
| QUICKSTART.md | **Quick reference** | For fast lookup |
| README-guide.md | **Architecture docs** | Learn before coding |
| This file | **Overview** | Get oriented |

---

## 🔧 Typical Workflow with Cursor

```
1. Open Cursor IDE
2. Open project folder: deadlock_detector/
3. Create .cursor/rules file → paste rules
4. For each of 10 prompts:
   a. Copy prompt from cursor-rules.md or QUICKSTART.md
   b. Paste into Cursor chat
   c. Cursor generates code
   d. Review and save to appropriate file
5. After all 10 prompts:
   a. Run: make clean && make
   b. Run: make test
   c. Fix any issues Cursor may have
   d. Run valgrind for memory check
6. Done! ✨
```

---

## 🐛 Common Issues & Fixes

### Compilation Warnings
- Cursor might leave unused variables
- Fix: Remove or initialize them
- Re-run make to verify

### Test Failures
- Might be graph representation issue
- Check: Are edges added correctly?
- Debug: Use gdb or add print statements

### Memory Leaks
- Check cleanup functions are called
- Verify all malloc() have free()
- Use valgrind to pinpoint

---

## 💡 Implementation Tips for Cursor

### Process Monitor
- Read `/proc/[PID]/status` for process name
- Parse format: `Name:\tprocess_name`
- Handle multi-digit PIDs correctly

### Resource Graph
- Use linked lists for adjacency list
- Each edge: source → destination
- Request edge: P → R
- Allocation edge: R → P

### Cycle Detection
- Mark vertex GRAY when visiting
- If neighbor is GRAY → back edge → cycle!
- Extract full cycle path for reporting

### Output Formatting
- TEXT: Plain summary, readable
- JSON: Valid JSON, machine parseable
- VERBOSE: Full details with explanations

---

## 📈 Performance Targets

| Scenario | Target | Notes |
|----------|--------|-------|
| 100 processes | <100ms | Should be instant |
| 1000 processes | <500ms | Acceptable |
| Memory for 1000 | <10MB | Efficient |
| Zero warnings | 100% | Must be clean build |

---

## 🎓 What You'll Learn

By completing this project:

✅ Operating Systems: Deadlock, synchronization, resources  
✅ C Programming: Pointers, memory management, system calls  
✅ Algorithms: Graph representation, DFS, cycle detection  
✅ Linux/Unix: /proc filesystem, system programming  
✅ Software Design: Modular architecture, interfaces  
✅ Debugging: GDB, Valgrind, profiling  

---

## 📞 Getting Help

**If Cursor code doesn't compile:**
- Check the generated code
- Look for obvious syntax errors
- Re-generate if needed

**If tests fail:**
- Debug with gdb
- Add debug_log() statements
- Check test data

**If memory leaks:**
- Use valgrind output to find where
- Check all malloc/free pairs
- Verify cleanup functions

---

## 🚀 Ready to Start?

1. **Read**: README-guide.md (understand architecture)
2. **Setup**: Create project directories
3. **Import**: Copy instruction.md to Cursor
4. **Generate**: Use 10 prompts in order
5. **Verify**: Run make test and valgrind
6. **Done!**: Working deadlock detector ✨

---

## 📝 Summary

You have:
✅ 5 complete documentation files  
✅ 10 ready-to-use prompts for Cursor  
✅ Complete architecture specification  
✅ Error handling patterns  
✅ Quality assurance checklist  
✅ Testing framework  
✅ Quick reference guides  

**Total code to generate: ~3500 lines**  
**Estimated time: 2.5-3.5 hours**  
**Result: Professional deadlock detector** ✨

---

**Bạn sẵn sàng để bắt đầu rồi!**

Hãy bắt đầu từ QUICKSTART.md để generate code với Cursor IDE.

Good luck! 🎉
