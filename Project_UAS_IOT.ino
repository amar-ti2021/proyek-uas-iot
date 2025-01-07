#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

#include "config.h"

#define KODE_SENSOR1 "8"
#define KODE_SENSOR2 "27"
#define KODE_SENSOR3 "28"
#define PEMANTAUAN_APIKEY "4e2b81262d82aa438205e25dc56a5488"

#define MQ2_PIN A0
#define DHT_PIN D4
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

void sendDataToSupabase(int mq2Value, float temperature, float humidity) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient https;
    String url = String(SUPABASE_URL) + "/rest/v1/device_statistics";

    String payload = "{";
    payload += "\"smoke_level\":" + String(mq2Value) + ",";
    payload += "\"temperature\":" + String(temperature) + ",";
    payload += "\"humidity\":" + String(humidity);
    payload += "}";

    https.begin(client, url);
    https.addHeader("Content-Type", "application/json");
    https.addHeader("apikey", SUPABASE_KEY);

    int httpResponseCode = https.POST(payload);

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(https.errorToString(httpResponseCode).c_str());
    }

    https.end();
  } else {
    Serial.println("WiFi not connected");
  }
}

void sendDataToPemantauan(int mq2Value, float temperature, float humidity) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient https;
    String url = "https://www.pemantauan.com/submission/";

    String httpRequestData = "apikey=" + String(PEMANTAUAN_APIKEY);
    httpRequestData += "&obyek1=" + String(KODE_SENSOR1) + "&value1=" + temperature;
    httpRequestData += "&obyek2=" + String(KODE_SENSOR2) + "&value2=" + humidity;
    httpRequestData += "&obyek3=" + String(KODE_SENSOR3) + "&value3=" + mq2Value;

    https.begin(client, url);
    https.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpResponseCode = https.POST(httpRequestData);

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code from Pemantauan: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Error on sending POST to Pemantauan: ");
      Serial.println(https.errorToString(httpResponseCode).c_str());
    }

    https.end();
  } else {
    Serial.println("WiFi not connected");
  }
}

void setup() {
  Serial.begin(9600);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  dht.begin();
}

void loop() {
  int mq2Value = analogRead(MQ2_PIN);
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  sendDataToSupabase(mq2Value, temperature, humidity);
  sendDataToPemantauan(mq2Value, temperature, humidity);

  delay(60000); // Send data every 1 minute
}
