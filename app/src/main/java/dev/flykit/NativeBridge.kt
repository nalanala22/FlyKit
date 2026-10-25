package dev.flykit

/** Jembatan tipis ke library native. Tidak menyimpan handle native -> tidak ada finalize()/use-after-free. */
object NativeBridge {
    @Volatile
    var loaded: Boolean = false
        private set

    init {
        loaded = try {
            System.loadLibrary("flykit")
            true
        } catch (e: UnsatisfiedLinkError) {
            false
        }
    }

    const val OK = 0
    const val UNSUPPORTED_VERSION = 1
    const val HOOK_FAILED = 2

    fun init(mcVersion: String): Int = if (loaded) nativeInit(mcVersion) else HOOK_FAILED
    fun setBool(key: String, v: Boolean): Boolean = loaded && nativeSetBool(key, v)
    fun setFloat(key: String, v: Float): Boolean = loaded && nativeSetFloat(key, v)
    fun setInt(key: String, v: Int): Boolean = loaded && nativeSetInt(key, v)

    fun elfReport(path: String): String = if (loaded) nativeElfReport(path) else "library native tidak dimuat"

    @JvmStatic private external fun nativeElfReport(path: String): String
    @JvmStatic private external fun nativeInit(mcVersion: String): Int
    @JvmStatic private external fun nativeSetBool(key: String, v: Boolean): Boolean
    @JvmStatic private external fun nativeSetFloat(key: String, v: Float): Boolean
    @JvmStatic private external fun nativeSetInt(key: String, v: Int): Boolean
}
