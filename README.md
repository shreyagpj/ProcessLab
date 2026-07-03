# ProcessLab

A **CLI-based operating system simulator** built in **C++** to demonstrate fundamental OS concepts such as **process management, Round Robin CPU scheduling, demand paging, virtual memory, and page fault handling**.

The project simulates how an operating system kernel manages processes and memory through an interactive command-line interface.

---

## Overview

ProcessLab is a simplified operating system simulator that models the interaction between a CPU scheduler, process manager, and memory manager.

Processes can be created dynamically through the CLI. Each process is automatically divided into pages, partially loaded into simulated RAM using demand paging, and executed using a **Round Robin scheduler**. Memory accesses that reference pages not currently in RAM generate page faults, which are handled by the simulator by loading the required pages into memory.

All kernel events are recorded in a timestamped log file for later inspection.

---

## Features

- Interactive command-line (`os>`) interface
- Dynamic process creation
- Round Robin CPU scheduling (Quantum = 4)
- Process Control Block (PCB) management
- Automatic page generation for every process
- Demand paging
- Simulated RAM and Virtual Memory
- Page fault handling
- Frame allocation and replacement
- RAM and Virtual Memory visualization
- Timestamped execution logs
- Five preloaded test processes for demonstration

---

## Operating System Concepts Implemented

| Concept | Description |
|---------|-------------|
| Process Control Block (PCB) | Stores process information such as PID, burst time, state, turnaround time, waiting time, and page faults |
| Round Robin Scheduling | Executes processes using a fixed time quantum |
| Ready Queue | Stores runnable processes using `std::deque` |
| Demand Paging | Initially loads only part of a process into RAM |
| Paging | Processes are automatically divided into fixed-size pages |
| Virtual Memory | Simulated backing store for pages not currently in RAM |
| Page Fault Handling | Loads missing pages into RAM when referenced |
| Memory Management | Allocates and manages physical and virtual memory |

---

## Memory Model

### Physical Memory

- Total RAM: **100 KB**
- 25 Frames
- Frame Size: **4 KB**

### Virtual Memory

- Total Size: **200 KB**
- 50 Frames
- Frame Size: **4 KB**

### Demand Paging

When a process is created:

- The process is automatically divided into pages.
- Only a portion of its pages are initially loaded into RAM.
- Remaining pages stay in virtual memory until accessed.
- Missing pages trigger page faults and are loaded on demand.

---

## Shell Commands

```text
help        Show available commands
create      Create a new process
ps          Display the process table
kill        Terminate a process by PID
mem         Display RAM usage and frame allocation
vm          Display Virtual Memory usage
logs        Display the execution log
stats       Display system statistics
exit        Shut down ProcessLab
```

---

## Logging

Every execution creates a timestamped log file:

```text
bootlog_<timestamp>.txt
```

The log records:

- System boot
- Process creation
- Process execution
- Process completion
- Memory allocation
- Page faults
- Frame replacements
- System shutdown
- Final process statistics

---

## Project Structure

```text
ProcessLab/
├── CMakeLists.txt
├── Makefile
└── src/
    ├── main.cpp
    ├── OS.h/.cpp
    ├── Scheduler.h/.cpp
    ├── Memory.h/.cpp
    ├── Process.h
    ├── Logger.h
    └── tests.h/.cpp
```
---
## Build & Run

### Prerequisites

- C++17 compatible compiler
- CMake 3.15+ (recommended)
- GNU Make (Linux/macOS)

### Clone the repository - Step 1

```bash
git clone https://github.com/<your-username>/ProcessLab.git
cd ProcessLab
```
or download the zip file.

### Windows (CMake) - Step 2

```powershell
mkdir build
cd build
cmake ..
cmake --build .
.\Debug\ProcessLab.exe
```


### Linux / macOS - Step 2

```bash
mkdir build
cd build
cmake ..
make
./ProcessLab
```

Alternatively, if using the provided Makefile:

```bash
make run
```

### Without CMake - Step 2

```powershell
g++ -std=c++17 src/*.cpp -o ProcessLab
./ProcessLab
```
---

## Technologies

- C++17
- Object-Oriented Programming (OOP)
- Standard Template Library (STL)
- `std::deque`
- `std::vector`
- `std::map`
- `std::fstream`
- Header/source file modular architecture
- CMake
- Make

No external libraries are used.

---

## Learning Objectives

ProcessLab was built to better understand how operating systems manage processes and memory internally.

The project demonstrates:

- Process scheduling using Round Robin
- Process Control Blocks (PCBs)
- Paging and virtual memory
- Demand paging
- Page fault handling
- Memory allocation
- CLI application design
- Object-oriented software design
- Modular C++ development using header and source files

---

## Project Goals

ProcessLab was developed to explore the core responsibilities of an operating system kernel through simulation. It provides a hands-on implementation of concepts commonly taught in operating systems courses, including process scheduling, paging, virtual memory, and memory management.

The project emphasizes clean C++ design using object-oriented programming, modular header/source organization, and standard data structures while providing an interactive CLI for experimenting with OS behavior.
