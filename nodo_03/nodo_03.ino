#include "Arduino.h"

/*  2017 LucaSOp */

const char* DESC = "nodo_03-DHT+buzzar-soggiorno";
const char* VERS = "v1.0.0";
const char* IP = "192.168.1.42";
const char* MAC = "18:FE:34:E1:22:B9";
const char* HA_ENTITY_ID = "sensor.itemperature2";

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

#define humidity_topic "sensor/humidity2"
#define temperature_topic "sensor/temperature2"
String name_client_id="ESP8266-"; //This text is concatenated with ChipId to get unique client_id
//String strTopic="";
//String msg="";
//String receivedChar="";
#define BUZZER_PIN  4


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
String buzzer1;



#define DHTTYPE DHT22
#define DHTPIN  5
#14
DHT dht(DHTPIN, DHTTYPE); 



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
  Serial.println("Project name: -- influxdb_sensor_DHT_switch_indoor --");
  
//  pinMode(BUZZER_PIN, OUTPUT);
//  digitalWrite(BUZZER_PIN, HIGH); 
  
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



/*
  Serial.println("Configuring MQTT server...");
  mqtt_client_id=mqtt_client_id+ESP.getChipId();
  Serial.println("   Cliend Id: "+mqtt_client_id); 
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
*/


  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, HIGH);


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
  if(strTopic == "ha/buzzer1")
    {
    buzzer1 = String((char*)payload);
    Serial.println(buzzer1);
    if(buzzer1 == "ON")
      {
        digitalWrite(BUZZER_PIN, LOW); 
        Serial.println(" Turn On BUZZER! ");
//        suona(); 
       }
    else
      {
        digitalWrite(BUZZER_PIN, HIGH);
        //noTone(BUZZER_PIN);     // Stop sound.. 
        Serial.println(" Turn Off BUZZER! ");

      }
    }
}
 
 
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
       if (client.connect("ESP8266Client_22", mqtt_user, mqtt_password)) {
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
float diff = 0.2;
float diffu = 1.0;

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
      dataObj.addTag("temperature", "T1"); // Add pin tag
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
      dataObj.addTag("humidity", "H1"); // Add pin tag
      dataObj.addField("value", hum); // Add value field
      Serial.println(influxdb.write(dataObj) == DB_SUCCESS ? "Object write success" : "Writing failed");
      // Empty field object.
      dataObj.empty();


      
      //client.publish(humidity_topic, String(hum).c_str(), true);
    }
  }
}
