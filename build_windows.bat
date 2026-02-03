@echo off
REM Build script for Windows

echo Building OpenKey for Windows...

REM Check if MinGW is available
where gcc >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: MinGW gcc not found in PATH
    echo Please install MinGW-w64 and add it to your PATH
    exit /b 1
)

REM Build
gcc -Wall -Wextra -O2 -D_WIN32 -c main.c -o main.o
if %ERRORLEVEL% NEQ 0 (
    echo Error compiling main.c
    exit /b 1
)

gcc -Wall -Wextra -O2 -D_WIN32 -c hook_windows.c -o hook_windows.o
if %ERRORLEVEL% NEQ 0 (
    echo Error compiling hook_windows.c
    exit /b 1
)

gcc -Wall -Wextra -O2 -o openkey.exe main.o hook_windows.o -lpsapi -luser32 -lgdi32 -lkernel32
if %ERRORLEVEL% NEQ 0 (
    echo Error linking
    exit /b 1
)

echo Build successful! Output: openkey.exe
echo.
echo To run: openkey.exe [log_file]
