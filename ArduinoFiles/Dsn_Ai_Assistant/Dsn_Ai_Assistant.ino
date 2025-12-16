#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "driver/i2s.h"
#include <LittleFS.h>

#include "AudioFileSourceLittleFS.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_SSID_PASSWORD";

String server_url = "https://YourUserID-YourServerName.hf.space/process_audio";
String server_base_url = "https://YourUserID-YourServerName.hf.space";

#define BUTTON_PIN 5

// I2C pins for SSD1306 OLED
#define OLED_SDA 21
#define OLED_SCL 22
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1  // Reset pin not used

#define I2S0_SCK 18  // Microphone
#define I2S0_WS 17
#define I2S0_DIN 15

#define I2S1_BCLK 3  // Speaker
#define I2S1_LRC 8
#define I2S1_DOUT 16
#define I2S_SD 46

const int SAMPLE_RATE = 16000;
const int BITS_PER_SAMPLE = 16;
const int BUFFER_SIZE = 1024;
const int MAX_RECORD_TIME_MS = 10000;
String getAnswer = "";

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

uint8_t* record_buffer = nullptr;
size_t record_buffer_size = 0;
size_t record_position = 0;
bool is_recording = false;
unsigned long record_start_time = 0;
bool i2s_initialized = false;

void controlSpeakerPower(bool enable) {
  digitalWrite(I2S_SD, enable ? HIGH : LOW);
  Serial.printf("Speaker power: %s\n", enable ? "ON" : "OFF");
}

bool initLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed!");
    return false;
  }
  Serial.println("LittleFS mounted successfully");
  return true;
}

void printMemoryInfo() {
  Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("Heap Size: %d bytes\n", ESP.getHeapSize());
  if (psramFound()) {
    Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
    Serial.printf("Total PSRAM: %d bytes\n", ESP.getPsramSize());
  }
}

void displayMessage(const char* msg, bool invert = false) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  
  if (invert) {
    display.fillRect(0, 0, SCREEN_WIDTH, 10, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  }
  
  display.println(msg);
  display.display();
}

void displayTwoLines(const char* line1, const char* line2, bool invert = false) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  if (invert) {
    display.fillRect(0, 0, SCREEN_WIDTH, 20, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  }
  
  display.setCursor(0, 0);
  display.println(line1);
  display.setCursor(0, 12);
  display.println(line2);
  display.display();
}

void setup() {
  Serial.begin(115200);

  // Initialize I2C for OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  
  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  display.clearDisplay();
  display.display();

  i2s_config_t rec_cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = (i2s_bits_per_sample_t)BITS_PER_SAMPLE,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 1024
  };

  i2s_pin_config_t rec_pin = {
    .bck_io_num = I2S0_SCK,
    .ws_io_num = I2S0_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S0_DIN
  };

  i2s_driver_install(I2S_NUM_0, &rec_cfg, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &rec_pin);

  i2s_config_t play_cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = (i2s_bits_per_sample_t)BITS_PER_SAMPLE,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 1024
  };

  i2s_pin_config_t play_pin = {
    .bck_io_num = I2S1_BCLK,
    .ws_io_num = I2S1_LRC,
    .data_out_num = I2S1_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_1, &play_cfg, 0, NULL);
  i2s_set_pin(I2S_NUM_1, &play_pin);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(I2S_SD, OUTPUT);
  digitalWrite(I2S_SD, LOW);

  if (!initLittleFS()) {
    Serial.println("LittleFS init failed!");
  }

  displayMessage("Starting...");

  Serial.println("System starting...");
  printMemoryInfo();

  WiFi.begin(ssid, password);
  int wifi_timeout = 0;
  while (WiFi.status() != WL_CONNECTED && wifi_timeout < 20) {
    delay(500);
    Serial.print(".");
    wifi_timeout++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi connection failed!");
    displayMessage("WiFi Failed!", true);
    while (1);
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  displayMessage("Checking server...");

  if (checkServerStatus()) {
    displayTwoLines("Assistant", "Ready");
  } else {
    displayMessage("Server Not Ready", true);
  }
}

bool checkServerStatus() {
  HTTPClient http;
  http.begin(server_base_url + "/status");
  http.setTimeout(5000);

  int httpCode = http.GET();
  if (httpCode == 200) {
    String response = http.getString();
    http.end();

    DynamicJsonDocument doc(512);
    deserializeJson(doc, response);

    return doc["ready"].as<bool>();
  }

  http.end();
  return false;
}

void loop() {
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

  if (is_recording) {
    recordAudioData();

    if (millis() - record_start_time > MAX_RECORD_TIME_MS) {
      stopRecording();
      processAudio();
    }
  }

  delay(10);
}

void startRecording() {
  Serial.println("Recording started...");
  is_recording = true;
  record_start_time = millis();

  record_buffer_size = 15 * SAMPLE_RATE * (BITS_PER_SAMPLE / 8) + 44;
  record_buffer = (uint8_t*)malloc(record_buffer_size);

  if (!record_buffer) {
    Serial.println("Memory allocation failed!");
    is_recording = false;
    return;
  }

  record_position = 44;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Recording...");
  display.setCursor(0, 20);
  display.println("0 sec");
  display.display();
}

void recordAudioData() {
  size_t bytes_read;
  size_t space_remaining = record_buffer_size - record_position;

  if (space_remaining > BUFFER_SIZE) {
    i2s_read(I2S_NUM_0, record_buffer + record_position, BUFFER_SIZE, &bytes_read, 0);
    record_position += bytes_read;

    int elapsed_sec = (millis() - record_start_time) / 1000;
    
    // Update only the time portion
    display.fillRect(0, 20, 60, 10, SSD1306_BLACK);
    display.setCursor(0, 20);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.printf("%d sec", elapsed_sec);
    display.display();
  }
}

void stopRecording() {
  Serial.println("Recording stopped...");
  is_recording = false;

  if (record_position > 44) {
    createWavHeader(record_buffer, record_position - 44);
    float duration = (record_position - 44) / (float)(SAMPLE_RATE * 2);
    Serial.printf("Recorded duration: %.1f seconds\n", duration);
  }
}

void processAudio() {
  if (!record_buffer || record_position <= 44) {
    Serial.println("No audio data to process");
    cleanupRecording();
    return;
  }

  displayMessage("Processing...");

  bool success = sendAudioToServer();

  if (success) {
    displayMessage("Success!");
  } else {
    displayMessage("Failed!", true);
    delay(2000);
  }

  cleanupRecording();

  displayTwoLines("Ready", "");
}

void cleanupRecording() {
  if (record_buffer) {
    free(record_buffer);
    record_buffer = nullptr;
  }
  record_position = 0;
}

bool sendAudioToServer() {
  HTTPClient http;
  http.begin(server_url);
  http.addHeader("Content-Type", "audio/wav");
  http.setTimeout(20000);

  Serial.printf("Sending %d bytes to server...\n", record_position);

  int httpCode = http.POST(record_buffer, record_position);

  if (httpCode == 200) {
    String response = http.getString();
    http.end();

    Serial.println("Audio sent successfully");

    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, response);

    if (!error) {
      String file_id = doc["file_id"].as<String>();
      String stream_url = doc["stream_url"].as<String>();
      String message = doc["message"].as<String>();
      getAnswer = message;
      Serial.printf("Response: %s\n", message.c_str());

      return downloadAndPlayAudio(server_base_url + stream_url, file_id);
    } else {
      Serial.printf("JSON parse error: %s\n", error.c_str());
      return false;
    }
  } else {
    String error = http.getString();
    http.end();
    Serial.printf("Server error: %d - %s\n", httpCode, error.c_str());
    return false;
  }
}

bool downloadAndPlayAudio(const String& stream_url, const String& file_id) {
  String local_filename = "/response_" + file_id + ".mp3";

  displayMessage("Thinking...");

  if (!downloadAudioFile(stream_url, local_filename)) {
    Serial.println("Download failed!");
    return false;
  }

  // Display AI response (truncated to fit screen)
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  
  // Wrap text to fit 128px width (approx 21 chars per line at size 1)
  int maxChars = 21;
  int lines = 0;
  int startIdx = 0;
  
  while (startIdx < getAnswer.length() && lines < 6) {
    int endIdx = startIdx + maxChars;
    if (endIdx > getAnswer.length()) {
      endIdx = getAnswer.length();
    }
    
    String line = getAnswer.substring(startIdx, endIdx);
    display.println(line);
    startIdx = endIdx;
    lines++;
  }
  
  display.display();

  bool playback_success = playAudioFile(local_filename);

  LittleFS.remove(local_filename);

  return playback_success;
}

bool downloadAudioFile(const String& url, const String& filename) {
  HTTPClient http;
  http.begin(url);
  http.setTimeout(15000);

  int httpCode = http.GET();
  Serial.printf("Download HTTP Code: %d\n", httpCode);

  if (httpCode == 200) {
    int contentLength = http.getSize();
    Serial.printf("Content length: %d bytes\n", contentLength);

    if (contentLength > 100) {
      fs::File file = LittleFS.open(filename, FILE_WRITE);
      if (!file) {
        Serial.println("Could not open file for writing!");
        http.end();
        return false;
      }

      WiFiClient* stream = http.getStreamPtr();
      uint8_t buffer[1024];
      size_t totalRead = 0;

      while (http.connected() && totalRead < contentLength) {
        size_t bytesAvailable = stream->available();
        if (bytesAvailable > 0) {
          size_t bytesToRead = min(bytesAvailable, sizeof(buffer));
          size_t bytesRead = stream->readBytes(buffer, bytesToRead);

          if (bytesRead > 0) {
            file.write(buffer, bytesRead);
            totalRead += bytesRead;
          }
        }
        delay(1);
      }

      file.close();
      http.end();

      Serial.printf("Download completed: %d bytes\n", totalRead);
      return totalRead > 100;
    }
  }

  http.end();
  return false;
}

bool playAudioFile(const String& filename) {
  fs::File file = LittleFS.open(filename, FILE_READ);
  if (!file) return false;

  controlSpeakerPower(true);
  delay(100);

  uint8_t buffer[1024];
  size_t bytesRead, bytesWritten;

  while ((bytesRead = file.read(buffer, sizeof(buffer))) > 0) {
    int16_t* samples = (int16_t*)buffer;
    for (int i = 0; i < bytesRead / 2; i++) {
      samples[i] = samples[i] * 0.4;  // %40 volume
    }
    i2s_write(I2S_NUM_1, buffer, bytesRead, &bytesWritten, portMAX_DELAY);
  }

  file.close();
  controlSpeakerPower(false);
  return true;
}

void createWavHeader(byte* header, int wavDataSize) {
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
  header[16] = 16;
  header[17] = 0;
  header[18] = 0;
  header[19] = 0;
  header[20] = 1;
  header[21] = 0;
  header[22] = 1;
  header[23] = 0;
  header[24] = (byte)(SAMPLE_RATE & 0xFF);
  header[25] = (byte)((SAMPLE_RATE >> 8) & 0xFF);
  header[26] = 0;
  header[27] = 0;
  unsigned int byteRate = SAMPLE_RATE * 1 * BITS_PER_SAMPLE / 8;
  header[28] = (byte)(byteRate & 0xFF);
  header[29] = (byte)((byteRate >> 8) & 0xFF);
  header[30] = 0;
  header[31] = 0;
  header[32] = (BITS_PER_SAMPLE / 8);
  header[33] = 0;
  header[34] = BITS_PER_SAMPLE;
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