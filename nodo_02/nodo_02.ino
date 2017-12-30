#include "Arduino.h"

/*  2017 LucaSOp */

const char* DESC = "nodo_02-DHT+switch-poggiolo";
const char* VERS = "v1.0.0";
const char* IP = "192.168.1.49";
const char* MAC = "18:FE:34:E1:2D:1E";
const char* HA_ENTITY_ID = "sensor.itemperature";


/* CHANGELOG
 *  20170426  aggiunta lettura pin Analogico A0 DUST
 * 
 * 
 */

/* TODO
 *  20170426  1. modificare lettura 5v arduino a 1v ESP8266
 *            2. il cicro di lettura del pin Analogico presenta dei delay locali, creare funzione asincrona
 *            3. usare più campioni per la lettura del pin A0 ( media mobile ? )
 *            
 * 
 * 
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266Influxdb.h>

/* File password in libraries/Secret/Secret.h */
#include <Secret.h>

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
/******************* OTA lib *********************/

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>


const char* wifi_ssid     = S_WIFI_SSID;
const char* wifi_password = S_WIFI_PASSWORD;

/******************* variabili MQTT  *********************/
#define mqtt_server S_MQTT_HOST
#define mqtt_user S_MQTT_USER
#define mqtt_password S_MQTT_PASSWORD

#define humidity_topic "sensor/humidity"
#define temperature_topic "sensor/temperature"
String name_client_id="ESP8266-"; //This text is concatenated with ChipId to get unique client_id
//String strTopic="";
//String msg="";
//String receivedChar="";



/*******************  Topic MQTT OTA *****************/
//#define mode_topic "sensor/mode"

char* mode_topic = "sensor/mode";
/****************************************************/

//MQTT client

WiFiClient espClient;
PubSubClient client(espClient);
int HeatingPin = 5;
String switch1;
String strTopic;
String strPayload;


#define DHTTYPE DHT22
#define DHTPIN  14
DHT dht(DHTPIN, DHTTYPE); 

/***************** DUST variale ********************/

int measurePin = 0; //Connect dust sensor to Arduino A0 pin
int ledPower = 3;   //Connect 3 led driver pins of dust sensor to Arduino D2
  
int samplingTime = 280;
int deltaTime = 40;
int sleepTime = 9680;
  
float voMeasured = 0;
float calcVoltage = 0;
float dustDensity = 0;

/****************************************************/
  


void setup() {
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
 


  Serial.println("\r\nBooting...");

  
  dht.begin();
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

  Serial.println("Configuring influx server...");
  name_client_id=name_client_id+ESP.getChipId();
  Serial.println("   Cliend Id: "+name_client_id); 
 
    
  influxdb.opendb(DATABASE, DB_USER, DB_PASSWORD);

  client.setServer(mqtt_server, S_MQTT_PORT);
  client.setCallback(callback);

  pinMode(HeatingPin, OUTPUT);
  digitalWrite(HeatingPin, HIGH);

  Serial.println("Setup completed! Running ...");
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

void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  strTopic = String((char*)topic);
  if(strTopic == "ha/switch1")
    {
    switch1 = String((char*)payload);
    if(switch1 == "ON")
      {
        Serial.println("ON");
        digitalWrite(HeatingPin, HIGH);
      }
    else
      {
        Serial.println("OFF");
        digitalWrite(HeatingPin, LOW);
      }
    }
}
 
 
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
       if (client.connect("ESP8266Client_03", mqtt_user, mqtt_password)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.subscribe("ha/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


bool checkBound(float newValue, float prevValue, float maxDiff) {
  return !isnan(newValue) &&
         (newValue < prevValue - maxDiff || newValue > prevValue + maxDiff);
}

long lastMsg = 0;
float temp = 0.0;
float hum = 0.0;
float dust = 0.0;
float diff = 0.2;
float diffu = 1.0;
float diffd = 0.1;

void loop() {
  ArduinoOTA.handle();
 
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  

  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;

    float newTemp = dht.readTemperature();
    float newHum = dht.readHumidity();
    
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
    dustDensity = ( 0.17 * calcVoltage - 0.1 ) * 1000; //( ug/m3 ) 
    
    
    float newDust = dustDensity;

    if (checkBound(newTemp, temp, diff)) {
      /*
      Serial.print("temp: ");
      Serial.print(String(temp).c_str());
      Serial.print(" newTemp: ");
      Serial.print(String(newTemp).c_str());
      Serial.print(" diff: ");
      Serial.println(String(diff).c_str());
      */
      temp = newTemp;
      Serial.print("New temperature:");
      Serial.println(String(temp).c_str());

      // Create field object with measurment name=power_read
      FIELD dataObj("temp_table");
      dataObj.addTag("method", "Field_object"); // Add method tag
      dataObj.addTag("temperature", "T0"); // Add pin tag
      dataObj.addField("value", temp); // Add value field
      Serial.println(influxdb.write(dataObj) == DB_SUCCESS ? "Object write success" : "Writing failed");
      // Empty field object.
      dataObj.empty();
  
      //client.publish(temperature_topic, String(temp).c_str(), true);
    }

    if (checkBound(newHum, hum, diffu)) {
      hum = newHum;
      Serial.print("New humidity:");
      Serial.println(String(hum).c_str());
      // Create field object with measurment name=power_read
      FIELD dataObj("Humidity_table");
      dataObj.addTag("method", "Field_object"); // Add method tag
      dataObj.addTag("humidity", "H0"); // Add pin tag
      dataObj.addField("value", hum); // Add value field
      Serial.println(influxdb.write(dataObj) == DB_SUCCESS ? "Object write success" : "Writing failed");
      // Empty field object.
      dataObj.empty();
      //client.publish(humidity_topic, String(hum).c_str(), true);
    }

     if (checkBound(newDust, dust, diffd)) {
      dust = newDust;
      Serial.print("New dust:");
      Serial.println(String(dust).c_str());
      // Create field object with measurment name=power_read
      FIELD dataObj("Dust_table");
      dataObj.addTag("method", "Field_object"); // Add method tag
      dataObj.addTag("dust", "D0"); // Add pin tag
      dataObj.addField("value", dust); // Add value field
      Serial.println(influxdb.write(dataObj) == DB_SUCCESS ? "Object write success" : "Writing failed");
      // Empty field object.
      dataObj.empty();
      //client.publish(humidity_topic, String(hum).c_str(), true);
    }

    
  }
}
