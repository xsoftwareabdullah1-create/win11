# 🪟 Win11 Lite

Windows 11 style desktop jo browser mein chalta hai (Android, low-end devices par bhi). Sirf **HTML + CSS + JavaScript**, koi install ya dependency nahi, ek hi file (`index.html`).

> ⚠️ **Zaroori:** ye Windows 11 ka *look* aur Winlator-style *settings simulator* hai. Ye asli `.exe` apps **nahi** chalata (x86 emulation browser mein possible nahi). Asli `.exe` ke liye [Winlator](https://github.com/brunodev85/winlator) use karein.

## Features

**Desktop**
- Windows 11 taskbar, Start menu, clock, draggable windows
- Light/Dark theme, wallpaper change
- Chhoti screen par windows auto full-screen
- Touch-friendly, low RAM

**Apps**
| App | Kaam |
|---|---|
| Notepad | Text likho (auto-save) |
| Calculator | Basic calculator |
| File Explorer | Folders browse |
| Paint | Touch se draw |
| Terminal | `help, dir, date, time, ver, echo, cls, whoami` |
| Settings | Theme, wallpaper |

**Winlator-style tools**
- **Winlator Containers**: Screen size, Graphics driver (VirGL/Turnip/Zink), DX wrapper (DXVK/VKD3D/WineD3D), Audio (ALSA/PulseAudio), Box64 preset; Launch log, Export (JSON copy), Delete
- **Install .exe**: file chuno, shortcut list mein add hoti hai (run nahi hoti)
- **Task Manager**: wine processes, End task
- **Registry Editor**: Wine Direct3D/Audio keys
- **Wine Configuration**: Windows version, DLL overrides, virtual desktop
- **Input Controls**: on-screen gamepad, FPS HUD, touchpad mouse mode

## GitHub Pages par chalane ka tarika
1. Is repo mein `index.html` upload karein.
2. **Settings → Pages → Source: Deploy from a branch → `main` / `(root)` → Save**.
3. 1-2 minute baad link milega: `https://<username>.github.io/<repo>/`

## Android par asli Windows apps ke liye
1. Winlator APK GitHub se download karein.
2. Container: **800x600**, driver **VirGL** (Adreno par **Turnip**), Box64 preset **Performance**.
3. Is project ke *Containers* app se export ki hui settings wahan daalein.

## License
MIT
