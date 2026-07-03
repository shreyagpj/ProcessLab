#pragma once

// ---------------------------------------------------------------------------
// System-wide constants. Centralised here so Process.h and Memory.h don't
// need to include each other just to share numbers.
// ---------------------------------------------------------------------------

constexpr int PAGE_SZ    = 4;                    // KB per page/frame
constexpr int RAM_KB     = 100;
constexpr int VRAM_KB    = 200;
constexpr int RAM_FRAMES = RAM_KB  / PAGE_SZ;     // 25
constexpr int VM_FRAMES  = VRAM_KB / PAGE_SZ;     // 50

constexpr int   DEFAULT_TIME_QUANTUM = 4;
constexpr float THRASHING_THRESHOLD  = 0.05f;     // 5% page-fault rate
constexpr int   THRASHING_MIN_SAMPLE = 10;        // min CPU units before judging
