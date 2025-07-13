
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
