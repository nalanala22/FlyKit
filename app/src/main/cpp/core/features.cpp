#include "features.h"
#include <cmath>

namespace fk {

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

Config& Config::get() { static Config c; return c; }

bool Config::setBool(std::string_view k, bool v) {
  if (k == "fly") fly = v;
  else if (k == "double_jump") doubleJump = v;
  else if (k == "reach") reach = v;
  else if (k == "blink") blink = v;
  else return false;
  return true;
}
bool Config::setFloat(std::string_view k, float v) {
  if (!std::isfinite(v)) return false;
  if (k == "fly_speed") flySpeed = clampf(v, kMinFlySpeed, kMaxFlySpeed);
  else if (k == "jump_impulse") jumpImpulse = clampf(v, 0.2f, 0.8f);
  else if (k == "reach_distance") reachDistance = clampf(v, kMinReach, kMaxReach);
  else return false;
  return true;
}
bool Config::setInt(std::string_view k, int v) {
  if (k == "max_jumps") maxJumps = clampi(v, kMinJumps, kMaxJumps);
  else if (k == "blink_max_ticks") blinkMaxTicks = clampi(v, kMinBlinkTicks, kMaxBlinkTicks);
  else return false;
  return true;
}

// ---- Fly ----
void FlyFeature::apply(const Config& c, PlayerState& s) {
  if (c.fly.load()) {
    if (!forcing_) { forcing_ = true; original_ = s.mayFly; }
    s.mayFly = true;
  } else if (forcing_) {
    forcing_ = false;
    s.mayFly = original_;
  }
}
float FlyFeature::speed(const Config& c, float base) const {
  return c.fly.load() ? base * c.flySpeed.load() : base;
}

// ---- Double jump ----
void DoubleJump::tick(const Config& c, PlayerState& s) {
  const bool edge = s.jumpPressed && !prevJump_;
  prevJump_ = s.jumpPressed;
  if (!c.doubleJump.load() || s.onGround) { airJumps_ = 0; return; }
  if (edge && airJumps_ < c.maxJumps.load() - 1) {
    s.velY = c.jumpImpulse.load();
    ++airJumps_;
  }
}

// ---- Reach ----
float reachDistance(const Config& c, float original) {
  if (!c.reach.load()) return original;
  float want = clampf(c.reachDistance.load(), kMinReach, kMaxReach);
  return want > original ? want : original;
}

// ---- Blink ----
bool isMovementPacket(uint32_t id) { return id == kPktMovePlayer || id == kPktPlayerAuthInput; }

uint32_t parsePacketId(const uint8_t* d, size_t len) {
  uint32_t v = 0;
  for (size_t i = 0; i < len && i < 5; ++i) {
    v |= uint32_t(d[i] & 0x7F) << (7 * i);
    if (!(d[i] & 0x80)) return v & 0x3FF;
  }
  return 0xFFFFFFFFu;  // header rusak -> bukan paket gerak
}

void BlinkQueue::flushLocked() {
  while (!q_.empty()) { send_(q_.front()); q_.pop_front(); }
}
bool BlinkQueue::intercept(const Config& c, const Packet& p) {
  std::lock_guard<std::mutex> l(m_);
  if (!c.blink.load()) {
    if (active_) { flushLocked(); active_ = false; held_ = 0; }  // jaga urutan
    return false;
  }
  if (!isMovementPacket(p.id)) return false;
  active_ = true;
  q_.push_back(p);
  if (q_.size() >= 4096) flushLocked();  // batas memori
  return true;
}
void BlinkQueue::tick(const Config& c) {
  std::lock_guard<std::mutex> l(m_);
  if (c.blink.load()) {
    active_ = true;
    if (++held_ >= c.blinkMaxTicks.load()) { flushLocked(); held_ = 0; }  // cegah timeout server
  } else if (active_) {
    flushLocked(); active_ = false; held_ = 0;
  }
}
size_t BlinkQueue::pending() const { std::lock_guard<std::mutex> l(m_); return q_.size(); }

}  // namespace fk
