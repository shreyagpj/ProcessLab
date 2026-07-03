#pragma once

#include <deque>
#include <vector>
#include <map>
#include <memory>
#include <string>
#include "Config.h"
#include "Process.h"
#include "Logger.h"
#include "Memory.h"

// Aggregate system-wide statistics, computed on demand from the PCB table.
struct SystemStats {
    int    processesCreated   = 0;
    int    processesCompleted = 0;
    int    totalCpuUnits      = 0;
    int    totalPageFaults    = 0;
    double cpuUtilization     = 0.0; // % of systemClock spent executing
    double throughput         = 0.0; // completed processes per CPU unit
    double avgTurnaround      = 0.0;
    double avgWaiting         = 0.0;
    int    thrashingEvents    = 0;
};

// ---------------------------------------------------------------------------
// Scheduler — Round Robin only.
//
// This class is also the process manager: it owns the single, central PCB
// table (pid -> unique_ptr<Process>). Every other structure (the ready
// queue, the completed list) stores PIDs, never Process copies, so a state
// change made during execute() is visible to ps/stats/kill immediately and
// there is exactly one Process object alive per PID for its whole lifetime.
// ---------------------------------------------------------------------------
class Scheduler {
public:
    Scheduler(Logger& logger, Memory& memory, int quantum = DEFAULT_TIME_QUANTUM);

    // --- Process lifecycle -------------------------------------------------
    // Creates a new PCB (state NEW) and returns its PID. Does not touch
    // memory or the ready queue — caller is expected to load() it into
    // Memory and then admit() it (see OS::cmdCreate for the full flow).
    int createProcess(int sizeKB, int priority);

    // Moves a NEW process to READY and pushes its PID onto the ready queue.
    void admit(int pid);

    // Removes a process that failed memory admission (never ran).
    void reject(int pid);

    // Terminates an in-flight process on user request (shell `kill`).
    bool kill(int pid);

    // --- Dispatch ------------------------------------------------------
    void dispatchNext();              // run exactly one RR quantum
    void runAll();                    // drain the ready queue completely

    // --- Access --------------------------------------------------------
    Process*       findProcess(int pid);
    const Process* findProcess(int pid) const;

    const std::deque<int>&  readyQueue()    const { return ready_; }
    const std::vector<int>& completedPids() const { return completedOrder_; }
    const std::map<int, std::unique_ptr<Process>>& pcbTable() const { return pcb_; }

    SystemStats computeStats() const;

    int  systemClock  = 0;   // advances once per executed CPU unit
    int  cpuBusyTime  = 0;   // total CPU units actually executed
    int  timeQuantum;
    int  totalCpuUnits   = 0;
    int  totalPageFaults = 0;
    int  thrashingEvents = 0;

private:
    std::map<int, std::unique_ptr<Process>> pcb_;   // the one true PCB table
    std::deque<int>   ready_;                        // PIDs only
    std::vector<int>  completedOrder_;                // PIDs only

    Logger& log_;
    Memory& mem_;

    int nextPid_ = 1;

    // Windowed page-fault-rate counters used purely for thrashing detection;
    // reset after each detection so we're always looking at a fresh burst.
    int windowUnits_ = 0;
    int windowFaults_ = 0;

    void execute(Process& p, int units);
    void checkThrashing();
};
