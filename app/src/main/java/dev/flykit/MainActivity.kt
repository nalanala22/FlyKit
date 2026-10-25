package dev.flykit

import android.app.Activity
import android.content.SharedPreferences
import android.os.Bundle
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.SeekBar
import android.widget.Switch
import android.widget.TextView

class MainActivity : Activity() {
    private lateinit var prefs: SharedPreferences

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

        setContentView(ScrollView(this).apply { addView(root) })
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
