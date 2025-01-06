#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

#include "config.h"

#define MQ2_PIN A0

#define DHT_PIN D4
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

ESP8266WebServer server(80);

String getSensorData()
{
  int mq2Value = analogRead(MQ2_PIN);
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  String json = "{";
  json += "\"mq2Value\":" + String(mq2Value) + ",";
  json += "\"temperature\":" + String(temperature) + ",";
  json += "\"humidity\":" + String(humidity);
  json += "}";
  return json;
}

void sendDataToSupabase(int mq2Value, float temperature, float humidity)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    WiFiClientSecure client;

    // Bypass SSL verification
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

    if (httpResponseCode > 0)
    {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    }
    else
    {
      Serial.print("Error on sending POST: ");
      Serial.println(https.errorToString(httpResponseCode).c_str());
      String errorResponse = https.getString();
      Serial.println("Error response body: ");
      Serial.println(errorResponse);
    }

    https.end();
  }
  else
  {
    Serial.println("WiFi not connected");
  }
}

void setup()
{
  Serial.begin(9600);
  Serial.println("Starting the server...");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  dht.begin();

  server.on("/", HTTP_GET, []()
            {
              String html = "<html><body><h1>Sensor Data</h1>";
              html += "<p><strong>MQ2 Value (Smoke): </strong><span id=\"mq2Value\"></span></p>";
              html += "<p><strong>Temperature: </strong><span id=\"temperature\"></span> °C</p>";
              html += "<p><strong>Humidity: </strong><span id=\"humidity\"></span> %</p>";

              // JavaScript for AJAX calls
              html += "<script>";
              html += "function fetchData() {";
              html += "  fetch('/data').then(response => response.json()).then(data => {";
              html += "    document.getElementById('mq2Value').textContent = data.mq2Value;";
              html += "    document.getElementById('temperature').textContent = data.temperature;";
              html += "    document.getElementById('humidity').textContent = data.humidity;";
              html += "  });";
              html += "}";
              html += "setInterval(fetchData, 2000);"; // Update every 2 seconds
              html += "fetchData();";                  // Initial data fetch
              html += "</script>";
              html += "</body></html>";

              server.send(200, "text/html", html); // Send HTML response
            });

  server.on("/data", HTTP_GET, []()
            {
              String sensorData = getSensorData();
              server.send(200, "application/json", sensorData); // Send sensor data as JSON
            });

  server.begin();
  Serial.println("Server started");
}

void loop()
{
  server.handleClient();

  int mq2Value = analogRead(MQ2_PIN);
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Send data to Supabase every 1 minutes
  sendDataToSupabase(mq2Value, temperature, humidity);
  delay(60000); // 1 minutes
}
