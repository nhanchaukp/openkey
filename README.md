# OpenKey Hook

Hook để theo dõi và log:
- Tên ứng dụng của cửa sổ đang active
- Tiêu đề cửa sổ đang active
- Các phím được người dùng gõ

## Yêu cầu

### macOS
- macOS (sử dụng CoreGraphics và Carbon APIs)
- Quyền Accessibility (cần cấp trong System Preferences)

### Windows
- Windows 7 trở lên
- MinGW-w64 hoặc MSVC compiler
- Quyền Administrator (để hook keyboard)

## Cấp quyền

### macOS - Cấp quyền Accessibility

1. Mở System Preferences (hoặc System Settings trên macOS Ventura+)
2. Vào Security & Privacy (hoặc Privacy & Security)
3. Chọn Accessibility
4. Thêm Terminal hoặc ứng dụng bạn dùng để chạy chương trình
5. Đảm bảo checkbox được bật

### Windows - Chạy với quyền Administrator

Trên Windows, bạn cần chạy chương trình với quyền Administrator để hook keyboard:

1. Click chuột phải vào `openkey.exe`
2. Chọn "Run as administrator"

## Build

### macOS

```bash
make
```

### Windows

#### Sử dụng MinGW (khuyến nghị)

1. Cài đặt MinGW-w64 và thêm vào PATH
2. Chạy build script:

```batch
build_windows.bat
```

Hoặc sử dụng Makefile:

```batch
mingw32-make -f Makefile.windows
```

#### Sử dụng MSVC

```batch
cl /D_WIN32 main.c hook_windows.c /link psapi.lib user32.lib gdi32.lib kernel32.lib /OUT:openkey.exe
```

## Chạy

```bash
make run
```

hoặc

```bash
./openkey
```

### Tùy chọn log file

Mặc định, chương trình sẽ log vào file `openkey.log` trong thư mục hiện tại. Bạn có thể chỉ định file log khác:

```bash
./openkey /path/to/logfile.log
```

Hoặc chạy không log file (chỉ hiển thị trên console):

```bash
./openkey /dev/null
```

Xem help:

```bash
./openkey -h
```

## Dừng chương trình

Nhấn `Ctrl+C` để dừng hook một cách an toàn.

## Output

Chương trình sẽ log ra cả console và file log:
- Khi có phím được gõ: `[timestamp] <keys>` (các phím được in liên tiếp trên cùng một dòng, chỉ xuống dòng khi nhấn Enter)
- Khi cửa sổ active thay đổi: `[timestamp] Active Window - App: <app_name> | Title: <window_title>`

File log sẽ được mở ở chế độ append, nên các session mới sẽ được thêm vào cuối file.

## Clean

```bash
make clean
```

## CI/CD

Project sử dụng GitHub Actions để tự động build phiên bản Windows x64 khi:
- Push code lên branch `main` hoặc `master`
- Tạo Pull Request
- Tạo tag bắt đầu với `v` (ví dụ: `v1.0.0`)
- Chạy thủ công qua GitHub Actions tab

Artifact sẽ được upload tự động và có thể download từ Actions tab hoặc Releases (nếu có tag).
