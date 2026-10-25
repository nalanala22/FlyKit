#pragma once
#include <string>
#include <vector>
namespace fk {
// Laporan ringkas ELF64 (arsitektur, section, simbol yang cocok kata kunci). Hanya baca, aman.
std::string elfReport(const char* path, const std::vector<std::string>& keys);
std::string elfReportDefault(const char* path);
}  // namespace fk
