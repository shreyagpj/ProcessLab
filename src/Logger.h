#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>

// ---------------------------------------------------------------------------
// Every log event the kernel can emit. Using an enum class (rather than raw
// strings scattered through the code) means typos become compile errors and
// call sites stay self-documenting.
// ---------------------------------------------------------------------------
enum class LogEvent {
    BOOT,
    PROCESS_CREATED,
    PROCESS_ADMITTED,
    PROCESS_REJECTED,
    PROCESS_DISPATCHED,
    PAGE_FAULT,
    PAGE_EVICTION,
    PROCESS_COMPLETED,
    PROCESS_KILLED,
    THRASHING_DETECTED,
    MEM_LOAD,
    MEM_FREE,
    SYSTEM,
    REPORT
};

inline std::string toString(LogEvent e) {
    switch (e) {
        case LogEvent::BOOT:                return "BOOT";
        case LogEvent::PROCESS_CREATED:     return "PROCESS_CREATED";
        case LogEvent::PROCESS_ADMITTED:    return "PROCESS_ADMITTED";
        case LogEvent::PROCESS_REJECTED:    return "PROCESS_REJECTED";
        case LogEvent::PROCESS_DISPATCHED:  return "PROCESS_DISPATCHED";
        case LogEvent::PAGE_FAULT:          return "PAGE_FAULT";
        case LogEvent::PAGE_EVICTION:       return "PAGE_EVICTION";
        case LogEvent::PROCESS_COMPLETED:   return "PROCESS_COMPLETED";
        case LogEvent::PROCESS_KILLED:      return "PROCESS_KILLED";
        case LogEvent::THRASHING_DETECTED:  return "THRASHING_DETECTED";
        case LogEvent::MEM_LOAD:            return "MEM_LOAD";
        case LogEvent::MEM_FREE:            return "MEM_FREE";
        case LogEvent::SYSTEM:              return "SYSTEM";
        case LogEvent::REPORT:              return "REPORT";
        default:                            return "UNKNOWN";
    }
}

// ---------------------------------------------------------------------------
// Logger — every line is timestamped with the simulator's own systemClock
// (not wall-clock time), since the whole point of the simulation is that
// time only advances when the CPU actually does work.
// ---------------------------------------------------------------------------
class Logger {
public:
    void init(const std::string& fname) {
        filename_ = fname;
        file_.open(fname);
    }

    void log(int simTime, LogEvent event, const std::string& msg) {
        std::ostringstream line;
        line << "[t=" << std::setw(5) << std::setfill('0') << simTime << "] "
             << std::setfill(' ')
             << std::left << std::setw(20) << toString(event)
             << msg;

        if (file_.is_open())
            file_ << line.str() << "\n" << std::flush;
    }

    void dump() const {
        std::ifstream in(filename_);
        if (!in.is_open()) {
            std::cout << "  (log file not found)\n";
            return;
        }
        std::cout << "\n--- " << filename_ << " ---\n";
        std::string line;
        while (std::getline(in, line))
            std::cout << "  " << line << "\n";
        std::cout << "---\n\n";
    }

    ~Logger() {
        if (file_.is_open()) file_.close();
    }

private:
    std::string   filename_;
    std::ofstream file_;
};
