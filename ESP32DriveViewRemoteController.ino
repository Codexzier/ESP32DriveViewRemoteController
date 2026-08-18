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

// Pin Connections with XIAO ESP32-S3 with GC9A01 TFT Display
// ESP32              |  TFT Display
// -------------------------------- 
// GND                | GND
// 3,3V               | VCC
// D10 (GPIO9, MOSI)  | SDA
// D8  (GPIO7, SCK)   | SCL
// D0  (GPIO1)        | CS
// D1  (GPIO2)        | DC
// D2  (GPIO3)        | RST

#include <Arduino.h>

// ========================================================================================
// Access Point and network server
#include <WiFi.h>
#include <NetworkClient.h>
#include <WiFiAP.h>

// Set these to your desired credentials.
const char *ssid = "fpv_remotecontroller";
const char *password = "12345678";

// ==================================================
// Client
const char* serverIP = "192.168.4.2";     // IP of ESP32-Server
const uint16_t serverPort = 5001;         // Port of Servers
WiFiClient _client;



// ==================================================
// Display GC9A01A

// libraries for TFT Display
#include "GC9A01_LTSM.hpp"
#include "fonts_LTSM/FontArialBold_LTSM.hpp" // 16x16 pixels

// Pin Mapping XIAO ESP32-S3
#define TFT_CS     D0 // Cable Select
#define TFT_DC     D1 // 
#define TFT_RST    D2 //

uint32_t mTFT_SCLK_FREQ = 8000000;  // Spi freq in Hertz
GC9A01_LTSM mTft;

const uint16_t mColorText = mTft.C_CYAN;
const uint16_t mColorOn = mTft.C_CYAN;
const uint16_t mColorOff = mTft.C_DCYAN;

#define IMAGE_WIDTH  240
#define IMAGE_HEIGHT 240
#define IMAGE_SIZE   (IMAGE_WIDTH * IMAGE_HEIGHT * 2)  // 115200 Bytes für RGB565

uint8_t imageBuffer[IMAGE_SIZE];
size_t receivedBytes = 0;

// ==================================================
// jpeg
#include <TJpg_Decoder.h>

// Callback-Funktion für TJpg_Decoder
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  //Serial.println("callback function for TJeg!");
  mTft.drawBitmap16Data(x, y, (uint8_t*)bitmap, w, h);
  
  return true;
}

long _timeoutForReconnect = 30;
long _lastmillis = 0;


void setup() {
  
  // activ signal LED
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("FPV Remote Controller");
  Serial.println("Configuring access point...");

  // setup access point
  if (!WiFi.softAP(ssid, password)) {
    log_e("Soft AP creation failed.");
    Serial.println("Soft AP creation failed.");
    while (1);
  }

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);


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

  TJpgDec.setJpgScale(1);
  
  TJpgDec.setSwapBytes(true);

  TJpgDec.setCallback(tft_output);
  delay(1000);


  connectToServer();

  mTft.setCursor(40, 90);
  mTft.print("Client connected");
  mTft.writeBuffer();
  Serial.println("New Client.");  // print a message out the serial port
  delay(2000);
}

void loop() {

  if(_client.connected()){ 
    //Serial.println("connected");
    digitalWrite(LED_BUILTIN, true);

    if (_client.available() >= sizeof(uint32_t)) {     // if there's bytes to read from the client

       // 1. Bildgröße empfangen
      uint8_t sizeBytes[4];
      _client.read(sizeBytes, 4);  // 4 Bytes empfangen
      uint32_t jpgSize = (sizeBytes[0] << 24) |
                  (sizeBytes[1] << 16) |
                  (sizeBytes[2] << 8)  |
                  sizeBytes[3];  // Big-Endian zu uint32_t umwandeln

      //Serial.println("read transfer");

      // Lies die verfügbaren Bytes in den Puffer
      // size_t bytesToRead = min((size_t)_client.available(), IMAGE_SIZE - receivedBytes);
      // size_t bytesRead = _client.read(&imageBuffer[receivedBytes], bytesToRead);
      // receivedBytes += bytesRead;
      // 2. JPEG-Daten empfangen
      //uint8_t *jpgBuffer = (uint8_t *)malloc(jpgSize);
      //size_t received = 0;

      

      // while (received < jpgSize) {
      //   if (_client.available()) {
      //     received += _client.read(jpgBuffer + received, jpgSize - received);
      //   }
      //   sprintf(buffer, "%d", received);
      //   Serial.print("received "); Serial.println(buffer);
      //   delay(1);
      // }
      // 2. JPEG-Daten empfangen
  uint8_t *jpgBuffer = (uint8_t *)malloc(jpgSize);
  size_t received = 0;

  while (received < jpgSize) {
    if (_client.available()) {
      received += _client.read(jpgBuffer + received, jpgSize - received);
      
    }
    delay(1);
  }

  // char buffer[12];
  //     sprintf(buffer, "%d", jpgSize);
  //     Serial.print("jpg size "); Serial.println(buffer);
  // sprintf(buffer, "%d", received);
  //     Serial.print("received "); Serial.println(buffer);

      TJpgDec.drawJpg(0, 0, jpgBuffer, jpgSize);
free(jpgBuffer);
    //Serial.println("Has write to draw jpg. Wait for Callback");
    mTft.writeBuffer();

    updateFPS();

      // Wenn das gesamte Bild empfangen wurde
      //if (receivedBytes == IMAGE_SIZE) {
        
        // Hier: Puffer an GC9A01_LTSM übergeben
        //mTft.drawBitmap16Data(0, 0, (uint8_t *)imageBuffer, 240, 240);
        //receivedBytes = 0;  // Zurücksetzen für nächstes Bild
        //mTft.writeBuffer();
      //}
    }

    digitalWrite(LED_BUILTIN, false);
  }
  else {
    Serial.println("connection break");
    delay(3000);
  }

  delay(10);
}

void connectToServer() {
  while(!_client.connect(serverIP, serverPort)){

    mTft.setCursor(40, 90);
    mTft.print("No connection");
    mTft.setCursor(40, 110);
    mTft.print("with a client!");
    mTft.writeBuffer();

    Serial.println("No connection with a client!");
    delay(1000);
  }
}

void updateFPS() {
  static uint32_t lastCheckTime = 0; // Zeitpunkt der letzten Messung
  static uint32_t frameCount = 0;    // Zähler für die Frames
  static float currentFPS = 0.0;      // Gespeicherter FPS-Wert

  frameCount++; // Wird bei jedem Aufruf (jedes gesendete Bild) erhöht

  // Prüfen, ob 1 Sekunde (1000 ms) vergangen ist
  if (millis() - lastCheckTime >= 1000) {
    // FPS berechnen (für den Fall, dass das Intervall leicht abweicht)
    currentFPS = (float)frameCount * 1000.0 / (millis() - lastCheckTime);
    
    // FPS auf der seriellen Schnittstelle ausgeben
    Serial.printf("Gesendete Bilder/Sekunde (FPS): %.2f\n", currentFPS);

    // Zähler und Timer zurücksetzen
    frameCount = 0;
    lastCheckTime = millis();
  }
}