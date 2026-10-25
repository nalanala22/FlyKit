#pragma once
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <vector>
#include "config.h"

namespace fk {

struct PlayerState {
  bool onGround = false;
  bool jumpPressed = false;
  float velY = 0.f;
  bool mayFly = false;
};

// Fly: paksa flag "boleh terbang"; kembalikan nilai asli saat dimatikan.
class FlyFeature {
 public:
  void apply(const Config& c, PlayerState& s);
  float speed(const Config& c, float base) const;
 private:
  bool forcing_ = false, original_ = false;
};

// Double jump: lompatan udara tambahan pada tepi (edge) tombol lompat.
class DoubleJump {
 public:
  void tick(const Config& c, PlayerState& s);
 private:
  int airJumps_ = 0;
  bool prevJump_ = false;
};

// Reach: tidak pernah memperkecil nilai asli, dan dibatasi kMaxReach.
float reachDistance(const Config& c, float original);

// Blink: tahan paket gerak, kirim semua berurutan saat dimatikan/timeout.
constexpr uint32_t kPktMovePlayer = 0x13;      // verifikasi untuk versi target
constexpr uint32_t kPktPlayerAuthInput = 0x90; // verifikasi untuk versi target

struct Packet {
  uint32_t id = 0;
  std::vector<uint8_t> bytes;
};

bool isMovementPacket(uint32_t id);
uint32_t parsePacketId(const uint8_t* data, size_t len);  // varint header & 0x3FF

class BlinkQueue {
 public:
  // Sender HARUS memanggil fungsi kirim ASLI (bukan hook), agar tidak re-entrant.
  using Sender = std::function<void(const Packet&)>;
  explicit BlinkQueue(Sender s) : send_(std::move(s)) {}
  bool intercept(const Config& c, const Packet& p);  // true = ditahan
  void tick(const Config& c);
  size_t pending() const;
 private:
  void flushLocked();
  mutable std::mutex m_;
  std::deque<Packet> q_;
  Sender send_;
  bool active_ = false;
  int held_ = 0;
};

}  // namespace fk
