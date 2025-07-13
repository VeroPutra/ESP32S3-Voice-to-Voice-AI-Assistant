
# 🎙️ EduBot – Robot Kuis Pintar Berbasis ChatGPT

![PlatformIO](https://img.shields.io/badge/development%20env-PlatformIO-orange.svg)
![Framework](https://img.shields.io/badge/framework-Arduino-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32--S3-blue.svg)
![License](https://img.shields.io/github/license/VeroPutra/ESP32S3-Voice-to-Voice-AI-Assistant)
![Made with Arduino](https://img.shields.io/badge/made%20with-Arduino-1abc9c.svg)
![Status](https://img.shields.io/badge/status-beta-yellow)

**EduBot** adalah sistem **asisten edukasi interaktif berbasis suara** yang dirancang khusus untuk siswa Sekolah Dasar. Menggunakan **ESP32-S3**, mikrofon **INMP441**, dan speaker **MAX98357A**, EduBot menggabungkan teknologi **ChatGPT** dan **Text-to-Speech (TTS)** serta **Speech-to-Text (STT)** untuk menciptakan sesi kuis seperti layaknya berinteraksi dengan seorang guru.

---

## 🎯 Tujuan Proyek

- Menyediakan media belajar berbasis AI yang menyenangkan dan adaptif.
- Mempermudah siswa SD belajar melalui kuis berbasis suara.
- Meningkatkan motivasi belajar dengan sistem skor dan umpan balik real-time.

---

## 📚 Fitur Utama

- 🎤 Input suara anak melalui mikrofon I2S (INMP441)
- 🧠 Interaksi natural menggunakan ChatGPT API
- 🔊 Output suara dengan kualitas tinggi via Google TTS dan MAX98357A
- 🎓 Mode kuis dengan soal pilihan ganda dan koreksi otomatis
- 🔁 Interaksi dua arah berbasis suara
- 🖥️ Tampilan OLED sebagai pendamping visual
- ⚙️ Semua berjalan mandiri di ESP32-S3 (tanpa komputer saat runtime)

---

## 🧰 Komponen Perangkat Keras

| Komponen              | Keterangan                             |
|----------------------|----------------------------------------|
| ESP32-S3 DevKitC-1   | Mikrokontroler utama                   |
| INMP441              | Mikrofon digital omnidirectional (I2S) |
| MAX98357A            | I2S DAC Amplifier                      |
| Speaker 3W 4Ω        | Output audio                           |
| OLED 0.96” I2C       | Tampilan informasi dan status sistem   |
| Tombol (Push Button) | Pemicu perekaman suara                |
| Catu daya            | USB Type-C                             |

---

## 🔌 Pinout ESP32-S3

| Fungsi          | GPIO |
|-----------------|------|
| MIC DIN (INMP)  | 42   |
| MIC BCLK        | 41   |
| MIC WS          | 40   |
| SPK DIN (OUT)   | 18   |
| SPK BCLK        | 17   |
| SPK WS          | 16   |
| I2C SDA (OLED)  | 8    |
| I2C SCL (OLED)  | 9    |
| Push Button     | 0    |

---

## 🧪 Arsitektur Sistem

### 🔁 Alur Kerja

1. Pengguna menekan tombol untuk memicu perekaman.
2. ESP32 merekam audio dari mikrofon (INMP441) via I2S.
3. Audio dikirim ke server lokal Flask → diproses ke teks (STT).
4. Hasil transkripsi dikirim ke ChatGPT → sistem mengirim soal atau merespons jawaban.
5. Balasan ChatGPT dikirim ke server Flask TTS → dikonversi jadi MP3.
6. ESP32 menerima dan memutar audio melalui speaker.

---

## 🧑‍🏫 Mode Kuis

- Deteksi kata kunci: “kuis”, “soal”, “jawaban A/B/C/D”
- EduBot memberikan soal dari mata pelajaran seperti Matematika, IPA, dll
- Pengguna menjawab secara lisan → sistem mengevaluasi jawaban
- Poin dan umpan balik langsung diberikan

---

## 💻 Instalasi

### 1. Clone Repositori

```bash
git clone https://github.com/VeroPutra/ESP32S3-Voice-to-Voice-AI-Assistant
```

### 2. Persiapkan PlatformIO

- Buka project di [VSCode + PlatformIO](https://platformio.org/)
- Pilih board `esp32-s3-devkitc-1`
- Flash program ke ESP32-S3 via USB Type-C

### 3. Jalankan Server Lokal (opsional jika tidak pakai cloud)

```bash
# Untuk STT
python stt_server.py

# Untuk TTS
python tts_server.py
```

---

## 📊 Hasil Pengujian

| Aspek             | Hasil Pengujian                             |
|-------------------|---------------------------------------------|
| Akurasi STT       | 100% (dalam jarak 5–60 cm)                  |
| Relevansi Soal    | Tinggi (sesuai topik dan jenjang SD)        |
| Kualitas TTS      | Baik, mudah dipahami, artikulasi jelas      |
| Delay Respons     | 11–28 detik (dipengaruhi jaringan lokal)    |

---

## 🧩 Keterbatasan Sistem

- STT & TTS masih bergantung pada server lokal (belum on-device)
- Koneksi WiFi wajib tersedia dan stabil
- Hanya mendukung soal pilihan ganda & isian singkat
- Belum diuji secara langsung oleh anak-anak (masih hipotesis)

---

## 💡 Saran Pengembangan

- Integrasi STT/TTS langsung di ESP32 (on-device/cloud lightweight)
- Penambahan jenis soal (esai, interaktif)
- Evaluasi langsung bersama siswa SD
- Perbaikan UX seperti suara lebih lambat dan ikon OLED menarik

---

## 📍 Lokasi Penelitian

Dilaksanakan di:
> Gedung Advanced Research Laboratory, Lt. 4, Ruang 4.13  
> Fakultas Teknik, Universitas Udayana  
> Juni – Juli 2025

---

## 📚 Referensi Penelitian

- Kennedy et al., "A Social Robot for Storytelling to Children", IEEE RO-MAN 2019
- ACM (2021), "Child-Robot Interaction in Education"
