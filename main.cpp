#include <tchar.h>
#include <windows.h>
#include "d3d_setup.h"
#include "commands.h"
#include "dearimgui/imgui.h"
#include "dearimgui/imgui_impl_dx11.h"
#include "dearimgui/imgui_impl_win32.h"

extern LRESULT WINAPI WndProc(HWND, UINT, WPARAM, LPARAM);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

extern ID3D11Device* g_pd3dDevice;
extern ID3D11DeviceContext* g_pd3dDeviceContext;
extern IDXGISwapChain* g_pSwapChain;
extern ID3D11RenderTargetView* g_mainRenderTargetView;

extern bool CreateDeviceD3D(HWND);
extern void CleanupDeviceD3D();
extern void CreateRenderTarget();
extern void CleanupRenderTarget();
void RenderAnsiColoredText(const char* text) {
    ImVec4 defaultColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
    ImVec4 currentColor = defaultColor;

    const char* p = text;
    const char* segmentStart = p;

    while (*p) {
        if (*p == '\x1b') {
            if (p > segmentStart) {
                ImGui::PushStyleColor(ImGuiCol_Text, currentColor);
                ImGui::TextUnformatted(segmentStart, p);
                ImGui::PopStyleColor();
                ImGui::SameLine(0, 0);
            }

            p++;
            if (*p != '[') {
                segmentStart = p;
                continue;
            }
            p++;

            int code = 0;
            bool parsedCode = false;
            while (*p && *p != 'm') {
                if (*p >= '0' && *p <= '9') {
                    code = code * 10 + (*p - '0');
                    parsedCode = true;
                }
                p++;
            }

            if (*p == 'm' && parsedCode) {
                switch (code) {
                case 0:  currentColor = defaultColor; break;
                case 30: currentColor = ImVec4(0, 0, 0, 1); break;
                case 31: currentColor = ImVec4(0.8f, 0, 0, 1); break;
                case 32: currentColor = ImVec4(0, 0.8f, 0, 1); break;
                case 33: currentColor = ImVec4(0.8f, 0.8f, 0, 1); break;
                case 34: currentColor = ImVec4(0, 0, 0.8f, 1); break;
                case 35: currentColor = ImVec4(0.8f, 0, 0.8f, 1); break;
                case 36: currentColor = ImVec4(0, 0.8f, 0.8f, 1); break;
                case 37: currentColor = ImVec4(0.8f, 0.8f, 0.8f, 1); break;
                case 90: currentColor = ImVec4(0.5f, 0.5f, 0.5f, 1); break;
                case 91: currentColor = ImVec4(1, 0, 0, 1); break;
                case 92: currentColor = ImVec4(0, 1, 0, 1); break;
                case 93: currentColor = ImVec4(1, 1, 0, 1); break;
                case 94: currentColor = ImVec4(0, 0, 1, 1); break;
                case 95: currentColor = ImVec4(1, 0, 1, 1); break;
                case 96: currentColor = ImVec4(0, 1, 1, 1); break;
                case 97: currentColor = ImVec4(1, 1, 1, 1); break;
                default: break;
                }
            }
            p++;   
            segmentStart = p;
        }
        else {
            p++;
        }
    }

    if (p > segmentStart) {
        ImGui::PushStyleColor(ImGuiCol_Text, currentColor);
        ImGui::TextUnformatted(segmentStart, p);
        ImGui::PopStyleColor();
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    WNDCLASSEX wc = {
        sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0, 0,
        GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
        _T("ImGuiExample"), NULL
    };
    RegisterClassEx(&wc);

    HWND hwnd = CreateWindow(
        wc.lpszClassName, _T("Modern CMD UI"),
        WS_POPUP | WS_VISIBLE,
        100, 100, 1280, 800,
        NULL, NULL, wc.hInstance, NULL
    );
    if (!hwnd) {
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return -1;
    }

    if (!CreateDeviceD3D(hwnd)) {
        DestroyWindow(hwnd);
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.FramePadding = ImVec2(8, 6);
    style.ItemSpacing = ImVec2(10, 8);
    style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.45f, 0.75f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.55f, 0.85f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.10f, 0.35f, 0.65f, 1.00f);

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    bool done = false;
    static char inputCmd[512] = {};
    static ImGuiTextBuffer textBuffer;
    bool focusInput = true;

    while (!done) {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoCollapse;

        ImGui::Begin("Modern CMD Interface", nullptr, flags);

        ImGui::SameLine(viewport->Size.x - style.FramePadding.x - 70);
        if (ImGui::Button("Exit", ImVec2(60, 0))) {
            done = true;
        }

        ImGui::Separator();

        float inputHeight = ImGui::GetFrameHeightWithSpacing() + style.ItemSpacing.y * 2;
        ImGui::BeginChild("OutputRegion", ImVec2(0, -inputHeight), false, ImGuiWindowFlags_HorizontalScrollbar);

        RenderAnsiColoredText(textBuffer.begin());

        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();

        ImGui::Separator();

        ImGui::PushItemWidth(-80);
        if (focusInput) {
            ImGui::SetKeyboardFocusHere();
            focusInput = false;
        }
        if (ImGui::InputText("##Input", inputCmd, sizeof(inputCmd), ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (inputCmd[0] != 0) {
                char cmdOutput[8192] = {};
                run_command(inputCmd, cmdOutput, sizeof(cmdOutput));
                textBuffer.appendf("> %s\n%s\n", inputCmd, cmdOutput);
                inputCmd[0] = 0;
            }
            focusInput = true;
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::Button("Run", ImVec2(60, 0))) {
            if (inputCmd[0] != 0) {
                char cmdOutput[8192] = {};
                run_command(inputCmd, cmdOutput, sizeof(cmdOutput));
                textBuffer.appendf("> %s\n%s\n", inputCmd, cmdOutput);
                inputCmd[0] = 0;
            }
            focusInput = true;
        }

        ImGui::End();

        ImGui::Render();
        const float clear_color[4] = { 0.06f, 0.06f, 0.06f, 1.00f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static bool focusInput = false;

    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_LBUTTONDOWN:
        PostMessage(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, lParam);
        return 0;

    case WM_NCHITTEST: {
        LRESULT hit = DefWindowProc(hWnd, msg, wParam, lParam);
        if (hit == HTCLIENT) {
            if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse)
                return HTCLIENT;
            return HTCAPTION;
        }
        return hit;
    }

    case WM_SETFOCUS:
        focusInput = true;
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}