# 智能家居控制 App

基于 Qt 5.14.2 开发的 Android 智能家居控制应用，通过 TCP/WiFi 连接 ESP8266 + STM32 硬件平台，实现对家居设备的远程控制和环境数据监测。

---

## 功能概述

### 设备控制
| 设备 | 数量 | 说明 |
|------|------|------|
| LED 灯 | 2 路 | 独立开关控制 |
| 蜂鸣器 | 1 路 | 开关控制 |
| 继电器 | 4 路 | 风扇 / 空调 / 水泵 / 窗户 |

### 环境传感器
| 传感器 | 范围 | 显示方式 |
|--------|------|----------|
| 可燃气体 | 0–1200 | 进度条 + 数值 |
| 光照强度 | 0–1100 | 进度条 + 数值 |
| 温度 | — | 文本标签 |
| 湿度 | — | 文本标签 |
| CO2 | — | 文本标签 |

### 通信日志
- 实时显示收发的 JSON 数据
- 带时间戳和颜色区分（发送/接收/错误）
- HTML 富文本渲染

---

## 技术架构

```
┌──────────────────┐      TCP/JSON        ┌──────────────┐      UART       ┌─────────┐
│  Android 手机     │ ◄──────────────────► │   ESP8266    │ ◄─────────────► │  STM32  │
│  Qt 5.14.2 App   │   WiFi 192.168.4.1   │  串口桥接     │                  │  主控   │
└──────────────────┘     端口 333          └──────────────┘                  └─────────┘
```

- **UI 框架**: Qt 5.14.2 Widgets（C++）
- **网络通信**: QTcpSocket，TCP 长连接
- **消息格式**: JSON + `\n` 行终止符
- **Android 构建**: Gradle (AGP 3.5.0) + androiddeployqt.exe
- **目标架构**: armeabi-v7a / arm64-v8a / x86 / x86_64
- **最低 SDK**: Android 5.0 (API 21)
- **目标 SDK**: Android 9.0 (API 28)

---

## 项目结构

```
Smart_app/
├── WiFiApp.pro              # Qt 项目文件
├── main.cpp                 # 应用入口，全局样式
├── mainwindow.h             # 主窗口声明
├── mainwindow.cpp           # 主窗口实现（UI/控制/协议）
├── mainwindow.ui            # Qt Designer 界面布局
├── tcpclient.h              # TCP 客户端声明
├── tcpclient.cpp            # TCP 客户端实现
├── resources.qrc            # 资源文件索引
├── icon.png                 # 应用图标 & 登录页 Logo
├── 智能家居.png             # 宣传图
├── build.bat                # 命令行编译脚本
├── android/                 # Android 平台配置
│   ├── AndroidManifest.xml  # 权限、Activity 声明
│   ├── build.gradle         # Gradle 构建配置
│   ├── gradlew / gradlew.bat
│   ├── gradle/wrapper/
│   └── res/
│       ├── drawable/icon.png
│       └── values/
└── ios/
    └── Info.plist
```

---

## 通信协议

### 发送命令（App → ESP8266 → STM32）

**设置设备状态：**
```json
{"cmd":"set","dev":"led","id":1,"val":1}
{"cmd":"set","dev":"led","id":2,"val":0}
{"cmd":"set","dev":"buzzer","val":1}
{"cmd":"set","dev":"relay","id":1,"val":1}
```

| 字段 | 说明 |
|------|------|
| `cmd` | 命令类型：`set`（控制）/ `query`（查询） |
| `dev` | 设备类型：`led` / `buzzer` / `relay` |
| `id` | 设备编号（led: 1–2, relay: 1–4），buzzer 无需 id |
| `val` | 值：`1` = 开，`0` = 关 |

**查询所有状态：**
```json
{"cmd":"query"}
```

### 接收状态（STM32 → ESP8266 → App）

```json
{
  "type":"status",
  "led":[1,0],
  "relay":[1,0,1,0],
  "buzzer":0,
  "gas":320,
  "light":580,
  "temp":26,
  "humi":65,
  "co2":420
}
```

### 通信细节
- 每条消息以 `\n` 结尾
- App 每 5 秒自动轮询一次传感器数据
- 发送控制命令后重启查询定时器，给 STM32 足够处理时间
- 命令冷却 300ms，防止按钮连点导致命令风暴
- 发命令后 1.5 秒内忽略设备状态更新，防止旧查询回复覆盖刚操作的状态

---

## 防抖/同步机制

| 机制 | 时间 | 作用 |
|------|------|------|
| `m_cmdCooldown` + `m_cmdCooldownTimer` | 300ms | 防止按钮快速连点，期间 `sendSetCommand` 直接返回 |
| `m_statusBlocked` + `m_statusBlockTimer` | 1.5s | 发命令后屏蔽状态更新，防止旧查询回包覆盖本地状态 |
| `QSignalBlocker` | — | 同步弹窗按钮时阻塞信号，避免触发 toggle 递归调用 |
| 查询定时器 | 5s 间隔 | 自动轮询传感器数据 |
| 延迟首次查询 | 500ms | 连接后等 ESP8266 串口桥初始化完成 |

---

## 界面说明

### 登录页
- 输入 ESP8266 的 IP 地址和端口（默认 `192.168.4.1:333`）
- 点击"连接服务器"建立 TCP 连接
- 连接成功后自动切换到主控页面

### 主控页
- **上半屏**：设备控制入口 + 传感器数据
  - "LED 灯 & 蜂鸣器"按钮 → 弹出控制弹窗
  - "继电器控制"按钮 → 弹出风扇/空调/水泵/窗户控制弹窗
  - 可燃气体 / 光照进度条 + 温度 / 湿度 / CO2 数值
- **下半屏**：通信日志，实时显示收发数据

### 弹窗控制
- 弹窗内按钮为**自锁按钮**（checkable），按下即保持状态
- 关闭弹窗后状态保留，可再次打开查看/操作

---

## 构建与部署

### 依赖
- Qt 5.14.2（for Android，含 Android armv7a/arm64-v8a/x86/x86_64 kit）
- Android NDK r21
- Android SDK (API 28)
- JDK 8

### 桌面测试编译
```bash
qmake WiFiApp.pro
make -j4
./WiFiApp
```

### Android APK 构建（Qt Creator）
1. 用 Qt Creator 打开 `WiFiApp.pro`
2. 选择 Android kit（如 `Android for armeabi-v7a (Clang Qt 5.14.2 for Android)`）
3. 点击 Build → Build Project
4. 连接 Android 手机，点击 Run 部署

### 命令行构建
```bash
# 参考 build.bat 脚本，根据实际 Qt 路径修改
```

### 注意事项
- APK 输出文件名**不能**自定义修改（`build.gradle` 中的 `outputFileName`），否则 androiddeployqt.exe 会找不到 APK
- 手机需要先连接 ESP8266 的 WiFi 热点（默认 SSID 由 ESP8266 固件决定）
- AndroidManifest 中已开启 `usesCleartextTraffic="true"`，允许明文 TCP 通信

---

## Android 权限

| 权限 | 用途 |
|------|------|
| `INTERNET` | TCP 网络通信 |
| `ACCESS_NETWORK_STATE` | 检测网络状态 |
| `ACCESS_WIFI_STATE` | 检测 WiFi 状态 |
| `CHANGE_WIFI_STATE` | 切换 WiFi 网络 |
| `CHANGE_NETWORK_STATE` | 网络切换 |
| `ACCESS_FINE_LOCATION` | Android 9+ WiFi 扫描需要位置权限 |

---

## 全局样式

应用采用绿色自然色系主题：
- 背景渐变：浅绿 `#E8F5E9` → `#C8E6C9`
- 主色调：`#66BB6A` / `#43A047`（Material Green 400/600）
- 连接按钮（已连接状态）切换为红色系 `#EF5350` / `#C62828`
- 所有控件大字体、大圆角，适合手机触控操作
