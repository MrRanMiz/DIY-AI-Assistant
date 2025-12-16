/*
 * OLED Test with Explicit ESP32 Pin Mapping
 * Board: Arduino Nano ESP32
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 
#define OLED_ADDRESS 0x3C

// Arduino Nano ESP32 I2C pins
#define I2C_SDA 11  // Try this first
#define I2C_SCL 12  // Try this first

// Alternative pins if above don't work:
// #define I2C_SDA 11
// #define I2C_SCL 12

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== OLED Hardware Test ===");
  Serial.print("Using SDA: GPIO");
  Serial.print(I2C_SDA);
  Serial.print(", SCL: GPIO");
  Serial.println(I2C_SCL);
  
  // Initialize I2C with explicit pins
  Wire.begin(I2C_SDA, I2C_SCL);
  delay(100); // Give I2C time to initialize

  // Scan for I2C devices
  Serial.println("Scanning I2C bus...");
  byte error, address;
  int nDevices = 0;
  bool oledFound = false;

  for(address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Device found at 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      nDevices++;
      
      if (address == OLED_ADDRESS || address == 0x3D) {
        oledFound = true;
      }
    }
  }

  if (nDevices == 0) {
    Serial.println("NO I2C devices found!");
    Serial.println("\nTroubleshooting:");
    Serial.println("1. Check wiring:");
    Serial.print("   OLED VCC -> ESP32 3.3V or 5V\n");
    Serial.print("   OLED GND -> ESP32 GND\n");
    Serial.print("   OLED SDA -> ESP32 GPIO");
    Serial.println(I2C_SDA);
    Serial.print("   OLED SCL -> ESP32 GPIO");
    Serial.println(I2C_SCL);
    Serial.println("2. Try different GPIO pins (see code comments)");
    Serial.println("3. Check OLED power LED (if present)");
    for(;;);
  }

  Serial.print("Found ");
  Serial.print(nDevices);
  Serial.println(" device(s)");

  if (!oledFound) {
    Serial.println("OLED not at expected address 0x3C or 0x3D");
    for(;;);
  }

  // Initialize display
  Serial.println("Initializing OLED driver...");
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) { 
    Serial.println("Display driver init failed!");
    for(;;);
  }

  Serial.println("OLED Ready!");

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(F("SYSTEM TEST:"));
  display.println(F("----------------"));
  display.setTextSize(2);
  display.println(F("OLED: OK!"));
  display.display();
}

void loop() {
  display.invertDisplay(true); 
  delay(500);
  display.invertDisplay(false);
  delay(500);
}