#include "probe.h"
#include <link.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace fk {
namespace {
struct Seg { uintptr_t lo, hi; };
struct Ctx {
  std::string libs;
  uintptr_t base = 0;
  std::vector<Seg> segs;
};

int cb(struct dl_phdr_info* info, size_t, void* data) {
  Ctx* c = static_cast<Ctx*>(data);
  const char* name = info->dlpi_name ? info->dlpi_name : "";
  if (!std::strstr(name, "com.mojang.minecraftpe")) return 0;
  const char* slash = std::strrchr(name, '/');
  c->libs += std::string(slash ? slash + 1 : name) + " ";
  if (std::strstr(name, "libminecraftpe.so")) {
    c->base = info->dlpi_addr;
    for (int i = 0; i < info->dlpi_phnum; ++i) {
      const ElfW(Phdr)& p = info->dlpi_phdr[i];
      if (p.p_type == PT_LOAD)
        c->segs.push_back({info->dlpi_addr + p.p_vaddr, info->dlpi_addr + p.p_vaddr + p.p_memsz});
    }
  }
  return 0;
}
}  // namespace

std::string probeMinecraft() {
  Ctx c;
  dl_iterate_phdr(cb, &c);
  std::string out = "Library Minecraft termuat: " + (c.libs.empty() ? std::string("(tidak ada)") : c.libs);
  if (!c.base) return out + "\nlibminecraftpe.so BELUM termuat.";

  char buf[96];
  std::snprintf(buf, sizeof buf, "\nlibminecraftpe.so base=0x%llx, segmen=%zu",
                static_cast<unsigned long long>(c.base), c.segs.size());
  out += buf;

  auto inSeg = [&](uintptr_t a, size_t n) {
    for (const Seg& s : c.segs)
      if (a >= s.lo && a + n <= s.hi) return true;
    return false;
  };

  const uintptr_t tab = c.base + 0x1331bf60;  // tabel nama ability, build 1.26.52.3
  static const int idx[] = {9, 10, 13, 14, 17};
  static const char* const want[] = {"flying", "mayfly", "flySpeed", "walkSpeed", "noclip"};
  int match = 0;
  out += "\nTabel ability di memori:";
  for (int k = 0; k < 5; ++k) {
    std::string name = "?";
    const uintptr_t slot = tab + 8 * idx[k];
    if (inSeg(slot, 8)) {
      uintptr_t p;
      std::memcpy(&p, reinterpret_cast<const void*>(slot), sizeof p);
      if (inSeg(p, 1)) {
        const char* s = reinterpret_cast<const char*>(p);
        size_t n = 0;
        while (n < 24 && inSeg(p + n, 1) && s[n]) ++n;
        name.assign(s, n);
      }
    }
    if (name == want[k]) ++match;
    out += " [" + std::to_string(idx[k]) + "]=" + name;
  }
  out += match == 5 ? "\nHASIL: COCOK, build di memori sama dengan hasil analisis."
                    : "\nHASIL: TIDAK cocok (" + std::to_string(match) + "/5).";
  return out;
}
}  // namespace fk
