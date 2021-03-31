#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <DHT.h>

#define DHTPIN 12 // Pin connected to the DHT sensor
#define DHTTYPE DHT11  // DHT11 or DHT22
DHT dht(DHTPIN, DHTTYPE);

int VentPinOut = 0;
int HeaterPinOut = 5;

// alle netwerk settings en settings van de borker om ermee te kunnen verbinden
const char* ssid = "LAPTOP-Vince";
const char* pass = "2#n0193F";
const char* mqttServer = "192.168.137.21";
const int mqttPort = 1883;
const char* mqttUser = "vincent";
const char* mqttPassword = "raspberry";
const char* clientID = "MQTTInfluxDBBridge"; // MQTT client ID

WiFiClient espClient;
PubSubClient client(espClient);

int TempModule = 26;


void setup() {
  Serial.begin(115200);

  pinMode(VentPinOut, OUTPUT);
  pinMode(HeaterPinOut, OUTPUT);

  digitalWrite(VentPinOut, LOW);
  digitalWrite(HeaterPinOut, LOW);

  WiFi.begin(ssid, pass);
  dht.begin();

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  client.setServer(mqttServer, mqttPort);

  delay(10);
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);
  int wifi_ctr = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("VIOTThermostatModule1", mqttUser, mqttPassword ))
    {
      Serial.println("connected");
    } else
    {
      Serial.print("failed with state ");
      Serial.println(client.state());
      delay(2000);
    }
  }

  Serial.println("WiFi connected");

  //set a callback function for when data is received from broker
  client.setCallback(callback);

  // subscribe to topic
  client.subscribe("Modules/Temp/Use");
  
}

void loop() {

  client.loop();

  delay(500);

  float TempTemp = dht.readTemperature();

  if (isnan(TempTemp)) {
    Serial.println("Can't read sensor");
  } else {
    TempModule = TempTemp;
  }

  //Serial.print("Temp.: ");
  //Serial.println(TempModule);

  String TempMessageModule = String ((int)TempModule);

  client.publish("Modules/TempModuleBedroom/temp", TempMessageModule.c_str());
}

void callback (char* topic, byte * payload, unsigned int length) {

  Serial.print("Message arrived in topic: ");
  Serial.println(topic);

  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  payload[length] = '\0';
  String PayloadMessage = (char *)payload;

  if (PayloadMessage == "Heater") {
    digitalWrite(HeaterPinOut, HIGH);
    digitalWrite(VentPinOut, LOW);
  } else if (PayloadMessage == "Vent") {
    digitalWrite(VentPinOut, HIGH);
    digitalWrite(HeaterPinOut, LOW);
  } else if (PayloadMessage == "None") {
    digitalWrite(VentPinOut, LOW);
    digitalWrite(HeaterPinOut, LOW);
  }

  Serial.println();
  Serial.println("-----------------------");

}
