package dev.flykit

/** Membaca log sistem milik aplikasi ini sendiri (UID yang sama) dan menyaring baris yang relevan dengan crash. */
object LogDump {
    private val keys = listOf(
        "pairip", "minecraft", "dlopen", "linker", "abort", "signal", "backtrace", "tombstone",
        "fatal", "killing", "died", "libc", "dev.flykit", "exception", "denied",
    )

    fun crashLines(): String {
        return try {
            val p = ProcessBuilder("logcat", "-d", "-b", "main,system,crash", "-t", "4000")
                .redirectErrorStream(true).start()
            val all = p.inputStream.bufferedReader().readLines()
            p.waitFor()
            val picked = all.filter { line ->
                val l = line.lowercase()
                keys.any { l.contains(it) } || line.contains(" F ") || line.contains(" DEBUG")
            }.takeLast(120)
            if (picked.isEmpty()) "Tidak ada baris relevan di log (buffer mungkin sudah tertimpa). " +
                "Ulangi: ketuk Uji loader, buka FlyKit lagi, lalu segera ketuk Ambil log crash."
            else picked.joinToString("\n").takeLast(9000)
        } catch (t: Throwable) {
            "Gagal membaca log: $t"
        }
    }
}
