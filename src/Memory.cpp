#include "Memory.h"
#include "Colors.h"
#include <iostream>

using namespace std;
using namespace Color;

namespace {
    // Green when comfortable, yellow when getting full, red when critical.
    const char* usageColor(int usedFrames, int totalFrames) {
        if (totalFrames <= 0) return Green;
        int pct = (usedFrames * 100) / totalFrames;
        if (pct >= 90) return Red;
        if (pct >= 70) return Yellow;
        return Green;
    }
}


Memory::Memory() {
    freeRam_.reserve(RAM_FRAMES);
    freeVm_.reserve(VM_FRAMES);
    // Fill back-to-front so popping from the back hands out frame 0 first —
    // purely cosmetic, keeps early runs looking like the old scan-based code.
    for (int i = RAM_FRAMES - 1; i >= 0; --i) freeRam_.push_back(i);
    for (int i = VM_FRAMES  - 1; i >= 0; --i) freeVm_.push_back(i);
}


int Memory::popFreeRam() {
    if (freeRam_.empty()) return -1;
    int f = freeRam_.back();
    freeRam_.pop_back();
    return f;
}

int Memory::popFreeVm() {
    if (freeVm_.empty()) return -1;
    int f = freeVm_.back();
    freeVm_.pop_back();
    return f;
}

void Memory::releaseRamFrame(int frame) {
    ram_[frame].clear();
    freeRam_.push_back(frame);
    ramUsed--;
}

void Memory::releaseVmFrame(int frame) {
    vm_[frame].clear();
    freeVm_.push_back(frame);
    vmUsed--;
}


bool Memory::loadProcess(Process& p, Logger& log, int simTime) {
    int forRAM = p.totalPages / 10;
    if (forRAM < 1) forRAM = 1;
    if (forRAM > p.totalPages) forRAM = p.totalPages;
    int forVM = p.totalPages - forRAM;

    if (ramFramesFree() < forRAM) forRAM = ramFramesFree();
    forVM = p.totalPages - forRAM;

    if (vmFramesFree() < forVM) {
        log.log(simTime, LogEvent::MEM_LOAD,
                "not enough VM for PID " + to_string(p.pid) +
                " (need " + to_string(forVM) + ", have " + to_string(vmFramesFree()) + ")");
        return false;
    }

    int loaded = 0;
    for (int pg = 0; pg < forRAM; ++pg) {
        int frame = popFreeRam();
        ram_[frame].set(p.pid, pg);
        ramFifo_.push_back(frame);

        p.pageTable[pg].ramFrame = frame;
        p.pageTable[pg].vmFrame  = -1;
        p.pageTable[pg].inRAM    = true;

        ramUsed++;
        loaded++;
    }

    int vloaded = 0;
    for (int i = 0; i < forVM; ++i) {
        int pgNo  = forRAM + i;
        int frame = popFreeVm();
        vm_[frame].set(p.pid, pgNo);

        p.pageTable[pgNo].vmFrame  = frame;
        p.pageTable[pgNo].ramFrame = -1;
        p.pageTable[pgNo].inRAM    = false;

        vmUsed++;
        vloaded++;
    }

    p.pagesInRAM = loaded;

    if (ramUsed > peakRamUsed) peakRamUsed = ramUsed;
    if (vmUsed  > peakVmUsed)  peakVmUsed  = vmUsed;

    log.log(simTime, LogEvent::MEM_LOAD,
            "PID " + to_string(p.pid) +
            " | RAM=" + to_string(loaded)  + " frames" +
            " | VM="  + to_string(vloaded) + " frames");

    return true;
}


bool Memory::handlePageFault(Process& p, int pgNo,
                              const function<Process*(int)>& lookupProcess,
                              Logger& log, int simTime) {
    p.pageFaults++;
    log.log(simTime, LogEvent::PAGE_FAULT,
            "PID " + to_string(p.pid) + " fault on page " + to_string(pgNo));

    Page& faultingPage = p.pageTable[pgNo];

    bool allocatedFromFreeList = true;
    int  frame = popFreeRam();

    if (frame == -1) {
        allocatedFromFreeList = false;

        // No free RAM frame — evict the oldest occupied one (lazy FIFO scan
        // that skips any stale/already-freed entries left in the queue).
        int victimFrame = -1;
        while (!ramFifo_.empty()) {
            int candidate = ramFifo_.front();
            ramFifo_.pop_front();
            if (ram_[candidate].used) { victimFrame = candidate; break; }
        }

        if (victimFrame == -1) {
            // Nothing evictable — genuinely out of memory.
            return false;
        }

        Frame& victim = ram_[victimFrame];
        Process* victimProc = (victim.pid == p.pid) ? &p : lookupProcess(victim.pid);

        int vmFrame = popFreeVm();
        if (vmFrame == -1) {
            // No VM space to swap the victim out to — put it back and fail.
            ram_[victimFrame].set(victim.pid, victim.pgNo);
            ramFifo_.push_back(victimFrame);
            return false;
        }

        if (victimProc) {
            Page& victimPage = victimProc->pageTable[victim.pgNo];
            vm_[vmFrame].set(victim.pid, victim.pgNo);

            victimPage.vmFrame  = vmFrame;
            victimPage.ramFrame = -1;
            victimPage.inRAM    = false;
            victimProc->pagesInRAM--;

            vmUsed++;

            log.log(simTime, LogEvent::PAGE_EVICTION,
                    "PID " + to_string(victim.pid) + " page " + to_string(victim.pgNo) +
                    " evicted from RAM frame " + to_string(victimFrame) +
                    " -> VM frame " + to_string(vmFrame) +
                    " (to load PID " + to_string(p.pid) + " page " + to_string(pgNo) + ")");
        }

        frame = victimFrame; // frame is now free for the faulting page
    }

    // If the faulting page was previously swapped out, reclaim its VM slot.
    if (faultingPage.vmFrame != -1) {
        releaseVmFrame(faultingPage.vmFrame);
    }

    ram_[frame].set(p.pid, pgNo);
    ramFifo_.push_back(frame);

    faultingPage.ramFrame = frame;
    faultingPage.vmFrame  = -1;
    faultingPage.inRAM    = true;

    p.pagesInRAM++;

    // Only a genuinely free frame increases how many RAM frames are in use.
    // On the eviction path the frame's *owner* changed but the total count
    // of occupied RAM frames did not, so ramUsed must stay put — otherwise
    // it climbs without bound every time a page gets evicted and reloaded.
    if (allocatedFromFreeList) {
        ramUsed++;
        if (ramUsed > peakRamUsed) peakRamUsed = ramUsed;
    }

    return true;
}


void Memory::freeProcess(Process& p, Logger& log, int simTime) {
    for (auto& pg : p.pageTable) {
        if (pg.inRAM && pg.ramFrame != -1) {
            releaseRamFrame(pg.ramFrame);
            pg.ramFrame = -1;
            pg.inRAM    = false;
        }
        if (pg.vmFrame != -1) {
            releaseVmFrame(pg.vmFrame);
            pg.vmFrame = -1;
        }
    }
    p.pagesInRAM = 0;

    log.log(simTime, LogEvent::MEM_FREE, "freed frames of PID " + to_string(p.pid));
}


void Memory::printRAM() const {
    int rPct = (RAM_FRAMES > 0) ? (ramUsed * 100) / RAM_FRAMES : 0;
    const char* c = usageColor(ramUsed, RAM_FRAMES);

    cout << "\n";
    cout << "  RAM  " << c << "[";
    for (int i = 0; i < 25; i++) cout << (i < rPct / 4 ? "#" : ".");
    cout << "]" << Reset << "  " << ramUsed << "/" << RAM_FRAMES
         << " frames  (" << ramUsed * PAGE_SZ << "/" << RAM_KB << " KB)"
         << "  peak: " << peakRamUsed << " frames\n\n";

    cout << "  Frame map (RAM):\n";
    cout << "  ";
    for (int i = 0; i < RAM_FRAMES; i++) {
        if (ram_[i].used)
            cout << "[P" << ram_[i].pid << "]";
        else
            cout << "[ . ]";
        if ((i + 1) % 5 == 0) cout << "\n  ";
    }
    cout << "\n";
}


void Memory::printVM() const {
    int vPct = (VM_FRAMES > 0) ? (vmUsed * 100) / VM_FRAMES : 0;
    const char* c = usageColor(vmUsed, VM_FRAMES);

    cout << "\n";
    cout << "  VM   " << c << "[";
    for (int i = 0; i < 25; i++) cout << (i < vPct / 4 ? "#" : ".");
    cout << "]" << Reset << "  " << vmUsed << "/" << VM_FRAMES
         << " frames  (" << vmUsed * PAGE_SZ << "/" << VRAM_KB << " KB)"
         << "  peak: " << peakVmUsed << " frames\n\n";

    cout << "  Frame map (VM):\n";
    cout << "  ";
    for (int i = 0; i < VM_FRAMES; i++) {
        if (vm_[i].used)
            cout << "[P" << vm_[i].pid << "]";
        else
            cout << "[ . ]";
        if ((i + 1) % 5 == 0) cout << "\n  ";
    }
    cout << "\n";
}
