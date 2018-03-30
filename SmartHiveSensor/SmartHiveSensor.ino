#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include "src/DHT.h"
#include "src/HX711.h"

const char* ssid = "m68";
const char* password = "bekonche";
const char* serverAddress = "192.168.0.110:5000";

DHT dht(D4, DHT22);
HX711 scale(D2, D3);

const int ledPin = 2;

void setup() {
  // Setup
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  Serial.begin(9600);

  // Read sensors data
  Serial.print("Reading Temprature and Humidity..");
  float temp = 0.0f, hum = 0.0f;
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
  float scl = scale.get_units(10);
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.print(", Humidity: ");
  Serial.print(hum);
  Serial.print(", Scale: ");
  Serial.println(scl);
  Serial.println();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conecting");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    // TODO: maybe add counter if pass backup data and sleep(reboot)
  }
  Serial.println("");
  
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  digitalWrite(ledPin, LOW);

  // TODO: get sensorId from EEPROM or from server
  // TODO: send backuped data to server if any
  // TODO: backup if cannot connect or server don't respond

  // Send data to server
  Serial.println("Sending data...");
  HTTPClient client;
  String addDataURL = String("http://") + serverAddress + "/AddData/1";
  client.begin(addDataURL + "?type=temp&value=" + temp);
  Serial.println(client.GET());
  Serial.println(client.getString());
  client.end();
  //if (client.GET() != 200 || client.getString() != "OK")
  // TODO: server not respond
  // TODO: send data in different minutes?
  client.begin(addDataURL + "?type=hum&value=" + hum);
  Serial.println(client.GET());
  Serial.println(client.getString());
  client.end();
  client.begin(addDataURL + "?type=scale&value=" + scl);
  Serial.println(client.GET());
  Serial.println(client.getString());
  client.end();
  
  //ESP.deepSleep(30e6); // 30 seconds
}

void loop() {
  // put your main code here, to run repeatedly:

}
