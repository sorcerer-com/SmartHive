#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include "src/DHT.h"
#include "src/HX711.h"
#include "DataSaver.h"

const char* ssid = "ARMBIAN";
const char* password = "12345678";
const char* serverAddress = "192.168.0.110:5000";

const int ledPin = D4;
const int threshold = 10; // seconds to wait for operation
const int dataCount = 3; // data variables count
const int savesBeforeSend = 4; // save 4 times before send
const uint64_t defaultSleepTime = 15 * 60e6; // 15 minutes default deep sleep

const long scaleOffset = 187000; // initial offset of the scale
const float scaleScale = -24; // unit scale of the scale

DHT dht(D4, DHT22);
HX711 scale(D2, D3);

unsigned long startTime = millis();

void setup()
{
  // Setup
  //pinMode(ledPin, OUTPUT);
  //digitalWrite(ledPin, LOW);

  Serial.begin(9600);
  DataSaver.init();

  // Print saved data if D5 is LOW
  pinMode(D5, INPUT_PULLUP);
  if (digitalRead(D5) == LOW)
    DataSaver.print();

  // Turn off the wifi until read the data
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(100);

  // Read sensors data
  readData();

  // Send data but not everytime
  if (DataSaver.count() % (savesBeforeSend * dataCount) == 1)
  {
    // Turn on the wifi again
    WiFi.forceSleepWake();
    delay(100);
    WiFi.mode(WIFI_STA);

    // Connect to WiFi and send data to server
    if (connect() && sendData())
      DataSaver.clear(); // if successful clear saves
  }

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
  float temp, hum, weight;

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
  Serial.println();

  scale.set_offset(scaleOffset);
  scale.set_scale(scaleScale);
  weight = scale.get_units(10);

  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.print(", Humidity: ");
  Serial.print(hum);
  Serial.print(", Weight: ");
  Serial.println(weight);

  // Save Data
  Serial.println("Saving data...");
  if (DataSaver.count() == 0) // save default sleep time in first place
    DataSaver.save(defaultSleepTime);
  DataSaver.save(temp);
  DataSaver.save(hum);
  DataSaver.save(weight);
  
  Serial.print("SavedData count: ");
  Serial.println(DataSaver.count() / dataCount);
  Serial.println();
}

bool connect()
{
  Serial.print("Conecting...");
  WiFi.persistent(false);
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

bool sendData()
{
  bool result = true;
  Serial.println("Sending data...");

  HTTPClient client;
  String addDataURL = String("http://") + serverAddress + "/AddData/" + WiFi.macAddress();

  int idx = 0;
  float value = 0;
  const int count = (DataSaver.count() - 1) / dataCount;
  for (int i = 0; i < count; i++)
  {
    idx = ((DataSaver.count() - 1) / dataCount) - i - 1;
    Serial.println(String("- ") + idx + " index");

    DataSaver.load(i * dataCount + 1, value);
    result &= sendData(client, addDataURL, -idx, "Temperature", value);
    DataSaver.load(i * dataCount + 2, value);
    result &= sendData(client, addDataURL, -idx, "Humidity", value);
    DataSaver.load(i * dataCount + 3, value);
    result &= sendData(client, addDataURL, -idx, "Weight", value);

    if (!result)
      break;
    delay(100);
  }

  if (result)
    Serial.println("Done");
  else
    Serial.println("Failed");
  Serial.println();
  return result;
}

bool sendData(HTTPClient& client, const String& addDataURL,
              const int& index, const char* type, const float& value)
{
  Serial.print(String("- ") + type + " " + value + " ");

  client.begin(addDataURL + "?index=" + index + "&type=" + type + "&value=" + value);
  int status = client.GET();
  String content = client.getString();
  client.end();

  Serial.print(status);
  Serial.print(" ");
  Serial.println(content);
  return status == 200 && content == "OK";
}

void sleep()
{
  uint64_t sleepTime = defaultSleepTime;
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("Receiving sleep time...");

    HTTPClient client;
    client.begin(String("http://") + serverAddress + "/GetSleepTime");
    int status = client.GET();
    String content = client.getString();
    client.end();

    Serial.print(" ");
    Serial.print(status);
    Serial.print(" ");
    Serial.print(content);
    Serial.println(" seconds");
    if (status == 200)
    {
      sleepTime = (uint64_t)content.toInt() * 1e6; // in seconds
      if (DataSaver.count() == 0)
        DataSaver.save((float)(sleepTime / 1e6));
    }

    WiFi.disconnect(true);
  }
  else
  {
    float temp;
    if (DataSaver.load(0, temp) && temp > 0)
      sleepTime = (uint64_t)temp * 1e6; // in seconds
    else if (DataSaver.count() == 0)
      DataSaver.save((float)(sleepTime / 1e6));
  }

  Serial.print("Sleep for ");
  Serial.print((int)(sleepTime / 1e6));
  Serial.println(" seconds");
  Serial.println();

  Serial.print("Execution time ");
  Serial.print((float)(millis() - startTime) / 1000);
  Serial.println(" seconds");
  Serial.println();

  delay(1);
  ESP.deepSleep(sleepTime, WAKE_RF_DISABLED);
}

