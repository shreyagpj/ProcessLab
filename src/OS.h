#pragma once

#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include "Logger.h"
#include "Memory.h"
#include "Scheduler.h"

// ---------------------------------------------------------------------------
// OS — the shell / top-level driver. Owns the Logger and Memory, and the
// Scheduler (which in turn owns the PCB table). All process creation goes
// through Scheduler::createProcess so there is only ever one place a PID
// is minted.
//
// A background thread keeps the CPU ticking (dispatching Round Robin
// quanta) on a real-time cadence even while the shell is just sitting idle
// at the "os>" prompt waiting for input — not only when a command runs.
// `mutex_` guards every access to `sched`/`mem` so the ticker thread and
// the command handlers on the main thread never race.
// ---------------------------------------------------------------------------
class OS {
    bool on = false;

    Logger    logger;
    Memory    mem;
    Scheduler sched;

    std::thread        ticker_;
    std::mutex          mutex_;
    std::atomic<bool>   running_{false};

    void boot();
    void shell();
    void tickLoop();   // runs on the background thread

    void cmdHelp();
    void cmdCreate();
    void cmdPS();
    void cmdKill();
    void cmdMem();
    void cmdVM();
    void cmdLogs();
    void cmdScheduler();
    void cmdStats();
    void cmdExit();

public:
    OS() : sched(logger, mem) {}
    void start();
};
