/*
  Copyright 2023 Basile Rouault and Le Petit Fablab de Paris
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute and/or sublicense,
  copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
  FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <Arduino.h>
#include <Servo.h>
#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Discord_WebHook.h> 

const char* ssid = "SSID";
const char* password = "password";
const String DISCORD_WEBHOOK = "Your Token";

Discord_Webhook discord; // Create a Discord_Webhook object 
const String autoCloseMessage[4] = {"Le Lab est/va bientôt fermé","J'ai fermé la Boite","Je me suis fermé toute seule","Le Chat est parti dormir :cat::zzz:"};
const String OpenMessage[4] = {"Le Lab est ouvert","Je vient d'être ouverte","Un Chat vient d'apparaitre :smiley_cat:","Coucou venez au Petit Fablab"};
const String CloseMessage[4] = {"Le Lab est fermé","Je viens de me faire fermé","Le Chat est parti en courant :cat2:","Bonne nuit le LPFP"};

String htmlReply = "O";

ESP8266WebServer server(5000);
void handleRoot() {
  server.send(200, "text/plain", htmlReply);
}

Servo myservo;  // create servo object to control a servo

const int buttonPin = 4;
byte buttonState;

// Declare our NeoPixel strip object:
#define LED_PIN    D6
#define LED_COUNT 4
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void ledSetup(){
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  delay(100);
  strip.clear();
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
}

void colorWipe(uint32_t color, int wait) {
  for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
    byte buttonState = digitalRead(buttonPin);
    if (buttonState == HIGH) {
    //myservo.write(0);
    }
  }
}

bool rainbow(bool fast) { //Modified Rainbow cycle to handle WebServer and early close
  for(long firstPixelHue = 60000; firstPixelHue > 5 ; firstPixelHue -= 1) {
    for(int i=0; i<strip.numPixels(); i++) {
      server.handleClient();
      int pixelHue = firstPixelHue + (i * 65536L / 40);
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show(); // Update strip with new contents
    byte buttonState = digitalRead(buttonPin);
    if (buttonState == HIGH) {delay(30);return 1;break;}
    if (fast){
      for(int pos=0; pos<190; pos++) {
        delay(1);
        server.handleClient();
      }
    }
  }
  return 0;
}

void boxOpen(){

  htmlReply = "O";
  
  for(int pos=0; pos<99; pos++) {
    myservo.write(pos);
    server.handleClient();
    delay(5);
    int colorValue = ((pos/10 & 1)*2.5) * pos;
    colorWipe(strip.Color(colorValue,colorValue,colorValue), 0);
  }
  for(int pos=99; pos>0; pos--) {
    myservo.write(pos);
    server.handleClient();
    delay(5);
    int colorValue = ((pos/10 & 1)*2.5) * pos;
    colorWipe(strip.Color(((pos/10 & 1)*245),   colorValue,   colorValue), 0);
  }
  colorWipe(strip.Color(245,0,0), 0); // Red
  discord.send(OpenMessage[random(0,4)]);
  myservo.write(0);

  bool earlyClose = rainbow(1);            // Flowing rainbow cycle along the whole strip: 0 for fast 1 for 3.5 hour rainbow

  myservo.write(160);
  while (buttonState == LOW){
    buttonState = digitalRead(buttonPin);
  }
  myservo.write(100);
  for(int pos=100; pos>0; pos--) {
    myservo.write(pos);
    delay(5);
    server.handleClient();
  }
  colorWipe(strip.Color(0,   255,   0), 100); // Red
  colorWipe(strip.Color(127,   255,   0), 100); // Red
  colorWipe(strip.Color(255,   127,   0), 100); // Red
  colorWipe(strip.Color(255,   0,   0), 100); // Red
  myservo.write(0);
  strip.clear();
  strip.show();

  if (earlyClose)
    discord.send(CloseMessage[random(0,4)]); // Discord Close message
  else
    discord.send(autoCloseMessage[random(0,4)]); // Discord Close message
}

void setup() {
  myservo.attach(2,450,2400);  // attaches the servo on GIO2 to the servo object
  myservo.write(0);
  pinMode(buttonPin,INPUT_PULLUP);
  
  pinMode(13, OUTPUT); //Turn OFF intergrated LED
  digitalWrite(13, 0);

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  server.on("/", handleRoot);
  server.begin();

  discord.begin(DISCORD_WEBHOOK); // Initialize the Discord_Webhook object 

  ledSetup();
}


void loop() {
  htmlReply = "C";
  server.handleClient();
  buttonState = digitalRead(buttonPin);
  if (buttonState == LOW) {
    boxOpen();
  }
  myservo.write(0);
}