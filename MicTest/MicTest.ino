#include <driver/i2s.h>
#include <esp_intr_alloc.h> // Required for ESP_INTR_FLAG_LEVEL1

// Pin Definitions - NOTE: Ensure only standard spaces are between the pin number and comment
#define I2S_WS 6    // LRCLK (Word Select)
#define I2S_SD 4    // DOUT (Data In)
#define I2S_SCK 5   // BCLK (Bit Clock)

#define I2S_NUM I2S_NUM_0   
#define I2S_SAMPLE_RATE 16000
// Define buffer size in *samples* for DMA
#define I2S_BUFFER_SAMPLES 1024 


void setup() {
    Serial.begin(115200);
    Serial.println("INMP441 Microphone Test - Serial Plotter");

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = I2S_SAMPLE_RATE,
        // INMP441 is 24-bit, read as 32-bit (4-byte) samples
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, 
        // Read both channels (L/R/L/R...)
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, 
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = I2S_BUFFER_SAMPLES, 
        // FIX: Corrected misspelling from 'use_appl' to 'use_apll'
        .use_apll = false 
    };

    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);


    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        // Use NO_CHANGE for the unused data-out pin in RX mode
        .data_out_num = I2S_PIN_NO_CHANGE, 
        .data_in_num = I2S_SD
    };

    i2s_set_pin(I2S_NUM, &pin_config);
}

void loop() {
    // Buffer to hold 32-bit (4-byte) samples
    int32_t audio_samples[I2S_BUFFER_SAMPLES]; 
    size_t bytes_read = 0;

    // Read a chunk of data into the buffer.
    i2s_read(I2S_NUM, audio_samples, sizeof(audio_samples), &bytes_read, portMAX_DELAY);
    
    if (bytes_read > 0) {
        int num_samples = bytes_read / sizeof(int32_t); 

        // Iterate through the buffer, stepping by 2 to pick the channel with data.
        // Index 0, 2, 4... is the Left Channel (assuming L/R pin is GND)
        for (int i = 0; i < num_samples; i += 2) { 

            int32_t raw_sample = audio_samples[i]; 
            
            // The 24-bit data is Left-Justified. Shift right by 8 bits to get the useful 16 bits.
            int16_t sample_16bit = (int16_t)(raw_sample >> 8); 

            // Convert the 16-bit value to 0-1023 range for the Serial Plotter
            int plotValue = map(sample_16bit, -32768, 32767, 0, 1023);
            Serial.println(plotValue);
        }
    }
}