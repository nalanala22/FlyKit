package dev.flykit

import android.content.Context
import android.net.Uri
import android.provider.DocumentsContract
import java.io.IOException
import java.io.OutputStream
import java.util.zip.GZIPOutputStream
import java.util.zip.ZipFile

/** Mengambil libminecraftpe.so dari APK Minecraft, dikompres gzip, disimpan per potongan ke folder pilihan pengguna. */
object LibExporter {
    private const val ENTRY = "lib/arm64-v8a/libminecraftpe.so"
    private const val PART_SIZE = 15L * 1024 * 1024

    private fun hasLib(path: String): Boolean = try {
        ZipFile(path).use { it.getEntry(ENTRY) != null }
    } catch (e: Exception) {
        false
    }

    fun export(ctx: Context, mc: McInfo, tree: Uri): String {
        val source = mc.apkPaths.firstOrNull { hasLib(it) }
            ?: return "Library arm64 tidak ditemukan di ${mc.apkPaths.size} file APK Minecraft."

        val parent = DocumentsContract.buildDocumentUriUsingTree(tree, DocumentsContract.getTreeDocumentId(tree))
        val split = SplitOut(ctx, parent, "libminecraftpe.so.gz", PART_SIZE)
        var originalSize = 0L
        ZipFile(source).use { z ->
            val e = z.getEntry(ENTRY)
            originalSize = e.size
            z.getInputStream(e).use { input ->
                val gz = GZIPOutputStream(split, 64 * 1024)
                input.copyTo(gz, 64 * 1024)
                gz.finish()
                gz.close()
            }
        }
        return "Selesai: ${split.parts} bagian disimpan (asli ${originalSize / 1024 / 1024} MB). " +
            "Unggah semua file libminecraftpe.so.gz.partXXX ke chat."
    }

    private class SplitOut(
        private val ctx: Context,
        private val parent: Uri,
        private val base: String,
        private val limit: Long,
    ) : OutputStream() {
        private var cur: OutputStream? = null
        private var written = 0L
        var parts = 0
            private set

        private fun next() {
            cur?.close()
            parts++
            val name = "%s.part%03d".format(base, parts)
            val doc = DocumentsContract.createDocument(ctx.contentResolver, parent, "application/octet-stream", name)
                ?: throw IOException("Tidak bisa membuat $name")
            cur = ctx.contentResolver.openOutputStream(doc)
                ?: throw IOException("Tidak bisa menulis $name")
            written = 0
        }

        override fun write(b: Int) {
            write(byteArrayOf(b.toByte()), 0, 1)
        }

        override fun write(b: ByteArray, off: Int, len: Int) {
            var o = off
            var l = len
            while (l > 0) {
                if (cur == null || written >= limit) next()
                val n = minOf(l.toLong(), limit - written).toInt()
                cur!!.write(b, o, n)
                written += n
                o += n
                l -= n
            }
        }

        override fun close() {
            cur?.close()
            cur = null
        }
    }
}
