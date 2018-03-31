#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include "src/DHT.h"
#include "src/HX711.h"

const char* ssid = "m68";
const char* password = "bekonche";
const char* serverAddress = "192.168.0.110:5000";

const int ledPin = D4;

DHT dht(D5, DHT22);
HX711 scale(D2, D3);

float temp = 0.0f;
float hum = 0.0f;
float weight = 0.0f;

void setup()
{
  // Setup
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  Serial.begin(9600);

  // Read sensors data
  readData();

  // Connect to WiFi
  connect();

  // TODO: get sensorId from EEPROM or from server
  // TODO: maybe get sleep interval from server too if fail EEPROM
  // TODO: send backuped data to server if any
  // TODO: backup if cannot connect or server don't respond

  // Send data to server
  sendData();
  
  WiFi.disconnect();
  digitalWrite(ledPin, HIGH);
  
  //ESP.deepSleep(30e6); // 30 seconds
  //Serial.print("Sleep for ");
  //Serial.print(30e6 / 1e6);
  //Serial.println("seconds");
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
  do
  {
    temp = dht.readTemperature();
    hum = dht.readHumidity();
    delay(500);
    Serial.print(".");
    // TODO: maybe add counter if pass sleep(reboot)
  } while (isnan(temp) || isnan(hum));
  Serial.println("");
  
  scale.set_offset(185697);
  scale.set_scale(-24);
  weight = scale.get_units(10);
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.print(", Humidity: ");
  Serial.print(hum);
  Serial.print(", Weight: ");
  Serial.println(weight);
  Serial.println();
}

void connect()
{
  Serial.print("Conecting...");
  WiFi.begin(ssid, password);
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    // TODO: maybe add counter if pass backup data and sleep(reboot)
  }
  Serial.println();
  
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void sendData()
{
  Serial.println("Sending data...");

  HTTPClient client;
  String addDataURL = String("http://") + serverAddress + "/AddData/1";
  sendData(client, addDataURL, "Temperature", temp);
  sendData(client, addDataURL, "Humidity", hum);
  sendData(client, addDataURL, "Weight", weight);
    
  Serial.println("Done");
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
  //if (client.GET() != 200 || client.getString() != "OK")
  // TODO: server not respond
  // TODO: send data in different minutes?
}

