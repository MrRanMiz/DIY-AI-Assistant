#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Typewriter effect with vertical scrolling for OLED
void typewriterPrintScrolling(int x, int y, String text, int delayMs = 40) {
  int charWidth = 6; // For text size 1, 6px per char
  int lineHeight = 8; // For text size 1, 8px per line
  int maxX = SCREEN_WIDTH;
  int maxY = SCREEN_HEIGHT;
  display.setCursor(x, y);
  String word = "";
  for (size_t i = 0; i <= text.length(); i++) {
    char c = text[i];
    // Build up a word or handle end of word
    if (c != ' ' && c != '\n' && c != '\0') {
      word += c;
    }
    if (c == ' ' || c == '\n' || c == '\0') {
      // Check if word fits on current line
      int wordPixelLen = word.length() * charWidth;
      if (x + wordPixelLen > maxX) {
        // Move to next line
        x = 0;
        y += lineHeight;
        display.setCursor(x, y);
        if (y >= maxY) {
          // Scroll up: copy buffer up by one line
          for (int row = 0; row < maxY - lineHeight; row++) {
            for (int col = 0; col < maxX; col++) {
              int pixel = display.getPixel(col, row + lineHeight);
              display.drawPixel(col, row, pixel);
            }
          }
          // Clear last line
          for (int col = 0; col < maxX; col++) {
            for (int row = maxY - lineHeight; row < maxY; row++) {
              display.drawPixel(col, row, SSD1306_BLACK);
            }
          }
          y = maxY - lineHeight;
          display.setCursor(x, y);
          display.display();
        }
      }
      // Print the word
      for (size_t j = 0; j < word.length(); j++) {
        display.write(word[j]);
        display.display();
        delay(delayMs);
        x += charWidth;
      }
      word = "";
      // Print the space or handle newline
      if (c == ' ') {
        display.write(' ');
        display.display();
        delay(delayMs);
        x += charWidth;
      } else if (c == '\n') {
        x = 0;
        y += lineHeight;
        display.setCursor(x, y);
        if (y >= maxY) {
          // Scroll up: copy buffer up by one line
          for (int row = 0; row < maxY - lineHeight; row++) {
            for (int col = 0; col < maxX; col++) {
              int pixel = display.getPixel(col, row + lineHeight);
              display.drawPixel(col, row, pixel);
            }
          }
          // Clear last line
          for (int col = 0; col < maxX; col++) {
            for (int row = maxY - lineHeight; row < maxY; row++) {
              display.drawPixel(col, row, SSD1306_BLACK);
            }
          }
          y = maxY - lineHeight;
          display.setCursor(x, y);
          display.display();
        }
      }
    }
  }

}
/*
 * ESP32 AI Voice Assistant - Text-Only (No Audio Output)
 * Target: Arduino Nano ESP32 (ESP32-S3, 8MB PSRAM)
 * 
 * Hardware Connections:
 * - INMP441 Microphone (I2S):
 *   SCK  -> D9  (GPIO 18)
 *   WS   -> D8  (GPIO 17)
 *   SD   -> D7  (GPIO 10)
 *   VDD  -> 3.3V
 *   GND  -> GND
 * 
 * - Button:
 *   One side -> D2 (GPIO 5)
 *   Other side -> GND
 * 
 * Serial Monitor: 115200 baud
 */


#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "driver/i2s.h"

// WiFi credentials
const char* ssid = "HUAWEI-2.4G-D87y";
const char* password = "Cbn9yxAU";

// Server URL
const char* server_url = "https://wesleyhuggingface-esp32-voice-assistant.hf.space/process_audio";
const char* server_status_url = "https://wesleyhuggingface-esp32-voice-assistant.hf.space/status";

// Pin definitions for Arduino Nano ESP32
#define BUTTON_PIN 5          // D2 -> GPIO 5
#define I2S_SCK 18            // D9 -> GPIO 18 (Clock)
#define I2S_WS 17             // D8 -> GPIO 17 (Word Select)
#define I2S_SD 10             // D7 -> GPIO 10 (Data In)

// Audio configuration
const int SAMPLE_RATE = 16000;
const int BITS_PER_SAMPLE = 16;
const int BUFFER_SIZE = 1024;
const int MAX_RECORD_TIME_MS = 10000;  // 10 seconds max

// Recording state
uint8_t* record_buffer = nullptr;
size_t record_buffer_size = 0;
size_t record_position = 0;
bool is_recording = false;
unsigned long record_start_time = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

    Wire.begin(7,8); // SDA = GPIO 8, SCL = GPIO 7
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed!");
    for(;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.display();
  
  Serial.println("\n=================================");
  Serial.println("ESP32 Voice Assistant - Text Only");
  Serial.println("Arduino Nano ESP32 (ESP32-S3)");
  Serial.println("=================================\n");
  
  // Initialize I2S for microphone
  initI2SMicrophone();
  
  // Initialize button
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.println("✓ Button initialized on GPIO 5 (D2)");
  
  // Print memory info
  printMemoryInfo();
  
  // Connect to WiFi
  connectWiFi();
  
  // Check server status
  if (checkServerStatus()) {
    Serial.println("\n✓ Server is ready!");
    Serial.println("\n>>> Press and hold button to record <<<\n");
  } else {
    Serial.println("\n✗ Server not ready. Check server URL.");
  }
}

void loop() {
  // Hold-to-record button logic (same as original)
  if (digitalRead(BUTTON_PIN) == LOW && !is_recording) {
    delay(50);  // Debounce
    if (digitalRead(BUTTON_PIN) == LOW) {
      startRecording();
    }
  }
  
  if (digitalRead(BUTTON_PIN) == HIGH && is_recording) {
    delay(50);  // Debounce
    if (digitalRead(BUTTON_PIN) == HIGH) {
      stopRecording();
      processAudio();
    }
  }
  
  // Continue recording while button held
  if (is_recording) {
    recordAudioData();
    
    // Auto-stop after max time
    if (millis() - record_start_time > MAX_RECORD_TIME_MS) {
      Serial.println("\n⚠ Max recording time reached");
      stopRecording();
      processAudio();
    }
  }
  
  delay(10);
}

void initI2SMicrophone() {
  Serial.println("Initializing I2S microphone...");
  
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = (i2s_bits_per_sample_t)BITS_PER_SAMPLE,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };
  
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };
  
  esp_err_t result = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  if (result != ESP_OK) {
    Serial.printf("✗ I2S driver install failed: %d\n", result);
    return;
  }
  
  result = i2s_set_pin(I2S_NUM_0, &pin_config);
  if (result != ESP_OK) {
    Serial.printf("✗ I2S set pin failed: %d\n", result);
    return;
  }
  
  Serial.println("✓ I2S microphone initialized");
  Serial.printf("  Sample Rate: %d Hz\n", SAMPLE_RATE);
  Serial.printf("  Bits/Sample: %d\n", BITS_PER_SAMPLE);
  Serial.printf("  SCK (Clock):  GPIO %d (D9)\n", I2S_SCK);
  Serial.printf("  WS (Select):  GPIO %d (D8)\n", I2S_WS);
  Serial.printf("  SD (Data):    GPIO %d (D7)\n", I2S_SD);
}

void startRecording() {
  Serial.println("\n🎤 Recording started...");
  is_recording = true;
  record_start_time = millis();
  
  // Allocate buffer: 15 seconds max + 44 byte WAV header
  record_buffer_size = 15 * SAMPLE_RATE * (BITS_PER_SAMPLE / 8) + 44;
  record_buffer = (uint8_t*)malloc(record_buffer_size);
  
  if (!record_buffer) {
    Serial.println("✗ Memory allocation failed!");
    is_recording = false;
    return;
  }
  
  // Start writing after WAV header (first 44 bytes)
  record_position = 44;
  
  Serial.printf("   Buffer allocated: %d bytes\n", record_buffer_size);
  Serial.println("   (Hold button, release to stop)");
}

void recordAudioData() {
  size_t bytes_read;
  size_t space_remaining = record_buffer_size - record_position;
  
  if (space_remaining > BUFFER_SIZE) {
    i2s_read(I2S_NUM_0, record_buffer + record_position, BUFFER_SIZE, &bytes_read, portMAX_DELAY);
    record_position += bytes_read;
    
    // Print progress every second
    int elapsed_sec = (millis() - record_start_time) / 1000;
    static int last_printed_sec = -1;
    if (elapsed_sec != last_printed_sec) {
      Serial.printf("   Recording: %d sec\n", elapsed_sec);
      last_printed_sec = elapsed_sec;
    }
  }
}

void stopRecording() {
  Serial.println("🛑 Recording stopped");
  is_recording = false;
  
  if (record_position > 44) {
    // Create WAV header
    createWavHeader(record_buffer, record_position - 44);
    
    float duration = (record_position - 44) / (float)(SAMPLE_RATE * 2);
    Serial.printf("   Duration: %.2f seconds\n", duration);
    Serial.printf("   Total size: %d bytes\n", record_position);
  } else {
    Serial.println("✗ No audio data recorded");
  }
}

void processAudio() {
  if (!record_buffer || record_position <= 44) {
    Serial.println("✗ No audio data to process");
    cleanupRecording();
    return;
  }
  
  Serial.println("\n📤 Sending audio to server...");
  
  bool success = sendAudioToServer();
  
  if (success) {
    Serial.println("\n✓ Processing complete!");
  } else {
    Serial.println("\n✗ Processing failed!");
  }
  
  cleanupRecording();
  Serial.println("\n>>> Ready for next recording <<<\n");
}

bool sendAudioToServer() {
  HTTPClient http;
  http.begin(server_url);
  http.addHeader("Content-Type", "audio/wav");
  http.setTimeout(20000);  // 20 second timeout
  
  Serial.printf("   Posting %d bytes to server...\n", record_position);
  
  int httpCode = http.POST(record_buffer, record_position);
  
  Serial.printf("   HTTP Response Code: %d\n", httpCode);
  
  if (httpCode == 200) {
    String response = http.getString();
    http.end();
    
    Serial.println("\n📥 Server response received:");
    Serial.println(response);
    
    // Parse JSON response
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error) {
      bool success = doc["success"].as<bool>();
      String transcript = doc["transcript"].as<String>();
      String ai_response = doc["response"].as<String>();
      
      Serial.println("\n┌─────────────────────────────────────────");
      Serial.println("│ TRANSCRIPT:");
      Serial.printf("│ %s\n", transcript.c_str());
      Serial.println("├─────────────────────────────────────────");
      Serial.println("│ AI RESPONSE:");
      Serial.printf("│ %s\n", ai_response.c_str());
      Serial.println("└─────────────────────────────────────────\n");

      display.clearDisplay();
      display.setCursor(0, 0);
      typewriterPrintScrolling(0, 0, "Transcript:\n" + transcript + "\nAI:\n" + ai_response);
      
      return success;
    } else {
      Serial.printf("✗ JSON parse error: %s\n", error.c_str());
      return false;
    }
  } else {
    String error = http.getString();
    http.end();
    Serial.printf("✗ Server error: %d\n", httpCode);
    Serial.println(error);
    return false;
  }
}

void cleanupRecording() {
  if (record_buffer) {
    free(record_buffer);
    record_buffer = nullptr;
  }
  record_position = 0;
  Serial.println("   Memory cleaned up");
}

void createWavHeader(byte* header, int wavDataSize) {
  // WAV file header (44 bytes)
  header[0] = 'R';
  header[1] = 'I';
  header[2] = 'F';
  header[3] = 'F';
  
  unsigned int fileSize = wavDataSize + 36;
  header[4] = (byte)(fileSize & 0xFF);
  header[5] = (byte)((fileSize >> 8) & 0xFF);
  header[6] = (byte)((fileSize >> 16) & 0xFF);
  header[7] = (byte)((fileSize >> 24) & 0xFF);
  
  header[8] = 'W';
  header[9] = 'A';
  header[10] = 'V';
  header[11] = 'E';
  header[12] = 'f';
  header[13] = 'm';
  header[14] = 't';
  header[15] = ' ';
  
  header[16] = 16;  // Format chunk size
  header[17] = 0;
  header[18] = 0;
  header[19] = 0;
  
  header[20] = 1;   // Audio format (1 = PCM)
  header[21] = 0;
  
  header[22] = 1;   // Number of channels (1 = mono)
  header[23] = 0;
  
  header[24] = (byte)(SAMPLE_RATE & 0xFF);
  header[25] = (byte)((SAMPLE_RATE >> 8) & 0xFF);
  header[26] = (byte)((SAMPLE_RATE >> 16) & 0xFF);
  header[27] = (byte)((SAMPLE_RATE >> 24) & 0xFF);
  
  unsigned int byteRate = SAMPLE_RATE * 1 * BITS_PER_SAMPLE / 8;
  header[28] = (byte)(byteRate & 0xFF);
  header[29] = (byte)((byteRate >> 8) & 0xFF);
  header[30] = (byte)((byteRate >> 16) & 0xFF);
  header[31] = (byte)((byteRate >> 24) & 0xFF);
  
  header[32] = (BITS_PER_SAMPLE / 8);  // Block align
  header[33] = 0;
  
  header[34] = BITS_PER_SAMPLE;  // Bits per sample
  header[35] = 0;
  
  header[36] = 'd';
  header[37] = 'a';
  header[38] = 't';
  header[39] = 'a';
  
  header[40] = (byte)(wavDataSize & 0xFF);
  header[41] = (byte)((wavDataSize >> 8) & 0xFF);
  header[42] = (byte)((wavDataSize >> 16) & 0xFF);
  header[43] = (byte)((wavDataSize >> 24) & 0xFF);
}



void connectWiFi() {
  Serial.printf("\nConnecting to WiFi: %s\n", ssid);
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi connected!");
    Serial.print("   IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n✗ WiFi connection failed!");
    Serial.println("   Check SSID and password");
  }
}

bool checkServerStatus() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }
  
  Serial.println("\nChecking server status...");
  
  HTTPClient http;
  http.begin(server_status_url);
  http.setTimeout(5000);
  
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String response = http.getString();
    http.end();
    
    DynamicJsonDocument doc(512);
    deserializeJson(doc, response);
    
    bool ready = doc["ready"].as<bool>();
    Serial.printf("   Server ready: %s\n", ready ? "YES" : "NO");
    
    return ready;
  }
  
  http.end();
  Serial.printf("   Status check failed (HTTP %d)\n", httpCode);
  return false;
}

void printMemoryInfo() {
  Serial.println("\nMemory Info:");
  Serial.printf("   Free Heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("   Heap Size: %d bytes\n", ESP.getHeapSize());
  
  if (psramFound()) {
    Serial.printf("   Free PSRAM: %d bytes\n", ESP.getFreePsram());
    Serial.printf("   Total PSRAM: %d bytes\n", ESP.getPsramSize());
  } else {
    Serial.println("   PSRAM: Not found");
  }
}
