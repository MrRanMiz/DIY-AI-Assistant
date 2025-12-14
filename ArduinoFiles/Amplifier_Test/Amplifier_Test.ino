#include <driver/i2s.h>
#include <cmath>

// --- I2S Configuration Parameters ---
#define I2S_NUM I2S_NUM_0
#define SAMPLE_RATE 16000
#define TONE_FREQ 440
#define AMPLITUDE 10000

// --- Pin Definitions for Nano ESP32 ---
#define I2S_BCK_PIN 4   // A3 = GPIO 4  (BCLK)
#define I2S_WS_PIN 3    // A2 = GPIO 3  (LRC/WS)
#define I2S_DOUT_PIN 9  // D6 = GPIO 9  (DIN)

// Buffer and time tracking for sine wave generation
#define DMA_BUF_LEN 64
int16_t sample_buffer[DMA_BUF_LEN];
float t = 0;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting MAX98357A Sine Wave Test");
    Serial.printf("BCLK: GPIO %d, WS: GPIO %d, DOUT: GPIO %d\n", 
                  I2S_BCK_PIN, I2S_WS_PIN, I2S_DOUT_PIN);

    // --- I2S TX Configuration (Amplifier Output) ---
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = 8,
        .dma_buf_len = DMA_BUF_LEN,
        .use_apll = false
    };

    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);

    // --- I2S Pin Configuration ---
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCK_PIN,
        .ws_io_num = I2S_WS_PIN,
        .data_out_num = I2S_DOUT_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    i2s_set_pin(I2S_NUM, &pin_config);
    
    Serial.println("I2S initialized - you should hear 440Hz tone");
       // Test 1: Verify buffer generation
    Serial.println("\n=== Buffer Test ===");
    for (int i = 0; i < 10; i++) {
        sample_buffer[i] = (int16_t)(AMPLITUDE * sin(2.0 * M_PI * TONE_FREQ * i / SAMPLE_RATE));
        Serial.printf("Sample[%d]: %d\n", i, sample_buffer[i]);
    }
}

void loop() {
    size_t bytes_written = 0;

    // --- 1. Generate Sine Wave Data ---
    for (int i = 0; i < DMA_BUF_LEN; i++) {
        sample_buffer[i] = (int16_t)(AMPLITUDE * sin(2.0 * M_PI * TONE_FREQ * t / SAMPLE_RATE));
        t += 1.0;
    }

    // --- 2. Write Data to I2S Peripheral ---
    i2s_write(I2S_NUM, sample_buffer, sizeof(sample_buffer), &bytes_written, portMAX_DELAY);

    // Reset 't' if necessary
    if (t > SAMPLE_RATE) {
        t -= SAMPLE_RATE;
    }
     i2s_write(I2S_NUM, sample_buffer, sizeof(sample_buffer), &bytes_written, portMAX_DELAY);
    
    static unsigned long last_print = 0;
    if (millis() - last_print > 1000) {
        Serial.printf("Bytes written: %d (expected: %d)\n", bytes_written, sizeof(sample_buffer));
        last_print = millis();
        }
}