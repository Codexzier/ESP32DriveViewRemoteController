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

// Set these to your desired credentials.
const char *ssid = "fpv_remotecontroller";
const char *password = "12345678";

const char* serverIP = "192.168.4.2";  // IP des ESP32-Servers
const uint16_t serverPort = 8080;         // Port des Servers
WiFiClient _client;

// ========================================================================================
// libraries for TFT Display
#include "GC9A01_LTSM.hpp"
#include "fonts_LTSM/FontArialBold_LTSM.hpp" // 16x16 pixels
//#include "fonts_LTSM/FontPico_LTSM.hpp" // 8x12 pixels
//#include "fonts_LTSM/FontSinclairS_LTSM.hpp" // 8x8 pixels

// ========================================================================================
// Display GC9A01A

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

  //_client.setConnectionTimeout(3000);
  connectToServer();

  mTft.setCursor(40, 90);
  mTft.print("Client connected");
  mTft.writeBuffer();
  Serial.println("New Client.");  // print a message out the serial port
  delay(2000);
}

void loop() {

  if(_client.connected()){ 
    Serial.println("connected");
    digitalWrite(LED_BUILTIN, true);

    if (_client.available()) {     // if there's bytes to read from the client

      Serial.println("read transfer");
      // Lies die verfügbaren Bytes in den Puffer
      size_t bytesToRead = min((size_t)_client.available(), IMAGE_SIZE - receivedBytes);
      size_t bytesRead = _client.read(&imageBuffer[receivedBytes], bytesToRead);
      receivedBytes += bytesRead;

      // Wenn das gesamte Bild empfangen wurde
      if (receivedBytes == IMAGE_SIZE) {
        // Hier: Puffer an GC9A01_LTSM übergeben
        mTft.drawBitmap16Data(0, 0, (uint8_t *)imageBuffer, 240, 240);
        receivedBytes = 0;  // Zurücksetzen für nächstes Bild
        mTft.writeBuffer();
      }
    }
    else {
      long actual = millis();
      if(actual > _lastmillis + 5000) {
        //Serial.println("reconnect");
        //_client.stop();
        //connectToServer();
      }
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
    //_client.stop();
    delay(1000);
  }
}