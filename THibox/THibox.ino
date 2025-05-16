#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager  version 2.0.17
#include <TFT_eSPI.h>
#include <ArduinoJson.h> // 7.1.0
#include <HTTPClient.h> // https://github.com/arduino-libraries/ArduinoHttpClient   version 0.6.1
#include <ESP32Time.h>  // https://github.com/fbiego/ESP32Time  verison 2.0.6
#include "Audio.h"
#include <UrlEncode.h>
#include "NotoSansBold15.h"
#include "tinyFont.h"
#include "smallFont.h"
#include "midleFont.h"
#include "bigFont.h"
#include "font18.h"

// Global object initialization
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);
TFT_eSprite errSprite = TFT_eSprite(&tft);
ESP32Time rtc(0);
Audio audio;

//#################### USER CONFIGURATION  ###################
// Time zone settings
const int kTimeZone = 2;  // Time zone
const String kTown = "Zagreb";  // City name
const String kOpenWeatherApiKey = "";  // Openweather API key
const String kUnits = "metric";  // metric, imperial

// Speaker interface configuration
#define I2S_DOUT 3
#define I2S_BCLK 2
#define I2S_LRC 1

// Baidu TTS parameters, documentation: https://ai.baidu.com/ai-doc/SPEECH/mlbxh7xie
const char* kApiKey = "";  // For token acquisition
const char* kSecretKey = "";  // For token acquisition
const bool kSpeakChinese = false;  // true for Chinese, false for English
//#################### END OF CONFIGURATION ###################

// Button and playback variables
bool lastButtonState = HIGH;
bool buttonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long kDebounceDelay = 50;  // Button debounce delay (ms)
const unsigned long kCooldownTime = 30000;  // Playback cooldown time (ms)
unsigned long lastPlayTime = 0;
bool initialPlayDone = false;  // Flag for initial playback on startup

// TTS related variables
String encodedText;
const int kPer = 4;  // Voice type
const int kSpd = 5;  // Speech speed
const int kPit = 5;  // Pitch
const int kVol = 5;  // Volume
const int kAue = 3;  // Audio format
const char* kTtsUrl = "http://tsn.baidu.com/text2audio";
String url = kTtsUrl;

// Baidu Token management
String baiduToken = "";  // Store the acquired token
unsigned long tokenTimestamp = 0;  // Timestamp when token was acquired
const unsigned long kTokenExpireTime = 1296000000;  // Token validity period (ms), default 15 days
const unsigned long kTokenRetryInterval = 120000;  // Retry interval after token acquisition failure (ms), default 1 minute
unsigned long lastTokenRetryTime = 0;  // Last time token acquisition was attempted

// Weather related variables
const char* kNtpServer = "pool.ntp.org";
String weatherApiUrl = "https://api.openweathermap.org/data/2.5/weather?q=" + kTown + 
                       "&appid=" + kOpenWeatherApiKey + "&units=" + kUnits;

// UI related variables
int ani = 100;  // Animation counter
float maxT;  // Maximum temperature
float minT;  // Minimum temperature
unsigned long timePased = 0;
int counter = 0;

// Color definitions
#define BCK_COLOR TFT_BLACK
unsigned short grays[13];

// Data labels
const char* kDataLabels[] = { "HUM", "PRESS", "WIND" };
const String kDataUnits[] = { "%", "hPa", "m/s" };

// Weather data
float temperature = 22.2;
float weatherData[3];  // Humidity, pressure, wind speed
float tempHistory[24] = {};  // Temperature history data
float tempHistoryCopy[24] = {};  // Copy of temperature history data
int tempGraph[24] = { 0 };  // Temperature graph data

// Scrolling message
String weatherMessage = "";

/**
 * Set system time from NTP server
 */
void setTime() {
  configTime(3600 * kTimeZone, 0, kNtpServer);
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    rtc.setTimeStruct(timeinfo);
  }
}

/**
 * Initialize audio settings
 */
void setAudio() {
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(8);
}

/**
 * Get TTS authorization token from Baidu API
 * @return Token string if successful, empty string if failed
 */
String getBaiduToken() {
  HTTPClient http;
  String tokenUrl = "https://aip.baidubce.com/oauth/2.0/token";
  tokenUrl += "?client_id=" + String(kApiKey);
  tokenUrl += "&client_secret=" + String(kSecretKey);
  tokenUrl += "&grant_type=client_credentials";
  
  http.begin(tokenUrl);
  int httpCode = http.POST("");
  String token = "";
  
  if (httpCode > 0 && httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      token = doc["access_token"].as<String>();
      Serial.println("Token acquired successfully: " + payload);
    } else {
      Serial.println("Token parsing failed: " + String(error.c_str()));
    }
  } else {
    Serial.println("Token acquisition failed");
    Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  
  http.end();
  return token;
}

/**
 * Get text-to-speech conversion
 */
void getTts() {
  const char* headerKeys[] = { "Content-Type", "Content-Length" };
  url = kTtsUrl;
  url += "?tex=" + encodedText;
  url += "&tok=" + baiduToken;
  url += "&per=" + String(kPer);
  url += "&spd=" + String(kSpd);
  url += "&pit=" + String(kPit);
  url += "&vol=" + String(kVol);
  url += "&aue=" + String(kAue);
  url += "&cuid=esp32s3";
  url += "&lan=zh";
  url += "&ctp=1";

  HTTPClient http;
  Serial.println("Requesting TTS service");
  Serial.print("URL: ");
  Serial.println(url);

  http.begin(url);
  http.collectHeaders(headerKeys, 2);
  int httpResponseCode = http.GET();
  
  if (httpResponseCode > 0) {
    if (httpResponseCode == HTTP_CODE_OK) {
      String contentType = http.header("Content-Type");
      Serial.print("Content-Type = ");
      Serial.println(contentType);
      
      if (contentType.startsWith("audio")) {
        Serial.println("TTS synthesis successful, audio file returned");
        Serial.println("Starting audio playback");
        audio.connecttohost(url.c_str());
      } else if (contentType.equals("application/json")) {
        Serial.println("TTS synthesis error, JSON returned");
        String errorPayload = http.getString();
        Serial.println("Error message: " + errorPayload);
      } else {
        Serial.println("Unknown Content-Type");
      }
    } else {
      Serial.println("TTS request failed, HTTP status code: " + String(httpResponseCode));
    }
  } else {
    Serial.print("HTTP request error: ");
    Serial.println(httpResponseCode);
  }
  
  http.end();
}

/**
 * Get weather data from API
 */
void getWeatherData() {
  HTTPClient http;
  http.begin(weatherApiUrl);
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String payload = http.getString();
    Serial.println("Weather API response:");
    Serial.println(payload);

    // Parse JSON response
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      temperature = doc["main"]["temp"];
      weatherData[0] = doc["main"]["humidity"];
      weatherData[1] = doc["main"]["pressure"];
      weatherData[2] = doc["wind"]["speed"];

      int visibility = doc["visibility"];
      const char* description = doc["weather"][0]["description"];

      weatherMessage = "#Description: " + String(description) + 
                      "  #Visibility: " + String(visibility) + 
                      " #Updated: " + rtc.getTime();
    } else {
      Serial.print("JSON parsing error: ");
      Serial.println(error.c_str());
    }
  } else {
    Serial.print("HTTP request error: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

/**
 * Draw the interface
 */
void drawInterface() {
  // Draw scrolling message
  errSprite.fillSprite(grays[10]);
  errSprite.setTextColor(grays[1], grays[10]);
  errSprite.drawString(weatherMessage, ani, 4);

  // Draw main interface
  sprite.fillSprite(BCK_COLOR);
  sprite.drawLine(138, 10, 138, 164, grays[6]);
  sprite.drawLine(100, 108, 134, 108, grays[6]);
  sprite.setTextDatum(0);

  // Left side area
  sprite.loadFont(midleFont);
  sprite.setTextColor(grays[1], BCK_COLOR);
  sprite.drawString("WEATHER", 6, 10);
  sprite.unloadFont();

  sprite.loadFont(font18);
  sprite.setTextColor(grays[7], BCK_COLOR);
  sprite.drawString("TOWN:", 6, 110);
  sprite.setTextColor(grays[2], BCK_COLOR);
  
  if (kUnits == "metric") {
    sprite.drawString("C", 14, 50);
  } else if (kUnits == "imperial") {
    sprite.drawString("F", 14, 50);
  }

  sprite.setTextColor(grays[3], BCK_COLOR);
  sprite.drawString(kTown, 46, 110);
  sprite.fillCircle(8, 52, 2, grays[2]);
  sprite.unloadFont();

  // Draw time (without seconds)
  sprite.loadFont(tinyFont);
  sprite.setTextColor(grays[4], BCK_COLOR);
  sprite.drawString(rtc.getTime().substring(0, 5), 6, 132);
  sprite.unloadFont();

  // Draw static text
  sprite.setTextColor(grays[5], BCK_COLOR);
  sprite.drawString("INTERNET", 86, 10);
  sprite.drawString("STATION", 86, 20);
  sprite.setTextColor(grays[7], BCK_COLOR);
  sprite.drawString("SECONDS", 92, 157);

  // Draw temperature
  sprite.setTextDatum(4);
  sprite.loadFont(bigFont);
  sprite.setTextColor(grays[0], BCK_COLOR);
  sprite.drawFloat(temperature, 1, 69, 80);
  sprite.unloadFont();

  // Draw seconds area
  sprite.fillRoundRect(90, 132, 42, 22, 2, grays[2]);
  sprite.loadFont(font18);
  sprite.setTextColor(TFT_BLACK, grays[2]);
  sprite.drawString(rtc.getTime().substring(6, 8), 111, 144);
  sprite.unloadFont();

  // Right side area
  sprite.setTextDatum(0);
  sprite.loadFont(font18);
  sprite.setTextColor(grays[1], BCK_COLOR);
  sprite.drawString("LAST 12 HOUR", 144, 10);
  sprite.unloadFont();

  sprite.fillRect(144, 28, 84, 2, grays[10]);

  sprite.setTextColor(grays[3], BCK_COLOR);
  sprite.drawString("MIN:" + String(minT), 254, 10);
  sprite.drawString("MAX:" + String(maxT), 254, 20);
  sprite.fillSmoothRoundRect(144, 34, 174, 60, 3, grays[10], BCK_COLOR);
  sprite.drawLine(170, 39, 170, 88, TFT_WHITE);
  sprite.drawLine(170, 88, 314, 88, TFT_WHITE);

  sprite.setTextDatum(4);

  // Draw temperature graph
  for (int j = 0; j < 24; j++) {
    for (int i = 0; i < tempGraph[j]; i++) {
      sprite.fillRect(173 + (j * 6), 83 - (i * 4), 4, 3, grays[2]);
    }
  }

  sprite.setTextColor(grays[2], grays[10]);
  sprite.drawString("MAX", 158, 42);
  sprite.drawString("MIN", 158, 86);

  sprite.loadFont(font18);
  sprite.setTextColor(grays[7], grays[10]);
  sprite.drawString("T", 158, 58);
  sprite.unloadFont();

  // Draw data area
  for (int i = 0; i < 3; i++) {
    sprite.fillSmoothRoundRect(144 + (i * 60), 100, 54, 32, 3, grays[9], BCK_COLOR);
    sprite.setTextColor(grays[3], grays[9]);
    sprite.drawString(kDataLabels[i], 144 + (i * 60) + 27, 107);
    sprite.setTextColor(grays[2], grays[9]);
    sprite.loadFont(font18);
    sprite.drawString(String((int)weatherData[i]) + kDataUnits[i], 144 + (i * 60) + 27, 124);
    sprite.unloadFont();
  }

  // Draw bottom info area
  sprite.fillSmoothRoundRect(144, 148, 174, 16, 2, grays[10], BCK_COLOR);
  errSprite.pushToSprite(&sprite, 148, 150);
  sprite.setTextColor(grays[4], BCK_COLOR);
  sprite.drawString("CURRENT WEATHER", 190, 141);
  sprite.setTextColor(grays[9], BCK_COLOR);
  sprite.drawString(String(counter), 310, 141);

  // Display to screen
  sprite.pushSprite(0, 0);
}

/**
 * Update data
 */
void updateData() {
  // Update scrolling message position
  ani--;
  if (ani < -420) {
    ani = 100;
  }

  // Update weather data every 3 minutes
  if (millis() > timePased + 180000) {
    timePased = millis();
    counter++;
    getWeatherData();

    // Update time and temperature history every 10 updates
    if (counter == 10) {
      setTime();
      counter = 0;
      maxT = -50;
      minT = 1000;
      
      // Update temperature history data
      tempHistory[23] = temperature;
      for (int i = 23; i > 0; i--) {
        tempHistory[i - 1] = tempHistoryCopy[i];
      }

      // Calculate max and min temperatures
      for (int i = 0; i < 24; i++) {
        tempHistoryCopy[i] = tempHistory[i];
        if (tempHistory[i] < minT && tempHistory[i] != 0) {
          minT = tempHistory[i];
        }
        if (tempHistory[i] > maxT) {
          maxT = tempHistory[i];
        }
      }

      // Calculate temperature graph data
      for (int i = 0; i < 24; i++) {
        if (tempHistory[i] != 0) {
          tempGraph[i] = map(tempHistory[i], minT, maxT, 0, 12);
        } else {
          tempGraph[i] = 0;
        }
      }
    }
  }
}

/**
 * Play audio weather report
 */
void playAudio() {
  // Play weather info once at startup
  if (!initialPlayDone) {
    if (temperature != 0) {
      String weatherInfo;
      if (kSpeakChinese) {
        weatherInfo = "当前时间" + rtc.getTime().substring(0, 5) + "。" + 
                     kTown + "的气温 " + String(temperature, 1) + "摄氏度，湿度 " + 
                     String((int)weatherData[0]) + "%，气压 " + 
                     String((int)weatherData[1]) + "hPa，风速 " + 
                     String(weatherData[2], 1) + "m/s。";
      } else {
        weatherInfo = "The current time is " + rtc.getTime().substring(0, 5) + 
                     " in " + kTown + ". The temperature is " + 
                     String(temperature, 1) + " degrees Celsius, humidity is " + 
                     String((int)weatherData[0]) + "%, pressure is " + 
                     String((int)weatherData[1]) + " hPa, and wind speed is " + 
                     String(weatherData[2], 1) + " m/s.";
      }
      encodedText = urlEncode(urlEncode(weatherInfo));
      getTts();  // Get TTS and play

      initialPlayDone = true;
      lastPlayTime = millis();
    }
  }

  // Button handling
  bool reading = digitalRead(10);

  // Button debounce
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  // Debounce processing
  if ((millis() - lastDebounceTime) > kDebounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
    }

    // Button pressed and not in cooldown period
    if (buttonState == LOW && (millis() - lastPlayTime > kCooldownTime)) {
      Serial.println("Button pressed, starting weather report");
      lastPlayTime = millis();  // Record this playback time

      String weatherInfo;
      if (kSpeakChinese) {
        weatherInfo = "当前时间" + rtc.getTime().substring(0, 5) + "。" + 
                     kTown + "的气温 " + String(temperature, 1) + "摄氏度，湿度 " + 
                     String((int)weatherData[0]) + "%，气压 " + 
                     String((int)weatherData[1]) + "hPa，风速 " + 
                     String(weatherData[2], 1) + "m/s。";
      } else {
        weatherInfo = "The current time is " + rtc.getTime().substring(0, 5) + 
                     " in " + kTown + ". The temperature is " + 
                     String(temperature, 1) + " degrees Celsius, humidity is " + 
                     String((int)weatherData[0]) + "%, pressure is " + 
                     String((int)weatherData[1]) + " hPa, and wind speed is " + 
                     String(weatherData[2], 1) + " m/s.";
      }
      encodedText = urlEncode(urlEncode(weatherInfo));
      getTts();  // Get TTS and play
    }
  }
  
  // Save current button state for next comparison
  lastButtonState = reading;
}

/**
 * Initialize setup
 */
void setup() {
  Serial.begin(115200);
  
  // Set pin modes
  pinMode(10, INPUT);
  pinMode(15, OUTPUT);
  digitalWrite(15, 1);
  
  // Initialize display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.drawString("Connecting to WiFi...", 30, 50, 4);
  sprite.createSprite(320, 170);
  errSprite.createSprite(164, 15);

  // Set screen brightness
  int channel = 0;
  ledcAttachChannel(38, 10000, 8, channel);
  ledcWriteChannel(channel, 130); 

  // Connect to WiFi
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(5000);

  if (!wifiManager.autoConnect("GCWifiConf", "")) {
    Serial.println("WiFi connection failed, timeout");
    delay(3000);
    ESP.restart();
  }
  
  Serial.println("WiFi connected successfully");
  setTime();
  getWeatherData();
  setAudio();
  
  // Get token on startup, retry every minute in loop if failed
  baiduToken = getBaiduToken();
  if (baiduToken != "") {
    Serial.println("Token acquired successfully");
    tokenTimestamp = millis();
  } else {
    Serial.println("Token acquisition failed, will retry every minute in loop");
    lastTokenRetryTime = millis();  // Record initial failure time for retry timing in loop
  }

  delay(100);

  // Generate grayscale color palette
  int colorValue = 210;
  for (int i = 0; i < 13; i++) {
    grays[i] = tft.color565(colorValue, colorValue, colorValue);
    colorValue -= 20;
  }

  // Initialize file system
  if (!FFat.begin(true)) {
    Serial.println("FFat mount failed");
  }
}

/**
 * Audio playback info callback
 */
void audio_info(const char *info) {
  Serial.print("[Audio Info] ");
  Serial.println(info);
}

/**
 * MP3 playback completion callback
 */
void audio_eof_mp3(const char *info) {
  Serial.print("[MP3 EOF] ");
  Serial.println(info);
}

/**
 * Main loop
 */
void loop() {
  unsigned long currentTime = millis();
  
  // If token is empty or expired, try to get a new one every minute until successful
  if (baiduToken == "" || (currentTime - tokenTimestamp > kTokenExpireTime)) {
    if (currentTime - lastTokenRetryTime >= kTokenRetryInterval) {
      // Print different logs based on the situation
      if (baiduToken == "") {
        Serial.println("Token is empty, attempting to acquire token");
      } else {
        Serial.println("Token expired, attempting to refresh");
      }
      
      // Get new token
      String newToken = getBaiduToken();
      lastTokenRetryTime = currentTime;  // Update last attempt time
      
      if (newToken != "") {
        Serial.println("Token acquired successfully");
        baiduToken = newToken;
        tokenTimestamp = currentTime;
      } else {
        Serial.println("Token acquisition failed, will retry in 1 minute");
        baiduToken = "";  // Ensure token is empty for next retry
      }
    }
  }

  // Update interface and data
  drawInterface();
  updateData();
  
  // Process audio and playback
  audio.loop();
  playAudio();
}





