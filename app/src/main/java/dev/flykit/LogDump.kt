package dev.flykit

/** Membaca log sistem milik aplikasi ini sendiri (UID yang sama) dan mengambil laporan crash native terakhir. */
object LogDump {
    private val keys = listOf(
        "pairip", "minecraft", "dlopen", "linker", "abort", "signal", "backtrace", "tombstone",
        "fatal", "killing", "died", "libc", "dev.flykit", "exception", "denied",
    )
    private val buildId = Regex(" \\(BuildId: [0-9a-f]+\\)")

    fun crashLines(): String {
        return try {
            val p = ProcessBuilder("logcat", "-d", "-b", "main,system,crash", "-t", "6000")
                .redirectErrorStream(true).start()
            val all = p.inputStream.bufferedReader().readLines()
            p.waitFor()

            val lastSig = all.indexOfLast { it.contains("Fatal signal") }
            if (lastSig >= 0) {
                // Laporan crash native terakhir: ambil dari awal (jenis sinyal + frame pertama).
                val win = all.subList(maxOf(0, lastSig - 3), minOf(all.size, lastSig + 260))
                val out = win
                    .filter { it.contains(" DEBUG ") || it.contains(" libc ") || it.lowercase().contains("pairip") }
                    .map { l -> buildId.replace(l.substringAfter(": ", l), "") }
                    .filter { l ->
                        val frame = l.trimStart().startsWith("#")
                        !(frame && (l.contains("libart.so") || l.contains("core-oj.jar") || l.contains("dalvik-classes")))
                    }
                return out.joinToString("\n").take(7000)
            }

            val picked = all.filter { line ->
                val l = line.lowercase()
                keys.any { l.contains(it) } || line.contains(" F ") || line.contains(" DEBUG")
            }.take(120)
            if (picked.isEmpty()) "Tidak ada laporan crash di log (buffer mungkin sudah tertimpa). " +
                "Ulangi: ketuk Uji loader, buka FlyKit lagi, lalu segera ketuk Ambil log crash."
            else picked.joinToString("\n").take(7000)
        } catch (t: Throwable) {
            "Gagal membaca log: $t"
        }
    }
}
