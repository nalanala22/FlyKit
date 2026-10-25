#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace fk {
struct Pattern { std::vector<uint8_t> bytes; std::vector<bool> mask; };  // mask=false -> wildcard

// "FD 7B ?? A9" -> Pattern. false jika format salah/kosong.
bool parsePattern(const std::string& text, Pattern& out);
const uint8_t* scanBuffer(const uint8_t* buf, size_t len, const Pattern& p);
// Cari pola di segmen readable+executable modul (mis. "libminecraftpe.so"). 0 jika tidak ketemu.
uintptr_t findInModule(const char* moduleName, const Pattern& p);
}  // namespace fk
