#pragma once

#include <string>
#include <vector>
#include "Config.h"

enum class ProcessState { NEW, READY, RUNNING, DONE };

inline std::string toString(ProcessState s) {
    switch (s) {
        case ProcessState::NEW:     return "NEW";
        case ProcessState::READY:   return "READY";
        case ProcessState::RUNNING: return "RUNNING";
        case ProcessState::DONE:    return "DONE";
        default:                    return "?";
    }
}

// ---------------------------------------------------------------------------
// A single page-table entry. A page is in exactly one of three states:
//   - not resident anywhere yet (inRAM=false, ramFrame=-1, vmFrame=-1)
//   - resident in RAM            (inRAM=true,  ramFrame>=0, vmFrame=-1)
//   - swapped out to VM          (inRAM=false, ramFrame=-1, vmFrame>=0)
// Keeping ramFrame/vmFrame separate (instead of one overloaded "frameNo")
// is what lets Memory keep the page table perfectly consistent with the
// actual frame arrays on every load/evict.
// ---------------------------------------------------------------------------
struct Page {
    int  pgNo;
    int  ramFrame = -1;
    int  vmFrame  = -1;
    bool inRAM    = false;

    explicit Page(int n) : pgNo(n) {}
};

// ---------------------------------------------------------------------------
// Process — the single Process Control Block (PCB) for a task. The kernel
// keeps exactly ONE Process object per PID (owned by Scheduler's PCB table,
// see Scheduler.h). Everything else (ready queue, done list, UI) refers to
// the process by PID or by reference — never by copy — so a state change
// made anywhere is visible everywhere.
// ---------------------------------------------------------------------------
class Process {
public:
    // Identity / static properties (set once at creation).
    int pid;
    int size;         // KB
    int burstTime;    // total CPU units needed
    int priority;      // informational only — RR does not use this
    int arrivalTime;

    // Scheduling / accounting state (mutated over the process lifetime).
    int remaining;
    int completionTime  = 0;
    int turnaroundTime  = 0;
    int waitingTime      = 0;
    int pageFaults       = 0;
    int dispatchCount    = 0;   // number of Round Robin quanta received
    std::size_t pageCursor = 0; // per-process RR page-access cursor (persists across quanta)

    ProcessState state = ProcessState::NEW;

    // Memory-management state.
    int totalPages  = 0;
    int pagesInRAM  = 0;
    std::vector<Page> pageTable;

    Process(int id, int sizeKB, int prio, int arrival)
        : pid(id), size(sizeKB), priority(prio), arrivalTime(arrival)
    {
        burstTime = sizeKB * 2;
        if (burstTime < 1) burstTime = 1;
        remaining = burstTime;

        // Ceiling division so a partially filled final page still counts.
        totalPages = (sizeKB + PAGE_SZ - 1) / PAGE_SZ;
        if (totalPages < 1) totalPages = 1;

        pageTable.reserve(totalPages);
        for (int i = 0; i < totalPages; ++i)
            pageTable.emplace_back(i);
    }
};
