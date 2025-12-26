#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
ESP8266WiFiMulti WiFiMulti;
const char* ssid = "b";
const char* password = "bohdan123";
const char* serverUrl = "http://bohdansuskyi.pythonanywhere.com/create-records/";
float temp;
float temp_1;
float temp_2;
float temp_3;
String uid;
String sensorAdress;
#define pinSDA D1
#define pinSCL D2
#define MLX_1_Adres 0X5A
#define MLX_2_Adres 0X5B
#define MLX_3_Adres 0X5C
#define RST_PIN D0
#define SS_1_PIN D3
#define SS_2_PIN D4
#define SS_3_PIN D8
#define Red_led 1     // TX
#define Yellow_led 3  // RX
#define Blue_led 10   // S3
#define NR_OF_READERS 3
unsigned long lastCheck = 0;
unsigned long interval = 500;
byte ssPins[] = {SS_1_PIN, SS_2_PIN, SS_3_PIN};
MFRC522 mfrc522[NR_OF_READERS];
TwoWire testWire;
Adafruit_MLX90614 mlx_1 = Adafruit_MLX90614();
Adafruit_MLX90614 mlx_2 = Adafruit_MLX90614();
Adafruit_MLX90614 mlx_3 = Adafruit_MLX90614();

void setup() 
{
testWire.begin(pinSDA,pinSCL);
mlx_1.begin(MLX_1_Adres);
mlx_2.begin(MLX_2_Adres);
mlx_3.begin(MLX_3_Adres);
SPI.begin();
for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) 
{
  pinMode(ssPins[reader], OUTPUT);
  digitalWrite(ssPins[reader], HIGH);  // deaktywuje wszystkie czytniki po starcie
  mfrc522[reader].PCD_Init(ssPins[reader], RST_PIN);
  delay(100); // małe opóźnienie między inicjalizacjami
}
pinMode(Red_led, OUTPUT);
pinMode(Yellow_led, OUTPUT);
pinMode(Blue_led, OUTPUT);
digitalWrite(Blue_led, LOW);
digitalWrite(Red_led, LOW);
digitalWrite(Yellow_led, LOW);
WiFi.mode(WIFI_STA);
WiFiMulti.addAP(ssid, password);
for (int i = 0; i < 4; i++) 
{
  mlx_1.readObjectTempC();
  mlx_2.readObjectTempC();
  mlx_3.readObjectTempC();
  delay(500); 
}
if ((WiFiMulti.run() == WL_CONNECTED))
{
digitalWrite(Blue_led, HIGH);
digitalWrite(Red_led, HIGH);
digitalWrite(Yellow_led, HIGH);
delay(1000);
digitalWrite(Blue_led, LOW);
digitalWrite(Red_led, LOW);
digitalWrite(Yellow_led, LOW);
}
}

String dump_byte_array(byte *buffer, byte bufferSize) 
{
  String content = "";
  for (byte i = 0; i < bufferSize; i++) 
  {
    if (buffer[i] < 0x10) content += "0";
    content += String(buffer[i], HEX);
  }
  content.toUpperCase();
   return content;
}

void blinkLed(int pin) 
{
  digitalWrite(pin, HIGH);
  delay(300);
  digitalWrite(pin, LOW);
}

void loop() 
{ 
  unsigned long currentMillis = millis();
  if (currentMillis - lastCheck >= interval) 
  {
  lastCheck = currentMillis;
  temp_1 = mlx_1.readObjectTempC();
  temp_2 = mlx_2.readObjectTempC();
  temp_3 = mlx_3.readObjectTempC();
  }
  for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) 
  {
    if (mfrc522[reader].PICC_IsNewCardPresent() && mfrc522[reader].PICC_ReadCardSerial()) 
    {
      uid = dump_byte_array(mfrc522[reader].uid.uidByte, mfrc522[reader].uid.size);
      
      switch (reader) 
      {
        case 0:
          temp = temp_1;
          sensorAdress = "0X5A";
          blinkLed(Red_led);
          
        break;
        case 1:
          temp = temp_2;
          sensorAdress = "0X5B";
          blinkLed(Blue_led);
        break;
        case 2:
          temp = temp_3;
          sensorAdress = "0X5C";
          blinkLed(Yellow_led);
        break;
      }
      mfrc522[reader].PICC_HaltA();
      mfrc522[reader].PCD_StopCrypto1();

      if ((WiFiMulti.run() == WL_CONNECTED))
      {
        WiFiClient client;
        HTTPClient http;
        String url = String(serverUrl) + "?MLX90614_adress=" + String(sensorAdress) + "&part=" + String(uid) + "&temperature=" + String(temp);
        if (http.begin(client, url))
        {
          int httpCode = http.GET();
          if (httpCode == 201)
          {
            digitalWrite(Blue_led, HIGH);
            digitalWrite(Red_led, HIGH);
            delay(300);
            digitalWrite(Blue_led, LOW);
            digitalWrite(Red_led, LOW);
          }
          http.end();
        }
      }
    }   
  }
  uid = "";
}