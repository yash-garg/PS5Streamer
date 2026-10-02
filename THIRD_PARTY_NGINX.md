# Third-party: nginx + RTMP

PS5Streamer bundles **nginx with the RTMP module** in GitHub Actions artifacts so the app can receive the PS5 stream on port `1935`.

## Sources

| Platform | What we ship | Upstream |
|---|---|---|
| **Linux** | `Binaries/nginx` built in CI from nginx.org + RTMP module | [nginx.org downloads](https://nginx.org/en/download.html) · [arut/nginx-rtmp-module](https://github.com/arut/nginx-rtmp-module) |
| **Windows** | `Binaries/nginx.exe` (+ conf/logs/temp) | [iliweii/nginx-rtmp-win64](https://github.com/iliweii/nginx-rtmp-win64) (nginx 1.28.1 + RTMP, static) |

## Layout next to the app

```
PS5Streamer(.exe)
Binaries/
  nginx(.exe)
  lib/                 # Linux: bundled shared libs (portable across distros)
  conf/ logs/ temp/ html/   # Windows package
```

The app prefers `Binaries/nginx` beside the executable, then falls back to system nginx.

## Manual download (if not using CI artifacts)

- Windows zip: https://github.com/iliweii/nginx-rtmp-win64/releases/latest  
- Module (source): https://github.com/arut/nginx-rtmp-module  
- Stock nginx **without** RTMP will not work.
