# ESP32-CAM-MJPEG-Streaming-Server
eal-time MJPEG video streaming on ESP32-CAM with a full-screen web UI. Designed for robotics, FPV systems, embedded vision, and low-latency camera projects.

This repository implements a native ESP32-CAM HTTP MJPEG streaming server using ESP-IDF internals.
The ESP32-CAM hosts its own Wi-Fi Access Point, serves a full-screen live video feed.

This project focuses on:

Stability
Low latency
Minimal dependencies
Clean architecture
No external servers. No cloud. No frame-refresh hacks.

✨ Key Features

📡 Live MJPEG video streaming
🌐 Built-in Wi-Fi Access Point (AP mode)
🖥️ Full-screen responsive web interface
⚙️ ESP-IDF HTTP server backend
🧠 PSRAM-aware memory management
📱 Mobile and desktop browser compatible

🔌 Single-file firmware, easy to extend

🧠 Typical Use Cases
🤖 Robotics vision systems
🚗 FPV / RC vehicle camera feed
🕵️ Surveillance & monitoring prototypes
📡 Portable camera nodes
🧪 Embedded systems experimentation
🎓 Learning ESP32-CAM internals (advanced)

🧩 Hardware Requirements

ESP32-CAM (AI Thinker variant)
USB-to-Serial programmer (FTDI / CP2102 / CH340)
Stable 5V power supply (≥ 1A recommended)

🖥️ Software Requirements

Arduino IDE or PlatformIO
ESP32 Board Support Package (latest stable)
Required headers:

esp_camera.h
WiFi.h
esp_http_server.h

No third-party streaming libraries are used.

🌐 Web Interface

Full-screen live video feed
Touch-friendly brightness slider
Overlay UI (does not interrupt streaming)

Works on:

Android
iOS
Windows
Linux
macOS

No app required. A standard browser is enough.

📷 Getting Started

Flash the firmware to ESP32-CAM

Power the board

Connect to Wi-Fi network:

ESP32-CAM-Stream

Open a browser and visit:

http://192.168.4.1


The live stream starts automatically.

🔌 Pin Configuration (AI Thinker)
Function	GPIO
Camera Data	5, 18, 19, 21, 36–39
XCLK	0
PCLK	22
VSYNC	25
HREF	23
SCCB SDA	26
SCCB SCL	27
Flash LED	4

⚠️ Pin mapping is specific to the AI Thinker ESP32-CAM.

⚠️ Known Limitations

Single stream client at a time
No authentication (open AP)
HTTP only (no TLS)
Limited Wi-Fi AP range

These trade-offs are intentional for performance and simplicity.

🔐 Security Notice

This project is intended for:

Local networks
Robotics and experimentation
Development environments

It is not production-hardened.
For secure deployments, add authentication or proxy through a gateway device.

🛠️ Future Improvements

RTSP or WebRTC streaming
ESP32 → Raspberry Pi stream bridge
Pan-tilt camera control
Authentication layer
Progressive Web App (PWA) UI
AI vision extensions (ESP32-S3 / Raspberry Pi)



