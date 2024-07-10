#include "config.h"
#include "dllhijack.h"
#include "hook.h"
#include "logger.h"
#include "shlwapi.h"
#pragma comment(lib, "shlwapi")

VOID DllHijack(HMODULE hMod) {
    TCHAR tszDllPath[MAX_PATH] = {0};
    LPWSTR tszDllNamePtr = nullptr;
    GetModuleFileName(hMod, tszDllPath, MAX_PATH);
    tszDllNamePtr = PathFindFileName(tszDllPath);
    wcscat_s(tszDllPath, L".1");
    SuperDllHijack(tszDllNamePtr, tszDllPath);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            DllHijack(hModule);
            logger::WriteLog("======================= Inject success =======================");
            Config::LoadConfig();
            Hook::InitHook();
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
        case DLL_PROCESS_DETACH:
            Config::SaveConfig();
            Hook::UninitHook();
            logger::WriteLog("===================== See you next time ======================");
            break;
    }
    return TRUE;
}
