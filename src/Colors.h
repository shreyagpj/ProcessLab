#pragma once

// ---------------------------------------------------------------------------
// Restrained ANSI palette — a handful of colours used only where they carry
// real meaning (prompt, errors, and RAM/VM usage health), not decoration.
// ---------------------------------------------------------------------------
namespace Color {
    constexpr const char* Reset   = "\033[0m";
    constexpr const char* Magenta = "\033[35m";
    constexpr const char* Green   = "\033[32m";
    constexpr const char* Yellow  = "\033[33m";
    constexpr const char* Red     = "\033[31m";
}
