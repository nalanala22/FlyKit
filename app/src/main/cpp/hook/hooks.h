#pragma once
#include "../core/features.h"
namespace fk {
enum class InitStatus : int { Ok = 0, UnsupportedVersion = 1, HookFailed = 2 };
InitStatus installHooks(const char* mcVersion);
BlinkQueue& blinkQueue();
}
