#include <RTCDS1307.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <splash.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>

#define RECALIBRATION_CONSUMPTION_KWH 5
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64 
#define OLED_RESET     -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
String s[8];
unsigned int total = 0;

#define READINGS 100

const char* ssid     = "SSID";       
const char* password = "PASSWORD";
String dsmrEndpointAddress = "http://DSMR_IP/api/v2";
const char* dsmrAPIKey = "DSMR_API_KEY";

double mean = 0;
double sd = 0;
double z = 1.96;
double sd2_pos = 0;
double dev_offset = 0;

double meterPosition = -1;

void calibrate() {
  int count = 5;
  bool conditions[count];
  for(;;) {

    display.print(".");
    display.display();

    int readings[READINGS];
    int sum = 0;
    for (int i = 0; i < READINGS; i++){
      readings[i] = analogRead(A0);
      sum += readings[i];
      delay(10);
    }
    mean = sum / (double)READINGS;
    double variance = 0;
    for (int i = 0; i < READINGS; i++){
      variance += (readings[i] - mean) * (readings[i] - mean);
    }
    sd = variance / READINGS;
    dev_offset = z * 2 * sd;
    sd2_pos = mean + dev_offset;

    conditions[0] = mean == 0;
    conditions[1] = sd == 0;
    conditions[2] = sd > mean;
    conditions[3] = mean + dev_offset > 1023;
    conditions[4] = mean - dev_offset <= 0;

    Serial.print("Calibration\nMean: " + String(mean) + "\n");
    Serial.print("SD:   " + String(sd) + "\n");

    bool ok = true;
    for (int i = 0; i < count; i++){
      if (conditions[i]){
        Serial.print("Failure reason [" + String(i) + "]");
        ok = false;
        break;
      }
    }
    if (ok){
      break;
    }
  }
  if (sd < (mean / 2)){
    sd = mean / 2;
    dev_offset = z * 2 * sd;
    sd2_pos = mean + dev_offset;
  }
}

void clearTerminal(){
   for (int i = 0; i < 8; i++){
     s[i] = "";
     total = 0;
   }
  display.clearDisplay();
  display.setCursor(0,0);
  display.display();
}

void terminalAppend(String message){
  s[total++ % 8] = message;
  display.clearDisplay();
  display.setCursor(0,0);
   for (int i = 0; i < 8; i++){
    display.print(s[i]);
  }
  display.display();
}

String dsmrGet(String address){
  HTTPClient http;
  http.begin(address);
  http.addHeader("X-AUTHKEY", dsmrAPIKey, true);
  int responseCode = http.GET();
  String result = http.getString();
  http.end();
  return result;
}

long getReadingCount(){
  String response = dsmrGet(dsmrEndpointAddress + "/consumption/electricity?limit=1&ordering=-timestamp");
  const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(17) + 550;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, response);
  return doc["count"];
}

double getMeterPosition(long totalReadingsCount){
  String response = dsmrGet(dsmrEndpointAddress + "/consumption/electricity?limit=1&ordering=-timestamp&offset=" + String(totalReadingsCount - 1));
  const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(17) + 550;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, response);
  char* stopstr;
  return strtod(doc["results"][0]["delivered_2"], &stopstr);
}

String getCurrentTime(){
  HTTPClient http;
  http.begin("http://worldtimeapi.org/api/timezone/Europe/Bucharest");
  int responseCode = http.GET();
  String result = http.getString();
  http.end();
  const size_t capacity = JSON_OBJECT_SIZE(15) + 360;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, result);
  return doc["datetime"];
}

//0x50 or 0x68
RTCDS1307 rtc(0x68);
uint8_t year, month, weekday, day, hour, minute, second;
bool period = 0;

void updateRTCTime(){
  rtc.getDate(year, month, day, weekday);
  rtc.getTime(hour, minute, second, period);
}


void setup() {
  Serial.begin(9600);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
     delay(2000);
     ESP.reset();
  }
  
  display.display();
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.print("ready");
  display.display();
  for (int i = 0; i < 8; i++){
    s[i] = "";
  }
 pinMode(LED_BUILTIN, OUTPUT);
 terminalAppend("Connecting...\n");
 WiFi.begin(ssid, password);
 digitalWrite(LED_BUILTIN, LOW);
 while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
  }
  terminalAppend("DSMR\n\tReadings: ");
  int readingsCount = getReadingCount();
  terminalAppend(String(readingsCount) + "\n\tPosition: ");
  meterPosition = getMeterPosition(readingsCount);
  terminalAppend(String(meterPosition) + "\n");

  rtc.begin(); 

  updateRTCTime();
  terminalAppend("RTCDS1307 ");
  if (year < 20 || year > 30) { //Planned obsolescence
    terminalAppend("updating");
    int c = 0;
    do {
      display.print(".");
      display.display();

      String time = getCurrentTime();
      rtc.setDate(atoi(time.substring(0, 4).c_str()) % 100, atoi(time.substring(5, 7).c_str()), atoi(time.substring(8, 10).c_str()));
      rtc.setTime(atoi(time.substring(11, 13).c_str()), atoi(time.substring(14, 16).c_str()), atoi(time.substring(17, 19).c_str()));
      updateRTCTime();

      if (c++ >= 1){
        delay(1000);
        if (c == 5){
          terminalAppend("\nRTCDS1307 sync error");
          delay(2000);
          ESP.reset();
        }
      }
    } while (year < 20 || year > 30);
  }
  else{
    terminalAppend("ok");
  }
  terminalAppend("\nCalibrating");
  calibrate();

  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  clearTerminal();
  refreshDisplay();
}

long lastTickTimestamp = -1;
double currentConsumption = 0;


void drawCentreString(const String &buf, int x, int y)
{
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(buf, x, y, &x1, &y1, &w, &h);
    display.setCursor(x - w / 2, y);
    display.print(buf);
}

int lastReading = -1;

void refreshDisplay(){
  display.clearDisplay();
  int kws = currentConsumption / 1000;
  int ws = (int)currentConsumption % 1000;
  String middleText = "";
  if (kws != 0){
    middleText += String(kws) + ".";
  }
  middleText += (ws < 100)?((ws < 10)?"00" + String(ws / 100):"0" + String(ws / 10)):String(ws);
  if (kws != 0){
    middleText += "k";
  }
  middleText += "Wh";
  display.setTextSize(2);
  drawCentreString(middleText, 64, 25);
  display.setTextSize(1);
  display.drawRect(0,0, ((kws > 5)?5:kws) * 128 / 5, 5, 0);
  display.display();
}

int threshold = 20;

String strPlusZero(uint8_t val){
  return (val < 10 && val >= 0)? "0" + String(val) : String(val);
}

void postReading(){
  HTTPClient http;
  http.begin(dsmrEndpointAddress + "/datalogger/dsmrreading");
  http.addHeader("X-AUTHKEY", dsmrAPIKey, true);
  http.addHeader("Content-Type", "application/json");
  String payload = "";
  const size_t capacity = JSON_OBJECT_SIZE(7) + 210;
  DynamicJsonDocument doc(capacity);
  
  int kws = currentConsumption / 1000;
  int ws = (int)currentConsumption % 1000;
  String cons = String(kws) + "." + ((ws < 100)?((ws < 10)?"00":"0"):"") + String(ws);
  
  doc["timestamp"] = "20" + strPlusZero(year) + "-" + strPlusZero(month) + "-" + strPlusZero(day) + "T" + strPlusZero(hour) + ":" + strPlusZero(minute) + ":" + strPlusZero(second);
  doc["electricity_delivered_1"] = 0;
  doc["electricity_returned_1"] = 0;
  doc["electricity_delivered_2"] = meterPosition;
  doc["electricity_returned_2"] = 0;
  doc["electricity_currently_delivered"] = cons;
  doc["electricity_currently_returned"] = 0;
  doc["phase_currently_delivered_l1"] = cons;

  serializeJson(doc, payload);
  int responseCode = http.POST(payload);
  Serial.println(String(responseCode) + "; \r\n" + payload);
  http.end();
}

void loop() {
  delay(20);
  yield();
  lastReading = analogRead(A0);
  
  if (abs(lastReading - mean) > dev_offset){
    long currentTickTimestamp = millis();
    currentConsumption = 3600 * 1000 / (currentTickTimestamp - lastTickTimestamp);
    
    if ((lastTickTimestamp != -1 || currentConsumption != 0) && (currentConsumption / 1000 >= RECALIBRATION_CONSUMPTION_KWH)) { //Recalibrate if readings are too high
      lastTickTimestamp = -1;
      currentConsumption = 0;
      calibrate();
      refreshDisplay();
      return;
    }
    
    meterPosition += 0.001;
    if (lastTickTimestamp != -1 || currentConsumption != 0){
      updateRTCTime();
      postReading();
    }
    lastTickTimestamp = currentTickTimestamp;
    refreshDisplay();
    digitalWrite(LED_BUILTIN, LOW);
    while (abs(analogRead(A0) - mean) > dev_offset){
      delay(20);
      yield();
    }
    digitalWrite(LED_BUILTIN, HIGH);
  }
}
