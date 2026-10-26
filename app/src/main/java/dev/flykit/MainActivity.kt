package dev.flykit

import android.app.Activity
import android.content.ClipData
import android.content.ClipboardManager
import android.graphics.Typeface
import android.widget.Toast
import android.content.Intent
import android.content.SharedPreferences
import android.os.Bundle
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.SeekBar
import android.widget.Switch
import android.widget.TextView

class MainActivity : Activity() {
    private lateinit var prefs: SharedPreferences
    private lateinit var exportStatus: TextView
    private val reqTree = 42

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        prefs = getSharedPreferences("flykit", MODE_PRIVATE)

        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            val p = (16 * resources.displayMetrics.density).toInt()
            setPadding(p, p, p, p)
        }
        root.addView(TextView(this).apply {
            text = if (NativeBridge.loaded) "Library native dimuat" else "Library native GAGAL dimuat"
        })
        val mc = McEnvironment.query(this)
        root.addView(TextView(this).apply {
            text = if (mc == null) "Minecraft tidak terpasang"
            else "Minecraft ${mc.versionName} (kode ${mc.versionCode}), ${mc.apkPaths.size} file APK"
        })
        root.addView(toggle("Fly", "fly"))
        root.addView(slider("Kecepatan fly", "fly_speed", 0.5f, 5f, 1f, false))
        root.addView(toggle("Double jump", "double_jump"))
        root.addView(slider("Total lompatan", "max_jumps", 2f, 4f, 2f, true))
        root.addView(toggle("Reach", "reach"))
        root.addView(slider("Jarak reach", "reach_distance", 3f, 12f, 6f, false))
        root.addView(toggle("Blink", "blink"))
        root.addView(slider("Batas tahan blink (tick)", "blink_max_ticks", 10f, 200f, 100f, true))

        val reportView = TextView(this).apply {
            setTextIsSelectable(true)
            textSize = 11f
            typeface = Typeface.MONOSPACE
        }
        root.addView(Button(this).apply {
            text = "Buat laporan library Minecraft"
            setOnClickListener {
                isEnabled = false
                reportView.text = "Memproses, tunggu sebentar..."
                Thread {
                    val r = try { LibReport.build(this@MainActivity) } catch (t: Throwable) { "Error: $t" }
                    runOnUiThread {
                        reportView.text = r
                        isEnabled = true
                        val cm = getSystemService(CLIPBOARD_SERVICE) as ClipboardManager
                        cm.setPrimaryClip(ClipData.newPlainText("laporan", r))
                        Toast.makeText(this@MainActivity, "Laporan disalin ke clipboard", Toast.LENGTH_SHORT).show()
                    }
                }.start()
            }
        })
        root.addView(reportView)

        exportStatus = TextView(this)
        root.addView(Button(this).apply {
            text = "Ekspor library Minecraft"
            isEnabled = mc != null
            setOnClickListener { startActivityForResult(Intent(Intent.ACTION_OPEN_DOCUMENT_TREE), reqTree) }
        })
        root.addView(exportStatus)

        val probeView = TextView(this).apply {
            setTextIsSelectable(true)
            textSize = 11f
            typeface = Typeface.MONOSPACE
            val prev = LoaderProbe.lastLog(this@MainActivity)
            text = if (prev.isBlank()) "" else "Hasil uji loader sebelumnya:\n$prev"
        }
        root.addView(Button(this).apply {
            text = "Uji loader (muat kode Minecraft)"
            isEnabled = mc != null
            setOnClickListener {
                isEnabled = false
                probeView.text = "Menguji... Jika aplikasi menutup sendiri, buka lagi: tahap terakhir tampil di sini."
                Thread {
                    val r = try { LoaderProbe.run(this@MainActivity) } catch (t: Throwable) { "Error: $t" }
                    runOnUiThread {
                        probeView.text = r
                        isEnabled = true
                        val cm = getSystemService(CLIPBOARD_SERVICE) as ClipboardManager
                        cm.setPrimaryClip(ClipData.newPlainText("uji loader", r))
                        Toast.makeText(this@MainActivity, "Hasil disalin ke clipboard", Toast.LENGTH_SHORT).show()
                    }
                }.start()
            }
        })
        root.addView(probeView)

        setContentView(ScrollView(this).apply { addView(root) })
    }

    @Deprecated("Deprecated in Java")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode != reqTree || resultCode != RESULT_OK) return
        val tree = data?.data ?: return
        val mc = McEnvironment.query(this) ?: return
        exportStatus.text = "Mengekspor... (bisa beberapa menit, jangan tutup aplikasi)"
        Thread {
            val msg = try {
                LibExporter.export(this, mc, tree)
            } catch (e: Exception) {
                "Gagal: ${e.message}"
            }
            runOnUiThread { exportStatus.text = msg }
        }.start()
    }

    private fun toggle(label: String, key: String) = Switch(this).apply {
        text = label
        isChecked = prefs.getBoolean(key, false)
        NativeBridge.setBool(key, isChecked)
        setOnCheckedChangeListener { _, checked ->
            prefs.edit().putBoolean(key, checked).apply()
            NativeBridge.setBool(key, checked)
        }
    }

    private fun slider(label: String, key: String, min: Float, max: Float, def: Float, isInt: Boolean): LinearLayout {
        val box = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
        val title = TextView(this)
        val bar = SeekBar(this).apply { this.max = 100 }

        fun fmt(v: Float) = if (isInt) v.toInt().toString() else String.format("%.1f", v)
        fun apply(v: Float) {
            val value = v.coerceIn(min, max)
            title.text = "$label: ${fmt(value)}"
            if (isInt) {
                prefs.edit().putInt(key, value.toInt()).apply()
                NativeBridge.setInt(key, value.toInt())
            } else {
                prefs.edit().putFloat(key, value).apply()
                NativeBridge.setFloat(key, value)
            }
        }

        val saved = if (isInt) prefs.getInt(key, def.toInt()).toFloat() else prefs.getFloat(key, def)
        bar.progress = (((saved - min) / (max - min)) * 100f).toInt().coerceIn(0, 100)
        apply(saved)
        bar.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(sb: SeekBar?, progress: Int, fromUser: Boolean) {
                if (fromUser) apply(min + (max - min) * progress / 100f)
            }
            override fun onStartTrackingTouch(sb: SeekBar?) {}
            override fun onStopTrackingTouch(sb: SeekBar?) {}
        })
        box.addView(title); box.addView(bar)
        return box
    }
}
