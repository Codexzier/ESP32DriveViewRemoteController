// ========================================================================================
//      Meine Welt in meinem Kopf
// ========================================================================================
// Projekt:       ESP32 Drive View - Remote Controller
// Author:        Johannes P. Langner
// Controller:    XIAO ESP32-S3
// Actor:         TFT GC9A01, XY-Analog Stick
// Description:   
// Stand:         17.08.2026
// ========================================================================================

#include <Arduino.h>

// ========================================================================================
// Access Point and network server
#include <WiFi.h>
#include <NetworkClient.h>
#include <WiFiAP.h>

const char *ssid = "fpv_remotecontroller";
const char *password = "12345678";

int _serverPort = 8080;  
WiFiServer _server;

// ========================================================================================
// Display GC9A01A
// TIP: you must enable the Screen Buffer.

// libraries for TFT Display
#include "GC9A01_LTSM.hpp"
#include "fonts_LTSM/FontArialBold_LTSM.hpp" // 16x16 pixels
//#include "fonts_LTSM/FontPico_LTSM.hpp" // 8x12 pixels
//#include "fonts_LTSM/FontSinclairS_LTSM.hpp" // 8x8 pixels

// Pin Connections with XIAO ESP32-S3
// ESP32              |  TFT Display
// -------------------------------- 
// GND                | GND
// 3,3V               | VCC
// D10 (GPIO9, MOSI)  | SDA
// D8  (GPIO7, SCK)   | SCL
// D0  (GPIO1)        | CS
// D1  (GPIO2)        | DC
// D2  (GPIO3)        | RST

// Pin Mapping XIAO ESP32-S3
#define TFT_CS     D0 // Cable Select
#define TFT_DC     D1 // 
#define TFT_RST    D2 //

uint32_t mTFT_SCLK_FREQ = 8000000;  // Spi freq in Hertz
GC9A01_LTSM mTft;

// Base Colers
const uint16_t mColorText = mTft.C_CYAN;
const uint16_t mColorOn = mTft.C_CYAN;
const uint16_t mColorOff = mTft.C_DCYAN;

// Screen buffer for receiving
#define IMAGE_WIDTH  240
#define IMAGE_HEIGHT 240
#define IMAGE_SIZE   (IMAGE_WIDTH * IMAGE_HEIGHT * 2)  // 115200 Bytes für RGB565
uint8_t imageBuffer[IMAGE_SIZE];
size_t _receivedBytes = 0;

// ========================================================================================
// any

long _lastmillis = 0;

void setup() {
  
  // activ signal LED
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("FPV Remote Controller");
  Serial.println("Configuring access point...");

  // ----------------------------------------------
  // setup access point
  if (!WiFi.softAP(ssid, password)) {
    log_e("Soft AP creation failed.");
    Serial.println("Soft AP creation failed.");
    while (1);
  }
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  // ----------------------------------------------
  // tft
  Serial.println("start TFT setup!");
  mTft.TFTsetupGPIO_SPI(mTFT_SCLK_FREQ, TFT_RST, TFT_DC, TFT_CS);
  mTft.TFTInitScreenSize(240, 240);
  mTft.TFTGC9A01Initialize();
  mTft.setTextCharPixelOrBuffer(true);
  mTft.fillScreen(mTft.C_BLACK);
  mTft.TFTsetRotation(display16_graphics_LTSM::Degrees_180);

  if(mTft.setBuffer() != DisLib16::Success){
    Serial.println("buffer not set");
    while(3){};
  }

  mTft.clearBuffer();
  mTft.setCursor(50, 50);
  mTft.setFont(FontDefault); 
  mTft.setTextColor(mColorText, mTft.C_BLACK);
  mTft.print("TFT has initialize!");
  mTft.writeBuffer();
  delay(1000);

  Serial.println("TFT has initialize!");

  mTft.clearBuffer();
  mTft.setTextColor(mColorText, mTft.C_BLACK);
  mTft.setCursor(40, 50);
  mTft.print("Try to connect");
  mTft.setCursor(40, 70);
  mTft.print("with client.");
  mTft.writeBuffer();
  delay(1000);

    // ----------------------------------------------
  // start server
  _server.begin(_serverPort); 
  _server.setNoDelay(true);  // Deaktiviert Nagle's Algorithmus (schnelleres Senden)
  //_server.setTimeout(30000);
  Serial.println("Server started!");
  char buffer[12];
  sprintf(buffer, "%d", _serverPort);
  Serial.print("Port: "); Serial.println(buffer);

  mTft.setCursor(40, 90);
  mTft.print("Server started! ");
  mTft.writeBuffer();
  
  delay(2000);
}

void loop() {

  digitalWrite(LED_BUILTIN, false);
  Serial.println("------------------------------");
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  char buffer[12];
  sprintf(buffer, "%d", _serverPort);
  Serial.print("Server Port: "); Serial.println(buffer);

  Serial.println("wait for client connecting");
  delay(200);
  WiFiClient client = _server.accept();

  if(client){
    Serial.println("connected client");

    if(client.connected()){ 
      Serial.println("connected");
      digitalWrite(LED_BUILTIN, true);
      _receivedBytes = 0; 

      while (client.available()) {     // if there's bytes to read from the client

        Serial.println("read transfer");
        // Lies die verfügbaren Bytes in den Puffer
        size_t bytesToRead = min((size_t)client.available(), IMAGE_SIZE - _receivedBytes);
        size_t bytesRead = client.read(&imageBuffer[_receivedBytes], bytesToRead);
        _receivedBytes += bytesRead;

        // Wenn das gesamte Bild empfangen wurde
        if (_receivedBytes == IMAGE_SIZE) {
          // Hier: Puffer an GC9A01_LTSM übergeben
          mTft.drawBitmap16Data(0, 0, (uint8_t *)imageBuffer, 240, 240);
          _receivedBytes = 0;  // Zurücksetzen für nächstes Bild
          mTft.writeBuffer();
        }
        sprintf(buffer, "%d", _receivedBytes);
        Serial.print("Received bytes: "); Serial.println(_receivedBytes, DEC);
        client.write("ACK");
        delay(1);
      }

      digitalWrite(LED_BUILTIN, false);
      Serial.println("Disconnected");
      client.stop();
    }
    else {
      Serial.println("connection break");
      delay(3000);
    }
  }
  else {
    Serial.println("Client is not set!");
    delay(1000);
  }

  delay(10);
}

