#include "tests.h"
#include <iostream>

using namespace std;


void runTests(Scheduler& sched, Memory& mem, Logger& logger) {
    cout << "  [TEST] Loading test processes...\n";
    logger.log(sched.systemClock, LogEvent::SYSTEM, "inserting test processes");

    const int sizes[5]      = { 20, 40, 16, 32, 24 };
    const int priorities[5] = {  3,  1,  7,  2,  5 };

    for (int i = 0; i < 5; ++i) {
        int sz   = sizes[i];
        int prio = priorities[i];

        int pid = sched.createProcess(sz, prio);
        Process* p = sched.findProcess(pid);

        if (!mem.loadProcess(*p, logger, sched.systemClock)) {
            cout << "  [TEST] PID " << pid << " rejected — not enough memory\n";
            sched.reject(pid);
            continue;
        }

        sched.admit(pid);

        cout << "  [TEST] PID " << pid
             << " | size=" << sz << "KB"
             << " | bt=" << p->burstTime
             << " | priority=" << prio
             << " | pages=" << p->totalPages << "\n";
    }

    cout << "  [TEST] Done. " << sched.readyQueue().size() << " processes in queue.\n\n";
    logger.log(sched.systemClock, LogEvent::SYSTEM,
               "test load complete | queue size=" + to_string(sched.readyQueue().size()));
}
