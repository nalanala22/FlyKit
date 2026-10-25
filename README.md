# FlyKit

Mod Minecraft Bedrock (Android) dari nol: **Fly, Double Jump, Reach, Blink**.
Gunakan di world sendiri / server yang mengizinkan; sebagian besar server melarang ini.

## Struktur
- `core/`  logika fitur murni C++ (teruji, tanpa dependensi Android)
- `hook/`  signature scanner + pemasang hook (backend: Dobby, opsional)
- `jni_bridge.cpp`  setelan UI -> native (atomic, tanpa handle native)
- `MainActivity.kt`  UI pengaturan (tanpa library tambahan)

## Tes logika (jalan di PC/sandbox)
    g++ -std=c++17 -fsanitize=address,undefined -o t tests/test_features.cpp \
        app/src/main/cpp/core/features.cpp app/src/main/cpp/hook/sigscan.cpp && ./t

## Yang BELUM ada (harus Anda isi)
1. `kVersions` di `hook/hooks.cpp` kosong: isi offset field LocalPlayer dan
   signature (tick, reach, send) dari `libminecraftpe.so` versi target. Selama
   kosong, semua fitur nonaktif dan `NativeBridge.init()` mengembalikan error.
2. Backend hook: build Dobby untuk arm64-v8a, lalu `-DFK_DOBBY_LIB=...`.
3. Loader: Minecraft harus berjalan di **proses yang sama** dengan library ini
   (seperti MC Toolbox asli memakai Activity turunan MainActivity). Itu bagian terpisah.
4. Konstanta ID paket (`kPktMovePlayer`, `kPktPlayerAuthInput`) harus diverifikasi
   untuk versi protokol target.

Build Gradle/Kotlin dan JNI belum dicoba di sandbox (tidak ada NDK/kotlinc);
`core/` dan `hooks.cpp` sudah dikompilasi dengan -Wall -Wextra.

## Mendapatkan APK (tanpa Android Studio)
1. Buat repo GitHub, unggah isi folder ini, push.
2. Tab **Actions** -> workflow "Build APK" -> unduh artifact `FlyKit-debug-apk`.
(Debug APK sudah ditandatangani otomatis, langsung bisa dipasang.)
