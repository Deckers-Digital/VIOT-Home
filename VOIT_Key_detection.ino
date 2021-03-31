#include <WiFiNINA.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <MFRC522.h>

char* ssid = "LAPTOP-Vince";
const char* pass = "2#n0193F";
const char* mqttServer = "192.168.137.21";
const int mqttPort = 1883;
const char* mqttUser = "vincent";
const char* mqttPassword = "raspberry";
const char* clientID = "MQTTInfluxDBBridge"; // MQTT client ID

WiFiClient wifiClient;
PubSubClient client(wifiClient);

#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);

#define SS_PIN 10
#define RST_PIN 9

long lastMsg = 0;
char msg[50];

//int Button = 12;
int GreenLed = 17;
int RedLed = 21;

int Sound = 8;

int ButtonState1 = 0;
int ButtonState2;

int pinData = 0;

bool WelcomeSound = true;
bool ByeSound = false;

void setup_wifi()
{
  delay(10);

  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void setup()
{

  pinMode(GreenLed, OUTPUT);
  pinMode(RedLed, OUTPUT);
  pinMode(Sound, OUTPUT);

  Serial.begin(115200);
  setup_wifi();

  client.setServer(mqttServer, mqttPort);
  // connect to broker
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("VIOTKeyDetection", mqttUser, mqttPassword ))
    {
      Serial.println("connected");
    } else
    {
      Serial.print("failed with state ");
      Serial.println(client.state());
      delay(2000);
    }
  }

  SPI.begin();      // Initiate  SPI bus
  mfrc522.PCD_Init();   // Initiate MFRC522
  Serial.println("Approximate your card to the reader...");
  Serial.println();
}


void loop()
{
  client.loop();

  long now = millis();
  if (now - lastMsg > 5000)
  {
    lastMsg = now;
    char payLoad[1];

  }

  if (pinData == 1) {
    Serial.println("State = On");
    analogWrite(RedLed, LOW);
    digitalWrite(GreenLed, HIGH);


  } else {
    Serial.println("State = Off");
    digitalWrite(GreenLed, LOW);
    digitalWrite(RedLed, HIGH);

  }

  delay(500);

  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial())
  {
    return;
  }
  //Show UID on serial monitor
  Serial.print("UID tag :");
  String content = "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  Serial.print("Message : ");
  content.toUpperCase();
  if (content.substring(1) == "DC C0 83 21") //change here the UID of the card/cards that you want to give access
  {
    if (ButtonState1 == 0) {

      if (ButtonState2 == 1)
      {
        Serial.println("Button Pressed 2");
        ButtonState2 = 0;
        pinData = 0;
        client.publish("Modules/Key/State", "Off");
        Bye();
      } else {
        Serial.println("Button Pressed");
        pinData = 1;
        ButtonState2 = 1;
        client.publish("Modules/Key/State", "On");
        Welcome();
      }
    }
  }

  else   {
    Serial.println(" Access denied");
    delay(3000);
  }

}

void Welcome () {

  digitalWrite(Sound, HIGH);
  delay(100);
  digitalWrite(Sound, LOW);
  delay(100);
  digitalWrite(Sound, HIGH);
  delay(100);
  digitalWrite(Sound, LOW);
}

void Bye() {

  digitalWrite(Sound, HIGH);
  delay(200);
  digitalWrite(Sound, LOW);
}
