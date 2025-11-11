#include <ESP8266WiFi.h>
#include <Wire.h>

// --- Configuration ---
const char* ssid = "abid";       // Put your Wi-Fi name here
const char* password = "abid2003"; // Put your Wi-Fi password here

#define I2C_ADDRESS 8 // This is the ESP8266's slave address

// This function runs when the Arduino (master) requests data
void onRequest() {
  if (WiFi.status() == WL_CONNECTED) {
    Wire.write(1); // Send '1' for "Connected"
  } else {
    Wire.write(0); // Send '0' for "Not Connected"
  }
}

void setup() {
  Serial.begin(9600);
  
  // --- Setup I2C Slave ---
  // The diagram uses D2 (GPIO 4) as SDA and D1 (GPIO 5) as SCL
  // These are the defaults, so we just need to begin.
  Wire.begin(I2C_ADDRESS);
  Wire.onRequest(onRequest); // Register the function to call on request
  
  Serial.println("\nI2C Slave Initialized.");
  
  // --- Connect to WiFi ---
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // The I2C communication is handled by interrupts
  // so the main loop can be empty.
  delay(100);
}
