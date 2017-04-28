
/*
 *  
 *  nodo poggiolo
 *  IP 192.168.1.43
 *  climate_01_online
 *  MAC 18:FE:34:E1:22:B9
 *  
 */

/*
 * File password in libraries/Secret/Secret.h
 */
 
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

#define humidity_topic "sensor/humidity"
#define temperature_topic "sensor/temperature"
String name_client_id="ESP8266-"; //This text is concatenated with ChipId to get unique client_id
//String strTopic="";
//String msg="";
//String receivedChar="";


const unsigned reading_count = 10; // Numbber of readings each time in order to stabilise
unsigned int analogVals[reading_count];
unsigned int counter = 0;
unsigned int values_avg = 0;
const int powersoil = D8; // Digital Pin 8 will power the sensor, acting as switch


/*******************  Topic MQTT OTA *****************/
//#define mode_topic "sensor/mode"

char* mode_topic = "sensor/mode";
/****************************************************/

//MQTT client

WiFiClient espClient;
PubSubClient client(espClient);
int HeatingPin = 5;
String switch2;
String strTopic;
String strPayload;


#define DHTTYPE DHT22
#define DHTPIN  14
DHT dht(DHTPIN, DHTTYPE); 



void setup() {
  Serial.begin(115200);
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



/*
  Serial.println("Configuring MQTT server...");
  mqtt_client_id=mqtt_client_id+ESP.getChipId();
  Serial.println("   Cliend Id: "+mqtt_client_id); 
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
*/


  client.setServer(mqtt_server, S_MQTT_PORT);
  client.setCallback(callback);

  pinMode(HeatingPin, OUTPUT);
  digitalWrite(HeatingPin, HIGH);

  pinMode(powersoil,OUTPUT);

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
  if(strTopic == "ha/switch2")
    {
    switch2 = String((char*)payload);
    if(switch2 == "ON")
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
       if (client.connect("ESP8266Client_04", mqtt_user, mqtt_password)) {
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
float soil = 0.0;
float diff = 1.0;
double analogValue = 0.0;

void loop() {
  ArduinoOTA.handle();
 
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  values_avg = 0;
  counter = 0;


  long now = millis();
  if (now - lastMsg > 1000*60*15) {
    lastMsg = now;

    Serial.println("Powering module ON");
    digitalWrite(powersoil, HIGH);
    delay(1000);

  //  float newSoil = 1024 - analogRead(A0); // read the analog signal;

      // read the input on analog pin 0:
    for( counter = 0; counter < reading_count; counter++){
   //   Serial.println("Reading probe value...:");
      analogVals[reading_count] = 1024 - analogRead(A0);
      delay(100);
      
      values_avg = (values_avg + analogVals[reading_count]);
 //     Serial.println(analogVals[reading_count]);
 //     Serial.print("Total Readings value...:");
 //     Serial.println(values_avg);
    }
    values_avg = values_avg/reading_count;
    Serial.println(values_avg);
    
    
    float newSoil = map(values_avg,600,1024,0,100);

    

    if (checkBound(newSoil, soil, diff)) {
      /*
      Serial.print("temp: ");
      Serial.print(String(temp).c_str());
      Serial.print(" newTemp: ");
      Serial.print(String(newTemp).c_str());
      Serial.print(" diff: ");
      Serial.println(String(diff).c_str());
      */
      soil = newSoil;
      Serial.print("New Soil:");
      Serial.println(String(soil).c_str());

      // Create field object with measurment name=power_read
      FIELD dataObj("soil_table");
      dataObj.addTag("method", "Field_object"); // Add method tag
      dataObj.addTag("soil", "S0"); // Add pin tag
      dataObj.addField("value", soil); // Add value field
      Serial.println(influxdb.write(dataObj) == DB_SUCCESS ? "Object write success" : "Writing failed");
      // Empty field object.
      dataObj.empty();
  
      //client.publish(temperature_topic, String(temp).c_str(), true);
    }

  Serial.println("Powering module OFF");
  digitalWrite(powersoil, LOW);
    
  }
}
