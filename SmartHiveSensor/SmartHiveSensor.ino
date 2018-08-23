#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

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
  if (DataSaver.size() % (savesBeforeSend * dataCount) == 0 ||
      DataSaver.isFull())
  {
    // Turn on the wifi again
    WiFi.forceSleepWake();
    delay(100);
    WiFi.mode(WIFI_STA);

    // Connect to WiFi and send data to server
    if (connect())
    {
      update();
      sendData();
    }
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
  Serial.print("Saving data... ");
  if (DataSaver.isInit())
  {
    DataSaver.enqueue(temp, false);
    DataSaver.enqueue(hum, false);
    DataSaver.enqueue(weight, false);
    DataSaver.commit();
    Serial.println();
  }
  else
    Serial.println("failed");

  Serial.print("SavedData count: ");
  Serial.println(DataSaver.size() / dataCount);
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

void update()
{
  Serial.print("Receiving software version...");

  HTTPClient client;
  client.begin(String("http://") + serverAddress + "/GetSoftwareVersion");
  int status = client.GET();
  String content = client.getString();
  client.end();

  Serial.print(" ");
  Serial.print(status);
  Serial.print(" ver: ");
  Serial.println(content);
  if (status == 200)
  {
    int version = content.toInt();
    if (version != DataSaver.getVersion())
    {
      // dequeue last values because after update the sensor will restart and collect new data
      float temp;
      DataSaver.dequeue(temp, false);
      DataSaver.dequeue(temp, false);
      DataSaver.dequeue(temp, false);
      DataSaver.commit();
    
      DataSaver.setVersion(version);
      Serial.print("Update software... ");

      wdt_disable();
      t_httpUpdate_return ret = ESPhttpUpdate.update(String("http://") + serverAddress + "/GetSoftware"); // IT DOESN'T WORK
      switch (ret) {
        case HTTP_UPDATE_FAILED:
          Serial.printf("failed (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
          break;
        case HTTP_UPDATE_NO_UPDATES:
          Serial.println("none");
          break;
        case HTTP_UPDATE_OK:
          Serial.println("successful");
          break;
      }
    }
  }
  Serial.println();
}

bool sendData()
{
  bool result = true;
  Serial.println("Sending data...");

  HTTPClient client;
  String addDataURL = String("http://") + serverAddress + "/AddData/" + WiFi.macAddress();

  int idx = 0;
  float value = 0;
  while (!DataSaver.isEmpty())
  {
    idx = DataSaver.size() / dataCount - 1;
    Serial.println(String("- ") + idx + " index");

    if (result) result &= DataSaver.getItem(0, value);
    if (result) result &= sendData(client, addDataURL, -idx, "Temperature", value);

    if (result) result &= DataSaver.getItem(1, value);
    if (result) result &= sendData(client, addDataURL, -idx, "Humidity", value);

    if (result) result &= DataSaver.getItem(2, value);
    if (result) result &= sendData(client, addDataURL, -idx, "Weight", value);

    if (!result)
      break;

    // dequeue the data whene all is already sent
    DataSaver.dequeue(value, false);
    DataSaver.dequeue(value, false);
    DataSaver.dequeue(value, false);
    DataSaver.commit();
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
      sleepTime = (uint64_t)content.toInt() * 1e6; // in seconds to microseconds
      DataSaver.setSleepTime((int)(sleepTime / 1e6));
    }

    WiFi.disconnect(true);
  }
  if (DataSaver.getSleepTime() > 0)
    sleepTime = (uint64_t)DataSaver.getSleepTime() * 1e6; // in seconds

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

