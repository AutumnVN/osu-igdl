#include "overlay.h"
#include "config.h"
#include "downloader.h"
#include "hook.h"
#include "map_db.h"

bool Overlay::showStatus = false;
bool Overlay::showSetting = false;
int Overlay::statusPinned = 1;

void Overlay::InitOverlay(HDC hdc) {
    PIXELFORMATDESCRIPTOR pfd =
        {
            sizeof(PIXELFORMATDESCRIPTOR),
            1,
            PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, // Flags
            PFD_TYPE_RGBA,                                              // The kind of framebuffer. RGBA or palette.
            32,                                                         // Colordepth of the framebuffer.
            0, 0, 0, 0, 0, 0,                                           // Color bits ignored
            0,                                                          // No alpha buffer
            0,                                                          // Shift bit ignored
            0,                                                          // No accumulation buff
            0, 0, 0, 0,                                                 // Accum bits ignored
            24,                                                         // Number of bits for the depthbuffer
            8,                                                          // Number of bits for the stencilbuffer
            0,                                                          // Number of Aux buffers in the framebuffer.
            PFD_MAIN_PLANE,                                             // Main layer
            0,                                                          // Reserved
            0, 0, 0                                                     // Layer masks ignored
        };
    int pixelFormat = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pixelFormat, &pfd);
    HGLRC glContext = wglCreateContext(hdc);
    gl3wInit();

    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    auto *style = &ImGui::GetStyle();
    style->WindowRounding = 5.3f;
    style->GrabRounding = style->FrameRounding = 2.3f;
    style->ScrollbarRounding = 5.0f;
    style->FrameBorderSize = 1.0f;
    style->ItemSpacing.y = 6.5f;
    style->Colors[ImGuiCol_TextDisabled] = {0.79f, 0.79f, 0.79f, 1.00f};
    style->Colors[ImGuiCol_WindowBg] = {0.23f, 0.24f, 0.25f, 0.94f};
    style->Colors[ImGuiCol_ChildBg] = {0.23f, 0.24f, 0.25f, 0.00f};
    style->Colors[ImGuiCol_PopupBg] = {0.23f, 0.24f, 0.25f, 0.94f};
    style->Colors[ImGuiCol_Border] = {0.33f, 0.33f, 0.33f, 0.50f};
    style->Colors[ImGuiCol_BorderShadow] = {0.15f, 0.15f, 0.15f, 0.00f};
    style->Colors[ImGuiCol_FrameBg] = {0.16f, 0.16f, 0.16f, 0.54f};
    style->Colors[ImGuiCol_FrameBgHovered] = {0.45f, 0.67f, 0.99f, 0.67f};
    style->Colors[ImGuiCol_FrameBgActive] = {0.47f, 0.47f, 0.47f, 0.67f};
    style->Colors[ImGuiCol_TitleBg] = {0.04f, 0.04f, 0.04f, 1.00f};
    style->Colors[ImGuiCol_TitleBgCollapsed] = {0.16f, 0.29f, 0.48f, 1.00f};
    style->Colors[ImGuiCol_TitleBgActive] = {0.00f, 0.00f, 0.00f, 0.80f};
    style->Colors[ImGuiCol_MenuBarBg] = {0.27f, 0.28f, 0.29f, 0.80f};
    style->Colors[ImGuiCol_ScrollbarBg] = {0.27f, 0.28f, 0.29f, 0.60f};
    style->Colors[ImGuiCol_ScrollbarGrab] = {0.21f, 0.30f, 0.41f, 0.51f};
    style->Colors[ImGuiCol_ScrollbarGrabHovered] = {0.21f, 0.30f, 0.41f, 1.00f};
    style->Colors[ImGuiCol_ScrollbarGrabActive] = {0.13f, 0.19f, 0.26f, 0.91f};
    style->Colors[ImGuiCol_CheckMark] = {0.90f, 0.90f, 0.90f, 0.83f};
    style->Colors[ImGuiCol_SliderGrab] = {0.70f, 0.70f, 0.70f, 0.62f};
    style->Colors[ImGuiCol_SliderGrabActive] = {0.30f, 0.30f, 0.30f, 0.84f};
    style->Colors[ImGuiCol_Button] = {0.59f, 0.63f, 0.65f, 0.49f};
    style->Colors[ImGuiCol_ButtonHovered] = {0.35f, 0.47f, 0.61f, 1.00f};
    style->Colors[ImGuiCol_ButtonActive] = {0.13f, 0.19f, 0.26f, 1.00f};
    style->Colors[ImGuiCol_Header] = {0.33f, 0.35f, 0.36f, 0.53f};
    style->Colors[ImGuiCol_HeaderHovered] = {0.45f, 0.67f, 0.99f, 0.67f};
    style->Colors[ImGuiCol_HeaderActive] = {0.47f, 0.47f, 0.47f, 0.67f};
    style->Colors[ImGuiCol_Separator] = {0.57f, 0.54f, 0.54f, 1.00f};
    style->Colors[ImGuiCol_SeparatorHovered] = {0.31f, 0.31f, 0.31f, 1.00f};
    style->Colors[ImGuiCol_SeparatorActive] = {0.31f, 0.31f, 0.31f, 1.00f};
    style->Colors[ImGuiCol_ResizeGrip] = {1.00f, 1.00f, 1.00f, 0.85f};
    style->Colors[ImGuiCol_ResizeGripHovered] = {1.00f, 1.00f, 1.00f, 0.60f};
    style->Colors[ImGuiCol_ResizeGripActive] = {1.00f, 1.00f, 1.00f, 0.90f};
    style->Colors[ImGuiCol_PlotLines] = {0.61f, 0.61f, 0.61f, 1.00f};
    style->Colors[ImGuiCol_PlotLinesHovered] = {1.00f, 0.43f, 0.35f, 1.00f};
    style->Colors[ImGuiCol_PlotHistogram] = {0.90f, 0.70f, 0.00f, 1.00f};
    style->Colors[ImGuiCol_PlotHistogramHovered] = {1.00f, 0.60f, 0.00f, 1.00f};
    style->Colors[ImGuiCol_TextSelectedBg] = {0.18f, 0.39f, 0.79f, 0.90f};
    ImGui_ImplWin32_Init(WindowFromDC(hdc));
    ImGui_ImplOpenGL3_Init();
}

bool Overlay::isShowingSettings() {
    return showSetting;
}

void Overlay::ReverseShowSettings() {
    showSetting = !showSetting;
    if (showSetting) {
        Hook::DisablRawInputDevices();
        ImGuiIO &io = ImGui::GetIO();
        io.MouseDrawCursor = true;
        statusPinned = 0;
    } else {
        Hook::RestoreRawInputDevices();
        ImGuiIO &io = ImGui::GetIO();
        io.MouseDrawCursor = false;
        statusPinned = 1;
    }
}

bool Overlay::isShowingStatus() {
    return showStatus;
}

void Overlay::ShowStatus() {
    showStatus = true;
}

void Overlay::HideStatus() {
    showStatus = false;
}

void Overlay::RenderOverlay(HDC hdc) {
    if (!showStatus && !showSetting) return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    //===================== MY UI START =====================

    // status window
    if (showStatus || showSetting) {
        ImGuiWindowFlags statusWindowFlag = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

        if (!statusPinned) statusWindowFlag &= ~ImGuiWindowFlags_NoMove;

        ImGui::Begin(STATUS_WINDOW_NAME, nullptr, ImVec2(0, 0), 0.8f, statusWindowFlag);
        DL::SetTaskReadLock();
        auto keyIter = DL::tasks.begin();

        if (keyIter == DL::tasks.end()) {
            ImGui::Text("  Status: Idle");
        } else {
            while (keyIter != DL::tasks.end()) {
                switch (keyIter->second.dlStatus) {
                    case PARSE:
                        ImGui::Text("  Status: Parsing %c", "|/-\\"[(int)(ImGui::GetTime() / 0.1f) & 3]);
                        ImGui::Text("    Link: %s", keyIter->second.songName);
                        break;
                    case DOWNLOAD:
                        ImGui::Text("  Status: Downloading %c", "|/-\\"[(int)(ImGui::GetTime() / 0.1f) & 3]);
                        ImGui::Text("MapsetID: %lu", keyIter->second.sid);
                        ImGui::Text("  Artist: %s", keyIter->second.artist.c_str());
                        ImGui::Text("SongName: %s", keyIter->second.songName.c_str());
                        ImGui::Text("Category: %s", keyIter->second.category.c_str());
                        ImGui::Text("FileSize: %.2fMB / %.2fMB", keyIter->second.downloaded / 0x100000, keyIter->second.fileSize / 0x100000);
                        ImGui::ProgressBar(keyIter->second.percent);
                        break;
                    default:
                        ImGui::Text("  Status: Idle");
                        break;
                }

                keyIter++;
                if (keyIter != DL::tasks.end()) ImGui::Separator();
            }
        }

        ImGui::End();
        DL::UnsetTaskLock();
    }

    // setting window
    if (showSetting) {
        ImGui::Begin(SETTING_WINDOW_NAME, nullptr, ImVec2(0, 0), -1, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);
        // downloader settings
        ImGui::Checkbox("Disable in-game downloader", &DL::dontDownload);
        ImGui::Text("- OSZ Version:");
        ImGui::Combo("##oszVersion1", &DL::downloadType, DL::DlTypeName, 2);
        ImGui::SameLine();
        HelpMarker("1. <Full> is full version.\n2. <No Video> doesn't contain video.");
        ImGui::Separator();
        // run tosu automatically
        ImGui::Text("Run tosu silently with osu! automatically");
        ImGui::InputTextWithHint("##input_tosu_path", "tosu.exe path", Config::tosuPath, IM_ARRAYSIZE(Config::tosuPath));
        ImGui::Separator();
        // manual download
        ImGui::Text("[ Manual Download ]");
        ImGui::SameLine();
        HelpMarker("bid and sid can be found in urls\n1. osu.ppy.sh/b/{bid}\n2. osu.ppy.sh/s/{sid}\n3. osu.ppy.sh/beatmapsets/{sid}#osu/{bid}\n4. osu.ppy.sh/beatmaps/{bid}");
        ImGui::RadioButton("sid", &DL::manualDlType, 0);
        ImGui::SameLine();
        ImGui::RadioButton("bid", &DL::manualDlType, 1);
        ImGui::InputTextWithHint("##input_song_id", "song id", DL::manualDlId, IM_ARRAYSIZE(DL::manualDlId));
        ImGui::SameLine();

        if (ImGui::Button("Download")) DL::ManualDownload(DL::manualDlId, DL::manualDlType);

        ImGui::Separator();

        if (ImGui::ButtonEx("Stop All Task", ImVec2(-1, 40))) {
            HANDLE EndTaskThread = reinterpret_cast<HANDLE>(_beginthreadex(0, 0, [](void *pData) -> unsigned int {
                    DL::StopAllTask();
                    return 0; }, NULL, 0, NULL));

            if (EndTaskThread) CloseHandle(EndTaskThread);
        }
        ImGui::End();
    }

    //===================== MY UI END =====================

    ImGui::Render();
    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Overlay::HelpMarker(const char *desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}
