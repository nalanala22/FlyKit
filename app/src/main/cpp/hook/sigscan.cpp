#include "sigscan.h"
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

uintptr_t findInModule(const char* module, const Pattern& p) {
  FILE* f = std::fopen("/proc/self/maps", "r");
  if (!f) return 0;
  char line[512];
  uintptr_t found = 0;
  while (!found && std::fgets(line, sizeof line, f)) {
    if (!std::strstr(line, module)) continue;
    unsigned long long lo = 0, hi = 0;
    char perms[8] = {0};
    if (std::sscanf(line, "%llx-%llx %7s", &lo, &hi, perms) != 3) continue;
    if (perms[0] != 'r' || perms[2] != 'x') continue;
    const uint8_t* hit = scanBuffer(reinterpret_cast<const uint8_t*>(lo), size_t(hi - lo), p);
    if (hit) found = reinterpret_cast<uintptr_t>(hit);
  }
  std::fclose(f);
  return found;
}
}  // namespace fk
