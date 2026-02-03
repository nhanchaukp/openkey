#include "hook.h"
#include <time.h>
#include <string.h>
#include <stdarg.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <signal.h>
#endif

static HookContext* g_hookContext = NULL;
static FILE* g_logFile = NULL;  // File log

// Biến static để track trạng thái dòng hiện tại
static int lineStarted = 0;  // Đã bắt đầu dòng mới chưa
static int hasContent = 0;   // Đã có nội dung trên dòng hiện tại chưa
static int lineStartedFile = 0;  // Đã bắt đầu dòng mới trong file chưa
static int hasContentFile = 0;   // Đã có nội dung trên dòng hiện tại trong file chưa

// Hàm helper để in vào cả stdout và file
static void log_output(const char* format, ...) {
    va_list args;
    
    // In ra stdout
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    fflush(stdout);
    
    // In ra file nếu đã mở
    if (g_logFile) {
        va_start(args, format);
        vfprintf(g_logFile, format, args);
        va_end(args);
        fflush(g_logFile);
    }
}

// Callback khi có keyboard event
#ifdef _WIN32
void on_keyboard_event(const char* keyString) {
#else
void on_keyboard_event(CGEventRef event, const char* keyString) {
#endif
    if (!keyString) {
        return;
    }
    
    // Kiểm tra xem có phải là Enter không
    int isEnter = (strcmp(keyString, "Return") == 0);
    
    // Nếu là Enter, chỉ xuống dòng và reset, không in gì
    if (isEnter) {
        if (lineStarted && hasContent) {
            printf("\n");
            fflush(stdout);
        }
        if (lineStartedFile && hasContentFile && g_logFile) {
            fprintf(g_logFile, "\n");
            fflush(g_logFile);
        }
        lineStarted = 1;
        hasContent = 0;
        lineStartedFile = 1;
        hasContentFile = 0;
        return;
    }
    
    // Nếu chưa bắt đầu dòng, in timestamp
    if (!lineStarted) {
        time_t now = time(NULL);
        char timeStr[64];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
        log_output("[%s] ", timeStr);
        lineStarted = 1;
        hasContent = 0;
    }
    
    // Tương tự cho file
    if (!lineStartedFile && g_logFile) {
        time_t now = time(NULL);
        char timeStr[64];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
        fprintf(g_logFile, "[%s] ", timeStr);
        lineStartedFile = 1;
        hasContentFile = 0;
    }
    
    // Kiểm tra xem có phải là phím chức năng không
    int isFunctionKey = is_function_key(keyString);
    
    // Thêm khoảng cách trước phím chức năng nếu đã có nội dung trước đó
    if (isFunctionKey && hasContent) {
        printf(" ");
        fflush(stdout);
    }
    if (isFunctionKey && hasContentFile && g_logFile) {
        fprintf(g_logFile, " ");
    }
    
    // In phím
    printf("%s", keyString);
    hasContent = 1;
    fflush(stdout);
    
    if (g_logFile) {
        fprintf(g_logFile, "%s", keyString);
        hasContentFile = 1;
        fflush(g_logFile);
    }
}

// Callback khi cửa sổ active thay đổi
void on_window_change(const char* appName, const char* windowTitle) {
    // Nếu đang có dòng chưa kết thúc, xuống dòng trước
    if (lineStarted && hasContent) {
        log_output("\n");
    }
    if (lineStartedFile && hasContentFile && g_logFile) {
        fprintf(g_logFile, "\n");
        fflush(g_logFile);
    }
    
    time_t now = time(NULL);
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    log_output("[%s] Active Window - App: %s | Title: %s\n", timeStr, appName, windowTitle);
    
    // Reset trạng thái dòng
    lineStarted = 0;
    hasContent = 0;
    lineStartedFile = 0;
    hasContentFile = 0;
}

// Handler cho signal để dừng chương trình gracefully
#ifdef _WIN32
BOOL WINAPI ConsoleHandler(DWORD dwType) {
    if (dwType == CTRL_C_EVENT || dwType == CTRL_CLOSE_EVENT) {
        printf("\nReceived signal, stopping hook...\n");
        if (g_logFile) {
            fprintf(g_logFile, "\n[%ld] Received signal, stopping hook...\n", time(NULL));
            fflush(g_logFile);
        }
        
        if (g_hookContext) {
            hook_stop(g_hookContext);
            hook_cleanup(g_hookContext);
            g_hookContext = NULL;
        }
        
        // Đóng file log
        if (g_logFile) {
            fclose(g_logFile);
            g_logFile = NULL;
        }
        
        exit(0);
    }
    return TRUE;
}
#else
void signal_handler(int sig) {
    printf("\nReceived signal %d, stopping hook...\n", sig);
    if (g_logFile) {
        fprintf(g_logFile, "\n[%ld] Received signal %d, stopping hook...\n", time(NULL), sig);
        fflush(g_logFile);
    }
    
    if (g_hookContext) {
        hook_stop(g_hookContext);
        hook_cleanup(g_hookContext);
        g_hookContext = NULL;
    }
    
    // Đóng file log
    if (g_logFile) {
        fclose(g_logFile);
        g_logFile = NULL;
    }
    
    exit(0);
}
#endif

int main(int argc, char* argv[]) {
    // Xử lý command line arguments
    const char* logFilePath = NULL;
    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            printf("Usage: %s [log_file]\n", argv[0]);
            printf("  log_file: Path to log file (optional, default: openkey.log)\n");
            printf("  -h, --help: Show this help message\n");
            return 0;
        }
        logFilePath = argv[1];
    } else {
        // Tên file mặc định
        logFilePath = "openkey.log";
    }
    
    // Mở file log
    g_logFile = fopen(logFilePath, "a");  // Append mode
    if (!g_logFile) {
        fprintf(stderr, "Warning: Failed to open log file '%s'. Continuing without file logging.\n", logFilePath);
        g_logFile = NULL;
    } else {
        // Ghi header vào file
        time_t now = time(NULL);
        char timeStr[64];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
        fprintf(g_logFile, "\n=== OpenKey Hook Session Started: %s ===\n", timeStr);
        fflush(g_logFile);
    }
    
    printf("OpenKey Hook - Monitoring keyboard and active window\n");
    if (g_logFile) {
        printf("Logging to: %s\n", logFilePath);
    }
    printf("Press Ctrl+C to exit\n\n");
    
    // Đăng ký signal handlers
#ifdef _WIN32
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
#else
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
#endif
    
    // Khởi tạo hook
    g_hookContext = hook_init(on_keyboard_event, on_window_change);
    if (!g_hookContext) {
        fprintf(stderr, "Failed to initialize hook\n");
        if (g_logFile) {
            fclose(g_logFile);
        }
        return 1;
    }
    
    // Bắt đầu hook
    if (hook_start(g_hookContext) != 0) {
#ifdef _WIN32
        fprintf(stderr, "Failed to start hook. Please run as administrator.\n");
#else
        fprintf(stderr, "Failed to start hook. Please grant accessibility permissions in System Preferences.\n");
#endif
        hook_cleanup(g_hookContext);
        if (g_logFile) {
            fclose(g_logFile);
        }
        return 1;
    }
    
    printf("Hook started successfully. Monitoring keyboard and window changes...\n\n");
    
    // Chạy run loop
#ifdef _WIN32
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
#else
    CFRunLoopRun();
#endif
    
    // Cleanup (sẽ không bao giờ đến đây vì CFRunLoopRun chạy vô hạn)
    hook_cleanup(g_hookContext);
    
    // Đóng file log
    if (g_logFile) {
        fclose(g_logFile);
    }
    
    return 0;
}
