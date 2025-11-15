# Deadlock Detector - Process-Level Deadlock Detection System

A comprehensive C-based deadlock detection system for Linux and WSL2 that monitors system processes and detects deadlocks involving pipes and file locks.

## 📋 Table of Contents

1. [Project Overview](#project-overview)
2. [Requirements](#requirements)
3. [Installation](#installation)
4. [Usage](#usage)
5. [Email Alert Feature](#email-alert-feature)
6. [Testing Deadlock Detection](#testing-deadlock-detection)
7. [Output Formats](#output-formats)
8. [System Architecture](#system-architecture)
9. [Code Reading Guide](#code-reading-guide)
10. [Development Guide](#development-guide)
11. [Troubleshooting](#troubleshooting)
12. [Project Structure](#project-structure)
13. [Features and Limitations](#features-and-limitations)

---

## 🎯 Project Overview

The Deadlock Detector is a system-level tool that monitors Linux processes and detects deadlock conditions in real-time. It scans the `/proc` filesystem to build a Resource Allocation Graph (RAG), uses Depth-First Search (DFS) with 3-color marking for cycle detection, and reports processes involved in deadlocks.

### Key Features

- **Process-Level Deadlock Detection**: Detects deadlocks between multiple processes
- **Pipe Deadlock Detection**: Identifies circular dependencies in pipe I/O operations
- **File Lock Deadlock Detection**: Detects deadlocks involving `flock()` file locks
- **Real-Time Monitoring**: Continuous monitoring mode with configurable intervals
- **Multiple Output Formats**: Text, JSON, and verbose output formats
- **Email Alert System**: Sends email notifications when deadlocks are detected
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
- `mail` or `sendmail` command (for email alerts, requires Postfix/ssmtp)

### System Requirements
- Access to `/proc` filesystem (standard on Linux)
- Sufficient permissions to read process information (usually requires root or same user)

---

## 🐧 Installation

### Installation on Linux

#### Step 1: Install Dependencies

```bash
# Update package list
sudo apt-get update

# Install build tools
sudo apt-get install -y build-essential gcc make

# Install valgrind (optional, for memory leak testing)
sudo apt-get install -y valgrind

# Install mail utilities (for email alerts)
sudo apt-get install -y postfix mailutils
```

#### Step 2: Clone or Navigate to Project Directory

```bash
cd /path/to/deadlock_detector
```

#### Step 3: Build the Project

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

#### Step 4: Run Tests

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

#### Step 5: Verify Installation

```bash
# Check if executable was created
ls -lh bin/deadlock_detector

# Run with help to verify
./bin/deadlock_detector --help
```

### Installation on WSL2 Ubuntu

#### Step 1: Install WSL2 (if not already installed)

On Windows, open PowerShell as Administrator:

```powershell
# Enable WSL2
wsl --install

# Set WSL2 as default
wsl --set-default-version 2
```

#### Step 2: Launch WSL2 Ubuntu

```bash
# Open Ubuntu from Start Menu or run:
wsl
```

#### Step 3: Install Dependencies in WSL2

```bash
# Update package list
sudo apt-get update

# Install build tools
sudo apt-get install -y build-essential gcc make

# Install valgrind (optional)
sudo apt-get install -y valgrind

# Install mail utilities
sudo apt-get install -y postfix mailutils
```

#### Step 4: Navigate to Project Directory

```bash
# If project is in Windows filesystem (e.g., D:\)
cd /mnt/d/DOCUMENT/System_Programing/deadlock_detector

# Or if project is in WSL filesystem
cd ~/deadlock_detector
```

#### Step 5: Build the Project

```bash
# Clean and build
make clean && make
```

#### Step 6: Run Tests

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
| `--alert` | - | Alert mechanism: email or none | none |
| `--email-to` | - | Comma-separated email recipients | - |
| `--log-file` | - | Append results to log file | - |
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

## 📧 Email Alert Feature

The deadlock detector can send email notifications when deadlocks are detected.

### Quick Start

#### Method 1: Using Configuration File (Recommended)

1. Create or edit `email.conf` in the project directory:

```conf
email_to=admin@example.com
sender_name=Deadlock Detector System
```

2. Run with email alert enabled:

```bash
./bin/deadlock_detector --alert email
```

Email will be automatically sent when a deadlock is detected.

#### Method 2: Using Command-Line Options

```bash
# Send email to single recipient
./bin/deadlock_detector --alert email --email-to admin@example.com

# Send email to multiple recipients
./bin/deadlock_detector --alert email \
  --email-to admin@example.com,manager@example.com

# Send email + log to file
./bin/deadlock_detector --alert email \
  --email-to admin@example.com \
  --log-file /var/log/deadlock.log

# Continuous monitoring with email alerts
./bin/deadlock_detector -c -i 30 \
  --alert email \
  --email-to admin@example.com \
  --log-file /var/log/deadlock.log
```

### Configuration File

The `email.conf` file can be placed in:
1. Project directory (current working directory) - **checked first**
2. `~/.deadlock_detector/email.conf` - fallback location

**Format:**
```conf
# Email Configuration for Deadlock Detector
# Format: key=value (one per line)
# Lines starting with # are comments

email_to=admin@example.com
sender_name=Deadlock Detector System
```

### Email Content

When a deadlock is detected, the email includes:
- **Subject**: `DEADLOCK ALERT: [timestamp]`
- **Body**:
  - Timestamp
  - Deadlock status (YES/NO)
  - Number of cycles detected
  - List of deadlocked processes (PID + process name)
  - Cycle details
  - Recommendations for resolution
  - Sender information

### Log File Format

If `--log-file` is specified, entries are written in this format:

```
[2025-11-10 21:52:30] DEADLOCK DETECTED
  Processes: PID 1234 (process_name), PID 1235 (process_name)
  Cycles: 4
  Email: Email sent to: admin@example.com (SUCCESS 1/1)

[2025-11-10 21:55:45] No deadlock detected
  Email: Email alert state: No deadlock detected (NOT_TRIGGERED)
```

### Email Sending Method

The detector uses the `mail` command (compatible with Postfix/ssmtp):

1. Creates a temporary file with email body
2. Executes: `mail -s "subject" "recipient" < temp_file`
3. Cleans up temporary file

**Requirements:**
- `mail` command must be available (install Postfix or ssmtp)
- Postfix/ssmtp must be configured for external email relay (if sending to external addresses)

### Configuring ssmtp for Email Sending

ssmtp is a simple sendmail replacement that forwards emails to an external SMTP server. It's easier to configure than Postfix for basic email sending.

#### Step 1: Install ssmtp

```bash
sudo apt-get update
sudo apt-get install -y ssmtp mailutils
```

#### Step 2: Configure ssmtp

Edit `/etc/ssmtp/ssmtp.conf`:

```bash
sudo nano /etc/ssmtp/ssmtp.conf
```

**For Gmail (using App Password):**

```conf
# /etc/ssmtp/ssmtp.conf

# The user that gets all mail for userids < 1000
root=your-email@gmail.com

# The place where the mail goes
mailhub=smtp.gmail.com:587

# Where will the mail seem to come from?
rewriteDomain=gmail.com

# The full hostname
hostname=your-hostname

# Use SSL/TLS
UseTLS=Yes
UseSTARTTLS=Yes

# Username and password for SMTP authentication
AuthUser=your-email@gmail.com
AuthPass=your-app-password

# Set this to never rewrite the "From:" line
FromLineOverride=YES
```

**For other SMTP servers:**

```conf
# /etc/ssmtp/ssmtp.conf

root=your-email@example.com
mailhub=smtp.example.com:587
rewriteDomain=example.com
hostname=your-hostname

# Use SSL/TLS
UseTLS=Yes
UseSTARTTLS=Yes

# SMTP authentication
AuthUser=your-email@example.com
AuthPass=your-password

FromLineOverride=YES
```

#### Step 3: Configure revaliases (optional)

Edit `/etc/ssmtp/revaliases` to set sender email for specific users:

```bash
sudo nano /etc/ssmtp/revaliases
```

Add:
```
root:your-email@gmail.com:smtp.gmail.com:587
```

#### Step 4: Set proper permissions

```bash
sudo chmod 640 /etc/ssmtp/ssmtp.conf
sudo chmod 640 /etc/ssmtp/revaliases
sudo chown root:mail /etc/ssmtp/ssmtp.conf
sudo chown root:mail /etc/ssmtp/revaliases
```

#### Step 5: Test email sending

```bash
# Test with mail command
echo "Test message" | mail -s "Test Subject" your-email@example.com

# Or test with ssmtp directly
echo "Test message" | ssmtp your-email@example.com
```

#### Gmail App Password Setup

If using Gmail, you need to create an App Password:

1. Go to [Google Account Settings](https://myaccount.google.com/)
2. Security → 2-Step Verification (must be enabled)
3. App passwords → Generate new app password
4. Select "Mail" and your device
5. Copy the 16-character password
6. Use this password in `AuthPass` in `/etc/ssmtp/ssmtp.conf`

**Important:** Never use your regular Gmail password. Always use App Passwords for SMTP authentication.

#### Alternative: Using Postfix

If you prefer Postfix instead of ssmtp:

```bash
# Install Postfix
sudo apt-get install -y postfix

# During installation, select "Internet Site"
# Enter your domain name when prompted

# Configure Postfix for Gmail relay
sudo nano /etc/postfix/main.cf
```

Add to `main.cf`:
```
relayhost = [smtp.gmail.com]:587
smtp_sasl_auth_enable = yes
smtp_sasl_password_maps = hash:/etc/postfix/sasl_passwd
smtp_sasl_security_options = noanonymous
smtp_tls_security_level = encrypt
smtp_tls_CAfile = /etc/ssl/certs/ca-certificates.crt
```

Create `/etc/postfix/sasl_passwd`:
```
[smtp.gmail.com]:587    your-email@gmail.com:your-app-password
```

Then:
```bash
sudo postmap /etc/postfix/sasl_passwd
sudo chmod 600 /etc/postfix/sasl_passwd
sudo systemctl restart postfix
```

### Troubleshooting Email

**Email not being sent?**

1. Check if `mail` command exists:
   ```bash
   which mail
   ```

2. Check Postfix status:
   ```bash
   sudo systemctl status postfix
   ```

3. Test mail command directly:
   ```bash
   echo "Test" | mail -s "Test Subject" your-email@example.com
   ```

4. Check email configuration:
   ```bash
   cat email.conf
   ```

5. Check debug logs (stderr):
   ```bash
   ./bin/deadlock_detector --alert email --email-to test@example.com 2>&1 | grep EMAIL
   ```

**Error: "mail command not found"**

- Install Postfix: `sudo apt-get install postfix -y`
- Or install ssmtp: `sudo apt-get install ssmtp mailutils -y`

**ssmtp Configuration Issues:**

1. **Check ssmtp configuration:**
   ```bash
   cat /etc/ssmtp/ssmtp.conf
   ```

2. **Check file permissions:**
   ```bash
   ls -l /etc/ssmtp/ssmtp.conf
   # Should be: -rw-r----- root mail
   ```

3. **Test ssmtp directly:**
   ```bash
   echo "Test" | ssmtp -v your-email@example.com
   ```

4. **Check ssmtp logs:**
   ```bash
   tail -f /var/log/mail.log
   # Or
   journalctl -u ssmtp -f
   ```

5. **Common ssmtp errors:**
   - **"Cannot open smtp.gmail.com:587"**: Check internet connection, firewall
   - **"Authentication failed"**: Verify App Password is correct (Gmail)
   - **"Permission denied"**: Check file permissions on `/etc/ssmtp/ssmtp.conf`
   - **"relay access denied"**: Check `mailhub` setting in config

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

## 🏗️ System Architecture

### Architecture Overview

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

### Core Modules

1. **Process Monitor** (`process_monitor.c/.h`)
   - Reads `/proc` filesystem
   - Collects process information (PID, name, state)
   - Identifies resources (pipes, file locks)
   - Parses `/proc/[PID]/status`, `/proc/[PID]/fd`, `/proc/[PID]/locks`

2. **Resource Graph** (`resource_graph.c/.h`)
   - Builds Resource Allocation Graph (RAG)
   - Uses adjacency list representation
   - Supports request edges (P→R) and allocation edges (R→P)
   - Handles single and multiple instance resources

3. **Cycle Detection** (`cycle_detection.c/.h`)
   - Implements DFS-based cycle detection
   - Uses 3-color marking (WHITE, GRAY, BLACK)
   - Time complexity: O(V+E)
   - Finds all cycles in the graph

4. **Deadlock Detection** (`deadlock_detection.c/.h`)
   - Orchestrates detection process
   - Analyzes cycles to identify deadlocks
   - Distinguishes single-instance (definite) vs multi-instance (potential) deadlocks
   - Generates comprehensive reports

5. **Output Handler** (`output_handler.c/.h`)
   - Formats results (Text, JSON, Verbose)
   - Generates recommendations
   - Exports to files

6. **Email Alert** (`email_alert.c/.h`)
   - Sends email notifications
   - Logs detection results
   - Configurable via file or command-line

7. **Utility** (`utility.c/.h`)
   - Helper functions (memory management, string operations)
   - Error handling macros
   - File I/O utilities

### Algorithms

#### Resource Allocation Graph (RAG)

- **Definition**: Directed graph with two vertex types:
  - Process vertices (P)
  - Resource vertices (R)
- **Edges**:
  - Request edge: P → R (process P waits for resource R)
  - Allocation edge: R → P (resource R is allocated to process P)
- **Deadlock Rule**: Cycle in RAG with single-instance resources = definite deadlock

#### DFS-Based Cycle Detection

```
1. Initialize all vertices = WHITE (unvisited)
2. For each unvisited vertex:
   a. Call DFS_VISIT(vertex)
   b. In DFS_VISIT(v):
      - Mark v = GRAY (processing)
      - For each neighbor u:
        * If u = WHITE: Recursively DFS_VISIT(u)
        * If u = GRAY: Found cycle! (back edge)
      - Mark v = BLACK (done)
3. Return all cycles found
```

**Time Complexity**: O(V+E)  
**Space Complexity**: O(V)

---

## 📖 Code Reading Guide

Để hiểu được dự án này, bạn nên đọc các file code theo thứ tự sau:

### Thứ Tự Đọc Code (Từ Cơ Bản Đến Phức Tạp)

#### **Bước 1: Đọc Constants & Utilities (Nền Tảng)**

1. **`src/config.h`** (50-100 dòng)
   - **Mục đích**: Định nghĩa tất cả constants, error codes, limits
   - **Nội dung chính**:
     - Error codes (`SUCCESS`, `ERROR_*`)
     - Maximum sizes (`MAX_PROCESSES`, `MAX_EMAIL_RECIPIENTS_LEN`)
     - Configuration constants
   - **Tại sao đọc đầu tiên**: Hiểu được các giới hạn và error codes dùng trong toàn bộ project

2. **`src/utility.h`** + **`src/utility.c`** (200-300 dòng)
   - **Mục đích**: Các hàm helper cơ bản
   - **Nội dung chính**:
     - Memory management: `safe_malloc()`, `safe_free()`
     - String operations: `str_trim()`, `str_split()`
     - File I/O: `read_entire_file()`
     - Logging macros: `error_log()`, `debug_log()`, `info_log()`
   - **Tại sao đọc tiếp theo**: Các module khác đều dùng các hàm này

#### **Bước 2: Đọc Process Monitoring (Thu Thập Dữ Liệu)**

3. **`src/process_monitor.h`** + **`src/process_monitor.c`** (300-400 dòng)
   - **Mục đích**: Đọc thông tin process từ `/proc` filesystem
   - **Nội dung chính**:
     - `get_all_processes()`: Liệt kê tất cả PIDs
     - `get_process_info()`: Parse `/proc/[PID]/status`
     - `get_process_resources()`: Xác định resources (pipes, file locks)
     - `read_proc_file()`: Đọc file trong `/proc`
   - **Tại sao đọc tiếp**: Đây là nguồn dữ liệu đầu vào cho toàn bộ hệ thống
   - **Điểm quan trọng**: Hiểu cách parse `/proc` filesystem

#### **Bước 3: Đọc Graph Data Structure (Cấu Trúc Dữ Liệu)**

4. **`src/resource_graph.h`** + **`src/resource_graph.c`** (400-500 dòng)
   - **Mục đích**: Xây dựng Resource Allocation Graph (RAG)
   - **Nội dung chính**:
     - `create_graph()`: Khởi tạo graph
     - `add_request_edge()`: Thêm cạnh P→R (process chờ resource)
     - `add_allocation_edge()`: Thêm cạnh R→P (resource được cấp phát)
     - `free_graph()`: Cleanup memory
   - **Tại sao đọc tiếp**: Graph là cấu trúc dữ liệu trung tâm
   - **Điểm quan trọng**: Hiểu adjacency list representation

#### **Bước 4: Đọc Cycle Detection (Thuật Toán Core)**

5. **`src/cycle_detection.h`** + **`src/cycle_detection.c`** (300-400 dòng)
   - **Mục đích**: Phát hiện chu trình trong graph bằng DFS
   - **Nội dung chính**:
     - `has_cycle()`: Điểm vào chính
     - `dfs_visit()`: DFS đệ quy với 3-color marking
     - `find_all_cycles()`: Tìm tất cả cycles
     - `extract_cycle_path()`: Trích xuất đường đi chu trình
   - **Tại sao đọc tiếp**: Đây là thuật toán core để phát hiện deadlock
   - **Điểm quan trọng**: Hiểu DFS với WHITE/GRAY/BLACK marking

#### **Bước 5: Đọc Deadlock Detection (Logic Chính)**

6. **`src/deadlock_detection.h`** + **`src/deadlock_detection.c`** (500-600 dòng)
   - **Mục đích**: Tích hợp tất cả modules, phát hiện deadlock
   - **Nội dung chính**:
     - `detect_deadlock_in_system()`: Hàm chính
     - `build_rag_from_processes()`: Xây dựng RAG từ process info
     - `analyze_cycles_for_deadlock()`: Phân tích cycles
     - `generate_explanations()`: Tạo giải thích
     - `generate_recommendations()`: Tạo khuyến nghị
   - **Tại sao đọc tiếp**: Đây là module điều phối toàn bộ
   - **Điểm quan trọng**: Hiểu workflow từ process → graph → cycle → deadlock

#### **Bước 6: Đọc Output Handler (Hiển Thị Kết Quả)**

7. **`src/output_handler.h`** + **`src/output_handler.c`** (400-500 dòng)
   - **Mục đích**: Format và hiển thị kết quả
   - **Nội dung chính**:
     - `display_deadlock_report()`: Hàm chính
     - `format_as_text()`: Format text
     - `format_as_json()`: Format JSON
     - `format_as_verbose()`: Format verbose
   - **Tại sao đọc tiếp**: Hiểu cách output được tạo ra
   - **Điểm quan trọng**: Hiểu 3 định dạng output

#### **Bước 7: Đọc Email Alert (Tính Năng Phụ)**

8. **`src/email_alert.h`** + **`src/email_alert.c`** (600-700 dòng)
   - **Mục đích**: Gửi email khi phát hiện deadlock
   - **Nội dung chính**:
     - `email_alert_set_options()`: Cấu hình email
     - `email_alert_handle_detection()`: Xử lý khi phát hiện deadlock
     - `send_email_alert()`: Gửi email qua `mail` command
     - `read_email_config()`: Đọc file `email.conf`
     - `write_log_file()`: Ghi log file
   - **Tại sao đọc tiếp**: Tính năng bổ sung, không ảnh hưởng đến core logic
   - **Điểm quan trọng**: Hiểu cách tích hợp email vào detection flow

#### **Bước 8: Đọc Main Entry Point (Tổng Hợp)**

9. **`src/main.c`** (600-700 dòng)
   - **Mục đích**: Entry point, CLI, điều phối toàn bộ
   - **Nội dung chính**:
     - `main()`: Entry point
     - `parse_arguments()`: Parse command-line arguments
     - `run_detection()`: Chạy một lần detection
     - `setup_signal_handlers()`: Xử lý SIGINT
     - `apply_email_configuration()`: Load email config
   - **Tại sao đọc cuối**: Tổng hợp tất cả modules
   - **Điểm quan trọng**: Hiểu flow từ CLI → detection → output

### Tóm Tắt Thứ Tự Đọc

```
1. config.h                    (Constants & error codes)
2. utility.h/c                 (Helper functions)
3. process_monitor.h/c         (Data collection)
4. resource_graph.h/c          (Data structure)
5. cycle_detection.h/c         (Core algorithm)
6. deadlock_detection.h/c      (Main logic)
7. output_handler.h/c          (Output formatting)
8. email_alert.h/c             (Email notifications)
9. main.c                      (Entry point & orchestration)
```

### Thời Gian Ước Tính

| File | Thời Gian Đọc | Độ Khó |
|------|---------------|--------|
| `config.h` | 5 phút | ⭐ Dễ |
| `utility.h/c` | 15 phút | ⭐ Dễ |
| `process_monitor.h/c` | 30 phút | ⭐⭐ Trung bình |
| `resource_graph.h/c` | 30 phút | ⭐⭐ Trung bình |
| `cycle_detection.h/c` | 45 phút | ⭐⭐⭐ Khó |
| `deadlock_detection.h/c` | 45 phút | ⭐⭐⭐ Khó |
| `output_handler.h/c` | 20 phút | ⭐ Dễ |
| `email_alert.h/c` | 30 phút | ⭐⭐ Trung bình |
| `main.c` | 30 phút | ⭐⭐ Trung bình |
| **Tổng cộng** | **~4 giờ** | - |

### Tips Đọc Code

1. **Đọc header file trước** (`.h`), sau đó mới đọc implementation (`.c`)
2. **Tập trung vào function signatures** trong header để hiểu interface
3. **Đọc comments** - mỗi function đều có mô tả chi tiết
4. **Trace function calls** - xem function nào gọi function nào
5. **Hiểu data flow**: Process → Graph → Cycle → Deadlock → Output
6. **Sử dụng IDE** để jump to definition và tìm references

### Các Khái Niệm Quan Trọng Cần Hiểu

- **Resource Allocation Graph (RAG)**: Graph có 2 loại vertices (Process, Resource)
- **DFS Cycle Detection**: Thuật toán tìm chu trình với 3-color marking
- **Deadlock Rule**: Cycle + single-instance resource = deadlock
- **`/proc` filesystem**: Cách Linux expose process information
- **Adjacency List**: Cách biểu diễn graph (không phải matrix)

### Câu Hỏi Để Kiểm Tra Hiểu Biết

Sau khi đọc xong, bạn nên trả lời được:

1. ✅ Làm thế nào để lấy danh sách tất cả processes?
2. ✅ RAG được xây dựng như thế nào?
3. ✅ DFS phát hiện cycle như thế nào?
4. ✅ Làm sao phân biệt deadlock chắc chắn vs tiềm năng?
5. ✅ Email được gửi khi nào và như thế nào?
6. ✅ Output được format như thế nào?

---

## 💻 Development Guide

### Code Structure

```
deadlock_detector/
├── src/                          # Source code
│   ├── main.c                   # Main program entry point
│   ├── process_monitor.c/.h     # Process monitoring from /proc
│   ├── resource_graph.c/.h      # Resource Allocation Graph (RAG)
│   ├── cycle_detection.c/.h     # DFS cycle detection algorithm
│   ├── deadlock_detection.c/.h  # Deadlock detection engine
│   ├── output_handler.c/.h      # Output formatting (text, JSON)
│   ├── email_alert.c/.h         # Email alert system
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
│
├── Makefile                     # Build configuration
├── README.md                    # This file
└── .gitignore                   # Git ignore file
```

### Coding Standards

#### Naming Conventions
- **Functions**: `snake_case` (e.g., `get_process_list`, `detect_deadlock`)
- **Constants**: `UPPER_CASE` (e.g., `MAX_PROCESSES`, `ERROR_FILE_NOT_FOUND`)
- **Structs**: `PascalCase` (e.g., `ResourceGraph`, `ProcessNode`)
- **Global variables**: `g_snake_case` (e.g., `g_process_count`)
- **Static variables**: `s_snake_case` within file

#### Error Handling
- Return codes: `SUCCESS` (0) for success, negative values for errors
- Always check return values
- Use `safe_malloc()` wrapper for memory allocation
- Log errors with `error_log()` macro

#### Memory Management
- Every `malloc()` must have corresponding `free()`
- Use cleanup functions for complex structures
- No memory leaks (verified with Valgrind)
- Avoid buffer overflows (use `snprintf` instead of `sprintf`)

#### Comments
- Function header comments with description, parameters, return value
- Inline comments for complex logic
- Algorithm explanations with time/space complexity

### Building from Source

```bash
# Clean previous build
make clean

# Build project
make

# Run tests
make test

# Check for memory leaks
valgrind --leak-check=full ./bin/deadlock_detector -v
```

### Debugging

#### Using GDB

```bash
gdb ./bin/deadlock_detector
(gdb) break main
(gdb) run -v
(gdb) print pids
(gdb) continue
```

#### Using Valgrind

```bash
valgrind --leak-check=full --show-leak-kinds=all \
  ./bin/deadlock_detector -v
```

#### Using Strace

```bash
strace -e open,read -o trace.txt ./bin/deadlock_detector
cat trace.txt
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

#### 9. Email Not Sending

**Problem:** Email alerts not working

**Solution:**
- Check if `mail` command exists: `which mail`
- Check Postfix status: `sudo systemctl status postfix`
- Test mail command: `echo "Test" | mail -s "Test" your-email@example.com`
- Check debug logs: `./bin/deadlock_detector --alert email 2>&1 | grep EMAIL`
- Verify email configuration: `cat email.conf`

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
│   ├── email_alert.c/.h         # Email alert system
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
│
├── Makefile                     # Build configuration
├── README.md                    # This file
├── email.conf                   # Email configuration (optional)
└── .gitignore                   # Git ignore file
```

### Key Files

- **`src/main.c`**: Command-line interface, argument parsing, signal handling
- **`src/process_monitor.c`**: Reads `/proc` filesystem, collects process information
- **`src/deadlock_detection.c`**: Main deadlock detection logic, RAG building
- **`src/cycle_detection.c`**: DFS algorithm for cycle detection
- **`src/email_alert.c`**: Email notification system
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

6. **Email Alert System**
   - Sends email notifications when deadlocks detected
   - Supports multiple recipients
   - Logs detection results to file
   - Configurable via file or command-line

7. **Comprehensive Testing**
   - 116 unit tests
   - Graph structure tests
   - Cycle detection tests
   - System integration tests

8. **Memory Safety**
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
- Real-time alerting and notifications (enhanced)
- Performance optimizations for large systems
- Web dashboard for monitoring

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
