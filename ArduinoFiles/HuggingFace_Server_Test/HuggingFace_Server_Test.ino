/*
 * HuggingFace Server Test Sketch
 * 
 * This sketch tests your HuggingFace Space server connection
 * No hardware needed except ESP32 with WiFi
 * 
 * Instructions:
 * 1. Update WiFi credentials below
 * 2. Update server URL with your HuggingFace Space URL
 * 3. Upload and open Serial Monitor (115200 baud)
 * 4. Type 'test' to check server status
 * 5. Type 'ping' to test connectivity
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "COmberge_2G";
const char* password = "w3$ley_WokeSince22.";

// HuggingFace server URL (update with your Space URL)
String server_base_url = "https://wesleyhuggingface-ai-voice-assistant-deskbot.hf.space";

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=================================");
  Serial.println("HuggingFace Server Tester");
  Serial.println("=================================\n");
  
  // Connect to WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  Serial.println();
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi connection FAILED!");
    Serial.println("Please check your SSID and password");
    return;
  }
  
  Serial.println("✓ WiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Signal Strength: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm\n");
  
  // Test server immediately on startup
  Serial.println("Testing server connection...\n");
  testServerStatus();
  
  Serial.println("\n=================================");
  Serial.println("Commands:");
  Serial.println("  test  - Check server status");
  Serial.println("  ping  - Test basic connectivity");
  Serial.println("  info  - Show connection info");
  Serial.println("=================================\n");
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();
    
    if (command == "test") {
      Serial.println("\n--- Server Status Test ---");
      testServerStatus();
      Serial.println();
    }
    else if (command == "ping") {
      Serial.println("\n--- Ping Test ---");
      pingServer();
      Serial.println();
    }
    else if (command == "info") {
      Serial.println("\n--- Connection Info ---");
      showConnectionInfo();
      Serial.println();
    }
    else if (command.length() > 0) {
      Serial.println("Unknown command: " + command);
      Serial.println("Try: test, ping, or info");
    }
  }
}

void testServerStatus() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi not connected!");
    return;
  }
  
  HTTPClient http;
  String status_url = server_base_url + "/status";
  
  Serial.println("Checking: " + status_url);
  
  http.begin(status_url);
  http.setTimeout(10000);  // 10 second timeout
  
  unsigned long start_time = millis();
  int httpCode = http.GET();
  unsigned long response_time = millis() - start_time;
  
  Serial.print("Response time: ");
  Serial.print(response_time);
  Serial.println(" ms");
  
  if (httpCode == 200) {
    Serial.println("✓ HTTP Status: 200 OK");
    
    String response = http.getString();
    Serial.println("\nRaw Response:");
    Serial.println(response);
    
    // Parse JSON response
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error) {
      Serial.println("\n--- Server Details ---");
      
      if (doc.containsKey("ready")) {
        bool ready = doc["ready"].as<bool>();
        Serial.print("Server Ready: ");
        if (ready) {
          Serial.println("✓ YES");
        } else {
          Serial.println("❌ NO (Server is starting up)");
        }
      }
      
      if (doc.containsKey("status")) {
        Serial.print("Status: ");
        Serial.println(doc["status"].as<String>());
      }
      
      if (doc.containsKey("models")) {
        Serial.println("\nLoaded Models:");
        JsonObject models = doc["models"];
        for (JsonPair kv : models) {
          Serial.print("  - ");
          Serial.print(kv.key().c_str());
          Serial.print(": ");
          Serial.println(kv.value().as<String>());
        }
      }
      
      Serial.println("\n✓✓✓ SERVER IS WORKING! ✓✓✓");
      
    } else {
      Serial.print("⚠ JSON parse error: ");
      Serial.println(error.c_str());
      Serial.println("Server responded but JSON is invalid");
    }
    
  } else if (httpCode > 0) {
    Serial.print("❌ HTTP Error Code: ");
    Serial.println(httpCode);
    String response = http.getString();
    Serial.println("Response: " + response);
    
    if (httpCode == 404) {
      Serial.println("\n⚠ 404 Error - Check your server URL!");
      Serial.println("Make sure it matches your HuggingFace Space URL exactly");
    } else if (httpCode == 503) {
      Serial.println("\n⚠ 503 Service Unavailable");
      Serial.println("Your Space might be sleeping. Try again in 30 seconds.");
    }
    
  } else {
    Serial.print("❌ Connection failed! Error: ");
    Serial.println(http.errorToString(httpCode));
    Serial.println("\nPossible causes:");
    Serial.println("  - Server URL is incorrect");
    Serial.println("  - HuggingFace Space is sleeping");
    Serial.println("  - Network firewall blocking request");
    Serial.println("  - SSL/TLS certificate issue");
  }
  
  http.end();
}

void pingServer() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi not connected!");
    return;
  }
  
  HTTPClient http;
  
  Serial.println("Pinging: " + server_base_url);
  
  http.begin(server_base_url);
  http.setTimeout(10000);
  
  unsigned long start_time = millis();
  int httpCode = http.GET();
  unsigned long response_time = millis() - start_time;
  
  Serial.print("Response time: ");
  Serial.print(response_time);
  Serial.println(" ms");
  
  if (httpCode > 0) {
    Serial.print("✓ Server responded with code: ");
    Serial.println(httpCode);
    
    if (httpCode == 200 || httpCode == 404 || httpCode == 405) {
      Serial.println("✓ Server is reachable!");
    }
  } else {
    Serial.print("❌ Connection failed: ");
    Serial.println(http.errorToString(httpCode));
  }
  
  http.end();
}

void showConnectionInfo() {
  Serial.println("WiFi Status: " + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected"));
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("DNS: ");
    Serial.println(WiFi.dnsIP());
    Serial.print("Signal: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  }
  
  Serial.print("\nServer URL: ");
  Serial.println(server_base_url);
  Serial.print("Status endpoint: ");
  Serial.println(server_base_url + "/status");
  Serial.print("Process endpoint: ");
  Serial.println(server_base_url + "/process_audio");
}
