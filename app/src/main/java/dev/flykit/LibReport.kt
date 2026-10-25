package dev.flykit

import android.content.Context
import android.os.Build
import java.io.File
import java.util.zip.ZipFile

/** Membaca libminecraftpe.so milik Minecraft yang terpasang (hanya baca, hasil berupa teks ringkas). */
object LibReport {
    private const val ENTRY = "lib/arm64-v8a/libminecraftpe.so"

    private fun extract(apk: String, out: File): Long {
        ZipFile(apk).use { z ->
            val e = z.getEntry(ENTRY) ?: return -1L
            z.getInputStream(e).use { i -> out.outputStream().use { o -> i.copyTo(o) } }
            return e.size
        }
    }

    fun build(ctx: Context): String {
        val mc = McEnvironment.query(ctx) ?: return "Minecraft tidak terpasang"
        val sb = StringBuilder()
        sb.append("Minecraft ${mc.versionName} (${mc.versionCode})\n")
        sb.append("Android ${Build.VERSION.RELEASE} SDK ${Build.VERSION.SDK_INT} ABI ${Build.SUPPORTED_ABIS.joinToString()}\n")
        val out = File(ctx.cacheDir, "libminecraftpe.so")
        var found = false
        for (p in mc.apkPaths) {
            try {
                val size = extract(p, out)
                if (size >= 0) {
                    sb.append("Library ada di ${File(p).name} ($size byte)\n")
                    found = true
                    break
                }
            } catch (e: Exception) {
                sb.append("Gagal membaca ${File(p).name}: ${e.message}\n")
            }
        }
        if (!found) {
            sb.append("libminecraftpe.so (arm64) tidak ditemukan di ${mc.apkPaths.size} APK\n")
            return sb.toString()
        }
        sb.append(NativeBridge.elfReport(out.absolutePath))
        out.delete()
        return sb.toString()
    }
}
