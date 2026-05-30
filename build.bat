@echo off
REM ============================================
REM  Qt 5.14.2 WiFi 通信助手 - 编译脚本
REM  使用前请确保已安装 Qt 5.14.2 并配置环境变量
REM ============================================

echo.
echo ===== Qt 5.14.2 WiFi App Build Script =====
echo.

REM 设置 Qt 5.14.2 路径（根据实际安装位置修改）
set QT_DIR=C:\Qt\Qt5.14.2\5.14.2

REM Android 编译
IF "%1"=="android" (
    echo Building for Android...
    set QT_PLATFORM=android_armv7
    "%QT_DIR%\%QT_PLATFORM%\bin\qmake.exe" WiFiApp.pro
    mingw32-make.exe -j4
    echo Android APK build complete!
    goto :end
)

REM iOS 编译
IF "%1"=="ios" (
    echo Building for iOS...
    set QT_PLATFORM=ios
    "%QT_DIR%\%QT_PLATFORM%\bin\qmake.exe" WiFiApp.pro
    make -j4
    echo iOS build complete!
    goto :end
)

REM 桌面编译（用于测试）
echo Building for Desktop (testing)...
qmake WiFiApp.pro
make -j4
echo Desktop build complete!
echo Run: ./WiFiApp  (or WiFiApp.exe on Windows)

:end
echo.
