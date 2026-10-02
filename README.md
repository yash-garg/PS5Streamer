# PS5 Stream Interceptor

Intercepts PS5 RTMP streams locally so you can route them anywhere — OBS, X, Discord, record — without Twitch ever seeing them.

```
PS5 ──RTMP──▶ Host (nginx-rtmp :1935)
                     │
                     ▼
                    OBS ──▶ X / Discord / Record / anywhere
```

## How it works

1. PS5 links a Twitch account (just to unlock the broadcast UI)
2. App runs a DNS interceptor that lies to the PS5 — Twitch ingest hosts → your machine IP
3. PS5 pushes RTMP to your machine instead of Twitch
4. nginx-rtmp receives it; OBS pulls it as a Media Source
5. You stream/record wherever you want from OBS

---

## Platforms

| Platform | UI | Build |
|---|---|---|
| **macOS** | SwiftUI menu bar app | `build.sh` / Xcode |
| **Linux** | Qt 5 Widgets | CMake + GitHub Actions |
| **Windows** | Qt 5 Widgets | CMake + GitHub Actions |

---

## Qt (Linux & Windows)

Sources live in [`qt/`](qt/). Requires [Qt 5.15+](https://doc.qt.io/qt-5/) (Core, Gui, Widgets, Network).

### Runtime dependencies

- **nginx with RTMP** — bundled as `Binaries/nginx` in CI artifacts (see [`THIRD_PARTY_NGINX.md`](THIRD_PARTY_NGINX.md))
  - Windows upstream: [iliweii/nginx-rtmp-win64](https://github.com/iliweii/nginx-rtmp-win64)
  - Linux: built in CI from [nginx.org](https://nginx.org/en/download.html) + [arut/nginx-rtmp-module](https://github.com/arut/nginx-rtmp-module)
- **Administrator / root** to bind DNS on UDP port `53`

### Build locally

```bash
cd qt
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Linux binary: `build/PS5Streamer`  
Windows binary: `build/Release/PS5Streamer.exe` (then run `windeployqt` if needed)

Place an RTMP-enabled nginx in `Binaries/` next to the app, or use a CI artifact that already includes it.

### GitHub Actions

Workflow [`.github/workflows/build-qt.yml`](.github/workflows/build-qt.yml) builds and **ships nginx-rtmp**.

Download the artifact from the Actions run — one unzip gives you the app ready to run:

- **Linux** → `PS5Streamer-linux-x64.zip` containing `PS5Streamer` + `Binaries/`
- **Windows** → `PS5Streamer-windows-x64.zip` containing `PS5Streamer.exe` + Qt DLLs + `Binaries/`
---

## macOS

### Prerequisites

```bash
# nginx with RTMP module (denji tap)
brew tap denji/nginx
brew install nginx-full

# dnsmasq
brew install dnsmasq
```

### Build

```bash
brew install xcodegen
cd ~/PS5Streamer
xcodegen generate
open PS5Streamer.xcodeproj
```

Or use `./build.sh` for a Release DMG.

Set your Team in Xcode → Signing, then `Cmd+R`.

---

## Usage

1. **Start** the app (admin/root required for DNS `:53`)
2. **Set PS5 DNS** — Settings → Network → Advanced → DNS → Manual
   - Primary: `<your LAN IP shown in the app>`
   - Secondary: `1.1.1.1`
3. **Go Live on PS5** — Twitch broadcast option
4. **Copy OBS URL** — paste as Media Source in OBS (uncheck Local File)
5. Stream/record from OBS to anywhere

---

## Restreaming (optional)

Uncomment lines in nginx.conf (generated under `~/.ps5streamer/nginx.conf`):

```nginx
# push rtmp://live.twitch.tv/app/YOUR_TWITCH_KEY;
# push rtmp://ingest.pscp.tv:80/x/YOUR_X_KEY;
# push rtmp://a.rtmp.youtube.com/live2/YOUR_YOUTUBE_KEY;
```

---

## Troubleshooting

| Problem | Fix |
|---|---|
| PS5 DNS not using host | Disable PS5 WiFi, reconnect, re-enter DNS |
| Stream key not detected | Check `~/.ps5streamer/nginx-error.log` |
| DNS bind failed | Run the app as administrator / root (port 53) |
| nginx not found | Install nginx with RTMP, or place binary next to the app |
| OBS shows black | Make sure PS5 is actively broadcasting |

---

## File layout

```
PS5Streamer/                 — macOS SwiftUI app
├── Models/AppState.swift
├── Views/ContentView.swift
└── Services/…

qt/                          — Linux & Windows Qt 5 app
├── CMakeLists.txt
└── src/
    ├── main.cpp
    ├── MainWindow.*
    ├── AppController.*
    ├── DnsInterceptor.*     — built-in UDP :53 spoof
    ├── ProcessManager.*     — nginx process
    ├── StreamKeyServer.*    — on_publish :9988
    └── …
```
