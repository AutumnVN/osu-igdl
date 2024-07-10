#include "hook.h"
#include "downloader.h"
#include "map_db.h"
#include "overlay.h"
#include "utils.h"
#include <MinHook.h>
#include <ShlObj.h>
#include <string>

using namespace std;

UINT Hook::raw_devices_count = -1;
RAWINPUTDEVICE *Hook::raw_devices = NULL;
_SwapBuffers Hook::OriSwapBuffers = nullptr;
_SwapBuffers Hook::BakOriSwapBuffers = nullptr;
_ShellExcuteExW Hook::OriShellExecuteExW = nullptr;
_ShellExcuteExW Hook::BakShellExecuteExW = nullptr;
HHOOK Hook::msgHook = nullptr;
HWND Hook::hwnd = NULL;

BOOL __stdcall InitPlugin(HDC hdc) {
    // hook msg for ingame overlay
    Hook::hwnd = WindowFromDC(hdc);
    DWORD tid = GetCurrentThreadId();
    CreateThread(NULL, NULL, MsgHookThread, &tid, 0, NULL);
    // init overlay
    Overlay::InitOverlay(hdc);
    // init sid database
    HANDLE InitDatabaseThread = reinterpret_cast<HANDLE>(_beginthreadex(0, 0, [](void *pData) -> unsigned int {
            DB::InitDataBase("osu!.db");
            return 0; }, NULL, 0, NULL));
    if (InitDatabaseThread) CloseHandle(InitDatabaseThread);
    // rehook swapbuffer
    Hook::ReHookSwapBuffers();
    logger::WriteLog("[+] Init Plugin Done");
    return Hook::OriSwapBuffers(hdc);
}

BOOL __stdcall DetourSwapBuffers(HDC hdc) {
    Overlay::RenderOverlay(hdc);
    return Hook::OriSwapBuffers(hdc);
}

BOOL CallOriShellExecuteExW(const char *lpFile) {
    LPSHELLEXECUTEINFOW pExecinfo = new _SHELLEXECUTEINFOW;
    ZeroMemory(pExecinfo, sizeof(_SHELLEXECUTEINFOW));
    pExecinfo->cbSize = sizeof(_SHELLEXECUTEINFOW);
    pExecinfo->lpFile = char2wchar(lpFile);
    pExecinfo->nShow = SW_SHOWNORMAL;
    pExecinfo->fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOASYNC | SEE_MASK_NOCLOSEPROCESS; // same with osu
    auto res = Hook::OriShellExecuteExW(pExecinfo);
    delete pExecinfo->lpFile;
    delete pExecinfo;
    return res;
}

// thread that perform download
DWORD WINAPI DownloadThread(LPVOID lpParam) {
    char *url = (char *)lpParam;
    int res;
    char tmpPath[MAX_PATH];
    UINT32 sid = 0;
    string songName = "";
    string fileName = "";
    int category = 0;
    DL::SetTaskReadLock();
    // already in process, skip
    if (DL::tasks.count(url) > 0) {
        DL::UnsetTaskLock();
        logger::WriteLogFormat("[*] Reject duplicated request: %s", url);
        return 0;
    }
    DL::UnsetTaskLock();
    // user already has this map
    if (DB::mapExistFast(url)) {
        CallOriShellExecuteExW(url);
        goto finish;
    }
    Overlay::ShowStatus();
    DL::SetTaskWriteLock();
    DL::tasks[url].dlStatus = PARSE;
    DL::tasks[url].songName = url;
    DL::UnsetTaskLock();
    // parse sid, song name and category
    res = DL::ParseInfo(url, sid, songName, category);
    if (res || DL::tasks.count(url) <= 0 || DB::mapExist(sid)) {
        CallOriShellExecuteExW(url);
        goto finish;
    }
    DL::SetTaskWriteLock();
    DL::tasks[url].dlStatus = DOWNLOAD;
    DL::tasks[url].songName = songName;
    DL::tasks[url].sid = sid;
    DL::tasks[url].category = (Category)category;
    DL::UnsetTaskLock();
    // download map
    GetTempPathA(MAX_PATH, tmpPath);
    fileName.append(tmpPath);
    fileName.append(to_string(sid));
    fileName.append(".osz");
    res = DL::Download(fileName, sid, url);
    if (res) {
        CallOriShellExecuteExW(url);
        goto finish;
    }

    // open map
    ShellExecuteA(0, NULL, fileName.c_str(), NULL, NULL, SW_HIDE);

    // insert map into database
    DB::insertSid(sid);

finish:
    DL::RemoveTaskInfo(url);
    DL::SetTaskReadLock();
    if (DL::tasks.empty()) {
        Overlay::HideStatus();
    }
    DL::UnsetTaskLock();
    delete url;
    return 0;
}

BOOL __stdcall DetourShellExecuteExW(LPSHELLEXECUTEINFOW pExecinfo) {
    int findPos1, findPos2, findPos3, findPos4;
    char *lpFile;
    HANDLE hThread;
    if (DL::dontDownload) goto call_api;

    findPos1 = wstring(pExecinfo->lpFile).find(L"osu.ppy.sh/b/");
    findPos2 = wstring(pExecinfo->lpFile).find(L"osu.ppy.sh/s/");
    findPos3 = wstring(pExecinfo->lpFile).find(L"osu.ppy.sh/beatmapsets/");
    findPos4 = wstring(pExecinfo->lpFile).find(L"osu.ppy.sh/beatmaps/");
    // not the target
    if ((findPos1 == -1) && (findPos2 == -1) && (findPos3 == -1) && (findPos4 == -1)) goto call_api;

    lpFile = wchar2char(pExecinfo->lpFile);
    hThread = CreateThread(NULL, NULL, DownloadThread, lpFile, 0, NULL);
    if (hThread) {
        CloseHandle(hThread);
    } else {
        goto call_api;
    }

    return true;

call_api:
    return Hook::OriShellExecuteExW(pExecinfo);
}

LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode != HC_ACTION) return CallNextHookEx(Hook::msgHook, nCode, wParam, lParam);

    MSG *msg = (MSG *)lParam;
    // the message has been removed from the queue
    if (wParam == PM_REMOVE) {
        // hotkey Alt+M press
        if (msg->message == WM_SYSKEYDOWN) {
            int contextCode = msg->lParam >> 29 & 1;
            int transitionState = msg->lParam >> 31 & 1;
            if (!transitionState && contextCode && (msg->wParam == 'M')) {
                Overlay::ReverseShowSettings();
            }
        }
        if (Overlay::isShowingSettings()) {
            ImGui_ImplWin32_WndProcHandler(msg->hwnd, msg->message, msg->wParam, msg->lParam);
        }
    }
    // block keyboard and mouse message to osu while setting is showing
    if (Overlay::isShowingSettings()) {
        if (msg->message == WM_CHAR) {
            msg->message = WM_NULL;
            return 1;
        }
        if ((WM_MOUSEFIRST <= msg->message && msg->message <= WM_MOUSELAST) || (msg->message == WM_NCHITTEST) || (msg->message == WM_SETCURSOR)) {
            msg->message = WM_NULL;
            return 1;
        }
    }
    return CallNextHookEx(Hook::msgHook, nCode, wParam, lParam);
}

DWORD WINAPI MsgHookThread(LPVOID lpParam) {
    DWORD Tid = *(DWORD *)lpParam;
    Hook::msgHook = SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc, GetModuleHandle(NULL), Tid);

    if (!Hook::msgHook) {
        logger::WriteLog("[-] Set message hook failed");
        return 1;
    } else {
        logger::WriteLog("[+] Set message hook success");
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}

// backup/disable/restore rawinput
int Hook::BackupRawInputDevices() {
    raw_devices_count = -1;
    GetRegisteredRawInputDevices(NULL, &raw_devices_count, sizeof(RAWINPUTDEVICE));

    if (raw_devices_count == -1) {
        logger::WriteLog("[-] Can't get registered raw input devices");
        return 1;
    }

    logger::WriteLogFormat("[*] Find %d raw input", raw_devices_count);

    if (!raw_devices_count) return 0;

    if (raw_devices) delete raw_devices;

    raw_devices = new RAWINPUTDEVICE[raw_devices_count];
    if (GetRegisteredRawInputDevices(raw_devices, &raw_devices_count, sizeof(RAWINPUTDEVICE)) == -1) {
        logger::WriteLog("[-] Backup registered raw input devices fail");
        return 2;
    }

    return 0;
}

int Hook::DisablRawInputDevices() {
    BackupRawInputDevices();

    if (raw_devices_count == -1) return 1;
    if (!raw_devices_count) return 0;

    logger::WriteLogFormat("[*] Try to disable %d raw input devices", raw_devices_count);

    for (UINT i = 0; i < raw_devices_count; i++) {
        RAWINPUTDEVICE p;
        memcpy(&p, raw_devices + i, sizeof(RAWINPUTDEVICE));
        p.dwFlags = RIDEV_REMOVE;
        p.hwndTarget = NULL;
        if (RegisterRawInputDevices(&p, 1, sizeof(RAWINPUTDEVICE)) == FALSE) {
            logger::WriteLogFormat("[-] Unregister index %d raw input device fail", i);
            logger::WriteLogFormat("[*] info: %p, %p, %p, %p", p.hwndTarget, p.dwFlags, p.usUsage, p.usUsagePage);
        }
    }

    UINT now_count = -1;
    GetRegisteredRawInputDevices(NULL, &now_count, sizeof(RAWINPUTDEVICE));

    if (now_count == -1) {
        logger::WriteLog("[-] Can't get registered raw input devices");
        return 2;
    }

    if (now_count) {
        logger::WriteLogFormat("[*] now has %d raw input", now_count);
        return 3;
    }

    return 0;
}

int Hook::RestoreRawInputDevices() {
    if (raw_devices_count == -1) return 1;
    if (!raw_devices_count) return 0;

    logger::WriteLogFormat("[*] Try to enable %d raw input devices", raw_devices_count);

    if (RegisterRawInputDevices(raw_devices, raw_devices_count, sizeof(RAWINPUTDEVICE)) == FALSE) {
        logger::WriteLogFormat("[-] Restore %d raw input device fail", raw_devices_count);
    }

    UINT now_count = -1;
    GetRegisteredRawInputDevices(NULL, &now_count, sizeof(RAWINPUTDEVICE));

    if (now_count == -1) {
        logger::WriteLog("[-] Can't get registered raw input devices");
        return 2;
    }

    if (now_count) {
        logger::WriteLogFormat("[*] now has %d raw input", now_count);
        return 3;
    }

    return 0;
}

int Hook::ReHookSwapBuffers() {
    if (MH_RemoveHook(BakOriSwapBuffers) != MH_OK) {
        logger::WriteLog("[-] ReHook: Remove old hook fail");
        return 1;
    }

    if (MH_CreateHookApiEx(L"gdi32", "SwapBuffers", DetourSwapBuffers, (LPVOID *)&OriSwapBuffers, (LPVOID *)&BakOriSwapBuffers) != MH_OK) {
        logger::WriteLog("[-] ReHook: Create new hook fail");
        return 2;
    }

    if (MH_CreateHookApiEx(L"shell32", "ShellExecuteExW", DetourShellExecuteExW, (LPVOID *)&OriShellExecuteExW, (LPVOID *)&BakShellExecuteExW) != MH_OK) {
        logger::WriteLog("[-] ReHook: Create new hook fail");
        return 2;
    }

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
        logger::WriteLog("[-] ReHook: Enable new hook fail");
        return 3;
    }

    logger::WriteLog("[+] Rehook gdi32.SwapBuffers success");
    return 0;
}

int Hook::InitHook() {
    if (MH_Initialize() != MH_OK) {
        logger::WriteLog("[-] MinHook: Initialize fail");
        return 1;
    }

    if (MH_CreateHookApiEx(L"gdi32", "SwapBuffers", InitPlugin, (LPVOID *)&OriSwapBuffers, (LPVOID *)&BakOriSwapBuffers) != MH_OK) {
        logger::WriteLog("[-] MinHook: Can't hook gdi32.SwapBuffers");
        return 1;
    }

    if (MH_EnableHook(BakOriSwapBuffers) != MH_OK) {
        logger::WriteLog("[-] MinHook: Enable hook fail");
        return 1;
    }

    logger::WriteLog("[+] Install hook success");
    return 0;
}

int Hook::UninitHook() {
    if (Hook::msgHook) {
        UnhookWindowsHookEx(Hook::msgHook);
    }

    if (MH_Uninitialize() != MH_OK) {
        logger::WriteLog("[-] MinHook uninitialize fail");
        return 1;
    }

    return 0;
}
