#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include "src/DHT.h"
#include "src/HX711.h"

const char* ssid = "m68";
const char* password = "bekonche";
const char* serverAddress = "192.168.0.110:5000";

const int ledPin = D4;
const int threshold = 5; // seconds to wait for operation
const long scaleOffset = 182000; // initial offset of the scale
const float scaleScale = -24; // unit scale of the scale
const uint64_t defaultSleepTime = 15 * 60e6; // 15 minutes

DHT dht(D5, DHT22);
HX711 scale(D2, D3);

float temp = 0.0f;
float hum = 0.0f;
float weight = 0.0f;

// TODO: backup data in EEPROM if cannot connect or server not respond
void setup()
{
  // Setup
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  Serial.begin(9600);

  // Read sensors data
  readData();

  // Connect to WiFi and send data to server
  if (connect())
    sendData();
  
  sleep();
}

void loop()
{
  if (Serial.available())
  {
    String command = Serial.readString();
    Serial.println(command + " received");
    if (command == "setup")
      setup();
  }
}


void readData()
{
  Serial.print("Reading data..");
  int count = 0;
  do
  {
    temp = dht.readTemperature();
    hum = dht.readHumidity();
    delay(500);
    Serial.print(".");

    count++;
    if (count > threshold * 2)
    {
      temp = 0.0f;
      hum = 0.0f;
      Serial.print("Failed");
      break;
    }
  } while (isnan(temp) || isnan(hum));
  Serial.println("");
  
  scale.set_offset(scaleOffset);
  scale.set_scale(scaleScale);
  weight = scale.get_units(10);
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.print(", Humidity: ");
  Serial.print(hum);
  Serial.print(", Weight: ");
  Serial.println(weight);
  Serial.println();
}

bool connect()
{
  Serial.print("Conecting...");
  WiFi.begin(ssid, password);
  
  // Wait for connection
  int count = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");

    count++;
    if (count > threshold * 2)
    {
      Serial.println("Failed");
      return false;
    }
  }
  Serial.println();
  
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  return true;
}

void sendData()
{
  Serial.println("Sending data...");

  HTTPClient client;
  String addDataURL = String("http://") + serverAddress + "/AddData/" + WiFi.macAddress();
  sendData(client, addDataURL, "Temperature", temp);
  sendData(client, addDataURL, "Humidity", hum);
  sendData(client, addDataURL, "Weight", weight);
  
  Serial.println("Done");
  Serial.println();
}

void sendData(HTTPClient& client, const String& addDataURL, const char* type, const float& value)
{
  Serial.print(String("- ") + type);
  client.begin(addDataURL + "?type=" + type + "&value=" + value);
  Serial.print(" ");
  Serial.print(client.GET());
  Serial.print(" ");
  Serial.println(client.getString());
  client.end();
}

void sleep()
{
  uint64_t sleepTime = defaultSleepTime;
  Serial.print("Receiving sleep time...");

  HTTPClient client;
  client.begin(String("http://") + serverAddress + "/GetSleepTime");
  Serial.print(" ");
  Serial.print(client.GET());
  Serial.print(" ");
  Serial.print(client.getString());
  Serial.println(" seconds");
  if (client.GET() == 200)
    sleepTime = (uint64_t)client.getString().toInt() * 1e6; // in seconds
  client.end();
  
  WiFi.disconnect();
  
  Serial.print("Sleep for ");
  Serial.print((int)(sleepTime / 1e6));
  Serial.println(" seconds");
  ESP.deepSleep(sleepTime); // 30 seconds
}

