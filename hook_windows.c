#ifdef _WIN32

#include "hook_windows.h"
#include <psapi.h>

// Global hook context
static HookContext* g_hookContext = NULL;

// Low-level keyboard hook procedure
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= HC_ACTION && g_hookContext && g_hookContext->isRunning) {
        KBDLLHOOKSTRUCT* kbdStruct = (KBDLLHOOKSTRUCT*)lParam;
        
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            DWORD flags = 0;
            if (kbdStruct->flags & LLKF_EXTENDED) flags |= 0x100;
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000) flags |= 0x200;
            if (GetAsyncKeyState(VK_MENU) & 0x8000) flags |= 0x400;
            if (GetAsyncKeyState(VK_SHIFT) & 0x8000) flags |= 0x800;
            if (GetAsyncKeyState(VK_LWIN) & 0x8000 || GetAsyncKeyState(VK_RWIN) & 0x8000) flags |= 0x1000;
            
            const char* keyString = vkey_to_string(kbdStruct->vkCode, flags);
            
            if (g_hookContext->keyboardCallback) {
                g_hookContext->keyboardCallback(keyString);
            }
        }
        
        // Kiểm tra thay đổi cửa sổ
        HWND currentWindow = GetForegroundWindow();
        if (currentWindow != g_hookContext->lastWindow) {
            char currentAppName[256] = {0};
            char currentWindowTitle[512] = {0};
            get_active_window_info(currentAppName, sizeof(currentAppName),
                                  currentWindowTitle, sizeof(currentWindowTitle));
            
            if (g_hookContext->windowChangeCallback) {
                g_hookContext->windowChangeCallback(currentAppName, currentWindowTitle);
            }
            
            g_hookContext->lastWindow = currentWindow;
            strncpy(g_hookContext->lastAppName, currentAppName, sizeof(g_hookContext->lastAppName) - 1);
            strncpy(g_hookContext->lastWindowTitle, currentWindowTitle, sizeof(g_hookContext->lastWindowTitle) - 1);
        }
    }
    
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// Lấy thông tin cửa sổ đang active
void get_active_window_info(char* appName, size_t appNameSize, char* windowTitle, size_t windowTitleSize) {
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) {
        strncpy(appName, "Unknown", appNameSize - 1);
        strncpy(windowTitle, "Unknown", windowTitleSize - 1);
        return;
    }
    
    // Lấy tiêu đề cửa sổ
    GetWindowTextA(hwnd, windowTitle, (int)windowTitleSize);
    
    // Lấy tên ứng dụng từ process
    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);
    
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess) {
        HMODULE hMod;
        DWORD cbNeeded;
        if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
            char processPath[MAX_PATH];
            if (GetModuleFileNameExA(hProcess, hMod, processPath, sizeof(processPath))) {
                // Lấy tên file từ đường dẫn
                char* fileName = strrchr(processPath, '\\');
                if (fileName) {
                    fileName++; // Bỏ qua dấu \
                } else {
                    fileName = processPath;
                }
                strncpy(appName, fileName, appNameSize - 1);
                // Bỏ extension
                char* ext = strrchr(appName, '.');
                if (ext) {
                    *ext = '\0';
                }
            } else {
                strncpy(appName, "Unknown", appNameSize - 1);
            }
        } else {
            strncpy(appName, "Unknown", appNameSize - 1);
        }
        CloseHandle(hProcess);
    } else {
        strncpy(appName, "Unknown", appNameSize - 1);
    }
    
    if (strlen(windowTitle) == 0) {
        strncpy(windowTitle, "No Title", windowTitleSize - 1);
    }
}

// Chuyển đổi virtual key code thành string
const char* vkey_to_string(WPARAM vkey, DWORD flags) {
    static char keyString[64];
    memset(keyString, 0, sizeof(keyString));
    
    // Kiểm tra modifier keys
    if (flags & 0x200) { // Ctrl
        strcat(keyString, "Ctrl+");
    }
    if (flags & 0x400) { // Alt
        strcat(keyString, "Alt+");
    }
    if (flags & 0x800) { // Shift
        strcat(keyString, "Shift+");
    }
    if (flags & 0x1000) { // Win
        strcat(keyString, "Win+");
    }
    
    // Chuyển đổi virtual key code
    BYTE keyboardState[256];
    GetKeyboardState(keyboardState);
    
    // Xử lý các phím đặc biệt
    switch (vkey) {
        case VK_RETURN: strcat(keyString, "Return"); break;
        case VK_TAB: strcat(keyString, "Tab"); break;
        case VK_SPACE: strcat(keyString, "Space"); break;
        case VK_BACK: strcat(keyString, "Backspace"); break;
        case VK_DELETE: strcat(keyString, "Delete"); break;
        case VK_ESCAPE: strcat(keyString, "Escape"); break;
        case VK_LEFT: strcat(keyString, "Left"); break;
        case VK_RIGHT: strcat(keyString, "Right"); break;
        case VK_UP: strcat(keyString, "Up"); break;
        case VK_DOWN: strcat(keyString, "Down"); break;
        case VK_HOME: strcat(keyString, "Home"); break;
        case VK_END: strcat(keyString, "End"); break;
        case VK_PRIOR: strcat(keyString, "PageUp"); break;
        case VK_NEXT: strcat(keyString, "PageDown"); break;
        case VK_INSERT: strcat(keyString, "Insert"); break;
        case VK_F1: strcat(keyString, "F1"); break;
        case VK_F2: strcat(keyString, "F2"); break;
        case VK_F3: strcat(keyString, "F3"); break;
        case VK_F4: strcat(keyString, "F4"); break;
        case VK_F5: strcat(keyString, "F5"); break;
        case VK_F6: strcat(keyString, "F6"); break;
        case VK_F7: strcat(keyString, "F7"); break;
        case VK_F8: strcat(keyString, "F8"); break;
        case VK_F9: strcat(keyString, "F9"); break;
        case VK_F10: strcat(keyString, "F10"); break;
        case VK_F11: strcat(keyString, "F11"); break;
        case VK_F12: strcat(keyString, "F12"); break;
        default: {
            // Thử lấy ký tự từ virtual key code
            char ch = 0;
            UINT scanCode = MapVirtualKeyA(vkey, MAPVK_VK_TO_VSC);
            int result = ToAscii(vkey, scanCode, keyboardState, (LPWORD)&ch, 0);
            if (result == 1 && ch != 0) {
                // Chỉ thêm nếu không phải control character
                if (ch >= 32 && ch < 127) {
                    char charStr[2] = {ch, '\0'};
                    strcat(keyString, charStr);
                } else {
                    snprintf(keyString + strlen(keyString), sizeof(keyString) - strlen(keyString), "VK%d", vkey);
                }
            } else {
                snprintf(keyString + strlen(keyString), sizeof(keyString) - strlen(keyString), "VK%d", vkey);
            }
            break;
        }
    }
    
    return keyString;
}

// Kiểm tra xem một keyString có phải là phím chức năng không
int is_function_key(const char* keyString) {
    if (!keyString) {
        return 0;
    }
    
    // Nếu có modifier keys thì là phím chức năng
    if (strstr(keyString, "Ctrl+") || strstr(keyString, "Alt+") || strstr(keyString, "Win+")) {
        return 1;
    }
    
    // Các phím đặc biệt là phím chức năng
    if (strstr(keyString, "Return") || strstr(keyString, "Tab") || 
        strstr(keyString, "Delete") || strstr(keyString, "Backspace") ||
        strstr(keyString, "Escape") || strstr(keyString, "Space") ||
        strstr(keyString, "Left") || strstr(keyString, "Right") ||
        strstr(keyString, "Down") || strstr(keyString, "Up") ||
        strstr(keyString, "Home") || strstr(keyString, "End") ||
        strstr(keyString, "PageUp") || strstr(keyString, "PageDown") ||
        strstr(keyString, "Insert") || strstr(keyString, "F") ||
        strstr(keyString, "VK")) {
        return 1;
    }
    
    return 0;
}

// Khởi tạo hook
HookContext* hook_init(KeyboardCallback kbCallback, WindowChangeCallback winCallback) {
    HookContext* ctx = (HookContext*)malloc(sizeof(HookContext));
    if (!ctx) {
        return NULL;
    }
    
    memset(ctx, 0, sizeof(HookContext));
    ctx->keyboardCallback = kbCallback;
    ctx->windowChangeCallback = winCallback;
    ctx->isRunning = 0;
    ctx->lastWindow = NULL;
    
    return ctx;
}

// Bắt đầu hook
int hook_start(HookContext* ctx) {
    if (!ctx) {
        return -1;
    }
    
    g_hookContext = ctx;
    
    // Install low-level keyboard hook
    ctx->keyboardHook = SetWindowsHookExA(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
    
    if (!ctx->keyboardHook) {
        return -1;
    }
    
    ctx->isRunning = 1;
    
    // Lấy thông tin cửa sổ ban đầu
    get_active_window_info(ctx->lastAppName, sizeof(ctx->lastAppName),
                          ctx->lastWindowTitle, sizeof(ctx->lastWindowTitle));
    ctx->lastWindow = GetForegroundWindow();
    
    if (ctx->windowChangeCallback) {
        ctx->windowChangeCallback(ctx->lastAppName, ctx->lastWindowTitle);
    }
    
    return 0;
}

// Dừng hook
void hook_stop(HookContext* ctx) {
    if (!ctx) {
        return;
    }
    
    ctx->isRunning = 0;
    
    if (ctx->keyboardHook) {
        UnhookWindowsHookEx(ctx->keyboardHook);
        ctx->keyboardHook = NULL;
    }
    
    g_hookContext = NULL;
}

// Giải phóng tài nguyên
void hook_cleanup(HookContext* ctx) {
    if (ctx) {
        hook_stop(ctx);
        free(ctx);
    }
}

#endif // _WIN32
