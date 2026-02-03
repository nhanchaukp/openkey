#include "hook.h"

// Biến global để lưu thông tin cửa sổ trước đó
static char lastAppName[256] = {0};
static char lastWindowTitle[512] = {0};

// Callback function được gọi khi có keyboard event
CGEventRef keyboardEventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon) {
    HookContext* ctx = (HookContext*)refcon;
    
    if (!ctx || !ctx->isRunning) {
        return NULL;
    }
    
    // Chỉ xử lý key down events
    if (type == kCGEventKeyDown) {
        CGKeyCode keycode = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
        CGEventFlags flags = CGEventGetFlags(event);
        
        const char* keyString = keycode_to_string(keycode, flags);
        
        if (ctx->keyboardCallback) {
            ctx->keyboardCallback(event, keyString);
        }
    }
    
    // Kiểm tra thay đổi cửa sổ active
    char currentAppName[256] = {0};
    char currentWindowTitle[512] = {0};
    get_active_window_info(currentAppName, sizeof(currentAppName), 
                          currentWindowTitle, sizeof(currentWindowTitle));
    
    // So sánh với cửa sổ trước đó
    if (strcmp(currentAppName, lastAppName) != 0 || 
        strcmp(currentWindowTitle, lastWindowTitle) != 0) {
        
        if (ctx->windowChangeCallback) {
            ctx->windowChangeCallback(currentAppName, currentWindowTitle);
        }
        
        strncpy(lastAppName, currentAppName, sizeof(lastAppName) - 1);
        strncpy(lastWindowTitle, currentWindowTitle, sizeof(lastWindowTitle) - 1);
    }
    
    return event;
}

// Lấy thông tin cửa sổ đang active
void get_active_window_info(char* appName, size_t appNameSize, char* windowTitle, size_t windowTitleSize) {
    // Lấy PID của ứng dụng đang active
    pid_t pid = 0;
    ProcessSerialNumber psn = {0, kNoProcess};
    
    if (GetFrontProcess(&psn) == noErr) {
        GetProcessPID(&psn, &pid);
    }
    
    if (pid == 0) {
        strncpy(appName, "Unknown", appNameSize - 1);
        strncpy(windowTitle, "Unknown", windowTitleSize - 1);
        return;
    }
    
    // Lấy tên ứng dụng từ PID
    CFStringRef appNameRef = NULL;
    OSStatus status = CopyProcessName(&psn, &appNameRef);
    
    if (status == noErr && appNameRef) {
        CFStringGetCString(appNameRef, appName, appNameSize, kCFStringEncodingUTF8);
        CFRelease(appNameRef);
    } else {
        strncpy(appName, "Unknown", appNameSize - 1);
    }
    
    // Lấy tiêu đề cửa sổ bằng Accessibility API
    AXUIElementRef appElement = AXUIElementCreateApplication(pid);
    if (appElement) {
        CFTypeRef windowTitleRef = NULL;
        AXUIElementRef windowRef = NULL;
        
        // Lấy cửa sổ đang focused
        if (AXUIElementCopyAttributeValue(appElement, kAXFocusedWindowAttribute, (CFTypeRef*)&windowRef) == kAXErrorSuccess) {
            if (windowRef) {
                // Lấy tiêu đề cửa sổ
                if (AXUIElementCopyAttributeValue(windowRef, kAXTitleAttribute, &windowTitleRef) == kAXErrorSuccess) {
                    if (windowTitleRef && CFGetTypeID(windowTitleRef) == CFStringGetTypeID()) {
                        CFStringGetCString((CFStringRef)windowTitleRef, windowTitle, windowTitleSize, kCFStringEncodingUTF8);
                        CFRelease(windowTitleRef);
                    }
                }
                CFRelease(windowRef);
            }
        }
        
        // Nếu không lấy được tiêu đề, thử lấy cửa sổ đầu tiên
        if (strlen(windowTitle) == 0) {
            CFArrayRef windowList = NULL;
            if (AXUIElementCopyAttributeValues(appElement, kAXWindowsAttribute, 0, 1, &windowList) == kAXErrorSuccess) {
                if (windowList && CFArrayGetCount(windowList) > 0) {
                    windowRef = (AXUIElementRef)CFArrayGetValueAtIndex(windowList, 0);
                    if (windowRef) {
                        if (AXUIElementCopyAttributeValue(windowRef, kAXTitleAttribute, &windowTitleRef) == kAXErrorSuccess) {
                            if (windowTitleRef && CFGetTypeID(windowTitleRef) == CFStringGetTypeID()) {
                                CFStringGetCString((CFStringRef)windowTitleRef, windowTitle, windowTitleSize, kCFStringEncodingUTF8);
                                CFRelease(windowTitleRef);
                            }
                        }
                    }
                    CFRelease(windowList);
                }
            }
        }
        
        CFRelease(appElement);
    }
    
    if (strlen(windowTitle) == 0) {
        strncpy(windowTitle, "No Title", windowTitleSize - 1);
    }
}

// Chuyển đổi keycode thành string
const char* keycode_to_string(CGKeyCode keycode, CGEventFlags flags) {
    static char keyString[64];
    memset(keyString, 0, sizeof(keyString));
    
    // Kiểm tra modifier keys
    if (flags & kCGEventFlagMaskControl) {
        strcat(keyString, "Ctrl+");
    }
    if (flags & kCGEventFlagMaskAlternate) {
        strcat(keyString, "Alt+");
    }
    if (flags & kCGEventFlagMaskShift) {
        strcat(keyString, "Shift+");
    }
    if (flags & kCGEventFlagMaskCommand) {
        strcat(keyString, "Cmd+");
    }
    
    // Chuyển đổi keycode thành ký tự
    TISInputSourceRef keyboardLayout = TISCopyCurrentKeyboardInputSource();
    CFDataRef layoutData = (CFDataRef)TISGetInputSourceProperty(keyboardLayout, kTISPropertyUnicodeKeyLayoutData);
    
    if (layoutData) {
        const UCKeyboardLayout* keyboardLayoutPtr = (const UCKeyboardLayout*)CFDataGetBytePtr(layoutData);
        UInt32 deadKeyState = 0;
        UniCharCount maxStringLength = 4;
        UniCharCount actualStringLength = 0;
        UniChar unicodeString[4];
        
        OSStatus status = UCKeyTranslate(keyboardLayoutPtr,
                                        keycode,
                                        kUCKeyActionDisplay,
                                        0,
                                        LMGetKbdType(),
                                        kUCKeyTranslateNoDeadKeysBit,
                                        &deadKeyState,
                                        maxStringLength,
                                        &actualStringLength,
                                        unicodeString);
        
        if (status == noErr && actualStringLength > 0) {
            char charBuffer[8];
            CFStringRef stringRef = CFStringCreateWithCharacters(NULL, unicodeString, actualStringLength);
            CFStringGetCString(stringRef, charBuffer, sizeof(charBuffer), kCFStringEncodingUTF8);
            strcat(keyString, charBuffer);
            CFRelease(stringRef);
        } else {
            // Fallback: sử dụng keycode trực tiếp cho các phím đặc biệt
            switch (keycode) {
                case 36: strcat(keyString, "Return"); break;
                case 48: strcat(keyString, "Tab"); break;
                case 49: strcat(keyString, "Space"); break;
                case 51: strcat(keyString, "Delete"); break;
                case 53: strcat(keyString, "Escape"); break;
                case 123: strcat(keyString, "Left"); break;
                case 124: strcat(keyString, "Right"); break;
                case 125: strcat(keyString, "Down"); break;
                case 126: strcat(keyString, "Up"); break;
                default:
                    snprintf(keyString + strlen(keyString), sizeof(keyString) - strlen(keyString), "Key%d", keycode);
                    break;
            }
        }
    }
    
    if (keyboardLayout) {
        CFRelease(keyboardLayout);
    }
    
    return keyString;
}

// Kiểm tra xem một keyString có phải là phím chức năng không
int is_function_key(const char* keyString) {
    if (!keyString) {
        return 0;
    }
    
    // Nếu có modifier keys (Ctrl, Alt, Cmd) thì là phím chức năng
    if (strstr(keyString, "Ctrl+") || strstr(keyString, "Alt+") || strstr(keyString, "Cmd+")) {
        return 1;
    }
    
    // Các phím đặc biệt là phím chức năng
    if (strstr(keyString, "Return") || strstr(keyString, "Tab") || 
        strstr(keyString, "Delete") || strstr(keyString, "Escape") ||
        strstr(keyString, "Left") || strstr(keyString, "Right") ||
        strstr(keyString, "Down") || strstr(keyString, "Up") ||
        strstr(keyString, "Space") || strstr(keyString, "Key")) {
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
    
    return ctx;
}

// Bắt đầu hook
int hook_start(HookContext* ctx) {
    if (!ctx) {
        return -1;
    }
    
    // Tạo event tap để bắt keyboard events
    CGEventMask eventMask = (1 << kCGEventKeyDown) | (1 << kCGEventKeyUp);
    
    ctx->eventTap = CGEventTapCreate(
        kCGHIDEventTap,
        kCGHeadInsertEventTap,
        kCGEventTapOptionDefault,
        eventMask,
        keyboardEventCallback,
        ctx
    );
    
    if (!ctx->eventTap) {
        fprintf(stderr, "Failed to create event tap. Make sure accessibility permissions are granted.\n");
        return -1;
    }
    
    // Tạo run loop source
    ctx->runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, ctx->eventTap, 0);
    if (!ctx->runLoopSource) {
        CFRelease(ctx->eventTap);
        return -1;
    }
    
    // Thêm vào run loop
    CFRunLoopAddSource(CFRunLoopGetCurrent(), ctx->runLoopSource, kCFRunLoopDefaultMode);
    
    ctx->isRunning = 1;
    
    // Lấy thông tin cửa sổ ban đầu
    get_active_window_info(lastAppName, sizeof(lastAppName), 
                          lastWindowTitle, sizeof(lastWindowTitle));
    if (ctx->windowChangeCallback) {
        ctx->windowChangeCallback(lastAppName, lastWindowTitle);
    }
    
    return 0;
}

// Dừng hook
void hook_stop(HookContext* ctx) {
    if (!ctx) {
        return;
    }
    
    ctx->isRunning = 0;
    
    if (ctx->runLoopSource) {
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(), ctx->runLoopSource, kCFRunLoopDefaultMode);
        CFRelease(ctx->runLoopSource);
        ctx->runLoopSource = NULL;
    }
    
    if (ctx->eventTap) {
        CFRelease(ctx->eventTap);
        ctx->eventTap = NULL;
    }
}

// Giải phóng tài nguyên
void hook_cleanup(HookContext* ctx) {
    if (ctx) {
        hook_stop(ctx);
        free(ctx);
    }
}
