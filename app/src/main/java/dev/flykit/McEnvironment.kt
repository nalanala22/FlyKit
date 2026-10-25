package dev.flykit

import android.content.Context
import android.content.pm.PackageManager
import android.os.Build

data class McInfo(
    val versionName: String,
    val versionCode: Long,
    val apkPaths: List<String>,   // base + split (library arm64 biasanya ada di split_config.*)
    val nativeLibDir: String?,
)

/** Mendeteksi Minecraft yang terpasang. Dasar untuk loader (classloader + library native). */
object McEnvironment {
    const val PKG = "com.mojang.minecraftpe"

    fun query(ctx: Context): McInfo? {
        val pm = ctx.packageManager
        val pi = try {
            if (Build.VERSION.SDK_INT >= 33) pm.getPackageInfo(PKG, PackageManager.PackageInfoFlags.of(0))
            else @Suppress("DEPRECATION") pm.getPackageInfo(PKG, 0)
        } catch (e: PackageManager.NameNotFoundException) {
            return null
        }
        val ai = pi.applicationInfo ?: return null
        val paths = ArrayList<String>()
        ai.sourceDir?.let { paths.add(it) }
        ai.splitSourceDirs?.let { paths.addAll(it) }
        val code = if (Build.VERSION.SDK_INT >= 28) pi.longVersionCode
        else @Suppress("DEPRECATION") pi.versionCode.toLong()
        return McInfo(pi.versionName ?: "", code, paths, ai.nativeLibraryDir)
    }
}
