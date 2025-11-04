# DEADLOCK DETECTOR - HƯỚNG DẪN SỬ DỤNG

## 📦 Giới thiệu

Chương trình phát hiện deadlock giữa các tiến trình trên hệ thống Unix/Linux (C). Hệ thống quét `/proc`, xây dựng Resource Allocation Graph (RAG), phát hiện chu trình bằng DFS (3-color), và báo cáo tiến trình bị deadlock.

---

## 🚀 Cài đặt (Ubuntu/WSL)

**Yêu cầu**
- Ubuntu hoặc WSL2 (Windows Subsystem for Linux)
- `gcc`, `make`, `valgrind`

### 1) Cài dependencies

```bash
sudo apt-get update
sudo apt-get install -y build-essential gcc make valgrind
```

### 2) Vào thư mục project

```bash
cd /path/to/deadlock_detector
```

### 3) Build project

```bash
make clean && make
```

> Nếu build thành công sẽ tạo: `bin/deadlock_detector`

### 4) Chạy tests

```bash
make test
```

---

## 🛠️ Cách sử dụng

Sau khi build, thực thi nằm ở `bin/deadlock_detector`.

### Chạy cơ bản

```bash
./bin/deadlock_detector
```

### Tuỳ chọn dòng lệnh

| Tuỳ chọn | Ý nghĩa |
|---|---|
| `-v`, `--verbose` | In chi tiết (verbose) |
| `-h`, `--help` | Hiển thị trợ giúp |
| `-c`, `--continuous` | Theo dõi liên tục, kiểm tra định kỳ |
| `-i`, `--interval <sec>` | Chu kỳ kiểm tra (mặc định 5 giây) |
| `-f`, `--format {text,json,verbose}` | Định dạng xuất (mặc định: text) |
| `-o`, `--output <file>` | Ghi kết quả ra file |
| `--version` | Hiển thị phiên bản |

**Ví dụ**

- Verbose một lần:

```bash
./bin/deadlock_detector -v
```

- Theo dõi liên tục mỗi 10 giây, xuất JSON ra file:

```bash
./bin/deadlock_detector -c -i 10 -f json -o results.json
```

---

## 📝 Ý nghĩa kết quả

- “No deadlock detected”: Không phát hiện deadlock tại thời điểm kiểm tra
- “DEADLOCK DETECTED!”: Phát hiện deadlock, kèm danh sách PID, chuỗi chờ (wait chain)

Ví dụ:

```
========================================
DEADLOCK DETECTED!
========================================

Deadlocked Processes:
PID: 1234, Name: process_a
PID: 1235, Name: process_b

Cycle Chain:
process_a → (waiting Resource_X) → process_b → (waiting Resource_Y) → process_a

Recommendations:
1. Terminate một tiến trình trong chu trình
2. Rà soát thứ tự giữ tài nguyên
3. Thêm cơ chế timeout
```

---

## 💡 Kiểm tra rò rỉ bộ nhớ

```bash
valgrind --leak-check=full ./bin/deadlock_detector -v
```

> Kỳ vọng: `ERROR SUMMARY: 0 errors from 0 contexts`

---

## 🧪 Kiểm thử

```bash
make test
```

Tất cả test nên PASS. Nếu lỗi, xem thông báo và sửa.

---

## 🛡️ Cấu trúc project

```
deadlock_detector/
├── src/
│   ├── main.c
│   ├── process_monitor.c/.h
│   ├── resource_graph.c/.h
│   ├── cycle_detection.c/.h
│   ├── deadlock_detection.c/.h
│   ├── output_handler.c/.h
│   ├── utility.c/.h
│   └── config.h
├── test/
│   ├── test_graph.c
│   ├── test_cycle.c
│   └── test_system.c
├── bin/
│   ├── deadlock_detector
│   ├── test_graph
│   ├── test_cycle
│   └── test_system
├── Makefile
└── README.md
```

---

## ⚡ Lỗi thường gặp

- Thiếu `make/gcc`: Cài theo hướng dẫn cài đặt
- Thiếu header: Kiểm tra đầy đủ file `.h`, `Makefile`
- “Permission denied” khi chạy: `chmod +x bin/deadlock_detector`
- Valgrind báo lỗi: Kiểm tra và sửa quản lý bộ nhớ

---

## 📑 Tài liệu tham khảo

- `/proc` filesystem (Linux): `man 5 proc`
- DFS cycle detection: https://en.wikipedia.org/wiki/Cycle_detection
- Operating System Concepts (Silberschatz et al.): Chương Deadlocks

---

## 📬 Liên hệ

Phát hiện bug/góp ý: tạo Issue trên repo hoặc liên hệ tác giả.

**Chúc bạn dùng project hiệu quả!**
