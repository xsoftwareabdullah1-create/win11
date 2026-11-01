// w11box - Winlator-style container manager for Wine + Box64 (C++17, POSIX/Android/Termux)
// Real .exe chalane ke liye Box64 aur Wine alag se install hona zaroori hai.
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using Cfg = std::map<std::string, std::string>;
using Env = std::vector<std::pair<std::string, std::string>>;
using Cmd = std::vector<std::string>;

static const std::map<std::string, std::vector<std::string>> kAllowed = {
    {"gpu", {"virgl", "turnip", "zink", "software"}},
    {"dx", {"dxvk", "vkd3d", "wined3d"}},
    {"audio", {"alsa", "pulse"}},
    {"preset", {"stability", "compatibility", "intermediate", "performance"}},
    {"win", {"win11", "win10", "win7", "winxp"}}};

static const char* kBoxKeys[] = {"SAFEFLAGS", "FASTNAN", "FASTROUND", "X87DOUBLE",
                                 "BIGBLOCK", "STRONGMEM", "FORWARD", "CALLRET"};
// Winlator jaise presets (approximate; apne device ke hisaab se tune karein)
static const std::map<std::string, std::vector<std::string>> kPresets = {
    {"stability", {"2", "0", "0", "1", "0", "2", "128", "0"}},
    {"compatibility", {"2", "0", "0", "1", "0", "1", "128", "0"}},
    {"intermediate", {"1", "1", "0", "1", "1", "1", "128", "0"}},
    {"performance", {"0", "1", "1", "0", "3", "0", "512", "1"}}};

static fs::path Root() {
  if (const char* h = getenv("W11BOX_HOME")) return h;
  const char* u = getenv("HOME");
  return fs::path(u ? u : ".") / ".w11box";
}
static fs::path Dir(const std::string& n) { return Root() / n; }
static bool Dry() { return getenv("W11BOX_DRY") != nullptr; }

static Cfg Defaults() {
  return {{"res", "800x600"}, {"gpu", "virgl"},   {"dx", "dxvk"},
          {"audio", "alsa"},  {"preset", "performance"}, {"win", "win11"},
          {"wine", "wine"}};
}

static bool Load(const std::string& n, Cfg& c) {
  std::ifstream f(Dir(n) / "config.ini");
  if (!f) return false;
  c = Defaults();
  std::string l;
  while (std::getline(f, l)) {
    auto p = l.find('=');
    if (p != std::string::npos) c[l.substr(0, p)] = l.substr(p + 1);
  }
  return true;
}

static void Save(const std::string& n, const Cfg& c) {
  fs::create_directories(Dir(n));
  std::ofstream f(Dir(n) / "config.ini");
  for (auto& kv : c) f << kv.first << "=" << kv.second << "\n";
}

static bool Valid(const std::string& k, const std::string& v) {
  if (k == "res") return std::regex_match(v, std::regex("[0-9]{3,4}x[0-9]{3,4}"));
  if (k == "wine") return !v.empty();
  auto it = kAllowed.find(k);
  if (it == kAllowed.end()) return false;
  for (auto& x : it->second) if (x == v) return true;
  return false;
}

static bool NativeX86() {
  struct utsname u;
  uname(&u);
  std::string m = u.machine;
  return m == "x86_64" || m == "i686" || m == "i386";
}

static Env BuildEnv(const std::string& n, const Cfg& c) {
  Env e;
  e.push_back({"WINEPREFIX", (Dir(n) / "prefix").string()});
  e.push_back({"WINEDEBUG", "-all"});
  if (!getenv("DISPLAY")) e.push_back({"DISPLAY", ":0"});  // Termux:X11
  e.push_back({"BOX64_LOG", "0"});
  auto& p = kPresets.at(c.at("preset"));
  for (size_t i = 0; i < p.size(); i++)
    e.push_back({std::string("BOX64_DYNAREC_") + kBoxKeys[i], p[i]});
  const std::string& g = c.at("gpu");
  if (g == "virgl") {
    e.push_back({"GALLIUM_DRIVER", "virpipe"});
    e.push_back({"MESA_GL_VERSION_OVERRIDE", "4.3"});
  } else if (g == "turnip") {
    e.push_back({"MESA_LOADER_DRIVER_OVERRIDE", "zink"});
    e.push_back({"TU_DEBUG", "noconform"});
  } else if (g == "zink") {
    e.push_back({"MESA_LOADER_DRIVER_OVERRIDE", "zink"});
  } else {
    e.push_back({"LIBGL_ALWAYS_SOFTWARE", "1"});
  }
  const std::string& dx = c.at("dx");
  if (dx == "dxvk") e.push_back({"WINEDLLOVERRIDES", "d3d8,d3d9,d3d10core,d3d11,dxgi=n,b"});
  if (dx == "vkd3d") e.push_back({"WINEDLLOVERRIDES", "d3d12,d3d12core=n,b"});
  return e;
}

static Cmd WineCmd(const Cfg& c, const Cmd& args) {
  Cmd v;
  if (!NativeX86()) v.push_back("box64");
  v.push_back(c.at("wine"));
  v.insert(v.end(), args.begin(), args.end());
  return v;
}

static int Spawn(const Env& e, const Cmd& cmd) {
  if (Dry()) {
    for (auto& kv : e) std::cout << "ENV " << kv.first << "=" << kv.second << "\n";
    std::cout << "CMD";
    for (auto& a : cmd) std::cout << " " << a;
    std::cout << "\n";
    return 0;
  }
  pid_t pid = fork();
  if (pid == 0) {
    for (auto& kv : e) setenv(kv.first.c_str(), kv.second.c_str(), 1);
    std::vector<char*> a;
    for (auto& s : cmd) a.push_back(const_cast<char*>(s.c_str()));
    a.push_back(nullptr);
    execvp(a[0], a.data());
    perror(a[0]);
    _exit(127);
  }
  int st = 0;
  waitpid(pid, &st, 0);
  return WIFEXITED(st) ? WEXITSTATUS(st) : 1;
}

static void RegAdd(const Env& e, const Cfg& c, const std::string& key,
                   const std::string& name, const std::string& val) {
  Spawn(e, WineCmd(c, {"reg", "add", key, "/v", name, "/d", val, "/f"}));
}

static void Apply(const std::string& n, const Cfg& c) {
  Env e = BuildEnv(n, c);
  Spawn(e, WineCmd(c, {"winecfg", "-v", c.at("win")}));
  RegAdd(e, c, "HKCU\\Software\\Wine\\Drivers", "Audio", c.at("audio"));
  const std::string ov = "HKCU\\Software\\Wine\\DllOverrides";
  if (c.at("dx") == "dxvk")
    for (auto d : {"d3d8", "d3d9", "d3d10core", "d3d11", "dxgi"}) RegAdd(e, c, ov, d, "native,builtin");
  if (c.at("dx") == "vkd3d")
    for (auto d : {"d3d12", "d3d12core"}) RegAdd(e, c, ov, d, "native,builtin");
}

static bool Need(const std::string& n, Cfg& c) {
  if (Load(n, c)) return true;
  std::cerr << "Container '" << n << "' nahi mila. Pehle: w11box create " << n << "\n";
  return false;
}

static int Doctor() {
  auto has = [](const char* b) {
    std::string cmd = std::string("command -v ") + b + " >/dev/null 2>&1";
    return system(cmd.c_str()) == 0;
  };
  struct utsname u;
  uname(&u);
  std::cout << "Arch: " << u.machine << (NativeX86() ? " (x86, box64 ki zaroorat nahi)\n" : " (ARM, box64 chahiye)\n");
  for (auto b : {"box64", "wine", "winetricks", "vulkaninfo"})
    std::cout << (has(b) ? "[OK]      " : "[MISSING] ") << b << "\n";
  std::cout << "Home: " << Root().string() << "\n";
  return 0;
}

static int Usage() {
  std::cout <<
R"(w11box - Winlator-style Wine/Box64 container manager

  w11box doctor                          tools check
  w11box create <name> [--res 800x600] [--gpu virgl|turnip|zink|software]
                [--dx dxvk|vkd3d|wined3d] [--audio alsa|pulse]
                [--preset stability|compatibility|intermediate|performance]
                [--win win11|win10|win7|winxp] [--wine wine]
  w11box list
  w11box show <name>
  w11box config <name> <key> <value>     settings badlo aur apply karo
  w11box run <name> <file.exe> [args]    .exe chalao
  w11box desktop <name>                  Windows desktop (explorer shell)
  w11box tool <name> taskmgr|regedit|winecfg|control|cmd|explorer
  w11box dxvk <name> <dxvk_folder>       x64/x32 dll prefix mein copy
  w11box env <name>                      environment variables dekho
  w11box delete <name> --yes

Test (bina wine ke): W11BOX_DRY=1 w11box run game game.exe
)";
  return 0;
}

int main(int argc, char** argv) {
  Cmd a(argv + 1, argv + argc);
  if (a.empty() || a[0] == "help" || a[0] == "--help") return Usage();
  const std::string cmd = a[0];
  if (cmd == "doctor") return Doctor();
  if (cmd == "list") {
    if (fs::exists(Root()))
      for (auto& d : fs::directory_iterator(Root()))
        if (d.is_directory()) std::cout << d.path().filename().string() << "\n";
    return 0;
  }
  if (a.size() < 2) return Usage();
  const std::string name = a[1];
  Cfg c;

  if (cmd == "create") {
    if (Load(name, c)) { std::cerr << "Container pehle se hai.\n"; return 1; }
    c = Defaults();
    for (size_t i = 2; i + 1 < a.size(); i += 2) {
      if (a[i].rfind("--", 0) != 0) { std::cerr << "Galat option: " << a[i] << "\n"; return 1; }
      std::string k = a[i].substr(2);
      if (!Valid(k, a[i + 1])) { std::cerr << "Galat " << k << ": " << a[i + 1] << "\n"; return 1; }
      c[k] = a[i + 1];
    }
    fs::create_directories(Dir(name) / "prefix");
    Save(name, c);
    Env e = BuildEnv(name, c);
    std::cout << "Prefix ban raha hai (pehli baar time lagega)...\n";
    Spawn(e, WineCmd(c, {"wineboot", "-u"}));
    Apply(name, c);
    std::cout << "Container '" << name << "' ready.\n";
    return 0;
  }
  if (!Need(name, c)) return 1;
  Env e = BuildEnv(name, c);

  if (cmd == "show") { for (auto& kv : c) std::cout << kv.first << " = " << kv.second << "\n"; return 0; }
  if (cmd == "env") { for (auto& kv : e) std::cout << "export " << kv.first << "='" << kv.second << "'\n"; return 0; }
  if (cmd == "config") {
    if (a.size() < 4 || !Valid(a[2], a[3])) { std::cerr << "Use: config <name> <key> <value> (valid value)\n"; return 1; }
    c[a[2]] = a[3];
    Save(name, c);
    Apply(name, c);
    return 0;
  }
  if (cmd == "run") {
    if (a.size() < 3) return Usage();
    Cmd args(a.begin() + 2, a.end());
    return Spawn(e, WineCmd(c, args));
  }
  if (cmd == "desktop")
    return Spawn(e, WineCmd(c, {"explorer", "/desktop=shell," + c["res"], "explorer.exe"}));
  if (cmd == "tool") {
    static const std::map<std::string, std::string> t = {{"taskmgr", "taskmgr"}, {"regedit", "regedit"},
        {"winecfg", "winecfg"}, {"control", "control"}, {"cmd", "cmd"}, {"explorer", "explorer"}};
    if (a.size() < 3 || !t.count(a[2])) return Usage();
    return Spawn(e, WineCmd(c, {t.at(a[2])}));
  }
  if (cmd == "dxvk") {
    if (a.size() < 3) return Usage();
    fs::path pre = Dir(name) / "prefix" / "drive_c" / "windows";
    int n = 0;
    std::error_code ec;
    for (auto [sub, dst] : {std::pair{"x64", "system32"}, std::pair{"x32", "syswow64"}}) {
      fs::path src = fs::path(a[2]) / sub;
      if (!fs::exists(src)) continue;
      fs::create_directories(pre / dst);
      for (auto& f : fs::directory_iterator(src))
        if (f.path().extension() == ".dll") {
          fs::copy_file(f.path(), pre / dst / f.path().filename(), fs::copy_options::overwrite_existing, ec);
          if (!ec) n++;
        }
    }
    std::cout << n << " dll copy hui.\n";
    c["dx"] = "dxvk";
    Save(name, c);
    Apply(name, c);
    return n ? 0 : 1;
  }
  if (cmd == "delete") {
    if (a.size() < 3 || a[2] != "--yes") { std::cerr << "Confirm ke liye --yes lagayein.\n"; return 1; }
    fs::remove_all(Dir(name));
    std::cout << "Deleted.\n";
    return 0;
  }
  return Usage();
}
