# Hướng Dẫn Bắt Đầu - Hệ Thống Phát Hiện Deadlock

Hướng dẫn nhanh để bắt đầu với dự án phát hiện Deadlock.

---

## 🚀 Thiết Lập Trong 30 Giây

```bash
# 1. Tạo cấu trúc dự án
mkdir -p deadlock_detector/{src,test,obj,bin}
cd deadlock_detector

# 2. Tạo thư mục .cursor cho rules
mkdir -p .cursor

# 3. Sao chép các file hướng dẫn
cp /đường/dẫn/DEVELOPMENT_GUIDE.md .
cp /đường/dẫn/CURSOR_SETUP.md .cursor/rules
cp /đường/dẫn/SYSTEM_ARCHITECTURE.md README.md
```

---

## 📝 Sử Dụng Với Cursor IDE

### Bước 1: Mở Dự Án
- Mở thư mục `deadlock_detector/` trong Cursor IDE

### Bước 2: Tạo File .cursor/rules
- Tạo file mới: `.cursor/rules`
- Sao chép nội dung từ `CURSOR_SETUP.md` PHẦN 1 (Quy tắc cho Cursor)

### Bước 3: Sử Dụng Các Prompt Theo Thứ Tự

**Dãy Prompt** (sao chép từng cái vào Cursor chat):

#### 1️⃣ Tạo Các File Header
```
Dựa trên kiến trúc hệ thống phát hiện deadlock được mô tả trong 
DEVELOPMENT_GUIDE.md, hãy tạo các file header sau:

1. src/config.h - các macro và hằng số (MAX_PROCESSES, ERROR_* codes)
2. src/utility.h - các hàm tiện ích (malloc, string, file ops)
3. src/process_monitor.h - thu thập thông tin tiến trình từ /proc
4. src/resource_graph.h - xây dựng Resource Allocation Graph
5. src/cycle_detection.h - phát hiện chu trình dựa trên DFS
6. src/deadlock_detection.h - logic phát hiện deadlock chính
7. src/output_handler.h - tạo và hiển thị báo cáo

Yêu cầu:
- Có #ifndef guards đúng cách trên tất cả headers
- Định nghĩa đầy đủ tất cả structs và hằng số
- Có đầy đủ function prototypes
- Có comments chi tiết
- Không có circular dependencies
- Tuân thủ naming: functions=snake_case, constants=UPPER_CASE, structs=PascalCase
```

#### 2️⃣ Tạo Module Utility
```
Tạo implementation utility.c để khớp với utility.h:

Implement các hàm sau:
- safe_malloc(), safe_realloc(), safe_free() với error checking
- str_trim(), str_starts_with(), str_split()
- free_str_array(), file_exists(), read_entire_file()
- error_exit() function
- error_log(), debug_log(), info_log() macros

Yêu cầu chất lượng:
- Phải compile không có warnings với -Wall -Wextra
- Không có memory leaks
- Xử lý lỗi đúng cách
- Tuân theo các pattern code hiện có
```

#### 3️⃣ Tạo Process Monitor
```
Tạo process_monitor.c để đọc thông tin tiến trình từ /proc:

Implement:
- get_all_processes() → liệt kê tất cả PIDs từ /proc
- get_process_info() → parse /proc/[PID]/status
- get_process_resources() → xác định resources đang giữ/đang chờ
- read_proc_file() → đọc an toàn bất kỳ file /proc nào
- free_process_info() → hàm cleanup

Phải xử lý:
- ENOENT khi tiến trình kết thúc
- EACCES permission denied
- Parse đúng format /proc/[PID]/status
- Cache các lần đọc để tránh I/O lặp lại
```

#### 4️⃣ Tạo Resource Graph
```
Tạo resource_graph.c để xây dựng Resource Allocation Graph:

Implement:
- create_graph() → khởi tạo cấu trúc graph
- add_process_vertex() → thêm nút tiến trình
- add_resource_vertex() → thêm nút tài nguyên
- add_request_edge() → cạnh P→R (tiến trình chờ)
- add_allocation_edge() → cạnh R→P (tài nguyên được gán)
- print_graph() → visualization để debug
- free_graph() → cleanup

Sử dụng adjacency list (không phải matrix) để hiệu quả.
Hỗ trợ single và multiple instance resources.
```

#### 5️⃣ Tạo Cycle Detection
```
Tạo cycle_detection.c - phát hiện chu trình dựa trên DFS:

Implement DFS với 3-color marking:
- WHITE (0) = chưa thăm
- GRAY (1) = đang trong recursion stack
- BLACK (2) = đã xong

Các hàm:
- has_cycle() → điểm vào chính
- dfs_visit() → duyệt DFS đệ quy
- find_all_cycles() → tìm TẤT CẢ các chu trình
- extract_cycle_path() → tái tạo các đỉnh trong chu trình
- print_cycle() → output để debug

PHẢI có độ phức tạp thời gian O(V+E).
Tìm tất cả chu trình, không chỉ cái đầu tiên.
Trả về đường đi chu trình hoàn chỉnh.
```

#### 6️⃣ Tạo Deadlock Detection
```
Tạo deadlock_detection.c - logic phát hiện chính:

Tích hợp tất cả các thành phần:
1. Thu thập dữ liệu tiến trình
2. Xây dựng RAG
3. Chạy cycle detection
4. Phân tích chu trình để tìm deadlock
5. Tạo báo cáo

Implement:
- detect_deadlock_in_system()
- build_rag_from_processes()
- analyze_cycles_for_deadlock()
- identify_deadlocked_processes()
- create_deadlock_report()
- free_deadlock_report()

Xử lý single-instance (deadlock chắc chắn) vs 
multi-instance (deadlock tiềm năng).
```

#### 7️⃣ Tạo Output Handler
```
Tạo output_handler.c - định dạng và hiển thị kết quả:

Hỗ trợ 3 định dạng output:

TEXT (mặc định):
- Tóm tắt deadlock rõ ràng
- Danh sách PIDs
- Chuỗi chu trình
- Khuyến nghị

JSON:
- Cấu trúc JSON hợp lệ
- Thông tin deadlock đầy đủ
- Timestamps
- Có thể parse bằng máy

VERBOSE:
- Thông tin tiến trình chi tiết
- Thông tin tài nguyên
- Chuỗi chờ đầy đủ
- Giải thích

Các hàm:
- display_deadlock_report()
- format_as_text/json/verbose()
- export_to_file()
```

#### 8️⃣ Tạo main.c
```
Tạo main.c - CLI và điều phối:

Tùy chọn dòng lệnh:
-h, --help
-v, --verbose
-c, --continuous (tiếp tục chạy)
-i, --interval SEC (mặc định 5)
-f, --format FORMAT (text, json, verbose)
-o, --output FILE
--version

Luồng chính:
1. Parse arguments
2. Khởi tạo hệ thống
3. Chạy detection (một lần hoặc liên tục)
4. Hiển thị kết quả
5. Cleanup và thoát

Phải xử lý:
- Arguments không hợp lệ
- Signal handling (SIGINT)
- Cleanup tài nguyên
```

#### 9️⃣ Tạo Tests
```
Tạo unit tests trong thư mục test/:

test_graph.c:
- Test tạo/xóa graph
- Test thêm edges
- Test graph lớn
- Kiểm tra memory

test_cycle.c:
- Không có chu trình
- Chu trình đơn giản
- Chu trình phức tạp
- Nhiều chu trình

test_system.c:
- Tích hợp end-to-end
- Dữ liệu tiến trình mock
- Định dạng output

Tất cả tests nên:
- In PASS/FAIL
- Kiểm tra memory (valgrind-safe)
- Trả về 0 (thành công) hoặc 1 (thất bại)
```

#### 🔟 Tạo Makefile
```
Tạo Makefile:

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

## ✅ Danh Sách Kiểm Tra Xác Minh

Sau khi Cursor tạo xong tất cả code:

```bash
# 1. Build dự án
cd deadlock_detector
make clean && make

# Kỳ vọng: Compilation thành công KHÔNG có warnings

# 2. Chạy tests
make test

# Kỳ vọng: Tất cả tests PASSED

# 3. Kiểm tra memory leaks
valgrind --leak-check=full ./bin/deadlock_detector -v

# Kỳ vọng: "no leaks are possible"

# 4. Chạy chương trình
./bin/deadlock_detector -v

# Kỳ vọng: Chương trình chạy, hiển thị thông tin tiến trình hoặc "No deadlock"

# 5. Test JSON output
./bin/deadlock_detector -f json

# Kỳ vọng: Output JSON hợp lệ

# 6. Test chế độ continuous (Ctrl+C để dừng)
./bin/deadlock_detector -c -i 2

# Kỳ vọng: Chạy trong 2 giây, hiển thị output, có thể bị gián đoạn
```

---

## 🐛 Khắc Phục Sự Cố

### Vấn đề: Cảnh báo khi biên dịch
**Giải pháp**: Kiểm tra code được tạo bởi Cursor:
- Biến không sử dụng → khởi tạo hoặc xóa
- Khai báo ngầm → include đúng headers
- Lỗi khớp kiểu → ép kiểu rõ ràng
- Lỗi format string → kiểm tra printf args

### Vấn đề: Memory leaks trong valgrind
**Giải pháp**: Kiểm tra rằng:
- Tất cả malloc() calls có free() trong cleanup functions
- Không có dangling pointers
- Cleanup đúng cách trong main() trước khi thoát

### Vấn đề: File /proc không đọc được
**Giải pháp**: 
- Một số file /proc cần quyền root
- Code từ Cursor nên xử lý lỗi EACCES một cách graceful
- Bỏ qua các tiến trình không thể đọc

### Vấn đề: Chu trình không được phát hiện
**Giải pháp**:
- Xác minh graph được xây dựng đúng (debug output)
- Kiểm tra implementation DFS (3-color marking)
- Xác minh adjacency list có tất cả edges
- Kiểm tra xem cycle detection có tìm thấy back edges không

### Vấn đề: Test failures
**Giải pháp**:
- Chạy từng test riêng với verbose output
- Sử dụng GDB để debug: `gdb ./bin/test_graph`
- Kiểm tra khởi tạo dữ liệu test
- Xác minh assertions đúng

---

## 📊 Cấu Trúc Mong Đợi Sau Khi Tạo

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

Tổng code: ~3500 dòng
Executables: 
  - bin/deadlock_detector (main)
  - bin/test_graph, bin/test_cycle, bin/test_system
```

---

## 📈 Chỉ Số Chất Lượng Code (Mong Đợi)

| Chỉ Số | Mong Đợi | Kiểm Tra |
|--------|----------|----------|
| Cảnh Báo Compiler | 0 | `make 2>&1 \| grep warning` |
| Memory Leaks | 0 | `valgrind --leak-check=full` |
| Code Coverage | >80% | Chạy tất cả tests |
| Thời Gian Thực Thi (1000 procs) | <500ms | `time ./bin/deadlock_detector` |
| Sử Dụng Memory | <10MB | `ps aux \| grep deadlock` |
| Chu Trình Được Phát Hiện | 100% | So sánh với xác minh thủ công |

---

## 🎯 Điểm Triển Khai Chính (cho Cursor)

### 1. Process Monitor
- Đọc từ `/proc/[PID]/status` để lấy tên tiến trình
- Đọc `/proc/[PID]/fd` để lấy các file đang mở
- Xử lý ENOENT một cách graceful khi tiến trình kết thúc

### 2. Resource Graph
- Sử dụng adjacency list (linked lists) KHÔNG phải matrix
- Hỗ trợ thêm edges: P→R (request), R→P (allocation)
- Hiệu quả cho sparse graphs (hầu hết graphs là sparse)

### 3. Cycle Detection
- Implement DFS với 3 màu: WHITE, GRAY, BLACK
- Back edge (tới GRAY vertex) = tìm thấy chu trình
- Trích xuất và lưu trữ đường đi chu trình hoàn chỉnh

### 4. Deadlock Detection
- Single-instance resource + cycle = DEADLOCK (chắc chắn 100%)
- Multi-instance resource + cycle = deadlock tiềm năng (có thể giải quyết)
- Báo cáo nên bao gồm: PIDs, chuỗi chu trình, chuỗi chờ

### 5. Output Handler
- Định dạng TEXT: tóm tắt dễ đọc cho người
- Định dạng JSON: có thể parse bằng máy, có thể tích hợp với các công cụ khác
- VERBOSE: thông tin debug chi tiết

---

## 🚀 Mẹo Hiệu Suất (cho Cursor)

1. **Tránh đọc /proc lặp lại**: Cache kết quả
2. **Biểu diễn graph**: Adjacency list cho sparse graphs
3. **Tối ưu DFS**: Một lần duyệt, không revisiting
4. **Cấp phát memory**: Pre-allocate kích thước hợp lý
5. **Tối ưu I/O**: Đọc /proc theo batch khi có thể

---

## 📚 Các Thuật Toán Chính (Tóm Tắt Cho Cursor)

### Phát Hiện Chu Trình DFS
```
Color[v] = WHITE (chưa thăm)
Color[v] = GRAY (đang xử lý)
Color[v] = BLACK (đã xong)

Với mỗi đỉnh chưa thăm v:
  DFS(v):
    Color[v] = GRAY
    Với mỗi neighbor u:
      Nếu Color[u] == WHITE: DFS(u)
      Nếu Color[u] == GRAY: TÌM THẤY CHU TRÌNH!
    Color[v] = BLACK

Thời gian: O(V+E), Không gian: O(V)
```

### Phát Hiện Deadlock
```
1. Với mỗi tiến trình P:
   - Đọc các tài nguyên nó đang giữ
   - Đọc các tài nguyên nó đang chờ

2. Xây dựng graph:
   - Các đỉnh: tiến trình + tài nguyên
   - Các cạnh: P→R (đang chờ), R→P (đã được gán)

3. Tìm chu trình sử dụng DFS

4. Với mỗi chu trình:
   - Trích xuất các tiến trình liên quan
   - Kiểm tra single-instance (chắc chắn) hoặc multi-instance (tiềm năng)
   - Báo cáo là deadlock

5. Xuất kết quả
```

---

## 💾 Các File Sử Dụng Với Cursor

1. **DEVELOPMENT_GUIDE.md** - Hướng dẫn chi tiết đầy đủ
2. **CURSOR_SETUP.md** - Quy tắc và prompts
3. **SYSTEM_ARCHITECTURE.md** - Tài liệu kiến trúc
4. **File này** - Tham khảo nhanh

---

## ⏱️ Dòng Thời Gian Ước Tính

| Nhiệm Vụ | Thời Gian | Trạng Thái |
|------|------|--------|
| Tạo headers | 5 phút | ⏳ |
| Tạo utility | 10 phút | ⏳ |
| Tạo process_monitor | 15 phút | ⏳ |
| Tạo resource_graph | 15 phút | ⏳ |
| Tạo cycle_detection | 20 phút | ⏳ |
| Tạo deadlock_detection | 20 phút | ⏳ |
| Tạo output_handler | 15 phút | ⏳ |
| Tạo main.c | 15 phút | ⏳ |
| Tạo tests | 15 phút | ⏳ |
| Tạo Makefile | 5 phút | ⏳ |
| **Testing & debugging** | **30-60 phút** | ⏳ |
| **TỔNG CỘNG** | **2.5-3.5 giờ** | ⏳ |

---

## ✨ Tiêu Chí Thành Công

Implementation cuối cùng của bạn nên:

✅ Compile không có warnings  
✅ Pass tất cả unit tests  
✅ Không có memory leaks (valgrind clean)  
✅ Phát hiện deadlock đúng cách  
✅ Xử lý edge cases một cách graceful  
✅ Code được tài liệu hóa tốt  
✅ Kiến trúc modular  
✅ Hiệu suất hiệu quả  

---

## 🎓 Kết Quả Học Tập

Bằng cách hoàn thành dự án này, bạn sẽ hiểu:

- **Hệ Điều Hành**: Deadlock, đồng bộ hóa, quản lý tài nguyên
- **Lập Trình C**: Con trỏ, cấu trúc, quản lý memory, file I/O
- **Thuật Toán**: Biểu diễn graph, phát hiện chu trình DFS, phân tích thuật toán
- **Linux/Unix**: Filesystem `/proc`, system programming, debugging tools
- **Thiết Kế Phần Mềm**: Kiến trúc modular, interfaces sạch, xử lý lỗi

---

**Bắt đầu với thiết lập 30 giây ở trên, sau đó làm theo các prompts theo thứ tự!**

Chúc may mắn! 🚀
