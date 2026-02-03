#ifndef HOOK_WINDOWS_H
#define HOOK_WINDOWS_H

#ifdef _WIN32

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Callback function type cho keyboard events
typedef void (*KeyboardCallback)(const char* keyString);

// Callback function type cho window change events
typedef void (*WindowChangeCallback)(const char* appName, const char* windowTitle);

// Cấu trúc để lưu trữ thông tin hook
typedef struct {
    HHOOK keyboardHook;
    KeyboardCallback keyboardCallback;
    WindowChangeCallback windowChangeCallback;
    int isRunning;
    HWND lastWindow;
    char lastAppName[256];
    char lastWindowTitle[512];
} HookContext;

// Khởi tạo hook
HookContext* hook_init(KeyboardCallback kbCallback, WindowChangeCallback winCallback);

// Bắt đầu hook
int hook_start(HookContext* ctx);

// Dừng hook
void hook_stop(HookContext* ctx);

// Giải phóng tài nguyên
void hook_cleanup(HookContext* ctx);

// Lấy tên ứng dụng và tiêu đề cửa sổ đang active
void get_active_window_info(char* appName, size_t appNameSize, char* windowTitle, size_t windowTitleSize);

// Chuyển đổi virtual key code thành string
const char* vkey_to_string(WPARAM vkey, DWORD flags);

// Kiểm tra xem một keyString có phải là phím chức năng không
int is_function_key(const char* keyString);

// Low-level keyboard hook procedure
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

#endif // _WIN32

#endif // HOOK_WINDOWS_H
