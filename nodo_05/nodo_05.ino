#include "Arduino.h"

/*  2017 LucaSOp */

const char* DESC = "nodo_01-Dust-soggiorno";
const char* VERS = "v1.0.0";
const char* IP = "";
const char* MAC = "";
const char* HA_ENTITY_ID = "";

/* File password in libraries/Secret/Secret.h */
#include <Secret.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266Influxdb.h>

/******************* influxdb  *********************/
const char *INFLUXDB_HOST = S_INFLUXDB_HOST;
const uint16_t INFLUXDB_PORT = S_INFLUXDB_PORT;
const char *DATABASE = S_INFLUXDB_DATABASE;
const char *DB_USER = S_INFLUXDB_DB_USER;
const char *DB_PASSWORD = S_INFLUXDB_DB_PASSWORD;
Influxdb influxdb(INFLUXDB_HOST, INFLUXDB_PORT);

/******************* OTA lib *********************/
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

/******************* WIFI *********************/
const char* wifi_ssid     = S_WIFI_SSID;
const char* wifi_password = S_WIFI_PASSWORD;

/******************* MQTT  *********************/
#define mqtt_server S_MQTT_HOST
#define mqtt_user S_MQTT_USER
#define mqtt_password S_MQTT_PASSWORD



int measurePin = 0; //Connect dust sensor to Arduino A0 pin
int ledPower = 3;   //Connect 3 led driver pins of dust sensor to Arduino D2
  
int samplingTime = 280;
int deltaTime = 40;
int sleepTime = 9680;
  
float voMeasured = 0;
float calcVoltage = 0;
float dustDensity = 0;
  
void setup(){
  Serial.begin(115200);

  // StartUp Banner
  Serial.println("#######################################################################");

  Serial.print( "Descrizione: ");
  Serial.println( DESC );
  
  Serial.print( "Version: ");
  Serial.println( VERS );
  Serial.println( "GITHUB repository HOME_ONE" );

  Serial.print( "IP: ");
  Serial.println( IP );

  Serial.print( "MAC_ADD: ");
  Serial.println( MAC );

  Serial.print( "HA_ENTITY_ID: ");
  Serial.println( HA_ENTITY_ID );

  Serial.println("#######################################################################");
  
  setup_wifi();
	Serial.print("Configuring OTA device...");
	  //TelnetServer.begin();   //Necesary to make Arduino Software autodetect OTA device
	  ArduinoOTA.onStart([]() {Serial.println("OTA starting...");});
	  ArduinoOTA.onEnd([]() {Serial.println("OTA update finished!");Serial.println("Rebooting...");});
	  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {Serial.printf("OTA in progress: %u%%\r\n", (progress / (total / 100)));});
	  ArduinoOTA.onError([](ota_error_t error) {
	    Serial.printf("Error[%u]: ", error);
	    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
	    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
	    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
	    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
	    else if (error == OTA_END_ERROR) Serial.println("End Failed");
	  });
	  ArduinoOTA.begin();
	  Serial.println("OK");

  pinMode(ledPower,OUTPUT);
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void loop(){

ArduinoOTA.handle();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  digitalWrite(ledPower,LOW); // power on the LED
  delayMicroseconds(samplingTime);
  
  voMeasured = analogRead(measurePin); // read the dust value
  
  delayMicroseconds(deltaTime);
  digitalWrite(ledPower,HIGH); // turn the LED off
  delayMicroseconds(sleepTime);
  
  // 0 - 5V mapped to 0 - 1023 integer values
  // recover voltage
  calcVoltage = voMeasured * (5.0 / 1024.0);
  
  // linear eqaution taken from http://www.howmuchsnow.com/arduino/airquality/
  // Chris Nafis (c) 2012
  dustDensity = 0.17 * calcVoltage - 0.1;
  if ( dustDensity > 0 ) {
    Serial.print("Raw Signal Value (0-1023): ");
    Serial.print(voMeasured);
  
    Serial.print(" - Voltage: ");
    Serial.print(calcVoltage);
  
  
    Serial.print(" - Dust Density: ");
    Serial.print(dustDensity * 1000); // ( ug/m3 )
    Serial.println(" ug/m3 ");
  }
  delay(1000);
  
}
