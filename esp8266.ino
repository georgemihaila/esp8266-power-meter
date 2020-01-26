#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>    

#define READINGS 100

const char* ssid     = "SSID";       
const char* password = "PASSWORD";
const char* endpoint = "http://DOTNETCORE_API_SERVER:PORT/API/AddReading";

float mean = 0;
float sd = 0;
float z = 1.96;
float sd2_pos = 0;

HTTPClient http;

void calibrate(){
 int readings[READINGS];
 int sum = 0;
 for (int i = 0; i < READINGS; i++){
  readings[i] = analogRead(A0);
  sum += readings[i];
  delay(10);
 }
 float mean = sum / (float)READINGS;
 float variance = 0;
 for (int i = 0; i < READINGS; i++){
  variance += (readings[i] - mean) * (readings[i] - mean);
 }
 sd = variance / READINGS;
 sd2_pos = mean + z * 2 * sd;
 Serial.print("Mean: ");
 Serial.println(mean);
 Serial.print("SD: ");
 Serial.println(sd);
}

void setup() {
 Serial.begin(9600);
 pinMode(LED_BUILTIN, OUTPUT);
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
  }
  calibrate();
  digitalWrite(LED_BUILTIN, LOW);
}

void postReading(){
  Serial.println("x");
  http.begin(endpoint);
  http.addHeader("Content-Type", "text/plain");
  int httpCode = http.POST("New reading");
  http.end();
}

void loop() {
  delay(20);
  yield();
  int val = analogRead(A0);
  if (abs(val - mean) > sd2_pos){
    postReading();
    while (abs(analogRead(A0) - mean) > sd2_pos){
      delay(100);
      yield();
    }
  }
}
