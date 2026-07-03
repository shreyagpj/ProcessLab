#pragma once

#include <array>
#include <vector>
#include <deque>
#include <functional>
#include <string>
#include "Config.h"
#include "Process.h"
#include "Logger.h"

// A physical (RAM) or backing-store (VM) frame slot.
struct Frame {
    bool used = false;
    int  pid  = -1;
    int  pgNo = -1;

    void set(int p, int pg) { used = true;  pid = p;  pgNo = pg; }
    void clear()             { used = false; pid = -1; pgNo = -1; }
};

// ---------------------------------------------------------------------------
// Memory — owns the RAM/VM frame arrays and keeps process page tables
// consistent with them at all times.
//
// Free-frame lists (freeRam_/freeVm_) mean allocation is O(1) instead of
// scanning the whole frame array. Eviction victim selection uses a lazy
// FIFO queue (ramFifo_) of frame indices in load order: we pop from the
// front until we find one that's still occupied (indices can go stale once
// a process frees its frames directly), which again avoids a full scan.
// ---------------------------------------------------------------------------
class Memory {
public:
    Memory();

    // Allocates initial RAM/VM frames for a newly admitted process.
    // Returns false (and touches nothing) if there isn't enough VM to
    // back the process at all.
    bool loadProcess(Process& p, Logger& log, int simTime);

    // Handles a page fault for page `pgNo` of process p. `lookupProcess`
    // resolves a PID to its live PCB so that, on eviction, the victim
    // process's own page table can be updated too.
    bool handlePageFault(Process& p, int pgNo,
                          const std::function<Process*(int)>& lookupProcess,
                          Logger& log, int simTime);

    // Releases every frame (RAM and VM) owned by p and clears its page table.
    void freeProcess(Process& p, Logger& log, int simTime);

    int ramFramesFree() const { return static_cast<int>(freeRam_.size()); }
    int vmFramesFree()  const { return static_cast<int>(freeVm_.size());  }

    int ramUsed     = 0;
    int vmUsed      = 0;
    int peakRamUsed = 0;
    int peakVmUsed  = 0;

    void printRAM() const;
    void printVM() const;

private:
    std::array<Frame, RAM_FRAMES> ram_{};
    std::array<Frame, VM_FRAMES>  vm_{};

    std::vector<int> freeRam_;   // stack of unused RAM frame indices
    std::vector<int> freeVm_;    // stack of unused VM frame indices
    std::deque<int>  ramFifo_;   // occupied RAM frames in load order (eviction order)

    int  popFreeRam();
    int  popFreeVm();
    void releaseRamFrame(int frame);
    void releaseVmFrame(int frame);
};
