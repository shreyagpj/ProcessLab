#pragma once

#include "Scheduler.h"
#include "Memory.h"
#include "Logger.h"

// Pre-loads a fixed set of test processes (sizes/priorities are constant so
// results are reproducible) into the scheduler before the shell starts.
void runTests(Scheduler& sched, Memory& mem, Logger& logger);
