#include "elfinfo.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace fk {
namespace {

struct Map {
  const uint8_t* p = nullptr;
  size_t n = 0;
  ~Map() { if (p) munmap(const_cast<uint8_t*>(p), n); }
};

template <class T> bool rd(const Map& m, uint64_t off, T& v) {
  if (off > m.n || sizeof(T) > m.n - off) return false;
  std::memcpy(&v, m.p + off, sizeof(T));
  return true;
}

std::string cstr(const Map& m, uint64_t off, uint64_t limit) {
  std::string s;
  if (limit > m.n) limit = m.n;
  for (uint64_t i = off; i < limit && s.size() < 300; ++i) {
    char c = static_cast<char>(m.p[i]);
    if (!c) break;
    s.push_back((c >= 32 && c < 127) ? c : '?');
  }
  return s;
}

struct Sec { uint32_t name = 0, type = 0, link = 0; uint64_t off = 0, size = 0, entsize = 0; };

bool readSec(const Map& m, uint64_t shoff, uint16_t entsz, uint16_t i, Sec& s) {
  uint64_t b = shoff + uint64_t(i) * entsz;
  return rd(m, b, s.name) && rd(m, b + 4, s.type) && rd(m, b + 24, s.off) &&
         rd(m, b + 32, s.size) && rd(m, b + 40, s.link) && rd(m, b + 56, s.entsize);
}

}  // namespace

std::string elfReportDefault(const char* path) {
  return elfReport(path, {"LocalPlayer", "Abilities", "PlayerAuthInput", "MovePlayerPacket",
                          "sendNetworkPacket", "ClientInstance"});
}

std::string elfReport(const char* path, const std::vector<std::string>& keys) {
  int fd = open(path, O_RDONLY);
  if (fd < 0) return "ELF: gagal membuka file\n";
  struct stat st;
  if (fstat(fd, &st) != 0 || st.st_size < 64) { close(fd); return "ELF: file tidak valid\n"; }
  void* a = mmap(nullptr, size_t(st.st_size), PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  if (a == MAP_FAILED) return "ELF: mmap gagal\n";
  Map m; m.p = static_cast<const uint8_t*>(a); m.n = size_t(st.st_size);

  if (std::memcmp(m.p, "\x7f" "ELF", 4) != 0 || m.p[4] != 2 || m.p[5] != 1)
    return "ELF: bukan ELF64 little-endian\n";

  uint16_t machine = 0, shentsize = 0, shnum = 0, shstrndx = 0;
  uint64_t shoff = 0;
  if (!rd(m, 18, machine) || !rd(m, 40, shoff) || !rd(m, 58, shentsize) ||
      !rd(m, 60, shnum) || !rd(m, 62, shstrndx))
    return "ELF: header terpotong\n";
  if (shentsize < 64 || shnum == 0 || shnum > 4096 || shstrndx >= shnum)
    return "ELF: tabel section tidak valid (mungkin sengaja dihapus)\n";

  char buf[256];
  std::string r;
  std::snprintf(buf, sizeof buf, "ELF64 machine=%u (183=arm64) ukuran=%llu byte, %u section\n",
                machine, (unsigned long long)m.n, shnum);
  r += buf;

  Sec shstr;
  if (!readSec(m, shoff, shentsize, shstrndx, shstr)) return r + "ELF: shstrtab rusak\n";

  std::vector<Sec> secs(shnum);
  r += "Section: ";
  for (uint16_t i = 0; i < shnum; ++i) {
    if (!readSec(m, shoff, shentsize, i, secs[i])) { secs[i] = Sec(); continue; }
    std::string nm = cstr(m, shstr.off + secs[i].name, shstr.off + shstr.size);
    if (!nm.empty()) { r += nm; r += ' '; }
  }
  r += "\n";

  std::vector<int> counts(keys.size(), 0);
  std::vector<std::string> lines(keys.size());
  for (uint16_t i = 0; i < shnum; ++i) {
    const Sec& s = secs[i];
    if ((s.type != 2 && s.type != 11) || s.entsize != 24 || s.link >= shnum) continue;
    const Sec& str = secs[s.link];
    uint64_t count = s.size / 24, defined = 0;
    for (uint64_t k = 0; k < count; ++k) {
      uint64_t b = s.off + k * 24;
      uint32_t nameOff = 0; uint16_t shndx = 0; uint64_t value = 0, size = 0;
      if (!rd(m, b, nameOff) || !rd(m, b + 6, shndx) || !rd(m, b + 8, value) || !rd(m, b + 16, size)) break;
      if (shndx == 0) continue;  // simbol impor, bukan definisi
      ++defined;
      std::string name = cstr(m, str.off + nameOff, str.off + str.size);
      for (size_t q = 0; q < keys.size(); ++q) {
        if (name.find(keys[q]) == std::string::npos) continue;
        if (++counts[q] <= 15) {
          std::snprintf(buf, sizeof buf, "  0x%llx size=%llu ", (unsigned long long)value, (unsigned long long)size);
          lines[q] += buf; lines[q] += name.substr(0, 160); lines[q] += "\n";
        }
      }
    }
    std::snprintf(buf, sizeof buf, "%s: %llu entri, %llu terdefinisi\n",
                  s.type == 11 ? ".dynsym" : ".symtab", (unsigned long long)count, (unsigned long long)defined);
    r += buf;
  }
  for (size_t q = 0; q < keys.size(); ++q) {
    std::snprintf(buf, sizeof buf, "[%s] cocok=%d\n", keys[q].c_str(), counts[q]);
    r += buf; r += lines[q];
  }
  return r;
}

}  // namespace fk
