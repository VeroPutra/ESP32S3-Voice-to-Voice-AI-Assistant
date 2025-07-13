#include <WiFi.h>
#include <HTTPClient.h>
#include <driver/i2s.h>
#include <ArduinoJson.h>
#include "AudioFileSourceHTTPStream.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ==== OLED CONFIGURATION ====
#define OLED_SDA 12
#define OLED_SCL 11
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ==== KONFIGURASI WIFI ====
const char* ssid = "Agni Udayana1";
const char* password = "agnijuara";

// ==== KONFIGURASI SERVER ====
const char* sttServerURL = "http://192.168.88.240:2401/upload"; // STT Server
const char* backend_host = "http://192.168.88.240:2400"; // TTS Backend
const String openai_key = "your_openai_api_key"; // Ganti dengan API Key OpenAI Anda
const String openai_url = "https://api.openai.com/v1/chat/completions";
const String model = "gpt-4o-mini";
const int TTS_CHAR_LIMIT = 1000;

// ==== PIN CONFIGURATION ====
// I2S pins untuk INMP441 (STT - Input)
#define I2S_WS_IN 16    // Word Select (LRCK)
#define I2S_SD_IN 17    // Serial Data (DOUT)
#define I2S_SCK_IN 15   // Serial Clock (BCLK)

// I2S pins untuk Speaker (TTS - Output)
#define I2S_DOUT  21
#define I2S_BCLK  47  
#define I2S_LRC   48

// Button pin
#define BUTTON_PIN 41

// ==== AUDIO CONFIGURATION ====
#define SAMPLE_RATE 16000
#define SAMPLE_BITS 16
#define CHANNEL_COUNT 1
#define MAX_RECORD_TIME 10  // seconds
#define CHUNK_SIZE 4096     // Smaller chunks for streaming
#define MAX_AUDIO_SIZE (SAMPLE_RATE * MAX_RECORD_TIME * sizeof(int16_t))

// ==== GLOBAL VARIABLES ====
// Audio streaming variables
uint8_t* audioChunks[64];  // Array of chunk pointers
int chunkCount = 0;
size_t totalAudioSize = 0;
bool isRecording = false;
bool buttonPressed = false;
bool lastButtonState = HIGH;

// TTS Variables
AudioGeneratorMP3 *mp3 = nullptr;
AudioFileSourceHTTPStream *file = nullptr;
AudioOutputI2S *out = nullptr;

// ==== QUIZ SYSTEM VARIABLES ====
String soalSekarang = "";
String kategoriSoal = "";
String jawabanBenar = "";
bool menungguJawaban = false;
bool menungguKategori = false;
bool sedangKuis = false;
int skorBenar = 0;
int totalSoal = 0;
bool isQuizMode = false;

// System state
enum SystemState {
  IDLE,
  RECORDING,
  PROCESSING_STT,
  PROCESSING_GPT,
  PROCESSING_TTS,
  PLAYING_AUDIO,
  QUIZ_MODE
};
SystemState currentState = IDLE;

// ==== OLED DISPLAY FUNCTIONS ====
void initOLED() {
    Wire.begin(OLED_SDA, OLED_SCL);
    
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);
    }
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("OLED Initialized");
    display.display();
    delay(1000);
}

void displayText(String text, bool clear = true) {
    if (clear) {
        display.clearDisplay();
    }
    
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    
    // Word wrap for long text
    int lineHeight = 8;
    int maxCharsPerLine = 21;
    int currentLine = 0;
    int maxLines = 8;
    
    for (int i = 0; i < text.length() && currentLine < maxLines; i += maxCharsPerLine) {
        String line = text.substring(i, min(i + maxCharsPerLine, (int)text.length()));
        display.setCursor(0, currentLine * lineHeight);
        display.println(line);
        currentLine++;
    }
    
    display.display();
}

// Bitmap data untuk Warning Dolphin
static const unsigned char PROGMEM image_DFU_bits[] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x1f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xf0,0xff,0xe3,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x8f,0xff,0xfc,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0e,0x7f,0xff,0xff,0x3f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfe,0x01,0xff,0xff,0xff,0xdf,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfc,0x00,0xff,0xff,0xff,0xef,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xf8,0x00,0xff,0xff,0xff,0xf7,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xf8,0x00,0xff,0xff,0xff,0xfb,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xf8,0x51,0xff,0xff,0xf0,0x03,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfe,0x00,0xaf,0xff,0xff,0x0f,0xf9,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xf9,0xf8,0x0f,0xff,0xfc,0xf8,0x0d,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xe7,0xff,0xe7,0xff,0xf3,0x87,0xf6,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x9f,0xff,0xe7,0xff,0xce,0x7f,0xfa,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfe,0x7e,0x00,0x67,0xff,0x39,0xff,0xfc,0xf0,0x1f,0xff,0xff,0xff,0xff,0xff,0xff,0xfd,0xf9,0x85,0x07,0xfe,0xe7,0xff,0xf8,0x8f,0xe7,0xff,0xff,0xff,0xff,0xff,0xff,0xfb,0xe7,0x0a,0x0f,0xfd,0x9f,0xff,0xe0,0x7f,0xfb,0xff,0xff,0xff,0xff,0xff,0xff,0xe7,0x9f,0x15,0x7f,0xfb,0x7f,0xff,0xe1,0xff,0x83,0xff,0xff,0xff,0xff,0xff,0xff,0xde,0x7e,0x0a,0xbf,0xf6,0xff,0xff,0xe3,0xfe,0x7d,0xff,0xff,0xff,0xff,0xff,0xff,0x39,0xfe,0x15,0x7f,0xed,0xff,0xff,0xf7,0xf9,0xfd,0xff,0xff,0xff,0xff,0xff,0xfc,0xf7,0xfe,0x2a,0xff,0xdb,0xff,0xff,0xef,0xf7,0xfd,0x03,0xff,0xff,0xff,0xff,0xe3,0xcf,0xfc,0x15,0x7f,0xb7,0xff,0xff,0xdf,0xef,0xfd,0xfc,0x0f,0xff,0xff,0xfc,0x1f,0x3f,0xf0,0x2a,0xff,0xaf,0xff,0xff,0xbf,0xdf,0xfd,0xff,0xf0,0x07,0xfe,0x03,0xfc,0xff,0xc0,0x55,0x7f,0x5f,0xf8,0xff,0x7f,0xdf,0xfd,0xff,0xff,0xf8,0x01,0xff,0xf3,0xff,0x00,0x2a,0xff,0x5f,0xf8,0xfe,0xff,0xbf,0xfd,0x0f,0xff,0xff,0xff,0xfe,0x0f,0xf8,0x0c,0x55,0x7e,0xbf,0xf0,0xfd,0xff,0xbf,0xfd,0xf0,0x0f,0xff,0xff,0x01,0xff,0x80,0x3c,0x2a,0xfe,0xbf,0xe1,0xfb,0xff,0x7f,0xfd,0xff,0xf0,0x00,0x00,0xff,0xf0,0x00,0xfc,0x54,0x7e,0x7f,0x81,0xf7,0xfe,0xff,0xfb,0xff,0xff,0xff,0xff,0xfc,0x00,0x0f,0xf8,0xa0,0x3e,0x7c,0x03,0xef,0xfe,0xff,0xfb,0xff,0xff,0xff,0xff,0x00,0x00,0x7f,0xe4,0x1f,0x3e,0x70,0x07,0xdf,0xfd,0xff,0xfb,0xff,0xf8,0x00,0x00,0x00,0x0f,0xfe,0x1c,0x7f,0x3e,0x70,0x0f,0xff,0xfb,0xff,0xf7,0xf8,0x00,0x00,0x00,0x03,0xfc,0x01,0xfd,0xfe,0x3e,0x78,0x7f,0xff,0xfb,0xff,0xe7,0xc0,0x00,0x00,0x00,0x3f,0x03,0xff,0xfb,0xc0,0x3e,0x7f,0xff,0xff,0xf7,0xff,0xe7,0x00,0x00,0x00,0x03,0xf0,0xff,0xff,0xf7,0x14,0x7f,0x3f,0xff,0xff,0xef,0xff,0xc7,0x00,0x00,0x7f,0xff,0x0f,0xff,0xff,0xee,0x2a,0xbf,0x38,0x7f,0xff,0xef,0xff,0xcf,0x00,0x1f,0xff,0xf8,0xff,0xff,0xff,0xdc,0x55,0x5f,0x87,0xff,0xff,0xdf,0xff,0x8f,0x01,0xff,0xff,0xc7,0xff,0xff,0xff,0x3a,0x2a,0xbf,0x9f,0xff,0xff,0xbf,0xff,0x0f,0x0f,0xff,0xfe,0x3f,0xff,0xff,0xfc,0xf7,0x15,0x5f,0xbf,0xff,0xff,0x7f,0xfe,0x1f,0x3f,0xff,0xf1,0xff,0xff,0xff,0xf3,0xef,0x2a,0xaf,0xbf,0xff,0xfc,0xff,0xfc,0x1f,0xff,0xff,0xcf,0xff,0xff,0xff,0xcf,0xde,0x15,0x57,0x7f,0xff,0xf3,0xff,0xf8,0x1f,0xff,0xff,0x3f,0xff,0xff,0xff,0x3f,0x3c,0x00,0xab,0x7f,0xff,0xcf,0xff,0xf0,0x0f,0xff,0xfc,0xff,0xff,0xff,0xf8,0xfc,0xf8,0x80,0x55,0x7f,0xbe,0x3f,0xff,0xe0,0x57,0xff,0xf3,0xff,0xff,0xff,0xc7,0xf3,0xe1,0x80,0xaa,0x7f,0xc1,0xff,0xff,0xc0,0x3b,0xff,0x8f,0xff,0xff,0xfc,0x3f,0xcf,0xc3,0x01,0x45,0x7f,0xff,0xff,0xff,0x00,0x5d,0xfc,0x7f,0xff,0xff,0x83,0xff,0xbf,0x0e,0x02,0x8a,0x3f,0xff,0xff,0xfc,0x00,0xbe,0xf3,0xff,0xff,0xfc,0x7f,0xfe,0x7c,0x3d,0x80,0x45,0x7f,0xff,0xff,0xf0,0x00,0x5f,0x0f,0xff,0xfe,0x03,0xff,0xf9,0xf0,0x73,0x90,0x8a,0x3f,0xff,0xff,0xa0,0x0a,0xbf,0xc0,0x00,0x01,0xff,0xff,0xe7,0xc1,0xef,0x3b,0x00,0x5f,0xff,0xff,0x40,0x55,0x7f,0xff,0xff,0xff,0xff,0xff,0x9e,0x07,0xde,0xf3,0x80,0xaf,0xff,0xff,0xaa,0xaa,0xff,0xff,0xff,0xff,0xff,0xfe,0x70,0x1f,0x3d,0xe7,0x00,0x57,0xff,0xff,0xd5,0x57,0xff,0xff,0xff,0xff,0xff,0xe0,0x00,0xfe,0xfb,0xce,0x00,0xaa,0xff,0xff,0xff,0xff,0xff};

void displayStatus(String status) {
    display.clearDisplay();
    
    // Display the Warning Dolphin bitmap di tengah
    display.drawBitmap(1, 13, image_DFU_bits, 128, 50, 1);
    display.fillRect(1, 0, 127, 13, 1);

    // Set text properties   
    display.setTextColor(0);
    display.setTextWrap(false);
    display.setCursor(3, 3);
    display.print("Status: ");

    display.setCursor(62, 2);
    display.print(".EduBot32S3");

    display.setCursor(3, 12);
    display.print(status);
   

   
    display.display();
}

void displayRecording(int seconds) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 10);
    display.println("RECORDING");
    
    display.setTextSize(1);
    display.setCursor(0, 35);
    display.println("Time: " + String(seconds) + "s");
    display.setCursor(0, 45);
    display.println("Chunks: " + String(chunkCount));
    display.setCursor(0, 55);
    display.println("Hold button to record");
    
    display.display();
}

void displayProcessing(String process) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("=== PROCESSING ===");
    
    display.setTextSize(2);
    display.setCursor(0, 20);
    display.println(process);
    
    display.setTextSize(1);
    display.setCursor(0, 50);
    display.println("Please wait...");
    
    display.display();
}

void displayResponse(String response) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("=== RESPONSE ===");
    
    // Display response with word wrap
    int lineHeight = 8;
    int maxCharsPerLine = 21;
    int currentLine = 1;
    int maxLines = 7;
    
    for (int i = 0; i < response.length() && currentLine < maxLines; i += maxCharsPerLine) {
        String line = response.substring(i, min(i + maxCharsPerLine, (int)response.length()));
        display.setCursor(0, currentLine * lineHeight);
        display.println(line);
        currentLine++;
    }
    
    display.display();
}

// ==== FUNCTION DECLARATIONS ====
void initWiFi();
void initI2SInput();
void initI2SOutput();
void recordAudioStreaming();
void freeAudioChunks();
String sendAudioToSTTServerStreaming();
String askGPT(String user_input);
String askGPTQuiz(String user_input, String mode);
String ttsRequest(const String& text);
bool playMP3(const char* urlmp3);
void stopAudio();
void processVoiceCommand();
void printSystemStatus();
bool isQuizTrigger(String input);
bool isAnswerInput(String input);
void processQuizAnswer(String answer);
void endQuiz();

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    // Initialize OLED first
    initOLED();
    
    Serial.println("\n=== ESP32-S3 Voice-to-Voice AI Chat dengan Kuis Edukatif ===");
    displayText("ESP32-S3 Voice AI\nStarting...");
    
    Serial.println("Streaming Audio Implementation with Educational Quiz System");
    Serial.printf("Free heap at start: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Chunk size: %d bytes\n", CHUNK_SIZE);
    Serial.printf("Max chunks: %d\n", sizeof(audioChunks)/sizeof(audioChunks[0]));
    
    // Initialize button
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    // Initialize audio chunks array
    for(int i = 0; i < sizeof(audioChunks)/sizeof(audioChunks[0]); i++) {
        audioChunks[i] = nullptr;
    }
    
    // Initialize WiFi
    initWiFi();
    
    // Initialize I2S for input (recording)
    initI2SInput();
    
    Serial.println("\n=== System Ready ===");
    displayText("System Ready!\nHold button to talk\nSay 'test' to verify\nSay 'kuis' for quiz");
    
    Serial.println("Usage:");
    Serial.println("1. Press and HOLD button to record voice");
    Serial.println("2. Release button to stop recording");
    Serial.println("3. System will process and respond with audio");
    Serial.println("4. Say 'test' to verify the system works");
    Serial.println("5. Say 'soal' or 'kuis' to start quiz mode");
    Serial.println("6. In quiz mode, say subject name to get questions");
    Serial.println("7. Answer with 'A', 'B', 'C', or 'D'");
    Serial.println("8. Say 'keluar' or 'selesai' to exit quiz");
    Serial.println("=======================================");
    
    currentState = IDLE;
    printSystemStatus();
}

void initWiFi() {
    Serial.printf("Connecting to WiFi: %s\n", ssid);
    displayText("Connecting to WiFi:\n" + String(ssid));
    
    WiFi.begin(ssid, password);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(1000);
        Serial.print(".");
        attempts++;
        
        displayText("Connecting WiFi...\nAttempt: " + String(attempts));
        
        if (attempts % 10 == 0) {
            Serial.println("\nRetrying WiFi connection...");
            WiFi.disconnect();
            delay(1000);
            WiFi.begin(ssid, password);
        }
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[SUCCESS] WiFi Connected!");
        Serial.printf("ESP32 IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("Signal strength: %d dBm\n", WiFi.RSSI());
        
        displayText("WiFi Connected!\nIP: " + WiFi.localIP().toString() + "\nSignal: " + String(WiFi.RSSI()) + " dBm");
        delay(2000);
    } else {
        Serial.println("\n[ERROR] WiFi connection failed!");
        displayText("WiFi Failed!\nCheck credentials");
        while(1);
    }
}
void cleanupI2S() {
    i2s_driver_uninstall(I2S_NUM_0);
    i2s_driver_uninstall(I2S_NUM_1);
    delay(1000);  // Beri waktu untuk cleanup
}

void initI2SInput() {
    Serial.println("Initializing I2S for audio input (INMP441)...");
    displayProcessing("WaitingI2C");
    
    // Uninstall existing I2S driver if present - menggunakan I2S_NUM_0 seperti kode 2
    esp_err_t uninstall_result = i2s_driver_uninstall(I2S_NUM_0);
    if (uninstall_result == ESP_OK) {
        Serial.println("Previous I2S driver uninstalled");
    }
    delay(200);
    
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };
    
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK_IN,
        .ws_io_num = I2S_WS_IN,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_SD_IN
    };
    
    // Menggunakan I2S_NUM_0 seperti kode 2
    esp_err_t result = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    if (result != ESP_OK) {
        Serial.printf("[ERROR] Failed to install I2S driver: %d\n", result);
        displayText("I2S Input Error!");
        return;
    }
    
    result = i2s_set_pin(I2S_NUM_0, &pin_config);
    if (result != ESP_OK) {
        Serial.printf("[ERROR] Failed to set I2S pins: %d\n", result);
        displayText("I2S Pin Error!");
        return;
    }
    
    Serial.println("[SUCCESS] I2S input initialized");
}

void initI2SOutput() {
    Serial.println("Initializing I2S for audio output (Speaker)...");
    displayProcessing("WaitingI2S");
    
    // Uninstall existing I2S driver - menggunakan I2S_NUM_0 seperti kode 2
    esp_err_t uninstall_result = i2s_driver_uninstall(I2S_NUM_0);
    if (uninstall_result == ESP_OK) {
        Serial.println("Previous I2S driver uninstalled for output");
    }
    delay(200);
    
    // Menggunakan konfigurasi DMA buffer dari kode 2
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 22050,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 40,  // Dari kode 2
        .dma_buf_len = 2560,  // Dari kode 2
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };
    
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_LRC,
        .data_out_num = I2S_DOUT,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    
    // Menggunakan I2S_NUM_0 seperti kode 2
    esp_err_t result = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    if (result != ESP_OK) {
        Serial.printf("[ERROR] Failed to install I2S output driver: %d\n", result);
        displayText("I2S Output Error!");
        return;
    }
    
    result = i2s_set_pin(I2S_NUM_0, &pin_config);
    if (result != ESP_OK) {
        Serial.printf("[ERROR] Failed to set I2S output pins: %d\n", result);
        displayText("I2S Output Pin Error!");
        return;
    }
    
    Serial.println("[SUCCESS] I2S output initialized");
}

void freeAudioChunks() {
    for(int i = 0; i < chunkCount; i++) {
        if(audioChunks[i]) {
            free(audioChunks[i]);
            audioChunks[i] = nullptr;
        }
    }
    chunkCount = 0;
    totalAudioSize = 0;
    Serial.printf("Audio chunks freed. Free heap: %d bytes\n", ESP.getFreeHeap());
}

void recordAudioStreaming() {
    Serial.println("\n=== RECORDING AUDIO (STREAMING) ===");
    Serial.println("🎤 Recording... Speak now!");
    
    currentState = RECORDING;
    isRecording = true;
    
    // Ensure I2S is in input mode
    initI2SInput();
    delay(200);
    
    // Free any existing chunks
    freeAudioChunks();
    
    uint32_t startTime = millis();
    uint32_t lastProgressTime = startTime;
    uint32_t lastDisplayTime = startTime;
    size_t bytesRead = 0;
    
    while (digitalRead(BUTTON_PIN) == LOW && 
           totalAudioSize < MAX_AUDIO_SIZE && 
           chunkCount < (sizeof(audioChunks)/sizeof(audioChunks[0]) - 1)) {
        
        // Allocate new chunk
        audioChunks[chunkCount] = (uint8_t*)malloc(CHUNK_SIZE);
        if (!audioChunks[chunkCount]) {
            Serial.println("[ERROR] Failed to allocate audio chunk!");
            displayText("Memory Error!");
            break;
        }
        
        // Read audio data into chunk - menggunakan I2S_NUM_0 seperti kode 2
        esp_err_t result = i2s_read(I2S_NUM_0, 
                                   audioChunks[chunkCount], 
                                   CHUNK_SIZE, 
                                   &bytesRead, 
                                   portMAX_DELAY);
        
        if (result == ESP_OK && bytesRead > 0) {
            totalAudioSize += bytesRead;
            chunkCount++;
            
            // Show progress every second
            if (millis() - lastProgressTime > 1000) {
                int elapsed = (millis() - startTime) / 1000;
                int recordedSeconds = totalAudioSize / (SAMPLE_RATE * sizeof(int16_t));
                Serial.printf("🎤 Recording... %ds (chunks: %d, bytes: %d)\n", 
                              elapsed, chunkCount, totalAudioSize);
                lastProgressTime = millis();
            }
            
            // Update display every 500ms
            if (millis() - lastDisplayTime > 500) {
                int elapsed = (millis() - startTime) / 1000;
                displayRecording(elapsed);
                lastDisplayTime = millis();
            }
        } else {
            // Free unused chunk
            free(audioChunks[chunkCount]);
            audioChunks[chunkCount] = nullptr;
        }
        
        delay(1);
    }
    
    isRecording = false;
    int recordDuration = (millis() - startTime) / 1000;
    Serial.printf("✅ Recording completed! Duration: %ds, Chunks: %d, Total bytes: %d\n", 
                  recordDuration, chunkCount, totalAudioSize);
    
    displayText("Recording Complete!\nDuration: " + String(recordDuration) + "s\nChunks: " + String(chunkCount));
    
    // Process the recorded audio
    processVoiceCommand();
}

String sendAudioToSTTServerStreaming() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[ERROR] WiFi not connected!");
        displayText("WiFi Error!");
        return "";
    }
    
    if (chunkCount == 0 || totalAudioSize == 0) {
        Serial.println("[ERROR] No audio data to send!");
        displayText("No Audio Data!");
        return "";
    }
    
    Serial.println("📤 Sending streaming audio to STT server...");
    displayProcessing("STT");
    currentState = PROCESSING_STT;
    
    HTTPClient http;
    http.begin(sttServerURL);
    http.setTimeout(30000);
    http.setReuse(false);
    http.addHeader("Content-Type", "application/octet-stream");
    http.addHeader("X-Sample-Rate", String(SAMPLE_RATE));
    http.addHeader("X-Channels", String(CHANNEL_COUNT));
    http.addHeader("X-Bits-Per-Sample", String(SAMPLE_BITS));
    http.addHeader("Content-Length", String(totalAudioSize));
    
    // Create a single buffer for HTTP POST
    uint8_t* combinedBuffer = (uint8_t*)malloc(totalAudioSize);
    if (!combinedBuffer) {
        Serial.println("[ERROR] Failed to allocate combined buffer!");
        displayText("Memory Error!");
        http.end();
        return "";
    }
    
    // Combine all chunks into single buffer
    size_t offset = 0;
    for(int i = 0; i < chunkCount; i++) {
        size_t chunkDataSize = (i == chunkCount - 1) ? 
                               (totalAudioSize - offset) : CHUNK_SIZE;
        memcpy(combinedBuffer + offset, audioChunks[i], chunkDataSize);
        offset += chunkDataSize;
    }
    
    Serial.printf("📤 Sending %d bytes to STT server...\n", totalAudioSize);
    
    int httpResponseCode = http.POST(combinedBuffer, totalAudioSize);
    String recognizedText = "";
    
    // Free combined buffer immediately
    free(combinedBuffer);
    
    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.printf("📥 STT Response Code: %d\n", httpResponseCode);
        
        // Parse JSON response
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, response);
        
        if (!error && doc.containsKey("text")) {
            recognizedText = doc["text"].as<String>();
            recognizedText.trim();
            
            Serial.println("=== RECOGNIZED TEXT ===");
            Serial.printf("🗣️  \"%s\"\n", recognizedText.c_str());
            Serial.println("=======================");
            
            displayText("Speech Recognized:\n" + recognizedText);
            delay(2000);
        } else {
            Serial.println("[ERROR] Failed to parse STT response");
            Serial.println("Response: " + response);
            displayText("STT Parse Error!");
        }
    } else {
        Serial.printf("[ERROR] STT HTTP Error: %d\n", httpResponseCode);
        displayText("STT HTTP Error:\n" + String(httpResponseCode));
    }
    
    http.end();
    
    // Free audio chunks after sending
    freeAudioChunks();
    
    return recognizedText;
}

bool isQuizTrigger(String input) {
    input.toLowerCase();
    return (input.indexOf("soal") >= 0 || input.indexOf("kuis") >= 0 || 
            input.indexOf("quiz") >= 0 || input.indexOf("belajar") >= 0);
}

bool isAnswerInput(String input) {
   input.toUpperCase();
   input.trim();
   
   // Check for single letter answers
   if (input == "A" || input == "B" || input == "C" || input == "D") {
       return true;
   }
   
   // Check for answers that start with letter followed by dot or space
   if (input.length() >= 2) {
       char firstChar = input.charAt(0);
       char secondChar = input.charAt(1);
       
       if ((firstChar == 'A' || firstChar == 'B' || firstChar == 'C' || firstChar == 'D') &&
           (secondChar == '.' || secondChar == ' ')) {
           return true;
       }
   }
   
   // Check for answers that start with "JAWABAN A", "JAWABAN B", etc.
   if (input.indexOf("JAWABAN A") >= 0 || input.indexOf("JAWABAN B") >= 0 || 
       input.indexOf("JAWABAN C") >= 0 || input.indexOf("JAWABAN D") >= 0) {
       return true;
   }
   
   return false;
}

String askGPT(String user_input) {
    Serial.println("🤖 Asking GPT...");
    displayProcessing("Answer");
    currentState = PROCESSING_GPT;
    
    Serial.printf("Free heap before GPT: %d bytes\n", ESP.getFreeHeap());
    
    HTTPClient http;
    http.begin(openai_url);
    http.setTimeout(35000);
    http.setReuse(false);
    
    String auth = "Bearer " + openai_key;
    http.addHeader("Authorization", auth);
    http.addHeader("Content-Type", "application/json");
    
    // Create payload with Indonesian system prompt
    String payload = "{\"model\":\"" + model + "\",\"messages\":[";
    payload += "{\"role\":\"system\",\"content\":\"Kamu adalah asisten AI yang ramah. Jawab dalam bahasa Indonesia, singkat dan jelas maksimal 100 kata. Jika user mengatakan 'test' atau 'tes', jawab dengan 'Halo! Sistem voice to voice berfungsi dengan baik. Saya dapat mendengar dan merespons suara Anda.' Jika user menyebutkan 'soal' atau 'kuis', arahkan mereka untuk menyebutkan mata pelajaran yang ingin dipelajari.\"},";
    payload += "{\"role\":\"user\",\"content\":\"" + user_input + "\"}";
    payload += "],\"max_tokens\":120,\"temperature\":0.7}";
    
    int httpResponseCode = http.POST(payload);
    String response = http.getString();
    http.end();
    
    if (httpResponseCode == 200) {
        DynamicJsonDocument doc(6144);
        DeserializationError err = deserializeJson(doc, response);
        
        if (!err) {
            String text = doc["choices"][0]["message"]["content"].as<String>();
            text.trim();
            
            // Clean text for TTS
            text.replace("\n", " ");
            text.replace("\r", "");
            text.replace("  ", " ");
            text.replace("*", "");
            text.replace("#", "");
            
            Serial.println("=== AI RESPONSE ===");
            Serial.printf("🤖 \"%s\"\n", text.c_str());
            Serial.println("==================");
            
            displayResponse(text);
            
            return text;
        } else {
            Serial.println("[ERROR] JSON parsing failed: " + String(err.c_str()));
            displayText("JSON Parse Error!");
        }
    } else {
        Serial.printf("[ERROR] OpenAI HTTP error: %d\n", httpResponseCode);
        displayText("OpenAI Error:\n" + String(httpResponseCode));
    }
    
    return "Maaf, terjadi kesalahan saat memproses permintaan Anda.";
}

String askGPTQuiz(String user_input, String mode) {
    Serial.println("🎓 Asking GPT for Quiz...");
    displayProcessing("Quiz GPT");
    currentState = PROCESSING_GPT;
    
    HTTPClient http;
    http.begin(openai_url);
    http.setTimeout(35000);
    http.setReuse(false);
    
    String auth = "Bearer " + openai_key;
    http.addHeader("Authorization", auth);
    http.addHeader("Content-Type", "application/json");
    
    String payload = "{\"model\":\"" + model + "\",\"messages\":[";
    
    if (mode == "question") {
        payload += "{\"role\":\"system\",\"content\":\"Kamu adalah guru SD yang ramah. Buatkan satu soal cukup satu soal saja pilihan ganda yang brfariasi dan bermacam macam serta brbagai jenis topik soal jangan itu itu saja untuk tingkat Sekolah Dasar dari mata pelajaran '" + user_input + "'. Berikan soal dengan pilihan jawaban A, B, C, dan D. Jangan berikan jawabannya. Format: [Soal]\\nA. [pilihan A]\\nB. [pilihan B]\\nC. [pilihan C]\\nD. [pilihan D]. Maksimal 100 kata.\"},";
        payload += "{\"role\":\"user\",\"content\":\"Buatkan soal " + user_input + " untuk siswa SD\"}";
    } else if (mode == "answer") {
        payload += "{\"role\":\"system\",\"content\":\"Kamu adalah guru SD yang ramah. Periksa jawaban siswa untuk soal berikut. Berikan penjelasan singkat apakah benar atau salah beserta alasannya. Maksimal 100 kata.\"},";
        payload += "{\"role\":\"user\",\"content\":\"Soal: " + soalSekarang + "\\n\\nJawaban siswa: " + user_input + "\"}";
    }
    
    payload += "],\"max_tokens\":200,\"temperature\":0.7}";
    
    int httpResponseCode = http.POST(payload);
    String response = http.getString();
    http.end();
    
    if (httpResponseCode == 200) {
        DynamicJsonDocument doc(6144);
        DeserializationError err = deserializeJson(doc, response);
        
        if (!err) {
            String text = doc["choices"][0]["message"]["content"].as<String>();
            text.trim();
            
            // Clean text for TTS
            text.replace("\n", " ");
text.replace("\r", "");
            text.replace("  ", " ");
            text.replace("*", "");
            text.replace("#", "");
            
            Serial.println("=== QUIZ RESPONSE ===");
            Serial.printf("🎓 \"%s\"\n", text.c_str());
            Serial.println("====================");
            
            displayResponse(text);
            
            return text;
        } else {
            Serial.println("[ERROR] JSON parsing failed: " + String(err.c_str()));
            displayText("JSON Parse Error!");
        }
    } else {
        Serial.printf("[ERROR] OpenAI HTTP error: %d\n", httpResponseCode);
        displayText("OpenAI Error:\n" + String(httpResponseCode));
    }
    
    return "Maaf, terjadi kesalahan saat memproses soal.";
}

void processQuizAnswer(String answer) {
    Serial.println("🎯 Processing quiz answer: " + answer);
    
    totalSoal++;
    String feedback = askGPTQuiz(answer, "answer");
    
    // Simple scoring (check if response contains "benar" or "salah")
    if (feedback.indexOf("benar") >= 0 || feedback.indexOf("Benar") >= 0) {
        skorBenar++;
        Serial.println("✅ Correct answer!");
    } else {
        Serial.println("❌ Wrong answer!");
    }
    
    // Speak the feedback
    String ttsUrl = ttsRequest(feedback);
    if (ttsUrl.length() > 0) {
        playMP3(ttsUrl.c_str());
    }
    
    // Reset quiz variables for next question
    menungguJawaban = false;
    soalSekarang = "";
    
    // Ask if user wants another question
    delay(2000);
    String nextQuestion = "Skormu sekarang " + String(skorBenar) + " dari " + String(totalSoal) + ". Mau soal lagi? Sebutkan mata pelajaran atau katakan keluar untuk mengakhiri.";
    String nextTtsUrl = ttsRequest(nextQuestion);
    if (nextTtsUrl.length() > 0) {
        playMP3(nextTtsUrl.c_str());
    }
}

void endQuiz() {
    Serial.println("🏁 Ending quiz mode");
    
    isQuizMode = false;
    sedangKuis = false;
    menungguJawaban = false;
    menungguKategori = false;
    
    String finalScore = "Kuis selesai! Skor akhir kamu " + String(skorBenar) + " benar dari " + String(totalSoal) + " soal. ";
    if (skorBenar == totalSoal) {
        finalScore += "Sempurna! Hebat sekali!";
    } else if (skorBenar >= totalSoal * 0.8) {
        finalScore += "Bagus sekali! Terus belajar!";
    } else if (skorBenar >= totalSoal * 0.6) {
        finalScore += "Cukup baik! Bisa lebih baik lagi!";
    } else {
        finalScore += "Semangat belajar lagi ya!";
    }
    
    Serial.println("📊 Final score: " + String(skorBenar) + "/" + String(totalSoal));
    displayText("Quiz Complete!\nScore: " + String(skorBenar) + "/" + String(totalSoal));
    
    String finalTtsUrl = ttsRequest(finalScore);
    if (finalTtsUrl.length() > 0) {
        playMP3(finalTtsUrl.c_str());
    }
    
    // Reset score
    skorBenar = 0;
    totalSoal = 0;
    soalSekarang = "";
    kategoriSoal = "";
    
    displayText("Quiz mode OFF\nHold button to talk\nSay 'kuis' to start quiz");
}

String ttsRequest(const String& text) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[ERROR] WiFi not connected for TTS!");
        displayText("WiFi Error!");
        return "";
    }
    
    if (text.length() == 0) {
        Serial.println("[ERROR] Empty text for TTS!");
        return "";
    }
    
    Serial.println("🔊 Requesting TTS...");
    displayProcessing("TTS");
    currentState = PROCESSING_TTS;
    
    // Advanced text processing dari kode 2
    String processedText = text;
    if (processedText.length() > TTS_CHAR_LIMIT) {
        processedText = processedText.substring(0, TTS_CHAR_LIMIT);
        int cutPoint = max(max(processedText.lastIndexOf('.'), 
                              processedText.lastIndexOf('!')), 
                              processedText.lastIndexOf('?'));
        if (cutPoint > TTS_CHAR_LIMIT * 0.6) {
            processedText = processedText.substring(0, cutPoint + 1);
        }
        Serial.println("[WARNING] Text truncated for TTS");
    }
    
    HTTPClient http;
    String endpoint = String(backend_host) + "/tts";
    http.begin(endpoint);
    http.setTimeout(25000);
    http.setReuse(false);
    http.addHeader("Content-Type", "application/json");
    
    // Enhanced escaping dari kode 2
    String escapedText = processedText;
    escapedText.replace("\\", "\\\\");
    escapedText.replace("\"", "\\\"");
    escapedText.replace("\n", "\\n");
    escapedText.replace("\r", "\\r");
    
    // Enhanced payload dari kode 2
    String payload = "{\"text\": \"" + escapedText + "\", \"format\": \"mp3\", \"bitrate\": 64, \"sample_rate\": 22050, \"channels\": 1}";
    
    Serial.printf("🔊 Sending TTS request: %s\n", payload.c_str());
    
    int httpCode = http.POST(payload);
    String resultMp3url = "";
    
    Serial.printf("🔊 TTS Response Code: %d\n", httpCode);
    
    if (httpCode == 200) {
        String resp = http.getString();
        Serial.printf("🔊 TTS Response: %s\n", resp.c_str());
        Serial.println("✅ TTS response received");
        
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, resp);
        
        if (!error) {
            // Check for different possible field names
            if (doc.containsKey("url")) {
                resultMp3url = doc["url"].as<String>();
                Serial.printf("🎵 TTS Audio URL: %s\n", resultMp3url.c_str());
            } else if (doc.containsKey("audio_url")) {
                resultMp3url = doc["audio_url"].as<String>();
                Serial.printf("🎵 TTS Audio URL: %s\n", resultMp3url.c_str());
            } else if (doc.containsKey("file_url")) {
                resultMp3url = doc["file_url"].as<String>();
                Serial.printf("🎵 TTS Audio URL: %s\n", resultMp3url.c_str());
            } else {
                // Fallback to default URL
                resultMp3url = String(backend_host) + "/audio.mp3";
                Serial.printf("🎵 Using default MP3 URL: %s\n", resultMp3url.c_str());
            }
            
            // Also check the status field
            if (doc.containsKey("status")) {
                String status = doc["status"].as<String>();
                Serial.printf("🎵 TTS Status: %s\n", status.c_str());
            }
        } else {
            Serial.println("[ERROR] Failed to parse TTS response");
            Serial.println("JSON Error: " + String(error.c_str()));
            displayText("TTS Parse Error!");
        }
    } else {
        Serial.printf("[ERROR] TTS HTTP error: %d\n", httpCode);
        String response = http.getString();
        Serial.println("Response: " + response);
        displayText("TTS HTTP Error:\n" + String(httpCode));
    }
    
    http.end();
    return resultMp3url;
}

void stopAudio() {
    Serial.println("🔇 Stopping audio playback...");
    
    // Stop MP3 playback first
    if (mp3 && mp3->isRunning()) { 
        mp3->stop(); 
        delay(100);
    }
    
    // Delete objects in proper order
    if (mp3) { 
        delete mp3; 
        mp3 = nullptr;
        Serial.println("MP3 generator deleted");
    }
    
    if (file) { 
        delete file; 
        file = nullptr;
        Serial.println("HTTP stream deleted");
    }
    
    if (out) { 
        delete out; 
        out = nullptr;
        Serial.println("I2S output deleted");
    }
    
    delay(300);
    Serial.printf("✅ Audio cleanup complete. Free memory: %d bytes\n", ESP.getFreeHeap());
}

// Enhanced playMP3 dari kode 2
bool playMP3(const char* urlmp3) {
    if (!urlmp3 || strlen(urlmp3) == 0) {
        Serial.println("[ERROR] Invalid MP3 URL!");
        return false;
    }
    
    Serial.println("🔊 Starting audio playback...");
    displayProcessing("Playing");
    currentState = PLAYING_AUDIO;
    
    // Clean up any existing audio objects
    stopAudio();
    
    // Wait for backend to prepare audio (dari kode 2)
    delay(3000);
    
    // Switch I2S to output mode  
    initI2SOutput();
    delay(500);
    
    // Initialize I2S output dengan konfigurasi enhanced
    out = new AudioOutputI2S();
    if (!out) {
        Serial.println("[ERROR] Failed to create I2S output");
        displayText("Audio Output Error!");
        stopAudio();
        return false;
    }
    
    // Enhanced output configuration dari kode 2
    out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    out->SetOutputModeMono(true);
    out->SetGain(0.9);
    out->SetBitsPerSample(16);
    out->SetRate(44100);
    
    // Create HTTP stream
    file = new AudioFileSourceHTTPStream(urlmp3);
    if (!file) {
        Serial.println("[ERROR] Failed to create HTTP stream");
        displayText("Audio Stream Error!");
        stopAudio();
        return false;
    }
    
    // Buffer time dari kode 2
    delay(2000);
    
    if (!file->isOpen()) {
        Serial.println("[ERROR] HTTP stream failed to open");
        displayText("Audio Stream Error!");
        stopAudio();
        return false;
    }
    
    // Create MP3 decoder
    mp3 = new AudioGeneratorMP3();
    if (!mp3) {
        Serial.println("[ERROR] Failed to create MP3 generator");
        displayText("MP3 Generator Error!");
        stopAudio();
        return false;
    }
    
    if (!mp3->begin(file, out)) {
        Serial.println("[ERROR] MP3 decoder initialization failed");
        displayText("MP3 Playback Error!");
        stopAudio();
        return false;
    }
    
    Serial.println("🎵 Audio playing...");
    displayText("Playing Audio...\nPress button to interrupt");
    
    // Enhanced playback loop dari kode 2
    unsigned long startTime = millis();
    unsigned long lastUpdateTime = startTime;
    unsigned long noProgressCount = 0;
    bool audioCompleted = false;
    
    while (mp3 && mp3->isRunning()) {
        bool loopResult = mp3->loop();
        
        if (!loopResult) {
            delay(10);
            noProgressCount++;
            
            // If no progress for too long, assume audio is finished
            if (noProgressCount > 100) {
                Serial.println("🎵 Audio stream appears to be finished (no progress)");
                audioCompleted = true;
                break;
            }
        } else {
            noProgressCount = 0; // Reset no-progress counter
        }
        
        // Check for button press to interrupt
        if (digitalRead(BUTTON_PIN) == LOW) {
            Serial.println("🎵 Audio interrupted by button press");
            break;
        }
        
        // Progress update every 3 seconds
        if (millis() - lastUpdateTime > 3000) {
            unsigned long elapsed = (millis() - startTime) / 1000;
            Serial.printf("🎵 Playing... %lus\n", elapsed);
            lastUpdateTime = millis();
        }
        
        // Enhanced timeout (3 minutes dari kode 2)
        if (millis() - startTime > 180000) {
            Serial.println("[WARNING] Audio playback timeout reached");
            break;
        }
        
        // Check if MP3 is no longer running
        if (mp3 && !mp3->isRunning()) {
            Serial.println("🎵 Audio playback completed normally");
            audioCompleted = true;
            break;
        }
        
        yield();
        delay(5);
    }
    
    Serial.println("🎵 Audio playback completed");
    stopAudio();
    
    if (audioCompleted) {
        Serial.println("✅ Audio playback completed successfully!");
    } else {
        Serial.println("⚠ Audio playback ended (may have been interrupted)");
    }
    
    // Enhanced delay sebelum kembali ke input mode
    delay(1000);
    
    // Switch back to input mode
    initI2SInput();
    
    currentState = IDLE;
    displayStatus("IDLE");
    Serial.println("🔄 System returned to IDLE state\n");
    
    return audioCompleted;
}

void processVoiceCommand() {
    if (chunkCount == 0) {
        Serial.println("[ERROR] No audio data to process!");
        displayText("No Audio Data!");
        return;
    }
    
    // Step 1: Speech to Text
    String recognizedText = sendAudioToSTTServerStreaming();
    
    if (recognizedText.length() == 0) {
        Serial.println("[ERROR] No text recognized!");
        displayText("No Speech Detected!");
        delay(2000);
        displayStatus("IDLE");
        return;
    }
    
    String response = "";
    String ttsUrl = "";
    
    // Check if user wants to exit quiz
    if (isQuizMode && (recognizedText.indexOf("keluar") >= 0 || 
                       recognizedText.indexOf("selesai") >= 0 || 
                       recognizedText.indexOf("berhenti") >= 0)) {
        endQuiz();
        return;
    }
    
    // Quiz mode logic
    if (isQuizMode) {
        if (menungguJawaban && isAnswerInput(recognizedText)) {
            // Process quiz answer
            processQuizAnswer(recognizedText);
            return;
        } else if (menungguKategori || !menungguJawaban) {
            // Generate new question
            soalSekarang = askGPTQuiz(recognizedText, "question");
            if (soalSekarang.length() > 0) {
                ttsUrl = ttsRequest(soalSekarang);
                if (ttsUrl.length() > 0) {
                    playMP3(ttsUrl.c_str());
                }
                menungguJawaban = true;
                menungguKategori = false;
                kategoriSoal = recognizedText;
            }
            return;
        }
    }
    
    // Check if user wants to start quiz
    if (!isQuizMode && isQuizTrigger(recognizedText)) {
        isQuizMode = true;
        sedangKuis = true;
        menungguKategori = true;
        menungguJawaban = false;
        
        response = "Ayo mulai kuis! Sebutkan mata pelajaran yang ingin kamu pelajari, misalnya matematika, bahasa Indonesia, IPA, IPS, atau PKN.";
        Serial.println("🎓 Quiz mode activated!");
        displayText("Quiz Mode ON!\nSay subject name");
        
        ttsUrl = ttsRequest(response);
        if (ttsUrl.length() > 0) {
            playMP3(ttsUrl.c_str());
        }
        return;
    }
    
    // Normal chat mode
    if (!isQuizMode) {
        // Step 2: Generate AI response
        response = askGPT(recognizedText);
        
        if (response.length() > 0) {
            // Step 3: Text to Speech
            ttsUrl = ttsRequest(response);
            
            if (ttsUrl.length() > 0) {
                // Step 4: Play audio response
                playMP3(ttsUrl.c_str());
            } else {
                Serial.println("[ERROR] Failed to get TTS URL!");
                displayText("TTS Failed!");
            }
        } else {
            Serial.println("[ERROR] Failed to get AI response!");
            displayText("AI Response Failed!");
        }
    }
    
    // Return to idle state
    currentState = IDLE;
    String statusText = isQuizMode ? "Quiz Mode" : "Chat Mode";
    displayStatus(statusText);
}

void printSystemStatus() {
    Serial.println("\n=== SYSTEM STATUS ===");
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("WiFi status: %s\n", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
    Serial.printf("Current state: %d\n", currentState);
    Serial.printf("Quiz mode: %s\n", isQuizMode ? "ON" : "OFF");
    Serial.printf("Recording: %s\n", isRecording ? "YES" : "NO");
    Serial.printf("Button state: %s\n", digitalRead(BUTTON_PIN) == LOW ? "PRESSED" : "RELEASED");
    Serial.printf("Audio chunks: %d\n", chunkCount);
    Serial.printf("Total audio size: %d bytes\n", totalAudioSize);
    
    if (isQuizMode) {
        Serial.printf("Quiz score: %d/%d\n", skorBenar, totalSoal);
        Serial.printf("Waiting for answer: %s\n", menungguJawaban ? "YES" : "NO");
        Serial.printf("Waiting for category: %s\n", menungguKategori ? "YES" : "NO");
    }
    
    Serial.println("====================\n");
}

void loop() {
    // Read button state
    bool currentButtonState = digitalRead(BUTTON_PIN);
    
    // Button press detection (falling edge)
    if (lastButtonState == HIGH && currentButtonState == LOW) {
        buttonPressed = true;
        Serial.println("🔘 Button pressed - Starting recording...");
        
        // Start recording immediately
        recordAudioStreaming();
        
        buttonPressed = false;
    }
    
    lastButtonState = currentButtonState;
    
    // Print system status every 30 seconds
    static unsigned long lastStatusTime = 0;
    if (millis() - lastStatusTime > 30000) {
        printSystemStatus();
        lastStatusTime = millis();
    }
    
    // Update display with current status
    static unsigned long lastDisplayUpdate = 0;
    if (millis() - lastDisplayUpdate > 5000) {
        String statusText = "IDLE";
        if (isQuizMode) {
            statusText = "Quiz Mode";
        }
        displayStatus(statusText);
        lastDisplayUpdate = millis();
    }
    
    // Small delay to prevent excessive CPU usage
    delay(50);
}