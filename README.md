# ProcessLab

A **C++ terminal-based operating system kernel simulator** that models core OS concepts including **process management, CPU scheduling, demand paging, virtual memory, and thrashing detection** through an interactive shell.

---

## Overview

ProcessLab simulates the core responsibilities of an operating system kernel. It manages processes using **Process Control Blocks (PCBs)**, schedules CPU execution using multiple scheduling algorithms, allocates memory through a paging-based model, handles page faults, monitors thrashing, and records system activity.

The simulator exposes these components through an interactive `os>` shell, allowing users to create and manage processes, inspect memory usage, switch scheduling algorithms, and observe how kernel subsystems interact in real time.

---

## Features

- Interactive `os>` shell with 9 built-in commands
- CPU scheduling:
  - First Come First Serve (FCFS)
  - Shortest Job First (SJF)
  - Round Robin (Quantum = 4)
  - Priority Scheduling
- Random scheduling algorithm selected during boot
- Automatic scheduler switching when thrashing is detected
- Process Control Block (PCB) management
- 100 KB simulated RAM (25 frames)
- 200 KB simulated Virtual Memory (50 frames)
- 4 KB page size
- Demand paging (only 10% of a process initially loaded into RAM)
- Page fault handling with frame replacement when RAM is full
- Thrashing detection (page fault rate > 5%)
- Timestamped boot log (`bootlog_<timestamp>.txt`)
- RAM and Virtual Memory frame visualization
- Five preloaded test processes (`tests.cpp`)

---

## Simulated Kernel Components

| Component          | Description                                                    |
| ------------------ | -------------------------------------------------------------- |
| Process Manager    | Creates, schedules, tracks, and terminates processes           |
| CPU Scheduler      | FCFS, SJF, Round Robin, and Priority scheduling                |
| Memory Manager     | Simulates RAM and Virtual Memory using paging                  |
| Page Fault Handler | Loads missing pages and performs frame replacement             |
| Thrashing Monitor  | Detects excessive page faults and switches scheduling strategy |
| Logger             | Records kernel events and execution statistics                 |
| Interactive Shell  | Provides a command-line interface to the simulator             |

---

## Core OS Concepts

| Concept                     | Implementation                                              |
| --------------------------- | ----------------------------------------------------------- |
| Process Control Block (PCB) | `pid`, `bt`, `at`, `ct`, `tat`, `wt`, `pf`, `state`         |
| Turnaround Time             | `TAT = CT - AT`                                             |
| Waiting Time                | `WT = TAT - BT`                                             |
| Ready Queue                 | `std::deque<Process>`                                       |
| PID Lookup                  | `std::map<int, int>`                                        |
| CPU Scheduling              | FCFS, SJF, Round Robin, Priority                            |
| Demand Paging               | Only 10% of pages loaded during process creation            |
| Physical Memory             | 100 KB RAM (25 frames)                                      |
| Virtual Memory              | 200 KB (50 frames)                                          |
| Page Size                   | 4 KB                                                        |
| Page Fault Handler          | Loads missing pages into RAM and evicts frames if necessary |
| Thrashing Detection         | Page fault rate > 5% triggers scheduler adaptation          |

---

## Shell Commands

```text
help        Show all available commands
create      Create a new process
ps          Display the process table
kill        Terminate a process by PID
mem         Display RAM usage and frame allocation
vm          Display Virtual Memory usage and frame allocation
logs        Display the current boot log
scheduler   View or change the scheduling algorithm
stats       Display system statistics
exit        Shut down ProcessLab
```

---

## Memory Model

### Physical Memory

- Total RAM: **100 KB**
- Frame Count: **25**
- Frame Size: **4 KB**

### Virtual Memory

- Total Size: **200 KB**
- Frame Count: **50**
- Frame Size: **4 KB**

### Demand Paging

When a process is created, only **10%** of its pages are initially loaded into RAM.

Remaining pages are loaded on demand whenever the process accesses memory that is not currently resident in physical memory.

If RAM is full, the memory manager performs frame replacement before loading the requested page.

---

## Thrashing Detection

ProcessLab continuously monitors the page fault rate.

If the overall fault rate exceeds **5%**, the simulator considers the system to be thrashing and automatically switches to another scheduling algorithm to improve system performance.

Every scheduler switch is recorded in the system log.

---

## Log File

Every execution generates a timestamped log file:

```text
bootlog_<timestamp>.txt
```

The log contains:

- Boot events and memory initialization
- Process creation and termination
- Scheduling decisions
- Page faults and frame evictions
- Thrashing detection events
- Scheduler switches
- Per-process statistics
- Final execution summary

---

## Build & Run

### Windows (CMake)

```powershell
mkdir build
cd build
cmake ..
cmake --build .
.\Debug\ProcessLab.exe
```

### Linux / macOS

```bash
make run
```

---

## Project Structure

```text
ProcessLab/
├── CMakeLists.txt
├── Makefile
└── src/
    ├── main.cpp          // Entry point
    ├── Process.h         // PCB definition
    ├── Memory.h/.cpp     // Paging, RAM, Virtual Memory
    ├── Scheduler.h/.cpp  // Scheduling algorithms
    ├── Logger.h          // Event logging
    ├── OS.h/.cpp         // Kernel controller + shell
    └── tests.h/.cpp      // Preloaded test processes
```

---

## Technologies

- C++17
- Standard Template Library (STL)
- `std::deque`
- `std::vector`
- `std::map`
- `std::fstream`
- CMake
- Make

No external libraries or dependencies are required.

---

## Why ProcessLab?

ProcessLab was built to better understand how an operating system kernel coordinates multiple subsystems.

The simulator demonstrates:

- Process lifecycle management
- CPU scheduling
- Process Control Blocks (PCBs)
- Demand paging
- Virtual memory management
- Page fault handling
- Memory allocation
- Thrashing detection and recovery
- Kernel event logging
- Interactive shell design

Although simplified, the simulator models the interaction between these kernel components using concepts commonly taught in undergraduate operating systems courses.

---

## Status

ProcessLab is an educational operating system simulator built for learning and experimentation. It is **not** a real operating system kernel and does not execute real processes or interact with hardware.

Instead, it provides a conceptual simulation of kernel behavior, making it easier to visualize how operating systems schedule processes, manage memory, handle page faults, and respond to memory pressure.
