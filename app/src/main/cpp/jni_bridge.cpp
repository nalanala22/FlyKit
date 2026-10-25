#include <jni.h>
#include <string>
#include "core/config.h"
#include "hook/hooks.h"
#include "hook/elfinfo.h"

namespace {
// Salin jstring -> std::string dengan aman (null / OOM tidak membuat crash).
bool toStd(JNIEnv* env, jstring s, std::string& out) {
  if (!s) return false;
  const char* c = env->GetStringUTFChars(s, nullptr);
  if (!c) return false;
  out.assign(c);
  env->ReleaseStringUTFChars(s, c);
  return true;
}
}  // namespace

extern "C" {

JNIEXPORT jint JNICALL Java_dev_flykit_NativeBridge_nativeInit(JNIEnv* env, jclass, jstring ver) {
  std::string v;
  if (!toStd(env, ver, v)) return static_cast<jint>(fk::InitStatus::UnsupportedVersion);
  return static_cast<jint>(fk::installHooks(v.c_str()));
}
JNIEXPORT jboolean JNICALL Java_dev_flykit_NativeBridge_nativeSetBool(JNIEnv* env, jclass, jstring k, jboolean v) {
  std::string key; return toStd(env, k, key) && fk::Config::get().setBool(key, v == JNI_TRUE);
}
JNIEXPORT jboolean JNICALL Java_dev_flykit_NativeBridge_nativeSetFloat(JNIEnv* env, jclass, jstring k, jfloat v) {
  std::string key; return toStd(env, k, key) && fk::Config::get().setFloat(key, v);
}
JNIEXPORT jboolean JNICALL Java_dev_flykit_NativeBridge_nativeSetInt(JNIEnv* env, jclass, jstring k, jint v) {
  std::string key; return toStd(env, k, key) && fk::Config::get().setInt(key, v);
}
JNIEXPORT jstring JNICALL Java_dev_flykit_NativeBridge_nativeElfReport(JNIEnv* env, jclass, jstring path) {
  std::string p;
  if (!toStd(env, path, p)) return env->NewStringUTF("path kosong\n");
  return env->NewStringUTF(fk::elfReportDefault(p.c_str()).c_str());
}
}  // extern "C"
