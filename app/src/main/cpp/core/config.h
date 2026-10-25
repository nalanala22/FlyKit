#pragma once
// Konfigurasi global. Semua field atomic -> aman dibaca dari thread game
// sementara UI menulis dari thread lain. Tidak ada handle/pointer native
// yang dipegang Java, jadi tidak ada risiko use-after-free di sisi JNI.
#include <atomic>
#include <string_view>

namespace fk {

constexpr float kMinReach = 3.0f, kMaxReach = 12.0f;
constexpr float kMinFlySpeed = 0.5f, kMaxFlySpeed = 5.0f;
constexpr int kMinJumps = 2, kMaxJumps = 4;          // total lompatan (tanah + udara)
constexpr int kMinBlinkTicks = 10, kMaxBlinkTicks = 200;  // 20 tick = 1 detik

struct Config {
  std::atomic<bool> fly{false};
  std::atomic<float> flySpeed{1.0f};
  std::atomic<bool> doubleJump{false};
  std::atomic<int> maxJumps{2};
  std::atomic<float> jumpImpulse{0.42f};
  std::atomic<bool> reach{false};
  std::atomic<float> reachDistance{6.0f};
  std::atomic<bool> blink{false};
  std::atomic<int> blinkMaxTicks{100};

  bool setBool(std::string_view key, bool v);
  bool setFloat(std::string_view key, float v);
  bool setInt(std::string_view key, int v);
  static Config& get();
};

}  // namespace fk
