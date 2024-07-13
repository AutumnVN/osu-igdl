#pragma once
#include <windows.h>

namespace Config {
    extern char tosuPath[MAX_PATH];

    void LoadConfig();
    void SaveConfig();
}
