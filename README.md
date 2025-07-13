
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

## English Version
# 🎙️ EduBot – Intelligent Quiz Robot Based on ChatGPT

![PlatformIO](https://img.shields.io/badge/development%20env-PlatformIO-orange.svg)
![Framework](https://img.shields.io/badge/framework-Arduino-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32--S3-blue.svg)
![License](https://img.shields.io/github/license/VeroPutra/ESP32S3-Voice-to-Voice-AI-Assistant)
![Made with Arduino](https://img.shields.io/badge/made%20with-Arduino-1abc9c.svg)
![Status](https://img.shields.io/badge/status-beta-yellow)

**EduBot** is a **voice-based interactive educational assistant** designed specifically for elementary school students. Powered by **ESP32-S3**, **INMP441 microphone**, and **MAX98357A speaker**, EduBot integrates **ChatGPT**, **Text-to-Speech (TTS)**, and **Speech-to-Text (STT)** technologies to create quiz sessions that feel like interacting with a real teacher.

---

## 🎯 Project Goals

- Provide a fun and adaptive AI-based learning media.
- Help elementary students learn through voice-based quizzes.
- Improve learning motivation with real-time scoring and feedback.

---

## 📚 Key Features

- 🎤 Voice input using I2S microphone (INMP441)
- 🧠 Natural interaction via ChatGPT API
- 🔊 High-quality voice output using Google TTS and MAX98357A
- 🎓 Quiz mode with multiple-choice questions and automatic evaluation
- 🔁 Real-time two-way voice interaction
- 🖥️ OLED display for visual assistance
- ⚙️ Fully standalone on ESP32-S3 (no computer required during use)

---

## 🧰 Hardware Components

| Component              | Description                              |
|------------------------|------------------------------------------|
| ESP32-S3 DevKitC-1     | Main microcontroller                     |
| INMP441                | Omnidirectional digital microphone (I2S) |
| MAX98357A              | I2S DAC Amplifier                        |
| Speaker 3W 4Ω          | Audio output                             |
| OLED 0.96” I2C         | System status display                    |
| Push Button            | Trigger for voice input                  |
| Power Supply           | USB Type-C                               |

---

## 🔌 ESP32-S3 Pinout

| Function         | GPIO |
|------------------|------|
| MIC DIN (INMP)   | 42   |
| MIC BCLK         | 41   |
| MIC WS           | 40   |
| SPK DIN (OUT)    | 18   |
| SPK BCLK         | 17   |
| SPK WS           | 16   |
| I2C SDA (OLED)   | 8    |
| I2C SCL (OLED)   | 9    |
| Push Button      | 0    |

---

## 🧪 System Architecture

### 🔁 Workflow

1. User presses the button to trigger recording.
2. ESP32 records audio from the INMP441 microphone via I2S.
3. Audio is sent to a Flask server for Speech-to-Text (STT).
4. The transcribed text is sent to ChatGPT → generates a question or feedback.
5. The response is sent to a TTS server and converted to MP3 audio.
6. ESP32 receives and plays the audio via speaker.

---

## 🧑‍🏫 Quiz Mode

- Detects keywords like “quiz”, “question”, “answer A/B/C/D”
- EduBot provides questions from subjects like Math, Science, etc.
- User answers by voice → system evaluates correctness
- Points and feedback are given instantly

---

## 💻 Installation

### 1. Clone the Repository

```bash
git clone https://github.com/VeroPutra/ESP32S3-Voice-to-Voice-AI-Assistant
```

### 2. Setup PlatformIO

- Open the project in [VSCode + PlatformIO](https://platformio.org/)
- Choose `esp32-s3-devkitc-1` as your board
- Flash the code to ESP32-S3 via USB Type-C

### 3. Run Local Servers (optional if not using cloud-based TTS/STT)

```bash
# STT Server
python stt_server.py

# TTS Server
python tts_server.py
```

---

## 📊 Testing Summary

| Aspect           | Result                                        |
|------------------|-----------------------------------------------|
| STT Accuracy     | 100% (distance range 5–60 cm)                 |
| Question Relevance | High (matched subject and education level) |
| TTS Quality      | Clear, understandable, good articulation      |
| Response Delay   | 11–28 seconds (affected by local network)     |

---

## 🧩 System Limitations

- STT & TTS still depend on local server (not on-device)
- Requires stable WiFi connection
- Only supports multiple-choice and short-answer quizzes
- Not yet tested directly with children (currently hypothetical)

---

## 💡 Future Improvements

- Integrate on-device STT/TTS or lightweight cloud-based APIs
- Support essay or open-ended question formats
- Conduct direct trials with elementary students
- Enhance user experience: slower speech, OLED icons, animations

---

## 📍 Research Location

Conducted at:
> Advanced Research Laboratory, 4th Floor, Room 4.13  
> Faculty of Engineering, Udayana University  
> June – July 2025

---

## 📚 Research References

- Kennedy et al., "A Social Robot for Storytelling to Children", IEEE RO-MAN 2019
- ACM (2021), "Child-Robot Interaction in Education"
