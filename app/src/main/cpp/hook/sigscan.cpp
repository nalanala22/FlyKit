#include "sigscan.h"
#include <link.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

namespace fk {

bool parsePattern(const std::string& text, Pattern& out) {
  out.bytes.clear(); out.mask.clear();
  std::istringstream ss(text);
  std::string tok;
  while (ss >> tok) {
    if (tok == "?" || tok == "??") { out.bytes.push_back(0); out.mask.push_back(false); continue; }
    if (tok.size() != 2) return false;
    char* end = nullptr;
    long v = std::strtol(tok.c_str(), &end, 16);
    if (*end != '\0' || v < 0 || v > 255) return false;
    out.bytes.push_back(uint8_t(v)); out.mask.push_back(true);
  }
  return !out.bytes.empty();
}

const uint8_t* scanBuffer(const uint8_t* buf, size_t len, const Pattern& p) {
  const size_t n = p.bytes.size();
  if (n == 0 || len < n) return nullptr;
  for (size_t i = 0; i + n <= len; ++i) {
    size_t j = 0;
    while (j < n && (!p.mask[j] || buf[i + j] == p.bytes[j])) ++j;
    if (j == n) return buf + i;
  }
  return nullptr;
}

namespace {
struct FindCtx { const char* module; const Pattern* pat; uintptr_t found; };
int findCb(struct dl_phdr_info* info, size_t, void* d) {
  FindCtx* c = static_cast<FindCtx*>(d);
  if (!info->dlpi_name || !std::strstr(info->dlpi_name, c->module)) return 0;
  for (int i = 0; i < info->dlpi_phnum; ++i) {
    const ElfW(Phdr)& p = info->dlpi_phdr[i];
    if (p.p_type != PT_LOAD || !(p.p_flags & PF_X) || !(p.p_flags & PF_R)) continue;
    const uint8_t* lo = reinterpret_cast<const uint8_t*>(info->dlpi_addr + p.p_vaddr);
    const uint8_t* hit = scanBuffer(lo, static_cast<size_t>(p.p_filesz), *c->pat);
    if (hit) { c->found = reinterpret_cast<uintptr_t>(hit); return 1; }
  }
  return 0;
}
}  // namespace

// Memakai dl_iterate_phdr (bukan /proc/self/maps) agar bekerja pada library yang dipetakan langsung dari APK.
uintptr_t findInModule(const char* module, const Pattern& p) {
  FindCtx c{module, &p, 0};
  dl_iterate_phdr(findCb, &c);
  return c.found;
}
}  // namespace fk
