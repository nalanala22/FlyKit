// Lapisan hook. PENTING: tabel kVersions di bawah sengaja kosong.
// Offset struct & signature fungsi Minecraft harus Anda ambil sendiri dari
// libminecraftpe.so versi target (Ghidra/IDA). Tanpa itu, semua fitur
// otomatis NONAKTIF (tidak crash) dan nativeInit mengembalikan status error.
#include "hooks.h"
#include <cstring>
#include "../core/features.h"
#include "sigscan.h"

#ifdef __ANDROID__
#include <android/log.h>
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, "FlyKit", __VA_ARGS__)
#else
#include <cstdio>
#define LOGW(...) (std::fprintf(stderr, __VA_ARGS__), std::fputc('\n', stderr))
#endif

#ifdef FK_USE_DOBBY
extern "C" int DobbyHook(void* address, void* replace, void** origin);
static bool hookFn(void* t, void* r, void** o) { return DobbyHook(t, r, o) == 0; }
#else
static bool hookFn(void*, void*, void**) { return false; }  // sambungkan backend hook Anda
#endif

namespace fk {
namespace {

struct GameLayout {                       // offset field di objek LocalPlayer (per versi)
  ptrdiff_t onGround = -1, velY = -1, mayFly = -1, jumpPressed = -1;
  bool ready() const { return onGround >= 0 && velY >= 0 && mayFly >= 0 && jumpPressed >= 0; }
};
struct VersionEntry {
  const char* version;
  GameLayout layout;
  const char* sigTick;   // LocalPlayer tick
  const char* sigReach;  // fungsi yang mengembalikan jarak serang/interaksi (float)
  const char* sigSend;   // fungsi kirim byte jaringan (self, data, len)
};
const VersionEntry kVersions[] = {
    // {"1.21.xx", {onGround, velY, mayFly, jumpPressed}, "sig tick", "sig reach", "sig send"},
    {nullptr, {}, "", "", ""},
};

GameLayout g_layout;
FlyFeature g_fly;
DoubleJump g_jump;
bool g_hooked[3] = {false, false, false};

template <class T> T rd(void* base, ptrdiff_t off) {
  T v; std::memcpy(&v, static_cast<char*>(base) + off, sizeof v); return v;
}
template <class T> void wr(void* base, ptrdiff_t off, T v) {
  std::memcpy(static_cast<char*>(base) + off, &v, sizeof v);
}

void (*g_origTick)(void*) = nullptr;
void hookTick(void* self) {
  g_origTick(self);
  if (!g_layout.ready()) return;
  const Config& c = Config::get();
  PlayerState s;
  s.onGround = rd<bool>(self, g_layout.onGround);
  s.jumpPressed = rd<bool>(self, g_layout.jumpPressed);
  s.velY = rd<float>(self, g_layout.velY);
  s.mayFly = rd<bool>(self, g_layout.mayFly);
  g_fly.apply(c, s);
  g_jump.tick(c, s);
  wr<bool>(self, g_layout.mayFly, s.mayFly);
  wr<float>(self, g_layout.velY, s.velY);
  blinkQueue().tick(c);
}

float (*g_origReach)(void*) = nullptr;
float hookReach(void* self) { return reachDistance(Config::get(), g_origReach(self)); }

void (*g_origSend)(void*, const uint8_t*, size_t) = nullptr;
void* g_netSelf = nullptr;
void hookSend(void* self, const uint8_t* data, size_t len) {
  g_netSelf = self;
  if (data && len) {
    Packet p;
    p.id = parsePacketId(data, len);
    if (isMovementPacket(p.id)) {
      p.bytes.assign(data, data + len);
      if (blinkQueue().intercept(Config::get(), p)) return;  // ditahan
    } else {
      blinkQueue().intercept(Config::get(), p);              // men-flush bila blink baru mati
    }
  }
  g_origSend(self, data, len);
}

bool install(const char* sig, void* repl, void** orig, const char* name) {
  if (!sig || !*sig) { LOGW("signature %s kosong", name); return false; }
  Pattern p;
  if (!parsePattern(sig, p)) { LOGW("signature %s tidak valid", name); return false; }
  uintptr_t addr = findInModule("libminecraftpe.so", p);
  if (!addr) { LOGW("signature %s tidak ditemukan", name); return false; }
  return hookFn(reinterpret_cast<void*>(addr), repl, orig);
}
}  // namespace

BlinkQueue& blinkQueue() {
  static BlinkQueue q([](const Packet& p) {
    if (g_origSend && g_netSelf) g_origSend(g_netSelf, p.bytes.data(), p.bytes.size());  // ASLI, bukan hook
  });
  return q;
}

InitStatus installHooks(const char* mcVersion) {
  const VersionEntry* e = nullptr;
  for (const VersionEntry* it = kVersions; it->version; ++it)
    if (mcVersion && std::strcmp(it->version, mcVersion) == 0) { e = it; break; }
  if (!e) { LOGW("versi %s belum didukung", mcVersion ? mcVersion : "?"); return InitStatus::UnsupportedVersion; }

  g_layout = e->layout;
  g_hooked[0] = install(e->sigTick, reinterpret_cast<void*>(&hookTick), reinterpret_cast<void**>(&g_origTick), "tick");
  g_hooked[1] = install(e->sigReach, reinterpret_cast<void*>(&hookReach), reinterpret_cast<void**>(&g_origReach), "reach");
  g_hooked[2] = install(e->sigSend, reinterpret_cast<void*>(&hookSend), reinterpret_cast<void**>(&g_origSend), "send");
  return (g_hooked[0] && g_hooked[1] && g_hooked[2]) ? InitStatus::Ok : InitStatus::HookFailed;
}

}  // namespace fk
