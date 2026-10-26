package dev.flykit

import android.content.Context
import android.content.pm.PackageManager
import java.io.File
import java.util.Date

/**
 * Uji kelayakan loader: apakah kode Minecraft bisa dimuat di proses FlyKit?
 * Tidak menjalankan game dan tidak mengubah apa pun. Setiap tahap dicatat ke file
 * sebelum lanjut, jadi bila proses mati mendadak, tahap terakhir tetap terbaca.
 */
object LoaderProbe {
    private const val MAIN_CLASS = "com.mojang.minecraftpe.MainActivity"

    private fun logFile(ctx: Context) = File(ctx.filesDir, "loader_probe.log")

    fun lastLog(ctx: Context): String {
        val f = logFile(ctx)
        return if (f.exists()) f.readText() else ""
    }

    fun run(ctx: Context): String {
        val f = logFile(ctx)
        f.writeText("")
        fun log(s: String) { f.appendText(s + "\n") }

        log("Uji loader mulai: ${Date()}")
        val mcCtx = try {
            ctx.createPackageContext(
                McEnvironment.PKG,
                Context.CONTEXT_INCLUDE_CODE or Context.CONTEXT_IGNORE_SECURITY,
            )
        } catch (t: Throwable) {
            log("GAGAL tahap 1 (createPackageContext): ${describe(t)}")
            return f.readText()
        }
        log("OK tahap 1: konteks paket Minecraft dibuat")

        val cl = mcCtx.classLoader
        log("ClassLoader: $cl")
        val ai = mcCtx.applicationInfo
        log("Kelas Application Minecraft: ${ai.className ?: "(bawaan)"}")
        log("nativeLibraryDir: ${ai.nativeLibraryDir}")

        val cls = try {
            Class.forName(MAIN_CLASS, false, cl)
        } catch (t: Throwable) {
            log("GAGAL tahap 2 (muat kelas): ${describe(t)}")
            return f.readText()
        }
        log("OK tahap 2: kelas dimuat. Rantai induk: ${chain(cls)}")

        log(permissionDiff(ctx))

        log("Tahap 3: inisialisasi statis kelas (dapat memuat library native; bila aplikasi menutup sendiri, berhenti di sini)")
        try {
            Class.forName(MAIN_CLASS, true, cl)
            log("OK tahap 3: inisialisasi statis selesai")
        } catch (t: Throwable) {
            log("GAGAL tahap 3: ${describe(t)}")
        }
        log("Tahap 4: pemeriksaan memori native")
        log(try { NativeBridge.probe() } catch (t: Throwable) { "GAGAL tahap 4: ${describe(t)}" })
        log("Selesai")
        return f.readText()
    }

    private fun permissionDiff(ctx: Context): String {
        return try {
            val pm = ctx.packageManager
            @Suppress("DEPRECATION")
            val mc = pm.getPackageInfo(McEnvironment.PKG, PackageManager.GET_PERMISSIONS).requestedPermissions
                ?: emptyArray()
            @Suppress("DEPRECATION")
            val ours = (pm.getPackageInfo(ctx.packageName, PackageManager.GET_PERMISSIONS).requestedPermissions
                ?: emptyArray()).toSet()
            val missing = mc.filter { it !in ours }
            "Izin diminta Minecraft: ${mc.size}. Belum ada di FlyKit: " +
                (if (missing.isEmpty()) "(tidak ada)" else missing.joinToString(", "))
        } catch (t: Throwable) {
            "Gagal membaca daftar izin: $t"
        }
    }

    private fun chain(c: Class<*>): String {
        val parts = ArrayList<String>()
        var k: Class<*>? = c
        while (k != null) {
            parts.add(k.name)
            k = k.superclass
        }
        return parts.joinToString(" -> ")
    }

    private fun describe(t: Throwable): String {
        val sb = StringBuilder()
        var cur: Throwable? = t
        var depth = 0
        while (cur != null && depth < 4) {
            if (depth > 0) sb.append("  disebabkan oleh: ")
            sb.append(cur.toString()).append('\n')
            for (el in cur.stackTrace.take(6)) sb.append("    at ").append(el).append('\n')
            cur = cur.cause
            depth++
        }
        return sb.toString().trimEnd()
    }
}
