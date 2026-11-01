# w11box

Winlator-style **container manager in C++17** for Wine + Box64. Ek hi source file (`w11box.cpp`), koi external library nahi. Android (Termux), Linux aur ARM/x86 boards par chalta hai.

> **Zaroori:** w11box khud x86 emulate nahi karta. Ye **Box64** aur **Wine** ko Winlator ki tarah configure aur launch karta hai. Asli `.exe` chalane ke liye dono alag se install hone chahiye (`w11box doctor` se check karein). Graphics env variables best-effort hain; Winlator apne custom builds use karta hai, isliye result device par depend karta hai. "Windows 11" matlab Wine ko Windows 11 report karwana hai, asli Windows 11 nahi.

## Features
- Containers (alag Wine prefix har ek ke liye)
- Screen size, Graphics driver (VirGL / Turnip / Zink / software)
- DX wrapper: DXVK, VKD3D, WineD3D (`dxvk` command se DLLs copy)
- Audio driver: ALSA / PulseAudio
- Box64 presets: Stability, Compatibility, Intermediate, Performance
- Windows version: win11 / win10 / win7 / winxp
- Windows desktop (explorer shell), Task Manager, Registry Editor, Wine Config, Control Panel, cmd
- `.exe` run, `env` export, dry-run test mode

## Build
```sh
g++ -std=c++17 -O2 w11box.cpp -o w11box
# ya
cmake -B build && cmake --build build
```
Termux: `pkg install clang` phir upar wala `clang++` command (g++ ki jagah).

## Use
```sh
./w11box doctor
./w11box create game --res 800x600 --gpu virgl --dx dxvk --preset performance
./w11box dxvk game ~/dxvk-2.x          # DXVK release folder (x64/ aur x32/)
./w11box run game ~/setup.exe
./w11box desktop game                  # Windows jaisa desktop
./w11box tool game taskmgr
./w11box config game preset stability
```
Display ke liye Termux:X11 (ya koi X server) chalu rakhein; `DISPLAY` default `:0` hai.

## Test (bina Wine ke)
```sh
W11BOX_DRY=1 ./w11box run game test.exe   # sirf command aur env print karta hai
```

## Low-end tips
800x600, VirGL, Performance preset, background apps band.

## License
MIT
