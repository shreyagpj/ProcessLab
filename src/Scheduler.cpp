#include "Scheduler.h"
#include <algorithm>

using namespace std;


Scheduler::Scheduler(Logger& logger, Memory& memory, int quantum)
    : timeQuantum(quantum), log_(logger), mem_(memory)
{
}


int Scheduler::createProcess(int sizeKB, int priority) {
    int pid = nextPid_++;
    auto proc = make_unique<Process>(pid, sizeKB, priority, systemClock);

    log_.log(systemClock, LogEvent::PROCESS_CREATED,
             "PID " + to_string(pid) +
             " | size=" + to_string(sizeKB) + "KB" +
             " | bt="   + to_string(proc->burstTime) +
             " | priority=" + to_string(priority) +
             " | pages=" + to_string(proc->totalPages));

    pcb_.emplace(pid, move(proc));
    return pid;
}


void Scheduler::admit(int pid) {
    Process* p = findProcess(pid);
    if (!p) return;

    p->state = ProcessState::READY;
    ready_.push_back(pid);

    log_.log(systemClock, LogEvent::PROCESS_ADMITTED, "PID " + to_string(pid));
}


void Scheduler::reject(int pid) {
    log_.log(systemClock, LogEvent::PROCESS_REJECTED, "PID " + to_string(pid));
    pcb_.erase(pid);
}


bool Scheduler::kill(int pid) {
    Process* p = findProcess(pid);
    if (!p || p->state == ProcessState::DONE) return false;

    // Remove from the ready queue if it's currently waiting there.
    auto it = find(ready_.begin(), ready_.end(), pid);
    if (it != ready_.end()) ready_.erase(it);

    p->state           = ProcessState::DONE;
    p->completionTime  = systemClock;
    p->turnaroundTime  = p->completionTime - p->arrivalTime;
    p->waitingTime      = max(0, p->turnaroundTime - p->burstTime);

    mem_.freeProcess(*p, log_, systemClock);
    completedOrder_.push_back(pid);

    log_.log(systemClock, LogEvent::PROCESS_KILLED, "PID " + to_string(pid) + " terminated by user");
    return true;
}


Process* Scheduler::findProcess(int pid) {
    auto it = pcb_.find(pid);
    return (it != pcb_.end()) ? it->second.get() : nullptr;
}

const Process* Scheduler::findProcess(int pid) const {
    auto it = pcb_.find(pid);
    return (it != pcb_.end()) ? it->second.get() : nullptr;
}


void Scheduler::execute(Process& p, int units) {
    p.state = ProcessState::RUNNING;
    p.dispatchCount++;

    log_.log(systemClock, LogEvent::PROCESS_DISPATCHED,
             "PID " + to_string(p.pid) +
             " | quantum=" + to_string(units) +
             " | rem_before=" + to_string(p.remaining) +
             " | dispatch#=" + to_string(p.dispatchCount));

    // The page-access cursor persists on the Process itself, so RR quanta
    // for the same process continue where the last quantum left off
    // (0 1 2 3 | 4 5 6 7 | 8 9 0 1 ...) instead of restarting at page 0
    // on every dispatch.
    for (int t = 0; t < units; ++t) {
        int pg = static_cast<int>(p.pageCursor % static_cast<size_t>(p.totalPages));

        if (!p.pageTable[pg].inRAM) {
            auto lookup = [this](int otherPid) -> Process* { return findProcess(otherPid); };
            mem_.handlePageFault(p, pg, lookup, log_, systemClock);
            totalPageFaults++;
            windowFaults_++;
        }

        p.pageCursor++;
        systemClock++;
        cpuBusyTime++;
        totalCpuUnits++;
        windowUnits_++;
    }

    p.remaining -= units;

    if (p.remaining <= 0) {
        p.remaining        = 0;
        p.state            = ProcessState::DONE;
        p.completionTime   = systemClock;
        p.turnaroundTime   = p.completionTime - p.arrivalTime;
        p.waitingTime       = p.turnaroundTime - p.burstTime;

        mem_.freeProcess(p, log_, systemClock);
        completedOrder_.push_back(p.pid);

        log_.log(systemClock, LogEvent::PROCESS_COMPLETED,
                 "PID " + to_string(p.pid) +
                 " | at="  + to_string(p.arrivalTime) +
                 " | ct="  + to_string(p.completionTime) +
                 " | tat=" + to_string(p.turnaroundTime) +
                 " | wt="  + to_string(p.waitingTime) +
                 " | bt="  + to_string(p.burstTime) +
                 " | pf="  + to_string(p.pageFaults) +
                 " | dispatches=" + to_string(p.dispatchCount));
    } else {
        // Quantum used up but work remains — back to READY until its next turn.
        p.state = ProcessState::READY;
    }

    checkThrashing();
}


void Scheduler::checkThrashing() {
    if (windowUnits_ < THRASHING_MIN_SAMPLE) return;

    float rate = static_cast<float>(windowFaults_) / static_cast<float>(windowUnits_);
    if (rate <= THRASHING_THRESHOLD) return;

    thrashingEvents++;

    string rateStr = to_string(rate * 100.0f).substr(0, 5);
    log_.log(systemClock, LogEvent::THRASHING_DETECTED,
             "rate=" + rateStr + "% > " + to_string(int(THRASHING_THRESHOLD * 100)) + "% threshold" +
             " | window_units=" + to_string(windowUnits_) +
             " | window_faults=" + to_string(windowFaults_) +
             " | scheduler unaffected (Round Robin retained)");

    windowUnits_  = 0;
    windowFaults_ = 0;
}


void Scheduler::dispatchNext() {
    if (ready_.empty()) return;

    int pid = ready_.front();
    ready_.pop_front();

    Process* p = findProcess(pid);
    if (!p) return; // shouldn't happen — PCB always outlives queue membership

    int units = min(timeQuantum, p->remaining);
    execute(*p, units);

    if (p->state != ProcessState::DONE)
        ready_.push_back(pid);
}


void Scheduler::runAll() {
    while (!ready_.empty()) dispatchNext();
}


SystemStats Scheduler::computeStats() const {
    SystemStats s;
    s.processesCreated   = static_cast<int>(pcb_.size());
    s.processesCompleted = static_cast<int>(completedOrder_.size());
    s.totalCpuUnits       = totalCpuUnits;
    s.totalPageFaults     = totalPageFaults;
    s.thrashingEvents     = thrashingEvents;

    s.cpuUtilization = (systemClock > 0)
        ? (static_cast<double>(cpuBusyTime) / systemClock) * 100.0
        : 0.0;

    s.throughput = (systemClock > 0)
        ? static_cast<double>(s.processesCompleted) / systemClock
        : 0.0;

    if (!completedOrder_.empty()) {
        double sumTAT = 0.0, sumWT = 0.0;
        for (int pid : completedOrder_) {
            const Process* p = findProcess(pid);
            if (!p) continue;
            sumTAT += p->turnaroundTime;
            sumWT  += p->waitingTime;
        }
        s.avgTurnaround = sumTAT / completedOrder_.size();
        s.avgWaiting    = sumWT  / completedOrder_.size();
    }

    return s;
}
