#include "config.h"
#include "dllhijack.h"
#include "hook.h"
#include "logger.h"
#include "shlwapi.h"
#include <windows.h>
#pragma comment(lib, "shlwapi")

PROCESS_INFORMATION TosuProcess = {0};

VOID DllHijack(HMODULE hMod) {
    TCHAR tszDllPath[MAX_PATH] = {0};
    GetModuleFileName(hMod, tszDllPath, MAX_PATH);
    wcscat_s(tszDllPath, L".1");
    SuperDllHijack(L"libEGL.dll", tszDllPath);
}

void StartTosu() {
    if (Config::tosuPath[0] == '\0') return;

    STARTUPINFO si = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    WCHAR szTosuPath[MAX_PATH];
    mbstowcs(szTosuPath, Config::tosuPath, MAX_PATH);

    WCHAR szTosuDir[MAX_PATH] = {0};
    wcscpy_s(szTosuDir, szTosuPath);
    WCHAR *pLastSlash = wcsrchr(szTosuDir, L'\\');
    if (pLastSlash) {
        *(pLastSlash + 1) = L'\0';
    }

    if (!CreateProcessW(NULL, szTosuPath, NULL, NULL, FALSE, 0, NULL, szTosuDir, &si, &TosuProcess)) {
        logger::WriteLog("[-] Start tosu failed");
    }
}

VOID KillTosu() {
    if (TosuProcess.hProcess) {
        TerminateProcess(TosuProcess.hProcess, 0);
        CloseHandle(TosuProcess.hProcess);
        CloseHandle(TosuProcess.hThread);
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            DllHijack(hModule);
            logger::WriteLog("======================= Inject success =======================");
            Config::LoadConfig();
            Hook::InitHook();
            StartTosu();
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
        case DLL_PROCESS_DETACH:
            Config::SaveConfig();
            Hook::UninitHook();
            logger::WriteLog("===================== See you next time ======================");
            KillTosu();
            break;
    }
    return TRUE;
}
