#pragma once
#include <GL/gl3w.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_win32.h>
#include <imgui_internal.h>

#define STATUS_WINDOW_NAME "IGDL - Status"
#define SETTING_WINDOW_NAME "IGDL - Settings"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Overlay {
    extern bool showStatus;
    extern bool showSetting;
    extern int statusPinned;

    void HelpMarker(const char *desc);
    bool isShowingSettings();
    void ReverseShowSettings();
    bool isShowingStatus();
    void ShowStatus();
    void HideStatus();
    void InitOverlay(HDC hdc);
    void RenderOverlay(HDC hdc);
}
