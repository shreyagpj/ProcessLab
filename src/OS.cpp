#include "OS.h"
#include "tests.h"
#include "Colors.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <ctime>
#include <chrono>
#include <algorithm>

using namespace std;
using namespace Color;

namespace
{
    constexpr auto TICK_INTERVAL = chrono::milliseconds(700);
}

// Wall-clock time is only used for the boot banner and the log filename —
// everything inside the simulation itself is timestamped with the
// simulated systemClock instead.
static string nowStr()
{
    time_t t = time(nullptr);
    string s = ctime(&t);
    s.pop_back();
    return s;
}

static string logFilename()
{
    time_t t = time(nullptr);
    string s = ctime(&t);
    s.pop_back();
    for (auto &c : s)
        if (c == ' ' || c == ':')
            c = '_';
    return "bootlog_" + s + ".txt";
}

void OS::boot()
{
    string fname = logFilename();
    logger.init(fname);

    cout << "\n";
    cout << " \n";
    cout << Magenta << "             PROCESS LAB  v1.0          \n"
         << Reset;
    cout << "           " << nowStr() << "  \n";
    cout << "  \n\n";

    cout << "  Starting.....\n\n";

    logger.log(sched.systemClock, LogEvent::BOOT, "system power on | " + nowStr());
    logger.log(sched.systemClock, LogEvent::BOOT,
               "RAM: " + to_string(RAM_KB) + "KB | " +
                   to_string(RAM_FRAMES) + " frames @ " + to_string(PAGE_SZ) + "KB each");
    logger.log(sched.systemClock, LogEvent::BOOT,
               "VM:  " + to_string(VRAM_KB) + "KB | " + to_string(VM_FRAMES) + " frames");
    logger.log(sched.systemClock, LogEvent::BOOT,
               "scheduler: Round Robin (quantum=" + to_string(sched.timeQuantum) + ")");

    const int bootSizes[3] = {8, 12, 6};
    for (int i = 0; i < 3; i++)
    {
        int pid = sched.createProcess(bootSizes[i], 0);
        Process *p = sched.findProcess(pid);

        logger.log(sched.systemClock, LogEvent::BOOT,
                   "init process PID=" + to_string(pid) + " size=" + to_string(bootSizes[i]) + "KB");

        if (mem.loadProcess(*p, logger, sched.systemClock))
            sched.admit(pid);
        else
            sched.reject(pid);
    }
    sched.runAll();

    logger.log(sched.systemClock, LogEvent::BOOT, "boot complete | log -> " + fname);

    // auto-load test processes into queue before shell starts
    runTests(sched, mem, logger);

    cout << "  Log file : " << fname << "\n";
    cout << "  Ready.     Type  help  for commands.\n";
}

void OS::cmdHelp()
{
    cout << "\n";
    cout << "  Available Commands:\n";
    cout << "  ------------------\n";
    cout << "  help               Show all commands\n";
    cout << "  create             Create a process\n";
    cout << "  ps                 Show process table\n";
    cout << "  kill <pid>         Terminate a process\n";
    cout << "  mem                Show RAM usage\n";
    cout << "  vm                 Show virtual memory\n";
    cout << "  logs               Show recent logs\n";
    cout << "  scheduler          Show current scheduler / change quantum\n";
    cout << "  stats              Show system statistics\n";
    cout << "  exit               Shutdown OS\n";
    cout << "\n";
}

void OS::cmdCreate()
{
    int size;

    cout << "\n  process size  (KB, 4-80): ";
    cin >> size;
    if (cin.fail())
    {
        cin.clear();
        cin.ignore(1000, '\n');
        return;
    }
    if (size < 4)
        size = 4;
    if (size > 80)
        size = 80;

    int prio = rand() % 10;

    lock_guard<mutex> lock(mutex_);

    int pid = sched.createProcess(size, prio);
    Process *p = sched.findProcess(pid);

    if (!mem.loadProcess(*p, logger, sched.systemClock))
    {
        cout << Red << "  error: not enough memory.\n"
             << Reset;
        sched.reject(pid);
        return;
    }

    sched.admit(pid);

    cout << "  created PID " << pid
         << " | " << size << " KB"
         << " | bt=" << p->burstTime
         << " | priority=" << prio
         << " | pages=" << p->totalPages << "\n";
}

void OS::cmdPS()
{
    lock_guard<mutex> lock(mutex_);

    cout << "\n";
    cout << "  " << left
         << setw(5) << "PID"
         << setw(9) << "SIZE"
         << setw(6) << "BT"
         << setw(6) << "REM"
         << setw(6) << "PRI"
         << setw(8) << "STATE"
         << setw(6) << "DISP"
         << "\n";
    cout << "  " << string(46, '-') << "\n";

    if (sched.readyQueue().empty())
    {
        cout << "  (queue empty)\n";
    }
    else
    {
        for (int pid : sched.readyQueue())
        {
            const Process *p = sched.findProcess(pid);
            if (!p)
                continue;
            cout << "  " << left
                 << setw(5) << p->pid
                 << setw(9) << (to_string(p->size) + " KB")
                 << setw(6) << p->burstTime
                 << setw(6) << p->remaining
                 << setw(6) << p->priority
                 << setw(8) << toString(p->state)
                 << setw(6) << p->dispatchCount
                 << "\n";
        }
    }

    if (!sched.completedPids().empty())
    {
        cout << "\n  Completed:\n";
        cout << "  " << left
             << setw(5) << "PID"
             << setw(9) << "SIZE"
             << setw(6) << "BT"
             << setw(6) << "AT"
             << setw(6) << "CT"
             << setw(6) << "TAT"
             << setw(6) << "WT"
             << setw(5) << "PF"
             << setw(6) << "DISP"
             << "\n";
        cout << "  " << string(55, '-') << "\n";

        for (int pid : sched.completedPids())
        {
            const Process *p = sched.findProcess(pid);
            if (!p)
                continue;
            cout << "  " << left
                 << setw(5) << p->pid
                 << setw(9) << (to_string(p->size) + " KB")
                 << setw(6) << p->burstTime
                 << setw(6) << p->arrivalTime
                 << setw(6) << p->completionTime
                 << setw(6) << p->turnaroundTime
                 << setw(6) << p->waitingTime
                 << setw(5) << p->pageFaults
                 << setw(6) << p->dispatchCount
                 << "\n";
        }
    }

    cout << "\n  scheduler: Round Robin (quantum=" << sched.timeQuantum << ")"
         << "  |  clk: " << sched.systemClock << "\n\n";
}

void OS::cmdKill()
{
    int pid;
    cout << "\n  PID to kill: ";
    cin >> pid;
    if (cin.fail())
    {
        cin.clear();
        cin.ignore(1000, '\n');
        return;
    }

    lock_guard<mutex> lock(mutex_);

    if (sched.kill(pid))
        cout << "  PID " << pid << " killed.\n";
    else
        cout << Red << "  PID " << pid << " not found (or already done).\n"
             << Reset;
}

void OS::cmdMem()
{
    lock_guard<mutex> lock(mutex_);
    mem.printRAM();
}

void OS::cmdVM()
{
    lock_guard<mutex> lock(mutex_);
    mem.printVM();
}

void OS::cmdLogs()
{
    lock_guard<mutex> lock(mutex_);
    logger.dump();
}

void OS::cmdScheduler()
{
    {
        lock_guard<mutex> lock(mutex_);
        cout << "\n";
        cout << "  Scheduler         : Round Robin (fixed)\n";
        cout << "  Time quantum      : " << sched.timeQuantum << " units\n";
        cout << "  Clock             : " << sched.systemClock << " units\n";
        cout << "  Queue size        : " << sched.readyQueue().size() << "\n";
        cout << "  Completed         : " << sched.completedPids().size() << "\n\n";
    }

    cout << "  KernelForge always schedules with Round Robin.\n";
    cout << "  Change time quantum? (current=" << sched.timeQuantum << ", 0 = keep): ";

    int q;
    cin >> q;
    if (cin.fail())
    {
        cin.clear();
        cin.ignore(1000, '\n');
        return;
    }

    if (q > 0)
    {
        lock_guard<mutex> lock(mutex_);
        sched.timeQuantum = q;
        cout << "  -> quantum set to " << q << "\n";
    }
}

void OS::cmdStats()
{
    lock_guard<mutex> lock(mutex_);
    SystemStats s = sched.computeStats();

    cout << "\n";
    cout << "  System Statistics\n";
    cout << "  -----------------\n";
    cout << fixed << setprecision(2);
    cout << "  Clock                    : " << sched.systemClock << " units\n";
    cout << "  Processes created        : " << s.processesCreated << "\n";
    cout << "  Processes completed      : " << s.processesCompleted << "\n";
    cout << "  Queue size               : " << sched.readyQueue().size() << "\n";
    cout << "  Total CPU units executed : " << s.totalCpuUnits << "\n";
    cout << "  Total page faults        : " << s.totalPageFaults << "\n";
    cout << "  CPU utilization          : " << s.cpuUtilization << "%\n";
    cout << "  Throughput               : " << s.throughput << " processes/unit\n";
    cout << "  Avg turnaround time      : " << s.avgTurnaround << "\n";
    cout << "  Avg waiting time         : " << s.avgWaiting << "\n";
    cout << "  Thrashing events         : " << s.thrashingEvents << "\n";
    cout << "  RAM used                 : " << mem.ramUsed * PAGE_SZ << "/" << RAM_KB << " KB\n";
    cout << "  VM  used                 : " << mem.vmUsed * PAGE_SZ << "/" << VRAM_KB << " KB\n";
    cout << "  Scheduler                : Round Robin (quantum=" << sched.timeQuantum << ")\n\n";
}

void OS::cmdExit()
{
    lock_guard<mutex> lock(mutex_);

    cout << "\n  Shutting down...\n";
    logger.log(sched.systemClock, LogEvent::SYSTEM, "shutdown | " + nowStr());

    SystemStats s = sched.computeStats();
    if (s.processesCompleted > 0)
    {
        logger.log(sched.systemClock, LogEvent::REPORT, "processes=" + to_string(s.processesCompleted));
        logger.log(sched.systemClock, LogEvent::REPORT, "avg_TAT=" + to_string(s.avgTurnaround));
        logger.log(sched.systemClock, LogEvent::REPORT, "avg_WT=" + to_string(s.avgWaiting));
        logger.log(sched.systemClock, LogEvent::REPORT, "total_page_faults=" + to_string(s.totalPageFaults));
        logger.log(sched.systemClock, LogEvent::REPORT, "cpu_utilization=" + to_string(s.cpuUtilization) + "%");
        logger.log(sched.systemClock, LogEvent::REPORT, "throughput=" + to_string(s.throughput));
        logger.log(sched.systemClock, LogEvent::REPORT, "thrashing_events=" + to_string(s.thrashingEvents));
        logger.log(sched.systemClock, LogEvent::REPORT, "total_clk=" + to_string(sched.systemClock));
        logger.log(sched.systemClock, LogEvent::REPORT, "final_scheduler=Round Robin");
    }

    cout << "  Log saved. Goodbye.\n\n";
    on = false;
}

// Runs on the background thread for the whole lifetime of the shell.
// Every TICK_INTERVAL it dispatches one Round Robin quantum for whichever
// process is at the front of the ready queue — so the CPU keeps working
// even while the user is just sitting at the "os>" prompt, not only when
// a command happens to run. `mutex_` keeps this safe against the main
// thread handling a command at the same moment.
//
// This runs silently — it only writes to the log file (via Scheduler's
// normal PROCESS_DISPATCHED/PROCESS_COMPLETED logging, plus a periodic
// idle heartbeat when the queue is empty). Nothing is printed to the
// terminal here; use `ps`, `stats`, or `logs` to see what happened.
void OS::tickLoop()
{
    int idleStreak = 0;

    while (running_)
    {
        this_thread::sleep_for(TICK_INTERVAL);
        if (!running_)
            break;

        lock_guard<mutex> lock(mutex_);

        if (sched.readyQueue().empty())
        {
            idleStreak++;
            if (idleStreak == 1 || idleStreak % 5 == 0)
                logger.log(sched.systemClock, LogEvent::SYSTEM, "CPU idle - ready queue empty");
            continue;
        }
        idleStreak = 0;

        sched.dispatchNext(); // execute() already logs dispatch/fault/completion events
    }
}

void OS::shell()
{
    string cmd;
    while (on)
    {
        cout << "\n"
             << Magenta << "os> " << Reset << flush;
        if (!(cin >> cmd))
            break;

        for (auto &c : cmd)
            c = static_cast<char>(tolower(static_cast<unsigned char>(c)));

        if (cmd == "help")
            cmdHelp();
        else if (cmd == "create")
            cmdCreate();
        else if (cmd == "ps")
            cmdPS();
        else if (cmd == "kill")
            cmdKill();
        else if (cmd == "mem")
            cmdMem();
        else if (cmd == "vm")
            cmdVM();
        else if (cmd == "logs")
            cmdLogs();
        else if (cmd == "scheduler")
            cmdScheduler();
        else if (cmd == "stats")
            cmdStats();
        else if (cmd == "exit")
        {
            cmdExit();
            break;
        }
        else
            cout << Red << "  unknown command -- type help\n"
                 << Reset;
    }
}

void OS::start()
{
    boot();
    on = true;
    running_ = true;
    ticker_ = thread(&OS::tickLoop, this);

    shell();

    running_ = false;
    if (ticker_.joinable())
        ticker_.join();
}
