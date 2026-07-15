# ST5004CEM — Operating Systems and Security
## 1. Overview
This submission implements four core Operating Systems concepts in C on Ubuntu Linux:

| Task|             Topic                   |
| 1   | Process Management and Threading    |
| 2   | Memory Management Simulation        | 
| 3   | File System Operations and Security |
| 4   | Network Programming and IPC         |

All programs were written and tested on **Ubuntu 24.04 (VMware Workstation pro)** using gcc compilar.
## 2. System Requirements
- Ubuntu(or WSL) with `gcc` and POSIX threads support.
- Install the compiler toolchain if not already present:

```bash
sudo apt update
sudo apt install build-essential
```
No external libraries are required — every program uses only the standard C library and POSIX
(`pthread`, `unistd.h`, `sys/socket.h`, etc.) headers that ship with Ubuntu.

---

## 3. Folder Structure

```
│
├── README.md                   
|
│
├── Task1_ProcessManagement/
│   ├── fork_demo.c
│   ├── thread.c
│   ├── race_condition.c
│   ├── race_fixed_mutex.c
│   ├── deadlock_demo.c
│   ├── deadlock_prevention.c
│   └── round_robin.c
│
├── Task2_MemoryManagement/
│   ├── frame_demo.c
│   └── paging_simulator.c
│
├── Task3_FileSystemSecurity/
│   ├── create_permissions.c
│   ├── read_write.c
│   └── secure_file_manager.c
│
└── Task4_NetworkProgramming/
    ├── protocol.h
    ├── server.c
    └── client.c
```

---

## 4. Task 1 — Process Management and Threading

**Files:** `fork_demo.c`, `thread.c`, `race_condition.c`, `race_fixed_mutex.c`,
`deadlock_demo.c`, `deadlock_prevention.c`, `round_robin.c`

### Compile and run

```bash
cd Task1_ProcessManagement

gcc fork_demo.c -o fork_demo && ./fork_demo
gcc thread.c -o thread -pthread && ./thread
gcc race_condition.c -o race -pthread && ./race
gcc race_fixed_mutex.c -o mutex -pthread && ./mutex
gcc deadlock_demo.c -o deadlock -pthread && ./deadlock        # Ctrl+C to stop (demonstrates deadlock hang)
gcc deadlock_prevention.c -o deadlock_prevention -pthread && ./deadlock_prevention
gcc round_robin.c -o round_robin && ./round_robin
```
### What each program shows
- `fork_demo.c` — parent/child process creation with `fork()`
- `thread.c` — creating and joining 4 POSIX threads
- `race_condition.c` — a shared counter corrupted by two threads with no synchronization
- `race_fixed_mutex.c` — the same counter fixed correctly using `pthread_mutex_t`
- `deadlock_demo.c` — two threads deadlocking by locking two mutexes in opposite order
- `deadlock_prevention.c` — the same scenario fixed using consistent lock ordering
- `round_robin.c` — Round-Robin CPU scheduling simulation with completion/turnaround/waiting time

---

## 5. Task 2 — Memory Management Simulation

**Files:** `frame_demo.c`, `paging_simulator.c`

### Compile and run
```bash
cd Task2_MemoryManagement

gcc frame_demo.c -o frame_demo && ./frame_demo
gcc paging_simulator.c -o paging_simulator && ./paging_simulator
```

### What each program shows
- `frame_demo.c` — basic paging concept: loading pages into empty frames, pointer
  dereferencing, and FIFO eviction once frames are full
- `paging_simulator.c` — full simulator with configurable page size / frame count,
  FIFO and LRU replacement algorithms, page fault/hit tracking, and two test cases
  comparing algorithm performance (hit ratio, miss ratio)

---

## 6. Task 3 — File System Operations and Security

**Files:** `create_permissions.c`, `read_write.c`, `secure_file_manager.c`

### Compile and run
```bash
cd Task3_FileSystemSecurity

gcc create_permissions.c -o create_permissions && ./create_permissions
gcc read_write.c -o read_write && ./read_write
gcc secure_file_manager.c -o secure_file_manager && ./secure_file_manager
```
### Demo login credentials (secure_file_manager)
| Username | Password |
|----------|----------|
| bhawana  | Bhawana@123 |

### What each program shows
- `create_permissions.c` — creates a file and sets owner/group/other read-write-execute
  permissions with `chmod()`
- `read_write.c` — writes to and reads from a file, using `access()` to check
  read permission before opening, and deletes the file with `unlink()`
- `secure_file_manager.c` — the complete secure file management system:
  1. User authentication (hashed password login, `MAX_LOGIN_TRY` limited attempts)
  2. File create / write / read / delete
  3. File permission management (owner/group/others r/w/x via `chmod()`)
  4. XOR encryption / decryption of sensitive files
  5. Audit logging of every action to `audit.log` (view from the in-app menu)

  Menu options: 1 Create file · 2 Write to file · 3 Read file · 4 Delete file ·
  5 Set file permissions · 6 Encrypt file · 7 Decrypt file · 8 View audit log · 9 Logout/Exit

---

## 7. Task 4 — Network Programming and IPC

**Files:** `protocol.h`, `server.c`, `client.c`

`protocol.h` is a shared header (not compiled on its own) that defines the message format
used by both `server.c` and `client.c` — it only needs to be in the same folder when compiling.

### Compile
```bash
cd Task4_NetworkProgramming

gcc server.c -o server  & ./server
gcc client.c -o client  & ./client
```

### Run

**Terminal 1 — start the server (leave running):**
```bash
./server
```
On first run this creates `users.txt` with demo accounts and starts listening on port 8080.

**Terminal 2, 3, 4... — run one or more clients:**
```bash
./client 127.0.0.1 8080
```

### Demo login credentials
| Username | Password |
|----------|----------|
| bhawana  | Bhawana@123 |
| sita     | Sita@123 |
| ram      | Ram@123 |
| gita     | Gita@123 |

Log in, then type a message and press **Enter** to broadcast it to every other connected
client. Type `/quit` to disconnect. Open several terminals with different accounts to test
concurrent client handling.

### Protocol summary
Text-based, colon-delimited, newline-terminated messages over TCP (port 8080):

| Message | Direction | Meaning |
|---------|-----------|---------|
| `AUTH:<user>:<pass>` | client → server | login request |
| `AUTH_OK:<user>` / `AUTH_FAIL:<reason>` | server → client | login result |
| `MSG:<user>:<text>` | client → server | send a chat message |
| `BCAST:<user>:<text>` | server → client | deliver a message from another user |
| `ERR:<code>:<description>` | server → client | protocol/validation error |
| `QUIT` | client → server | graceful disconnect |

Every connection, login, message and disconnect is logged with a timestamp to
`server_audit.log`.

---

## 8. Notes on Security Features (Tasks 3 & 4)

- Passwords are never stored or sent in plaintext for storage purposes — a djb2 hash is
  stored in `users.txt`. This is sufficient for a coursework demonstration; a production
  system should use a salted, slow hash such as bcrypt or Argon2.
- XOR encryption (Task 3) demonstrates the encrypt/decrypt workflow only and is **not**
  cryptographically secure; a real deployment should use AES-256.
- All programs validate input (file permission checks, protocol field validation) before
  acting on it, and log actions for accountability (`audit.log` / `server_audit.log`).
- Running any of these programs as `root` bypasses the normal Linux permission checks —
  run as a regular user to see the permission system enforced correctly.

---

## 10. Author

Bhawana Kumari Bhatta — Coventry ID 15938314
Module: ST5004CEM Operating Systems and Security, Softwarica College of IT & E-Commerce,
in collaboration with Coventry University.
