#pragma once

#include <string>
#include <cstdint>

void MgLogInitWrite(const char *extra = nullptr);
void MgLogEvent(const char *fmt, ...);
void MgLogCrashWrite(const char *reason, bool include_map_state = true);
std::string MgLogDirUtf8();

// Windows first-chance SEH observer
void MgLogWindowsFirstChanceException(void *exception_pointers) noexcept;

#if defined(_WIN32)
void MgLogSetWindowsVehInstalled(bool installed) noexcept;
void MgLogSetWindowsModuleBase(uintptr_t base) noexcept;
#endif
