#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <FastLED.h>
#include <WiFiUdp.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>  
#include <EEPROM.h>

const int eepromAddress = 0; // EEPROM address to store the API key
char apiKey[65];

#define DATA_PIN 4

// Define the array of leds
const int NUM_LEDS = 12;
CRGB leds[NUM_LEDS];

String apiPath = "https://app.divera247.com/api/v2/alarms?accesskey="; 
String apiKeyString = "";

WiFiManager wm;
   
void setup()
{
  Serial.begin(115200);
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);  //m GRB ordering is typical
  FastLED.setBrightness(255);
  allOff();
  
  EEPROM.begin(65); // Adjust the size based on your needs
  EEPROM.get(eepromAddress, apiKey);

  WiFiManagerParameter customApiKey("api_key", "API Key", apiKey, 64);
  wm.addParameter(&customApiKey);
  
  if(!wm.autoConnect("Divera Alarm Light")){
    Serial.println("Failed to connect and hit timeout");
    leds[0] = CRGB::Red;
    show();
    delay(3000);
    ESP.reset();
    delay(5000);
  }
  allOff();
  Serial.println("CONNECTED");
  leds[0] = CRGB::Green;
  show();
  delay(1000);
  allOff();
  Serial.println("Stetup done");

  strncpy(apiKey, customApiKey.getValue(), sizeof(apiKey) - 1);
  apiKey[sizeof(apiKey) - 1] = '\0'; // Ensure null-termination
  EEPROM.put(eepromAddress, apiKey);
  EEPROM.commit();

  apiKeyString = String(apiKey);
}

void loop() {
  if(doRequest()){
    alarm();
    //15 min delay
    delay(300000 * 3); 
  }
  delay(30000);
}

void alarm(){
  for(short i = 0; i < 90; i++){
    spinColor(CRGB::Blue, 6, 1);
    spinColor(CRGB::Red, 6, 1);
  }
  allOff();
}

void allOff(){
  FastLED.clear();  // clear all pixel data
  show();
}

void show(){
  FastLED.show();
  FastLED.delay(5);
}

bool doRequest(){
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  bool alarm = false;
  HTTPClient http;
  client->setInsecure();
  Serial.println((apiPath + apiKeyString).c_str());
  http.begin(*client, (apiPath + apiKeyString).c_str());
  int httpResponseCode = http.GET();
      
  if (httpResponseCode == HTTP_CODE_OK) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    if(httpResponseCode != 200){
      return false;
    }
    //Serial.println(payload);
    if(payload.length() < 64){
      return false;
    }

    if(payload.indexOf("\"closed\":false") > 0){
      return true;
    }
    return false;
  }
  else {
    leds[0] = CRGB::Red;
    show();
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
    Serial.println("Resetting WiFi settings...");
    wm.resetSettings();
    delay(5000);
    ESP.reset();
    delay(5000);
  }
  http.end();
  return alarm;
}

void spinColor(CRGB color, short numOfLedsToSpin, int spinForSeconds){
  int offset = 0;
  for(int i=0 ; i<spinForSeconds*20; i++){
    for(int a=0; a<offset; a++){
        if(a > NUM_LEDS){
          leds[a - NUM_LEDS] = CRGB::Black;
        }else{
          leds[a] = CRGB::Black;
        }
      }
    for(int b=offset; b<offset+numOfLedsToSpin; b++){
      if(b > NUM_LEDS){
        leds[b - NUM_LEDS] = color;
      }else{
        leds[b] = color;
      }
    }
    for(int c=offset+numOfLedsToSpin; c<NUM_LEDS; c++){
      if(c > NUM_LEDS){
        leds[c - NUM_LEDS] = CRGB::Black;
      }else{
        leds[c] = CRGB::Black;
      }
    }
    show();
    delay(50);
    offset++;
    if(offset >= 11){
      offset = 0;
    }
  }

}
