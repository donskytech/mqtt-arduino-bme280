#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ArduinoJson.h>

// For SPI
#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme; // I2C
// Adafruit_BME280 bme(BME_CS); // hardware SPI
// Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // software SPI

/*
  Replace the SSID and Password according to your wifi
*/
const char *ssid = "<YOUR_WIFI_SSID_HERE>";
const char *password = "<YOUR_WIFI_SSID_PASSWORD_HERE>";

// Your MQTT broker ID
const char *mqttBroker = "192.168.100.22";
const int mqttPort = 1883;
// MQTT topics
const char *publishTopic = "sensorReadings";
const char *subscribeTopic = "sampletopic";

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
const int READ_CYCLE_TIME = 3000;

// Connect to Wifi
void setup_wifi()
{
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// Callback function whenever an MQTT message is received
void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String message;
  for (int i = 0; i < length; i++)
  {
    Serial.print(message += (char)payload[i]);
  }
  Serial.println();
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");

    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    // Attempt to connect
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
      // Subscribe to topic
      // client.subscribe(subscribeTopic);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setupBME280()
{
  unsigned status;

  // default settings
  status = bme.begin(0x76);
  // You can also pass in a Wire library object like &Wire2
  // status = bme.begin(0x76, &Wire2)
  if (!status)
  {
    Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
    Serial.print("SensorID was: 0x");
    Serial.println(bme.sensorID(), 16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1)
      delay(10);
  }
}

void setup()
{
  Serial.begin(115200);
  // Setup the wifi
  setup_wifi();
  // setup bme280
  setupBME280();
  // setup the mqtt server and callback
  client.setServer(mqttBroker, mqttPort);
  client.setCallback(callback);
}

void loop()
{
  // Listen for mqtt message and reconnect if disconnected
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  // publish BME280 sensor readings periodically
  unsigned long now = millis();
  if (now - lastMsg > READ_CYCLE_TIME)
  {
    lastMsg = now;

    //  Publish MQTT messages
    char buffer[256];
    StaticJsonDocument<96> doc;
    doc["temperature"] = bme.readTemperature();
    doc["humidity"] = bme.readHumidity();
    doc["pressure"] = bme.readPressure() / 100.0F;
    doc["altitude"] = bme.readAltitude(SEALEVELPRESSURE_HPA);
    size_t n = serializeJson(doc, buffer);
    client.publish(publishTopic, buffer, n);
  }
}