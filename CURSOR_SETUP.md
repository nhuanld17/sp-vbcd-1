# Quy Tắc Tích Hợp Cursor IDE & Prompts

Tài liệu này cung cấp các rules và prompts tối ưu để sử dụng với Cursor IDE.

---

## PHẦN 1: QUY TẮC CHO CURSOR IDE

### Tạo File `.cursor/rules` trong Dự Án

```
# =============================================================================
# HỆ THỐNG PHÁT HIỆN DEADLOCK - QUY TẮC CURSOR IDE
# =============================================================================
# Các quy tắc này hướng dẫn Cursor tạo code hiệu quả cho dự án phát hiện 
# Deadlock trên hệ thống Unix/Linux

# =============================================================================
# QUY TẮC 1: NGUYÊN TẮC TẠO CODE
# =============================================================================

Rule: CodeGenerationStyle
- Luôn tạo **code modular, có tổ chức tốt**
- Mỗi module .c phải có file .h tương ứng
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
TRƯỚC mỗi function, viết:
/*
 * function_name - mô tả ngắn gọn
 * @param1: mô tả
 * @param2: mô tả
 * @return: giá trị trả về
 * Mô tả: giải thích chi tiết, thuật toán, độ phức tạp
 * Xử lý lỗi: cách báo lỗi
 */

Rule: ErrorHandling
- LUÔN định nghĩa error codes chung (SUCCESS=0, ERROR_*=-1,-2,...)
- Mỗi function return >= 0 cho success, < 0 cho errors
- Kiểm tra tất cả pointer returns từ malloc
- Sử dụng safe_malloc() wrapper từ utility.c
- Luôn log errors với error_log() macro

Rule: MemoryManagement
- Mỗi malloc() phải có free() tương ứng hoặc cleanup_*() function
- Không để dangling pointers
- Sử dụng valgrind-safe coding patterns
- Tránh buffer overflows (sử dụng snprintf thay vì sprintf)
- Khởi tạo tất cả variables trước khi dùng

# =============================================================================
# QUY TẮC 2: QUY TẮC MODULE CỤ THỂ
# =============================================================================

Rule: ProcessMonitor
- PHẢI đọc từ /proc filesystem (không sử dụng system calls khác)
- Implement: get_all_processes(), get_process_info(), read_proc_file()
- Parse /proc/[PID]/status, /proc/[PID]/fd, /proc/[PID]/locks
- Xử lý process termination một cách graceful (ENOENT)
- Cache các lần đọc để tránh I/O lặp lại
- Độ phức tạp: O(n) với n = số lượng tiến trình

Rule: ResourceGraph
- Sử dụng adjacency list, không phải adjacency matrix (vì sparse)
- Implement: add_request_edge(), add_allocation_edge()
- Hỗ trợ cả single-instance và multiple-instance resources
- Chuyển đổi RAG sang WFG nếu cần
- Memory: O(V+E) với V=vertices, E=edges

Rule: CycleDetection
- PHẢI implement DFS-based cycle detection
- Sử dụng 3-color marking: WHITE(0), GRAY(1), BLACK(2)
- Phát hiện back edges khi gặp GRAY vertex
- Trích xuất đường đi chu trình hoàn chỉnh, không chỉ node
- Trả về TẤT CẢ cycles tìm được, không dừng ở cái đầu tiên
- Độ phức tạp thời gian: PHẢI là O(V+E)
- Độ phức tạp không gian: O(V)

Rule: DeadlockDetection
- Gọi cycle detection
- Lọc cycles để xác định processes bị deadlock
- Xử lý cả single-instance (deadlock chắc chắn) và 
  multiple-instance (deadlock tiềm năng)
- Khuyến nghị kiến thức về Coffman conditions

Rule: OutputHandler
- Hỗ trợ 3 định dạng: TEXT (mặc định), JSON, VERBOSE
- Bao gồm process names, PIDs, wait chain
- Hiển thị thông tin tài nguyên (ID, type, holders)
- Cung cấp các khuyến nghị có thể thực hiện
- Làm cho output có thể parse (cho integration với các công cụ khác)

Rule: UtilityModule
- safe_malloc(), safe_realloc(), safe_free()
- String utilities: trim, split, starts_with
- File utilities: file_exists, read_file
- Error logging: error_log, debug_log, info_log
- Các hàm này PHẢI được sử dụng khắp codebase

# =============================================================================
# QUY TẮC 3: ĐẢM BẢO CHẤT LƯỢNG
# =============================================================================

Rule: CompilerWarnings
- PHẢI compile với: gcc -Wall -Wextra -std=c99 -pedantic
- KHÔNG được có compiler warnings
- Sử dụng -Werror nếu cần strict mode

Rule: MemorySafety
- PHẢI pass: valgrind --leak-check=full
- KHÔNG được memory leaks
- KHÔNG được out-of-bounds access
- KHÔNG được use-after-free

Rule: Robustness
- Xử lý: process không tồn tại (ENOENT)
- Xử lý: permission denied (EACCES)
- Xử lý: out of memory
- Xử lý: graph với >10000 vertices
- Xử lý: circular references trong resource hierarchy

Rule: Testing
- Tạo unit tests cho mỗi module
- Test cases: no deadlock, simple deadlock, complex deadlock
- Test edge cases: empty graph, single process, disconnected graph

Rule: Documentation
- Header files: chứa đầy đủ function descriptions
- Source files: giải thích các thuật toán phức tạp
- Main.c: có usage examples và CLI help
- README: hướng dẫn build, run, debug

# =============================================================================
# QUY TẮC 4: QUY TẮC HIỆU SUẤT
# =============================================================================

Rule: GraphRepresentation
- Sử dụng adjacency list (không phải matrix) cho sparse graphs
- Cấp phát dần dần, không pre-allocate quá lớn
- Tái sử dụng graph structures nếu continuous monitoring

Rule: AlgorithmOptimization
- DFS: tối ưu để tránh revisiting nodes
- Process discovery: cache kết quả
- File reading: minimize /proc accesses
- Chỉ recompute khi có changes

Rule: ResourceUsage
- Max memory: < 10MB cho 1000 processes
- Thời gian chấp nhận được: < 500ms cho 1000 processes
- Không có infinite loops hoặc busy waiting

# =============================================================================
# QUY TẮC 5: WORKFLOW TẠO CODE
# =============================================================================

Rule: GenerationOrder
1. ĐẦU TIÊN: Header files (.h) cho tất cả modules
2. SAU ĐÓ: Utility module (utility.c/.h)
3. SAU ĐÓ: Process monitor module
4. SAU ĐÓ: Resource graph module
5. SAU ĐÓ: Cycle detection module
6. SAU ĐÓ: Deadlock detection module
7. SAU ĐÓ: Output handler module
8. SAU ĐÓ: main.c integration
9. CUỐI CÙNG: Unit tests

Rule: GenerationSize
- Mỗi file: 100-300 dòng (kích thước quản lý được)
- Không viết files quá dài (>500 dòng)
- Chia logic thành multiple functions

Rule: IncompleteCodePattern
Nếu code không hoàn chỉnh, LUÔN:
- Thêm TODO comments
- Cung cấp skeleton functions
- Giải thích cần implement gì

# =============================================================================
# QUY TẮC 6: XỬ LÝ ĐẶC BIỆT
# =============================================================================

Rule: /procFilesystemHandling
- LUÔN kiểm tra file tồn tại trước khi đọc
- LUÔN xử lý ENOENT (process terminated)
- LUÔN xử lý EACCES (permission)
- Sử dụng open()/read() hoặc fopen()/fgets()
- Parse từng dòng, không assume format

Rule: GraphCycleHandling
- Một cycle có thể liên quan đến 2-N processes
- Một process có thể trong multiple cycles
- Trích xuất cycle dưới dạng ordered list: P1→R1→P2→R2→P1
- Lưu trữ cả processes và resources trong cycle

Rule: EdgeCaseHandling
- Xử lý process list rỗng
- Xử lý process với 0 resources
- Xử lý graph với 1 vertex (không cycle)
- Xử lý PIDs nhiều chữ số (> 9999)

# =============================================================================
# QUY TẮC 7: CÔNG CỤ DEBUG
# =============================================================================

Rule: DebugOutput
- LUÔN cung cấp debug_log() macro implementation
- Tạo với ifdef DEBUG blocks
- Bao gồm timing info: printf timing khi -v flag
- Bao gồm graph visualization output

Rule: ErrorMessages
- Mô tả: "Failed to open /proc/1234/status: Permission denied"
- Bao gồm errno details
- Đề xuất giải pháp khi có thể
- Log to stderr, info_log to stdout

# =============================================================================
# QUY TẮC 8: HỆ THỐNG BUILD
# =============================================================================

Rule: MakefileGeneration
- CC := gcc
- CFLAGS := -Wall -Wextra -O2 -std=c99 -D_POSIX_C_SOURCE=200809L
- LDFLAGS := -lpthread
- Targets: all, clean, test
- Output: bin/deadlock_detector
- Objects: obj/*.o

# =============================================================================
# QUY TẮC 9: CẤU TRÚC DỰ ÁN
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
├── DEVELOPMENT_GUIDE.md
└── .gitignore

# =============================================================================
# KẾT THÚC QUY TẮC
# =============================================================================
```

---

## PHẦN 2: PROMPTS CHO CURSOR

### Prompt 1: Tạo Các File Header (Tất cả cùng lúc)

```
Dựa trên kiến trúc hệ thống phát hiện deadlock, hãy tạo các file header sau 
cho dự án:

1. src/config.h - các macro và hằng số
2. src/utility.h - các hàm tiện ích
3. src/process_monitor.h - thu thập thông tin tiến trình
4. src/resource_graph.h - resource allocation graph
5. src/cycle_detection.h - thuật toán phát hiện chu trình
6. src/deadlock_detection.h - engine phát hiện deadlock
7. src/output_handler.h - tạo báo cáo

Yêu cầu:
- Bao gồm #ifndef guards đúng cách
- Thêm mô tả function toàn diện
- Định nghĩa tất cả data structures cần thiết
- Bao gồm error codes
- Thêm function prototypes với đầy đủ parameters
- Không có implementation, chỉ declarations
- Đảm bảo headers tự chứa (minimal dependencies)

Tuân theo naming conventions:
- Functions: snake_case
- Constants: UPPER_CASE
- Structs: PascalCase
```

### Prompt 2: Tạo Module Utility

```
Tạo module utility (utility.c và xác minh tính nhất quán với utility.h):

Các hàm cần implement:
1. safe_malloc() - wrapper malloc với error checking
2. safe_realloc() - wrapper realloc
3. safe_free() - wrapper free an toàn
4. str_trim() - loại bỏ whitespace từ string
5. str_starts_with() - kiểm tra string có bắt đầu bằng prefix
6. str_split() - chia string theo delimiter
7. free_str_array() - giải phóng mảng strings
8. file_exists() - kiểm tra file có tồn tại
9. read_entire_file() - đọc file vào string
10. error_exit() - in lỗi và thoát
11. Implement debug_log, error_log, info_log macros

Yêu cầu:
- Xử lý lỗi đúng cách
- An toàn memory (không có leaks)
- Tuân thủ POSIX
- Bao gồm comments inline toàn diện
- Xử lý NULL pointers một cách graceful
- Độ phức tạp thời gian: O(n) cho string operations
```

### Prompt 3: Tạo Module Process Monitor

```
Tạo implementation process_monitor.c dựa trên process_monitor.h:

Các hàm chính:
1. get_all_processes() - liệt kê tất cả tiến trình đang chạy từ /proc
2. get_process_info() - đọc /proc/[PID]/status, parse Name/State/PPID
3. get_process_resources() - xác định tài nguyên đang giữ và đang chờ
4. read_proc_file() - đọc file /proc tùy ý với error handling
5. get_open_files() - đọc /proc/[PID]/fd
6. get_file_locks() - đọc /proc/[PID]/locks
7. free_process_info() - hàm cleanup

Yêu cầu:
- Đọc trực tiếp từ /proc filesystem
- Xử lý ENOENT (process terminated) một cách graceful
- Xử lý EACCES (permission denied) một cách graceful
- Parse đúng format (/proc/[PID]/status format)
- Implement caching để tránh đọc lặp lại
- Độ phức tạp thời gian: O(n) với n = số lượng tiến trình
- Memory: được cấp phát với safe_malloc()
- Bao gồm error handling và validation

Tập trung:
- Độ chính xác khi parse các file /proc
- Tính robust khi tiến trình kết thúc
- I/O file hiệu quả
```

### Prompt 4: Tạo Module Resource Graph

```
Tạo implementation resource_graph.c:

Các hàm chính:
1. create_graph() - cấp phát và khởi tạo resource graph
2. add_request_edge() - thêm cạnh P→R (process chờ tài nguyên)
3. add_allocation_edge() - thêm cạnh R→P (tài nguyên được gán cho process)
4. add_process_vertex() - thêm nút tiến trình vào graph
5. add_resource_vertex() - thêm nút tài nguyên vào graph
6. get_vertex_id() - chuyển đổi PID/RID sang vertex ID
7. find_vertex_by_id() - tra cứu vertex theo PID/RID
8. print_graph() - output debug của cấu trúc graph
9. free_graph() - cleanup memory graph

Yêu cầu:
- Sử dụng adjacency list representation (không phải matrix)
- Hỗ trợ cả single và multiple instance resources
- Tra cứu edge hiệu quả: O(1) hoặc O(log E)
- Hiệu quả memory cho sparse graphs
- Có thể chuyển đổi sang Wait-For Graph (WFG) nếu cần
- Vertex capacity nên được pre-allocate
- Edges được cấp phát động

Cấu trúc dữ liệu:
- Adjacency list sử dụng linked lists hoặc arrays
- Mỗi vertex lưu: id, type (process/resource), instance count
- Theo dõi cả outgoing và incoming edges cho mỗi vertex

Tập trung:
- Xây dựng graph hiệu quả
- Quản lý memory đúng cách
- Rõ ràng về vertex/edge semantics
```

### Prompt 5: Tạo Module Cycle Detection

```
Tạo implementation cycle_detection.c cho DFS-based cycle detection:

Thuật toán: Depth-First Search với color marking
- WHITE (0): chưa thăm
- GRAY (1): đang được xử lý (trong recursion stack)
- BLACK (2): đã xử lý xong

Các hàm chính:
1. has_cycle() - điểm vào chính, trả về 1 nếu có cycle
2. dfs_visit() - duyệt DFS đệ quy
3. find_all_cycles() - tìm tất cả cycles trong graph
4. extract_cycle_path() - tái tạo các vertices chu trình từ parent array
5. detect_back_edge() - xác định back edge (đánh dấu deadlock cycle)
6. print_cycle() - output debug cho một cycle
7. free_cycle_info() - cleanup cấu trúc cycle

Yêu cầu:
- Độ phức tạp thời gian: PHẢI là O(V+E)
- Độ phức tạp không gian: O(V)
- Tìm TẤT CẢ cycles, không chỉ cái đầu tiên
- Trích xuất đường đi chu trình hoàn chỉnh dưới dạng ordered vertex lists
- Xử lý multiple connected components
- Trả về cycles dưới dạng CycleInfo[] array

Chi tiết thuật toán:
1. Khởi tạo color[v] = WHITE cho tất cả vertices
2. Với mỗi vertex chưa thăm, gọi DFS_VISIT
3. Trong DFS_VISIT(v):
   - Đặt color[v] = GRAY
   - Với mỗi neighbor u:
     * Nếu color[u] == WHITE: đệ quy DFS_VISIT(u)
     * Nếu color[u] == GRAY: tìm thấy back edge! Trích xuất cycle
   - Đặt color[v] = BLACK
4. Lưu trữ đường đi chu trình để phân tích sau

Tập trung:
- Tính đúng đắn của việc phát hiện back edge
- Trích xuất cycle hiệu quả
- Xử lý graphs lớn (lên đến 10000 vertices)
```

### Prompt 6: Tạo Module Deadlock Detection

```
Tạo deadlock_detection.c - tích hợp tất cả các thành phần:

Các hàm chính:
1. detect_deadlock_in_system() - điểm vào phát hiện chính
2. build_rag_from_processes() - tạo RAG từ ProcessResourceInfo
3. analyze_cycles_for_deadlock() - xác định tiến trình nào bị deadlock
4. filter_actual_deadlocks() - phân biệt deadlock chắc chắn vs tiềm năng
5. create_deadlock_report() - tạo cấu trúc DeadlockReport
6. identify_deadlocked_processes() - trích xuất PIDs từ cycles
7. generate_recommendations() - đề xuất giải pháp
8. free_deadlock_report() - cleanup

Workflow:
1. Gọi get_all_processes() qua process_monitor
2. Với mỗi process, lấy ProcessResourceInfo
3. Xây dựng RAG sử dụng resource_graph module
4. Chạy cycle detection qua cycle_detection module
5. Phân tích mỗi cycle để xác định các tiến trình bị deadlock
6. Tạo báo cáo có cấu trúc

Yêu cầu:
- Tích hợp process_monitor, resource_graph, cycle_detection
- Xử lý single-instance: cycle = deadlock chắc chắn
- Xử lý multi-instance: cycle = deadlock tiềm năng (cần phân tích sâu hơn)
- Trích xuất PIDs bị deadlock từ cycle paths
- Bao gồm thông tin chuỗi/chu trình deadlock trong báo cáo
- Cung cấp các khuyến nghị có ý nghĩa

Quy tắc:
- Single-instance resource: BẤT KỲ cycle → DEADLOCK
- Multi-instance resource: cycle → deadlock có thể (có thể giải quyết)
- Báo cáo nên bao gồm: danh sách PID, chuỗi chu trình, chuỗi chờ

Tập trung:
- Xác định deadlock đúng
- Ánh xạ cycle-to-process có ý nghĩa
- Chất lượng và độ rõ ràng của báo cáo
```

### Prompt 7: Tạo Module Output Handler

```
Tạo output_handler.c - định dạng và hiển thị kết quả:

Các hàm chính:
1. display_deadlock_report() - hàm output chính
2. print_summary() - in tóm tắt deadlock
3. print_detailed_wait_chain() - in chu trình dưới dạng chuỗi
4. print_process_info() - in chi tiết từng tiến trình
5. print_resource_info() - in thông tin tài nguyên
6. print_recommendations() - in các đề xuất giải quyết
7. export_to_file() - ghi báo cáo vào file
8. format_as_text() - định dạng báo cáo dưới dạng plain text
9. format_as_json() - định dạng báo cáo dưới dạng JSON
10. format_as_verbose() - định dạng output verbose chi tiết

Định dạng Output:

TEXT (mặc định):
========================================
DEADLOCK DETECTED!
========================================
Deadlocked Processes: PID1, PID2, ...
Cycle Chain: P1 → R1 → P2 → R2 → P1
Recommendations: [danh sách hành động]

JSON:
{
  "deadlock_detected": true,
  "timestamp": "...",
  "deadlocked_processes": [...],
  "cycle": [...],
  "recommendations": [...]
}

VERBOSE:
[Dạng dài với đầy đủ chi tiết tiến trình, thông tin tài nguyên, chuỗi chờ, v.v.]

Yêu cầu:
- Tạo output dễ đọc, chuyên nghiệp
- Hỗ trợ 3 định dạng output
- Có thể ghi vào stdout hoặc file
- Output JSON phải hợp lệ
- Bao gồm timestamps
- Làm cho các khuyến nghị có thể thực hiện
- Pretty-print để dễ đọc cho người

Tập trung:
- Chất lượng output
- Tuân thủ định dạng
- Truyền đạt thông tin deadlock rõ ràng
```

### Prompt 8: Tạo main.c

```
Tạo main.c - giao diện dòng lệnh và điều phối:

Yêu cầu:
1. Phân tích đối số dòng lệnh:
   -h, --help: Hiển thị usage
   -v, --verbose: Verbose output
   -c, --continuous: Chế độ giám sát liên tục
   -i, --interval SEC: Khoảng thời gian giám sát (mặc định 5)
   -f, --format FORMAT: text, json, verbose
   -o, --output FILE: File output
   --version: Hiển thị phiên bản

2. Luồng chương trình:
   - Parse arguments
   - Khởi tạo systems
   - Phát hiện deadlock một lần hoặc liên tục
   - Hiển thị kết quả
   - Cleanup và thoát

3. Tính năng:
   - Phát hiện một lần (mặc định)
   - Giám sát liên tục với interval
   - Nhiều định dạng output
   - Hỗ trợ output file
   - Xử lý signal đúng cách (SIGINT cho Ctrl+C)

4. Xử lý lỗi:
   - Arguments không hợp lệ
   - Lỗi mở file
   - Lỗi hệ thống
   - Graceful degradation

Yêu cầu:
- Bao gồm thông báo usage help
- Hỗ trợ version string
- Thoát chương trình sạch sẽ
- Cleanup tài nguyên (giải phóng tất cả memory trước khi thoát)
- Làm việc với cả text và file output
- Xử lý chế độ continuous một cách graceful

Tập trung:
- Trải nghiệm người dùng
- Validation arguments
- Tích hợp sạch sẽ các modules
```

### Prompt 9: Tạo Unit Tests

```
Tạo unit tests toàn diện:

test_graph.c:
- Test tạo/khởi tạo graph
- Test thêm vertices và edges
- Test tra cứu edge
- Test cleanup graph (memory safety)
- Test graphs lớn (1000+ vertices)

test_cycle.c:
- Test graph không có cycles
- Test cycle đơn giản (2 nodes)
- Test cycles phức tạp
- Test multiple disjoint cycles
- Test hiệu suất với graphs lớn

test_system.c:
- Integration test: process monitor + graph + cycle detection
- Test với dữ liệu tiến trình mock
- Test phát hiện deadlock end-to-end
- Test tạo output

Yêu cầu:
- Mỗi test: in PASS hoặc FAIL
- Sử dụng assert() macros
- Bao gồm kiểm tra memory (không có leaks)
- Mock data cho /proc khi cần
- Đo thời gian thực thi
- Thoát với 0 nếu tất cả pass, 1 nếu fail

Tập trung:
- Xác minh tính đúng đắn
- Bao phủ edge cases
- Xác minh memory safety
```

### Prompt 10: Tạo Makefile

```
Tạo Makefile để build dự án:

Yêu cầu:
- Compiler: gcc
- Flags: -Wall -Wextra -O2 -std=c99 -D_POSIX_C_SOURCE=200809L
- Libraries: -lpthread
- Objects: obj/*.o
- Binary: bin/deadlock_detector
- Tests: bin/test_*

Targets:
- all: build main executable
- test: build and run tất cả tests
- clean: xóa build artifacts
- help: hiển thị các targets có sẵn

Tính năng:
- Hỗ trợ parallel build (-j)
- Tạo thư mục tự động
- Theo dõi dependencies
- Tùy chọn verbose mode
- Install target (tùy chọn)

Tập trung:
- Tốc độ build
- Sạch sẽ
- Dễ sử dụng
```

---

## PHẦN 3: DANH SÁCH KIỂM TRA TÍCH HỢP

Trước khi bắt đầu sử dụng Cursor:

- [ ] Tạo cấu trúc thư mục dự án
- [ ] Sao chép DEVELOPMENT_GUIDE.md vào dự án
- [ ] Setup file .cursor/rules (từ PHẦN 1)
- [ ] Đọc SYSTEM_ARCHITECTURE.md để hiểu kiến trúc
- [ ] Chuẩn bị Prompt 1-10 để sử dụng

Workflow:

1. [ ] Tạo headers (Prompt 1)
2. [ ] Tạo utility (Prompt 2)
3. [ ] Tạo process_monitor (Prompt 3)
4. [ ] Tạo resource_graph (Prompt 4)
5. [ ] Tạo cycle_detection (Prompt 5)
6. [ ] Tạo deadlock_detection (Prompt 6)
7. [ ] Tạo output_handler (Prompt 7)
8. [ ] Tạo main.c (Prompt 8)
9. [ ] Tạo tests (Prompt 9)
10. [ ] Tạo Makefile (Prompt 10)
11. [ ] Build: `make clean && make`
12. [ ] Test: `make test`
13. [ ] Debug: giải quyết các vấn đề
14. [ ] Kiểm tra cuối: `make clean && make` (không có warnings)

---

## PHẦN 4: ĐẦU RA MONG ĐỢI

Sau khi chạy tất cả prompts, bạn sẽ có:

```
deadlock_detector/
├── src/
│   ├── config.h (150 dòng)
│   ├── utility.h (60 dòng)
│   ├── utility.c (200 dòng)
│   ├── process_monitor.h (50 dòng)
│   ├── process_monitor.c (250 dòng)
│   ├── resource_graph.h (70 dòng)
│   ├── resource_graph.c (300 dòng)
│   ├── cycle_detection.h (60 dòng)
│   ├── cycle_detection.c (280 dòng)
│   ├── deadlock_detection.h (50 dòng)
│   ├── deadlock_detection.c (250 dòng)
│   ├── output_handler.h (50 dòng)
│   └── output_handler.c (300 dòng)
├── test/
│   ├── test_graph.c (150 dòng)
│   ├── test_cycle.c (200 dòng)
│   └── test_system.c (150 dòng)
├── src/main.c (300 dòng)
├── Makefile (100 dòng)
└── [compiled binaries]

Tổng: ~3500+ dòng production code
Tổng: ~500+ dòng tests
Thời gian build: < 2 giây
Kích thước binary: < 100KB
```

---

## PHẦN 5: XÁC MINH CHẤT LƯỢNG

Sau khi Cursor tạo xong:

```bash
# Kiểm tra compile
gcc -Wall -Wextra -std=c99 -pedantic src/*.c test/*.c -o test_app

# Kiểm tra memory
valgrind --leak-check=full ./bin/deadlock_detector -v

# Kiểm tra warnings
make clean && make 2>&1 | grep -i warning
# Mong đợi: (không có output = không có warnings)

# Chạy test
make test
# Mong đợi: Tất cả tests passed ✓

# Test chức năng
./bin/deadlock_detector -v
# Mong đợi: Thực thi thành công, output đúng
```

---

**Kết thúc Hướng Dẫn Tích Hợp Cursor IDE**
