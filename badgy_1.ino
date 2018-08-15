/* e-paper display lib */
#include <GxEPD.h>
#include <GxGDEH029A1/GxGDEH029A1.cpp>
#include <GxIO/GxIO_SPI/GxIO_SPI.cpp>
#include <GxIO/GxIO.cpp>
/* include any other fonts you want to use https://github.com/adafruit/Adafruit-GFX-Library */
#include <Fonts/Picopixel.h>
/* WiFi  libs*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <WiFiManager.h>
#include <Hash.h>
extern "C" {
  #include "user_interface.h"
}
#include <avr/pgmspace.h>

/* Always include the update server, or else you won't be able to do OTA updates! */
/**/const int port = 8888;
/**/ESP8266WebServer httpServer(port);
/**/ESP8266HTTPUpdateServer httpUpdater;

/* Configure pins for display */
GxIO_Class io(SPI, SS, 0, 2);
GxEPD_Class display(io); // default selection of D4, D2

byte buttonState = 0;
byte lastButtonState = 0;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
const uint8_t offsetx = 8;
const uint8_t offsety = 18;
const uint8_t columns = 7;
const uint8_t rows = 16;
const uint8_t gridSize = 16;
const uint8_t width = 122;
const uint8_t height = 250;
uint8_t playerx = 4;
uint8_t playery = 9;
const GFXfont* font = &Picopixel;
char cstr[16];

void setup(){  
  display.init();
  
  pinMode(1,INPUT_PULLUP); //down
  pinMode(3,INPUT_PULLUP); //left
  pinMode(5,INPUT_PULLUP); //center
  pinMode(12,INPUT_PULLUP); //right
  pinMode(10,INPUT_PULLUP); //up
  
  /* Enter OTA mode if the center button is pressed */
  if(digitalRead(5)  == 0){
    /* WiFi Manager automatically connects using the saved credentials, if that fails it will go into AP mode */
    WiFiManager wifiManager;
    wifiManager.setAPCallback(configModeCallback);
    wifiManager.autoConnect("Badgy AP");
    /* Once connected to WiFi, startup the OTA update server if the center button is held on boot */
    httpUpdater.setup(&httpServer);
    httpServer.begin();
    showIP();
    while(1){
      httpServer.handleClient();
    }
  }
  update();
}

void showIP(){
  clearScreen();
  setFont();
  String url = WiFi.localIP().toString() + ":"+String(port)+"/update";
  byte charArraySize = url.length() + 1;
  char urlCharArray[charArraySize];
  url.toCharArray(urlCharArray, charArraySize);
  showText(urlCharArray, 0, 10);
  display.update();  
}

void configModeCallback (WiFiManager *myWiFiManager){
  display.setRotation(3); //even = portrait, odd = landscape
  display.fillScreen(GxEPD_WHITE);
  const GFXfont* f = &Picopixel ;
  display.setTextColor(GxEPD_BLACK);
  display.setFont(f);
  display.setCursor(0,50);
  display.println("Connect to Badgy AP");
  display.println("to setup your WiFi!");
  display.update();  
}

void clearScreen(){
  display.setRotation(2);
  //display.fillScreen(GxEPD_WHITE);
}

void setFont(){
  display.setTextColor(GxEPD_BLACK);
  display.setFont(font);
}

void showMemory(){
  showText(itoa(system_get_free_heap_size(), cstr, 10),1,5);
}


void showText(char* text, uint8_t x, uint8_t y){
  setFont();
  display.setCursor(x,y);
  display.println(text);
}

void drawSquare(uint8_t x, uint8_t y){
  for(uint16_t i=0; i < gridSize; i++){
    display.drawPixel(offsetx+(x*gridSize)+i, offsety+(y*gridSize), GxEPD_BLACK);
    display.drawPixel(offsetx+(x*gridSize)+i, offsety+(y*gridSize)+gridSize, GxEPD_BLACK);
    display.drawPixel(offsetx+(x*gridSize), offsety+(y*gridSize)+i, GxEPD_BLACK);
    display.drawPixel(offsetx+(x*gridSize)+gridSize, offsety+(y*gridSize)+i, GxEPD_BLACK);
  }
}

void drawGrid(){
  for (uint8_t x=0; x < columns; x++){
    for (uint8_t y=0; y < rows; y++){
      drawSquare(x,y);
    }
  }
}

const unsigned char player[] PROGMEM = {
0xff, 0xff, 0xe7, 0xdb, 0xdb, 0xe7, 0xff, 0xff
};

void drawPlayer(){
  display.drawBitmap(player, offsetx+(playerx*gridSize)+4, offsety+(playery*gridSize)+4, 8, 8, GxEPD_BLACK);
//  display.drawPixel(offsetx+(playerx*gridSize)+(gridSize/2), offsety+(playery*gridSize)+(gridSize/2), GxEPD_BLACK);
}

void update()
{
  clearScreen();
  showMemory();
  drawGrid();
  drawPlayer();
  display.update();
}

void loop()
{  
  byte reading =  (digitalRead(1)  == 0 ? 0 : (1<<0)) | //down
                  (digitalRead(3)  == 0 ? 0 : (1<<1)) | //left
                  (digitalRead(5)  == 0 ? 0 : (1<<2)) | //center
                  (digitalRead(12) == 0 ? 0 : (1<<3)) | //right
                  (digitalRead(10) == 0 ? 0 : (1<<4));  //up
                  
  if(reading != lastButtonState){
    lastDebounceTime = millis();
  }
  if((millis() - lastDebounceTime) > debounceDelay){
    if(reading != buttonState){
      buttonState = reading;
      for(int i=0; i<5; i++){
        if(bitRead(buttonState, i) == 0){
          switch(i){
            case 0:
              //left
              if(playerx > 0 && playerx < columns){
                playerx--;
              }
              break;
            case 1:
              //up
              if(playery > 0 && playery < rows){
                playery--;
              }
              break;
            case 2:
              playerx = 4;
              playery = 9;
              break;
            case 3:
              //down
              if(playery <= rows){
                playery++;
              }
              break;
            case 4:
              //right
              if(playerx <= columns){
                playerx++;
              }
              break;                              
            default:
              break;
          }
        }
      }
      update();
    }
  }
  lastButtonState = reading;
}
