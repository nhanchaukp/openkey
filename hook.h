#ifndef HOOK_H
#define HOOK_H

#ifdef _WIN32
#include "hook_windows.h"
#else
// macOS implementation
#include <CoreGraphics/CoreGraphics.h>
#include <Carbon/Carbon.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Callback function type cho keyboard events
typedef void (*KeyboardCallback)(CGEventRef event, const char* keyString);

// Callback function type cho window change events
typedef void (*WindowChangeCallback)(const char* appName, const char* windowTitle);

// Cấu trúc để lưu trữ thông tin hook
typedef struct {
    CFMachPortRef eventTap;
    CFRunLoopSourceRef runLoopSource;
    KeyboardCallback keyboardCallback;
    WindowChangeCallback windowChangeCallback;
    int isRunning;
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

// Chuyển đổi key code thành string
const char* keycode_to_string(CGKeyCode keycode, CGEventFlags flags);

// Kiểm tra xem một keyString có phải là phím chức năng không
int is_function_key(const char* keyString);
#endif // !_WIN32

#endif // HOOK_H
