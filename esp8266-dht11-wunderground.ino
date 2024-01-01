#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include <TimeLib.h>

#define DHTPIN D1
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

ESP8266WebServer server(80);

const char *WUNDERGROUND_ID = "xxxxxxx";
const char *WUNDERGROUND_KEY = "xxxxxxx";

unsigned long previousMillis = 0;
const long interval = 600000;  // 10 минути в милисекунди

void sendToWunderground(float temperature, float humidity) {
  WiFiClient client;
  if (client.connect("weatherstation.wunderground.com", 80)) {
    String path = "/weatherstation/updateweatherstation.php?";
    path += "ID=" + String(WUNDERGROUND_ID);
    path += "&PASSWORD=" + String(WUNDERGROUND_KEY);
    path += "&dateutc=" + String(now());
    path += "&tempf=" + String(temperature * 9.0 / 5.0 + 32.0);
    path += "&humidity=" + String(humidity);
    
    client.print(String("GET ") + path + " HTTP/1.1\r\n" +
                 "Host: weatherstation.wunderground.com\r\n" +
                 "Connection: close\r\n\r\n");
    delay(1000);

    Serial.println("Data sent to Wunderground");
    Serial.print("Temperature sent: ");
    Serial.println(temperature);
    Serial.print("Humidity sent: ");
    Serial.println(humidity);
  } else {
    Serial.println("Connection to Wunderground failed");
  }
  client.stop();
}

void setup() {
  Serial.begin(115200);
  delay(10);

  dht.begin();
  WiFi.begin("user", "pass");

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Стартиране на уеб сървъра
  server.on("/", HTTP_GET, []() {
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Failed to read from DHT sensor!");
      server.send(500, "text/plain", "Failed to read from DHT sensor!");
      return;
    }

    String message = "Temperature: " + String(temperature) + " &#8451;<br>Humidity: " + String(humidity) + " %";
    message += "<br><form action='/manualupdate' method='get'><input type='submit' value='Manual Update'></form>";
    server.send(200, "text/html", message);
  });

  // Добавяне на нова рута за изпълнение на ръчно извличане при натискане на бутона
  server.on("/manualupdate", HTTP_GET, []() {
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Failed to read from DHT sensor!");
      server.send(500, "text/plain", "Failed to read from DHT sensor!");
      return;
    }

    String message = "Temperature: " + String(temperature) + " &#8451;<br>Humidity: " + String(humidity) + " %";
    server.send(200, "text/html", message);

    // Изпращане на данните към Wunderground
    sendToWunderground(temperature, humidity);
  });

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    if (!isnan(humidity) && !isnan(temperature)) {
      Serial.print("Humidity: ");
      Serial.print(humidity);
      Serial.print(" %\t");
      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.println(" *C");

      sendToWunderground(temperature, humidity);
    }
  }
}
