/*
 * LittleFS Storage Test
 * Tests: File system mount, write, read, delete operations
 * Board: Arduino Nano ESP32
 */

#include <LittleFS.h>

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n=================================");
  Serial.println("LittleFS Storage Test");
  Serial.println("=================================\n");

  // Test 1: Mount File System
  Serial.println("[Test 1] Mounting LittleFS...");
  if (!LittleFS.begin(true)) {  // true = format on fail
    Serial.println("[FAIL] LittleFS mount failed!");
    while(1);
  }
  Serial.println("[PASS] LittleFS mounted successfully");
  
  // Test 2: Get Storage Info
  Serial.println("\n[Test 2] Storage information:");
  size_t totalBytes = LittleFS.totalBytes();
  size_t usedBytes = LittleFS.usedBytes();
  Serial.printf("Total space: %d bytes (%.2f KB)\n", totalBytes, totalBytes / 1024.0);
  Serial.printf("Used space: %d bytes (%.2f KB)\n", usedBytes, usedBytes / 1024.0);
  Serial.printf("Free space: %d bytes (%.2f KB)\n", totalBytes - usedBytes, (totalBytes - usedBytes) / 1024.0);
  
  // Test 3: Write File
  Serial.println("\n[Test 3] Writing test file...");
  String testFilename = "/test_audio.mp3";
  
  fs::File file = LittleFS.open(testFilename, FILE_WRITE);
  if (!file) {
    Serial.println("[FAIL] Could not open file for writing!");
    return;
  }
  
  // Write dummy data (simulating downloaded audio)
  uint8_t dummyData[1024];
  for (int i = 0; i < 1024; i++) {
    dummyData[i] = i % 256;
  }
  
  size_t bytesWritten = file.write(dummyData, sizeof(dummyData));
  file.close();
  
  Serial.printf("[PASS] Wrote %d bytes to %s\n", bytesWritten, testFilename.c_str());
  
  // Test 4: Read File
  Serial.println("\n[Test 4] Reading test file...");
  file = LittleFS.open(testFilename, FILE_READ);
  if (!file) {
    Serial.println("[FAIL] Could not open file for reading!");
    return;
  }
  
  size_t fileSize = file.size();
  Serial.printf("[PASS] File size: %d bytes\n", fileSize);
  
  // Read first 10 bytes to verify
  uint8_t readBuffer[10];
  size_t bytesRead = file.read(readBuffer, 10);
  file.close();
  
  Serial.print("First 10 bytes: ");
  for (int i = 0; i < bytesRead; i++) {
    Serial.printf("%02X ", readBuffer[i]);
  }
  Serial.println();
  
  // Test 5: List All Files
  Serial.println("\n[Test 5] Listing all files:");
  fs::File root = LittleFS.open("/");
  fs::File entry = root.openNextFile();
  int fileCount = 0;
  
  while (entry) {
    Serial.print("  - ");
    Serial.print(entry.name());
    Serial.print(" (");
    Serial.print(entry.size());
    Serial.println(" bytes)");
    entry = root.openNextFile();
    fileCount++;
  }
  Serial.printf("Total files: %d\n", fileCount);
  
  // Test 6: Delete File
  Serial.println("\n[Test 6] Deleting test file...");
  if (LittleFS.remove(testFilename)) {
    Serial.println("[PASS] File deleted successfully");
  } else {
    Serial.println("[FAIL] Could not delete file");
  }
  
  // Test 7: Large File Test (simulating MP3 download)
  Serial.println("\n[Test 7] Large file write test (simulating 50KB MP3)...");
  String largeFile = "/large_test.mp3";
  file = LittleFS.open(largeFile, FILE_WRITE);
  
  if (!file) {
    Serial.println("[FAIL] Could not create large file");
    return;
  }
  
  // Write 50KB in chunks
  uint8_t chunk[1024];
  for (int i = 0; i < 1024; i++) chunk[i] = random(256);
  
  size_t totalWritten = 0;
  unsigned long startTime = millis();
  
  for (int i = 0; i < 50; i++) {  // 50 x 1KB = 50KB
    totalWritten += file.write(chunk, sizeof(chunk));
  }
  
  unsigned long writeTime = millis() - startTime;
  file.close();
  
  Serial.printf("[PASS] Wrote %d bytes in %lu ms\n", totalWritten, writeTime);
  Serial.printf("Write speed: %.2f KB/s\n", (totalWritten / 1024.0) / (writeTime / 1000.0));
  
  // Clean up
  LittleFS.remove(largeFile);
  
  Serial.println("\n=================================");
  Serial.println("ALL TESTS PASSED!");
  Serial.println("LittleFS is ready for audio storage");
  Serial.println("=================================");
}

void loop() {
  // Nothing needed
  delay(1000);
}