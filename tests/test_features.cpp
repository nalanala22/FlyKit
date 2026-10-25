#include <cassert>
#include <cmath>
#include <cstdio>
#include "../app/src/main/cpp/core/features.h"
#include "../app/src/main/cpp/hook/sigscan.h"
using namespace fk;

static void testConfig() {
  Config c;
  assert(c.setFloat("reach_distance", 99) && c.reachDistance == kMaxReach);
  assert(c.setFloat("reach_distance", 0) && c.reachDistance == kMinReach);
  assert(c.setInt("max_jumps", 99) && c.maxJumps == kMaxJumps);
  assert(c.setInt("blink_max_ticks", 1) && c.blinkMaxTicks == kMinBlinkTicks);
  assert(!c.setFloat("fly_speed", NAN));
  assert(!c.setBool("unknown", true));
}
static void testFly() {
  Config c; FlyFeature f; PlayerState s;
  s.mayFly = false;
  c.fly = true; f.apply(c, s); assert(s.mayFly);
  c.fly = false; f.apply(c, s); assert(!s.mayFly);           // dikembalikan
  PlayerState s2; s2.mayFly = true;                           // creative: tetap true
  c.fly = true; f.apply(c, s2); c.fly = false; f.apply(c, s2); assert(s2.mayFly);
  c.flySpeed = 2.f; c.fly = true; assert(f.speed(c, 0.05f) == 0.1f);
  c.fly = false; assert(f.speed(c, 0.05f) == 0.05f);
}
static void testDoubleJump() {
  Config c; c.doubleJump = true; DoubleJump d; PlayerState s;
  s.onGround = true; s.jumpPressed = true; d.tick(c, s); assert(s.velY == 0.f);   // lompat tanah = urusan game
  s.onGround = false; s.jumpPressed = false; d.tick(c, s);
  s.jumpPressed = true; d.tick(c, s); assert(s.velY == c.jumpImpulse);            // lompat udara 1
  s.velY = 0; s.jumpPressed = false; d.tick(c, s);
  s.jumpPressed = true; d.tick(c, s); assert(s.velY == 0.f);                      // maxJumps=2 -> habis
  s.onGround = true; s.jumpPressed = false; d.tick(c, s);                         // mendarat -> reset
  s.onGround = false; d.tick(c, s); s.jumpPressed = true; d.tick(c, s); assert(s.velY == c.jumpImpulse);
  s.velY = 0; s.jumpPressed = true; d.tick(c, s); assert(s.velY == 0.f);          // ditahan != edge baru
  c.doubleJump = false; s.jumpPressed = false; d.tick(c, s); s.jumpPressed = true; d.tick(c, s); assert(s.velY == 0.f);
  c.doubleJump = true; c.maxJumps = 4; DoubleJump d2; PlayerState t; t.onGround = false;
  int n = 0;
  for (int i = 0; i < 6; ++i) { t.jumpPressed = false; d2.tick(c, t); t.velY = 0; t.jumpPressed = true; d2.tick(c, t); n += t.velY > 0; }
  assert(n == 3);
}
static void testReach() {
  Config c;
  assert(reachDistance(c, 3.f) == 3.f);
  c.reach = true; c.reachDistance = 6.f; assert(reachDistance(c, 3.f) == 6.f);
  assert(reachDistance(c, 8.f) == 8.f);                                           // tidak memperkecil
}
static void testBlink() {
  Config c; std::vector<uint32_t> sent;
  BlinkQueue q([&](const Packet& p) { sent.push_back(p.id); });
  Packet mv; mv.id = kPktPlayerAuthInput; Packet other; other.id = 0x09;
  assert(!q.intercept(c, mv));                                                    // blink off -> lewat
  c.blink = true;
  assert(q.intercept(c, mv)); assert(!q.intercept(c, other)); assert(q.pending() == 1);
  Packet mv2; mv2.id = kPktMovePlayer; assert(q.intercept(c, mv2));
  c.blink = false; q.tick(c);
  assert(sent.size() == 2 && sent[0] == kPktPlayerAuthInput && sent[1] == kPktMovePlayer && q.pending() == 0);
  sent.clear(); c.blink = true; c.blinkMaxTicks = kMinBlinkTicks;                 // timeout
  q.intercept(c, mv);
  for (int i = 0; i < kMinBlinkTicks; ++i) q.tick(c);
  assert(sent.size() == 1 && q.pending() == 0);
  sent.clear(); q.intercept(c, mv); c.blink = false;                              // intercept saat off men-flush dulu
  assert(!q.intercept(c, other) && sent.size() == 1);
  uint8_t h1[] = {0x90, 0x01}; assert(parsePacketId(h1, 2) == 0x90);              // varint 144
  uint8_t h2[] = {0x13}; assert(parsePacketId(h2, 1) == 0x13);
  uint8_t bad[] = {0x80, 0x80, 0x80, 0x80, 0x80}; assert(parsePacketId(bad, 5) == 0xFFFFFFFFu);
}
static void testSigScan() {
  Pattern p; assert(parsePattern("DE ?? BE EF", p) && p.bytes.size() == 4);
  uint8_t buf[] = {0, 1, 0xDE, 0x77, 0xBE, 0xEF, 9};
  assert(scanBuffer(buf, sizeof buf, p) == buf + 2);
  uint8_t miss[] = {0xDE, 0x77, 0xBE, 0xEE};
  assert(!scanBuffer(miss, sizeof miss, p));
  assert(!parsePattern("", p) && !parsePattern("ZZ", p) && !parsePattern("123", p));
}
int main() {
  testConfig(); testFly(); testDoubleJump(); testReach(); testBlink(); testSigScan();
  std::puts("SEMUA TES LULUS");
}
