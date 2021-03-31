#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <analogWrite.h>
#include <PubSubClient.h>
#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "ESPAsyncWebServer.h"

#define BLYNK_PRINT Serial
#define DHTPIN 15
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);



WiFiClient espClient;
PubSubClient client(espClient);

const int rs = 33, en = 32, d4 = 25, d5 = 26, d6 = 27, d7 = 14;
LiquidCrystal Lcd(rs, en, d4, d5, d6, d7);

//================================================================================================================================================================

int MainSwitch = 1;

int Menu = 1;

int ThermometerButtonPin = 19;
int RoomPin = 21;

int CLK = 18;
int DT = 4;
int SW = 15;

int pinA = 18; // Connected to CLK on KY-040
int pinB = 4; // Connected to DT on KY-040
int encoderPosCount = 0;
int pinALast;
int aVal;
int bVal;
boolean bCW;


int lamp = 12;
int ventLamp = 13;
int sliderData;

const int oneWireBus = 22;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

const int potPin = 34;
const int HeaterUsagePin = 35;
const int VentUsagePin = 34;

int RoomPinState;

int tempRead;
int potRead;

int HeaterUsage;
int VentUsage;

float MinVolt = 0;
float MaxVolt = 3.3;

int Intensity;
int SetIntensity = 5;

int ButtonActive;

bool RoomClick = false;

int pinDataMain;
int pinDataKitchen;
int pinDataBedroom;
int pinDataGarage;
int pinDataEncoder;
int pinDataEncoderHold;
int pinDataMenu;

int ButtonStateMain1 = 0;
int ButtonStateMain2;

int ButtonStateKitchen1 = 0;
int ButtonStateKitchen2;

int ButtonStateBedroom1 = 0;
int ButtonStateBedroom2;

int ButtonStateGarage1 = 0;
int ButtonStateGarage2;

int ButtonStateEncoder1 = 0;
int ButtonStateEncoder2;

int ButtonStateEncoderHold1 = 0;
int ButtonStateEncoderHold2;

int ButtonStateMenu1 = 0;
int ButtonStateMenu2;

int DoubleClick1 = 0;
int DoubleClick2;
int Doubleclicked = 0;

int LongHold1 = 0;
int LongHold2;
int Status = 0;

int PressTime;
int PressTime2;

int Room = 1;

float PayloadTempBedroom;

int WantedTempMain = 25;
int WantedTempKitchen = 25;
int WantedTempBedroom = 25;
int WantedTempGarage = 25;

float TempMain = 24;
float TempKitchen = 25;
float TempBedroom = 27;
float TempGarage = 17;

int counter = 0;
int currentStateCLK;
int lastStateCLK;
String currentDir = "";
unsigned long lastButtonPress = 0;

bool SettingIntensity = false;

int ModuleCount = 2;

WidgetLED MainLedHeater(V20);
WidgetLED MainLedVent(V21);
WidgetLED KitchenLedHeater(V22);
WidgetLED KitchenLedVent(V23);
WidgetLED BedroomLedHeater(V24);
WidgetLED BedroomLedVent(V25);
WidgetLED GarageLedHeater(V26);
WidgetLED GarageLedVent(V27);

BlynkTimer Timer;


//-----------------------------------------------------------------------

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "[Token]";

// Your WiFi credentials.
// Set password to "" for open networks.
const char* ssid = "[Network Name]";
const char* pass = "[Network password]";

const char* temperature_topic = "home/livingroom/temperature";
const char* humidity_topic = "home/livingroom/humidity";

const char* mqttServer = "[MQTT Server IP]";
const int mqttPort = 1883;
const char* mqttUser = "[MQTT User]";
const char* mqttPassword = "[MQTT User Password]";
const char* clientID = "[MQTT client ID]"; // MQTT client ID

// Set your access point network credentials
const char* AccespointSsid = "ESP32-Access-Point";
const char* AccespointPassword = "123456789";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

//================================================================================================================================================================

void setup() {
  Serial.begin(115200);
  Lcd.begin(16, 2);
  Lcd.setCursor(0, 0);
  Lcd.print("Welcome");
  Lcd.setCursor(0, 1);
  Lcd.print("Starting up");

  //MainSwitch = 1;

  pinMode(ThermometerButtonPin, INPUT_PULLUP);
  pinMode(RoomPin, INPUT_PULLUP);

  pinMode(lamp, OUTPUT);
  pinMode(ventLamp, OUTPUT);

  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  pinMode(SW, INPUT_PULLUP);

  Status = 1;

  dht.begin();

  delay(10);

  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);
  int wifi_ctr = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  //set up wifi connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");

  client.setServer(mqttServer, mqttPort);
  // connect to broker
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("VIOTMainModule", mqttUser, mqttPassword ))
    {
      Serial.println("connected");
    } else
    {
      Serial.print("failed with state ");
      Serial.println(client.state());
      delay(2000);
    }
  }


  Blynk.begin(auth, ssid, pass, "server.wyns.it", 8081);

  //set a callback function for when data is received from broker
  client.setCallback(Callback);

  // subscribe to topic
  //client.subscribe("Modules/#");
  client.subscribe("Modules/Key/State");
  client.subscribe("Modules/TempModuleBedroom/temp");


  // Read the initial state of CLK
  lastStateCLK = digitalRead(CLK);

  // Setting the ESP as an access point
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(AccespointSsid, AccespointPassword);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);



  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", "Hello world test");
  });

  bool status;

  // Start server
  server.begin();

  sensors.begin();

  pinALast = digitalRead(pinA);

  Timer.setInterval(20L, BlynkWrites);

  Lcd.clear();
  Lcd.setCursor(0, 0);
  Lcd.print("Welcome");
  Lcd.setCursor(0, 1);
  Lcd.print("Starting up.");
  delay(500);
  Lcd.clear();
  Lcd.setCursor(0, 0);
  Lcd.print("Welcome");
  Lcd.setCursor(0, 1);
  Lcd.print("Starting up..");
  delay(500);
  Lcd.clear();
  Lcd.setCursor(0, 0);
  Lcd.print("Welcome");
  Lcd.setCursor(0, 1);
  Lcd.print("Starting up...");
  delay(500);

}

//================================================================================================================================================================

/*BLYNK_CONNECTED() {
  Blynk.syncAll();
  }*/

BLYNK_WRITE(V0) {
  MainSwitch = param.asInt();
}

BLYNK_WRITE(V1) {

  pinDataMain = param.asInt();
}

BLYNK_WRITE(V2) {

  pinDataKitchen = param.asInt();
}

BLYNK_WRITE(V3) {

  pinDataBedroom = param.asInt();
}

BLYNK_WRITE(V4) {

  pinDataGarage = param.asInt();
}

BLYNK_WRITE(V10) {

  WantedTempMain = param.asInt();

}

BLYNK_WRITE(V11) {

  WantedTempKitchen = param.asInt();

}

BLYNK_WRITE(V12) {

  WantedTempBedroom = param.asInt();

}

BLYNK_WRITE(V13) {

  WantedTempGarage = param.asInt();

}

BLYNK_WRITE(V5) {

  Intensity = param.asInt();

}

//================================================================================================================================================================

void Callback (char* topic, byte * payload, unsigned int length) {

  Serial.print("Message arrived in topic: ");
  Serial.println(topic);

  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  payload[length] = '\0';
  String PayloadMessage = (char *)payload;
  String TopicMessage = (char *)topic;

  if (TopicMessage == "Modules/Key/State") {
    if (PayloadMessage == "Off") {
      MainSwitch = 0;
    } else if (PayloadMessage == "On") {
      MainSwitch = 1;
    }
  } else if (TopicMessage == "Modules/TempModuleBedroom/temp") {

    PayloadTempBedroom = atof(PayloadMessage.c_str());

    //Serial.print("Temperature bedroom: ");
    //Serial.println(PayloadTempBedroom);

  }

  Serial.println();
  Serial.println("-----------------------");

}


//================================================================================================================
//  MULTI-CLICK:  One Button, Multiple Events

// Button timing variables
int debounce = 20;          // ms debounce period to prevent flickering when pressing or releasing the button
int DCgap = 250;            // max ms between clicks for a double click event
int holdTime = 1000;        // ms hold period: how long to wait for press+hold event
int longHoldTime = 3000;    // ms long hold period: how long to wait for press+hold event

// Button variables
boolean buttonVal = LOW;   // value read from button
boolean buttonLast = HIGH;  // buffered value of the button's previous state
boolean DCwaiting = false;  // whether we're waiting for a double click (down)
boolean DConUp = false;     // whether to register a double click on next release, or whether to wait and click
boolean singleOK = true;    // whether it's OK to do a single click
long downTime = -1;         // time the button was pressed down
long upTime = -1;           // time the button was released
boolean ignoreUp = false;   // whether to ignore the button release because the click+hold was triggered
boolean waitForUp = false;        // when held, whether to wait for the up event
boolean holdEventPast = false;    // whether or not the hold event happened already
boolean longHoldEventPast = false;// whether or not the long hold event happened already


int checkButton() {
  int event = 0;
  buttonVal = digitalRead(ThermometerButtonPin);
  // Button pressed down
  if (buttonVal == LOW && buttonLast == HIGH && (millis() - upTime) > debounce)
  {
    downTime = millis();
    ignoreUp = false;
    waitForUp = false;
    singleOK = true;
    holdEventPast = false;
    longHoldEventPast = false;
    if ((millis() - upTime) < DCgap && DConUp == false && DCwaiting == true)  DConUp = true;
    else  DConUp = false;
    DCwaiting = false;
  }
  // Button released
  else if (buttonVal == HIGH && buttonLast == LOW && (millis() - downTime) > debounce)
  {
    if (not ignoreUp)
    {
      upTime = millis();
      if (DConUp == false) DCwaiting = true;
      else
      {
        event = 2;
        DConUp = false;
        DCwaiting = false;
        singleOK = false;
      }
    }
  }
  // Test for normal click event: DCgap expired
  if ( buttonVal == HIGH && (millis() - upTime) >= DCgap && DCwaiting == true && DConUp == false && singleOK == true && event != 2)
  {
    event = 1;
    Serial.println("Button pressed");

    ButtonActive = 1;

    DCwaiting = false;
  }
  // Test for hold
  if (buttonVal == LOW && (millis() - downTime) >= holdTime) {
    // Trigger "normal" hold
    if (not holdEventPast)
    {
      event = 3;
      waitForUp = true;
      ignoreUp = true;
      DConUp = false;
      DCwaiting = false;
      //downTime = millis();
      holdEventPast = true;
    }
    // Trigger "long" hold
    if ((millis() - downTime) >= longHoldTime)
    {
      if (not longHoldEventPast)
      {
        event = 4;
        longHoldEventPast = true;
      }
    }
  }
  buttonLast = buttonVal;
  return event;
}

//================================================================================================================
//  MULTI-CLICK:  One Button, Multiple Events

// Button timing variables
int debounce2 = 20;          // ms debounce period to prevent flickering when pressing or releasing the button
int DCgap2 = 250;            // max ms between clicks for a double click event
int holdTime2 = 1000;        // ms hold period: how long to wait for press+hold event
int longHoldTime2 = 3000;    // ms long hold period: how long to wait for press+hold event

// Button variables
boolean buttonVal2 = LOW;   // value read from button
boolean buttonLast2 = HIGH;  // buffered value of the button's previous state
boolean DCwaiting2 = false;  // whether we're waiting for a double click (down)
boolean DConUp2 = false;     // whether to register a double click on next release, or whether to wait and click
boolean singleOK2 = true;    // whether it's OK to do a single click
long downTime2 = -1;         // time the button was pressed down
long upTime2 = -1;           // time the button was released
boolean ignoreUp2 = false;   // whether to ignore the button release because the click+hold was triggered
boolean waitForUp2 = false;        // when held, whether to wait for the up event
boolean holdEventPast2 = false;    // whether or not the hold event happened already
boolean longHoldEventPast2 = false;// whether or not the long hold event happened already


int checkButton2() {
  int event2 = 0;
  buttonVal2 = digitalRead(RoomPin);
  // Button pressed down
  if (buttonVal2 == LOW && buttonLast2 == HIGH && (millis() - upTime2) > debounce2)
  {
    downTime2 = millis();
    ignoreUp2 = false;
    waitForUp2 = false;
    singleOK2 = true;
    holdEventPast2 = false;
    longHoldEventPast2 = false;
    if ((millis() - upTime2) < DCgap2 && DConUp2 == false && DCwaiting2 == true)  DConUp2 = true;
    else  DConUp2 = false;
    DCwaiting2 = false;
  }
  // Button released
  else if (buttonVal2 == HIGH && buttonLast2 == LOW && (millis() - downTime2) > debounce2)
  {
    if (not ignoreUp2)
    {
      upTime2 = millis();
      if (DConUp2 == false) DCwaiting2 = true;
      else
      {
        event2 = 2;
        DConUp2 = false;
        DCwaiting2 = false;
        singleOK2 = false;
      }
    }
  }
  // Test for normal click event: DCgap expired
  if ( buttonVal2 == HIGH && (millis() - upTime2) >= DCgap2 && DCwaiting2 == true && DConUp2 == false && singleOK2 == true && event2 != 2)
  {
    event2 = 1;

    DCwaiting2 = false;
  }
  // Test for hold
  if (buttonVal2 == LOW && (millis() - downTime2) >= holdTime2) {
    // Trigger "normal" hold
    if (not holdEventPast2)
    {
      event2 = 3;
      waitForUp2 = true;
      ignoreUp2 = true;
      DConUp2 = false;
      DCwaiting2 = false;
      //downTime = millis();
      holdEventPast2 = true;
    }
    // Trigger "long" hold
    if ((millis() - downTime2) >= longHoldTime2)
    {
      if (not longHoldEventPast2)
      {
        event2 = 4;
        longHoldEventPast2 = true;
      }
    }
  }
  buttonLast2 = buttonVal2;
  return event2;
}

//================================================================================================================
//  MULTI-CLICK:  One Button, Multiple Events

// Button timing variables
int Encoderdebounce = 20;          // ms debounce period to prevent flickering when pressing or releasing the button
int EncoderDCgap = 250;            // max ms between clicks for a double click event
int EncoderholdTime = 1000;        // ms hold period: how long to wait for press+hold event
int EncoderlongHoldTime = 3000;    // ms long hold period: how long to wait for press+hold event

// Button variables
boolean EncoderbuttonVal = LOW;   // value read from button
boolean EncoderbuttonLast = HIGH;  // buffered value of the button's previous state
boolean EncoderDCwaiting = false;  // whether we're waiting for a double click (down)
boolean EncoderDConUp = false;     // whether to register a double click on next release, or whether to wait and click
boolean EncodersingleOK = true;    // whether it's OK to do a single click
long EncoderdownTime = -1;         // time the button was pressed down
long EncoderupTime = -1;           // time the button was released
boolean EncoderignoreUp = false;   // whether to ignore the button release because the click+hold was triggered
boolean EncoderwaitForUp = false;        // when held, whether to wait for the up event
boolean EncoderholdEventPast = false;    // whether or not the hold event happened already
boolean EncoderlongHoldEventPast = false;// whether or not the long hold event happened already
int Encoderevent = 0;

void MultiEventEncoder() {

  //int EncodercheckButton (){
  Encoderevent = 0;
  EncoderbuttonVal = digitalRead(SW);
  // Button pressed down
  if (EncoderbuttonVal == LOW && EncoderbuttonLast == HIGH && (millis() - EncoderupTime) > Encoderdebounce)
  {
    EncoderdownTime = millis();
    EncoderignoreUp = false;
    EncoderwaitForUp = false;
    EncodersingleOK = true;
    EncoderholdEventPast = false;
    EncoderlongHoldEventPast = false;
    if ((millis() - EncoderupTime) < EncoderDCgap && EncoderDConUp == false && EncoderDCwaiting == true)  EncoderDConUp = true;
    else  EncoderDConUp = false;
    EncoderDCwaiting = false;
  }
  // Button released
  else if (EncoderbuttonVal == HIGH && EncoderbuttonLast == LOW && (millis() - EncoderdownTime) > Encoderdebounce)
  {
    if (not EncoderignoreUp)
    {
      EncoderupTime = millis();
      if (EncoderDConUp == false) EncoderDCwaiting = true;
      else
      {
        Encoderevent = 2;
        EncoderDConUp = false;
        EncoderDCwaiting = false;
        EncodersingleOK = false;
      }
    }
  }
  // Test for normal click event: DCgap expired
  if ( EncoderbuttonVal == HIGH && (millis() - EncoderupTime) >= EncoderDCgap && EncoderDCwaiting == true && EncoderDConUp == false && EncodersingleOK == true && Encoderevent != 2)
  {
    Encoderevent = 1;

    EncoderDCwaiting = false;
  }
  // Test for hold
  if (EncoderbuttonVal == LOW && (millis() - EncoderdownTime) >= EncoderholdTime) {
    // Trigger "normal" hold
    if (not EncoderholdEventPast)
    {
      Encoderevent = 3;
      EncoderwaitForUp = true;
      EncoderignoreUp = true;
      EncoderDConUp = false;
      EncoderDCwaiting = false;
      //downTime = millis();
      EncoderholdEventPast = true;
    }
    // Trigger "long" hold
    if ((millis() - EncoderdownTime) >= EncoderlongHoldTime)
    {
      if (not EncoderlongHoldEventPast)
      {
        Encoderevent = 4;
        EncoderlongHoldEventPast = true;
      }
    }
  }
  EncoderbuttonLast = EncoderbuttonVal;
  //return Encoderevent;
}



//================================================================================================================================================================

void SerialPrints() {

  //Serial.println(analogRead(34));
  //Serial.println(HeaterUsage);
  //Serial.println(VentUsage);
  //Serial.println(pinData);
  //Serial.println(temperature);
  //Serial.println(WantedTemp);
  //Serial.println(MainSwitch);
  //Serial.println(pinDataEncoder);

}

//================================================================================================================================================================


void clickEvent() {

  if (Room == 1) {

    if (ButtonStateMain1 == 0) {

      if (ButtonStateMain2 == 1)
      {
        Serial.println("Button Pressed 2");
        ButtonStateMain2 = 0;
        analogWrite(lamp, 0);
        analogWrite(ventLamp, 0);
        pinDataMain = 0;

      } else {
        Serial.println("Button Pressed");
        pinDataMain = 1;
        ButtonStateMain2 = 1;
      }
    }
  }

  else if (Room == 2) {

    if (ButtonStateKitchen1 == 0) {

      if (ButtonStateKitchen2 == 1)
      {
        Serial.println("Button Pressed 2");
        ButtonStateKitchen2 = 0;
        analogWrite(lamp, 0);
        analogWrite(ventLamp, 0);
        pinDataKitchen = 0;

      } else {
        Serial.println("Button Pressed");
        pinDataKitchen = 1;
        ButtonStateKitchen2 = 1;
      }
    }
  }

  else if (Room == 3) {

    if (ButtonStateBedroom1 == 0) {

      if (ButtonStateBedroom2 == 1)
      {
        Serial.println("Button Pressed 2");
        ButtonStateBedroom2 = 0;
        analogWrite(lamp, 0);
        analogWrite(ventLamp, 0);
        pinDataBedroom = 0;

      } else {
        Serial.println("Button Pressed");
        pinDataBedroom = 1;
        ButtonStateBedroom2 = 1;
      }
    }
  }
  else if (Room == 4) {

    if (ButtonStateGarage1 == 0) {

      if (ButtonStateGarage2 == 1)
      {
        Serial.println("Button Pressed 2");
        ButtonStateGarage2 = 0;
        analogWrite(lamp, 0);
        analogWrite(ventLamp, 0);
        pinDataGarage = 0;

      } else {
        Serial.println("Button Pressed");
        pinDataGarage = 1;
        ButtonStateGarage2 = 1;
      }

    }
  }
}

void EncoderClick() {

  if (ButtonStateEncoder1 == 0) {

    if (ButtonStateEncoder2 == 1)
    {
      Serial.println("Encoder Button Pressed 2");
      ButtonStateEncoder2 = 0;
      pinDataEncoder = 0;
      delay(500);

    } else {
      Serial.println("Encoder Button Pressed");
      pinDataEncoder = 1;
      ButtonStateEncoder2 = 1;
      Lcd.setCursor(0, 1);
      Lcd.print("Setting Temp.:    ");
      delay(500);
    }
  }
}

void RotorEncoder () {

  if (Room == 1 ) {
    while (true) {

      aVal = digitalRead(pinA);
      if (digitalRead(SW) == LOW) {
        Serial.println("Encoder Button Pressed 2");
        ButtonStateEncoder2 = 0;
        pinDataEncoder = 0;
        break;
      }
      if (aVal != pinALast) { // Means the knob is rotating
        // if the knob is rotating, we need to determine direction
        // We do that by reading pin B.
        if (digitalRead(pinB) != aVal) { // Means pin A Changed first - We're Rotating Clockwise
          WantedTempMain ++;
          Serial.print("Ingestelde temperatuur: ");
          Serial.println(WantedTempMain);
          Lcd.setCursor(0, 1);
          Lcd.print("Set Temp.: ");
          Lcd.print(WantedTempMain);
           Lcd.print("      ");
        } else {// Otherwise B changed first and we're moving CCW
          if (WantedTempMain >= 1) {
            WantedTempMain --;
            Lcd.setCursor(0, 1);
            Serial.print("Ingestelde temperatuur: ");
            Serial.println(WantedTempMain);
            Lcd.print("Set Temp.: ");
            Lcd.print(WantedTempMain);
            Lcd.print("      ");
          }
        }
      }
      pinALast = aVal;
    }


  } else if (Room == 2) {
    while (true) {
      aVal = digitalRead(pinA);

      if (digitalRead(SW) == LOW) {
        Serial.println("Encoder Button Pressed 2");
        ButtonStateEncoder2 = 0;
        pinDataEncoder = 0;
        break;
      }
      else if (aVal != pinALast) { // Means the knob is rotating
        // if the knob is rotating, we need to determine direction
        // We do that by reading pin B.
        if (digitalRead(pinB) != aVal) { // Means pin A Changed first - We're Rotating Clockwise
          WantedTempKitchen ++;
          Serial.print("Ingestelde temperatuur: ");
          Serial.println(WantedTempKitchen);
          Lcd.setCursor(0, 1);
          Lcd.print("Set Temp.: ");
          Lcd.print(WantedTempKitchen);
          Lcd.print("      ");
        } else {// Otherwise B changed first and we're moving CCW
          if (WantedTempKitchen >= 1) {
            WantedTempKitchen --;
            Lcd.setCursor(0, 1);
            Serial.print("Ingestelde temperatuur: ");
            Serial.println(WantedTempKitchen);
            Lcd.print("Set Temp.: ");
            Lcd.print(WantedTempKitchen);
            Lcd.print("      ");
          }
        }
      }
      pinALast = aVal;
    }

  } else if (Room == 3) {
    while (true) {

      aVal = digitalRead(pinA);
      if (digitalRead(SW) == LOW) {
        Serial.println("Encoder Button Pressed 2");
        ButtonStateEncoder2 = 0;
        pinDataEncoder = 0;
        break;
      }
      if (aVal != pinALast) { // Means the knob is rotating
        // if the knob is rotating, we need to determine direction
        // We do that by reading pin B.
        if (digitalRead(pinB) != aVal) { // Means pin A Changed first - We're Rotating Clockwise
          WantedTempBedroom ++;
          Serial.print("Ingestelde temperatuur: ");
          Serial.println(WantedTempBedroom);
          Lcd.setCursor(0, 1);
          Lcd.print("Set Temp.: ");
          Lcd.print(WantedTempBedroom);
          Lcd.print("      ");
        } else {// Otherwise B changed first and we're moving CCW
          if (WantedTempBedroom >= 1) {
            WantedTempBedroom --;
            Lcd.setCursor(0, 1);
            Serial.print("Ingestelde temperatuur: ");
            Serial.println(WantedTempBedroom);
            Lcd.print("Set Temp.: ");
            Lcd.print(WantedTempBedroom);
            Lcd.print("      ");
          }
        }
      }
      pinALast = aVal;
    }

  } else if (Room == 4) {
    while (true) {

      aVal = digitalRead(pinA);
      if (digitalRead(SW) == LOW) {
        Serial.println("Encoder Button Pressed 2");
        ButtonStateEncoder2 = 0;
        pinDataEncoder = 0;
        break;
      }
      if (aVal != pinALast) { // Means the knob is rotating
        // if the knob is rotating, we need to determine direction
        // We do that by reading pin B.
        if (digitalRead(pinB) != aVal) { // Means pin A Changed first - We're Rotating Clockwise
          WantedTempGarage ++;
          Serial.print("Ingestelde temperatuur: ");
          Serial.println(WantedTempGarage);
          Lcd.setCursor(0, 1);
          Lcd.print("Set Temp.: ");
          Lcd.print(WantedTempGarage);
          Lcd.print("      ");
        } else {// Otherwise B changed first and we're moving CCW
          if (WantedTempGarage >= 1) {
            WantedTempGarage --;
            Lcd.setCursor(0, 1);
            Serial.print("Ingestelde temperatuur: ");
            Serial.println(WantedTempGarage);
            Lcd.print("Set Temp.: ");
            Lcd.print(WantedTempGarage);
            Lcd.print("      ");
          }
        }
      }
      pinALast = aVal;
    }
  }
}


void RoomclickEvent() {

  Serial.println("Room switch");
  if (Room <= 3) {
    Room++;
    ButtonActive = 0;
    Serial.print("Room: ");
    Serial.println(Room);
  } else {
    Room = 1;
  }

}

void MenuHold () {

  if (ButtonStateMenu1 == 0) {

    if (ButtonStateMenu2 == 1)
    {
      Serial.println("Menu Button Hold 2");
      ButtonStateMenu2 = 0;
      pinDataMenu = 0;
      delay(500);

    } else {
      Serial.println("Menu Button Hold");
      pinDataMenu = 1;
      ButtonStateMenu2 = 1;
      if (Menu == 1) {
        Lcd.setCursor(0, 0);
        Lcd.print("Choose Menu:      ");
        Lcd.setCursor(0, 1);
        Lcd.print("Idle              ");
      } else if (Menu == 2) {
        Lcd.setCursor(0, 0);
        Lcd.print("Choose Menu:      ");
        Lcd.setCursor(0, 1);
        Lcd.print("Thermostat        ");

      } else if (Menu == 3) {
        Lcd.setCursor(0, 0);
        Lcd.print("Choose Menu:      ");
        Lcd.setCursor(0, 1);
        Lcd.print("Add Device        ");

      } else if (Menu == 4) {
        Lcd.setCursor(0, 0);
        Lcd.print("Choose Menu:      ");
        Lcd.setCursor(0, 1);
        Lcd.print("Settings          ");

      }
    }
  }
}

void EncoderHold() {

  if (ButtonStateEncoderHold1 == 0) {

    if (ButtonStateEncoderHold2 == 1)
    {
      Serial.println("Encoder Button Hold 2");
      SettingIntensity = false;
      ButtonStateEncoderHold2 = 0;
      pinDataEncoderHold = 0;
      delay(500);

    } else {
      Serial.println("Encoder Button Hold");
      pinDataEncoderHold = 1;
      ButtonStateEncoderHold2 = 1;
      SettingIntensity = true;
      Lcd.setCursor(0, 1);
      Lcd.print("Set Intensity   ");
      delay(500);
    }
  }

}

void SetUseIntensity () {

  while (true) {

    aVal = digitalRead(pinA);
    if (digitalRead(SW) == LOW) {
      MultiEventEncoder();
      Serial.println("Button pressed");
      break;
    }
    if (aVal != pinALast) { // Means the knob is rotating
      // if the knob is rotating, we need to determine direction
      // We do that by reading pin B.
      if (digitalRead(pinB) != aVal) { // Means pin A Changed first - We're Rotating Clockwise
        if (SetIntensity <= 9) {
          SetIntensity ++;
          Serial.print("Ingestelde Intensity: ");
          Serial.println(SetIntensity);
          Lcd.setCursor(0, 1);
          Lcd.print("Set Intensity:");
          Lcd.print(SetIntensity);
        }
      } else {// Otherwise B changed first and we're moving CCW
        if (SetIntensity >= 1) {
          SetIntensity --;
          Lcd.setCursor(0, 1);
          Serial.print("Ingestelde Intensity: ");
          Serial.println(SetIntensity);
          Lcd.print("Set Intensity:");
          Lcd.print(SetIntensity);
        }
      }
    }
    pinALast = aVal;
  }
}

//================================================================================================================================================================

void DisplayRoom1 () {
  /*Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" *C");*/
  Lcd.setCursor(0, 0);
  Lcd.print("-Main- ");
  Lcd.setCursor(0, 1);
  Lcd.print("Temp.: ");
  Lcd.print(TempMain);
  Lcd.println(" *C ");
}

void TempRoom1() {

  if (pinDataMain == 1 && Doubleclicked == 0) {
    if (WantedTempMain == 0) {

    } else if (WantedTempMain + 1 > TempMain) {
      Serial.println("Heater on   ");
      MainLedHeater.on();
      MainLedVent.off();
      Blynk.virtualWrite(V1, 1);
      Lcd.setCursor(7, 0);
      Lcd.print("Heat: On    ");
      analogWrite(lamp, Intensity);
      analogWrite(ventLamp, 0);
    } else if (WantedTempMain < TempMain) {
      MainLedVent.on();
      MainLedHeater.off();
      Blynk.virtualWrite(V1, 1);
      Serial.println("Vent on");
      Lcd.setCursor(7, 0);
      analogWrite(ventLamp, Intensity);
      analogWrite(lamp, 0);
      Lcd.print("Vent: On     ");
    }
  } else {
    Blynk.virtualWrite(V1, 0);
    Blynk.virtualWrite(V1, 0);
    MainLedHeater.off();
    MainLedVent.off();
    analogWrite(lamp, 0);
    analogWrite(ventLamp, 0);
    Lcd.setCursor(7, 0);
    Lcd.print("Natural     ");
  }
}

void DisplayRoom2 () {

  Lcd.setCursor(0, 0);
  Lcd.print("-Kit.- ");
  Lcd.setCursor(0, 1);
  Lcd.print("Temp.: ");
  Lcd.print(TempKitchen);
  Lcd.println(" *C ");
}

void TempRoom2 () {

  if (pinDataKitchen == 1 && Doubleclicked == 0) {
    if (WantedTempKitchen == 0) {

    } else if (WantedTempKitchen + 1 > TempKitchen) {
      KitchenLedHeater.on();
      KitchenLedVent.off();
      Blynk.virtualWrite(V2, 1);
      Serial.println("Heater on");
      Lcd.setCursor(7, 0);
      Lcd.print("Heat: On    ");
      analogWrite(lamp, Intensity);
      analogWrite(ventLamp, 0);
    } else if (WantedTempKitchen < TempKitchen) {
      KitchenLedVent.on();
      KitchenLedHeater.off();
      Blynk.virtualWrite(V2, 1);
      Serial.println("Vent on");
      Lcd.setCursor(7, 0);
      analogWrite(ventLamp, Intensity);
      analogWrite(lamp, 0);
      Lcd.print("Vent: On    ");
    }
  } else {
    Blynk.virtualWrite(V2, 0);
    KitchenLedHeater.off();
    KitchenLedVent.off();
    analogWrite(lamp, 0);
    analogWrite(ventLamp, 0);
    Lcd.setCursor(7, 0);
    Lcd.print("Natural     ");
  }
}

void DisplayRoom3 () {

  Lcd.setCursor(0, 0);
  Lcd.print("-Bed.- ");
  Lcd.setCursor(0, 1);
  Lcd.print("Temp.: ");
  Lcd.print(TempBedroom);
  Lcd.println(" *C ");
}

void TempRoom3 () {

  if (pinDataBedroom == 1 && Doubleclicked == 0) {
    if (WantedTempBedroom == 0) {

    } else if (WantedTempBedroom + 1 > TempBedroom) {
      BedroomLedHeater.on();
      BedroomLedVent.off();
      Blynk.virtualWrite(V3, 1);
      Serial.println("Heater on");
      Lcd.setCursor(7, 0);
      Lcd.print("Heat: On   ");
      analogWrite(lamp, Intensity);
      analogWrite(ventLamp, 0);
      client.publish("Modules/Temp/Use", "Heater");
    } else if (WantedTempBedroom < TempBedroom) {
      BedroomLedVent.on();
      BedroomLedHeater.off();
      Blynk.virtualWrite(V3, 1);
      Serial.println("Vent on");
      Lcd.setCursor(7, 0);
      analogWrite(ventLamp, Intensity);
      analogWrite(lamp, 0);
      Lcd.print("Vent: On   ");
      client.publish("Modules/Temp/Use", "Vent");
    }
  } else {
    BedroomLedHeater.off();
    BedroomLedVent.off();
    Blynk.virtualWrite(V3, 0);
    analogWrite(lamp, 0);
    analogWrite(ventLamp, 0);
    Lcd.setCursor(7, 0);
    Lcd.print("Natural   ");
    client.publish("Modules/Temp/Use", "None");
  }
}


void DisplayRoom4 () {

  Lcd.setCursor(0, 0);
  Lcd.print("-Gar.- ");
  Lcd.setCursor(0, 1);
  Lcd.print("Temp.: ");
  Lcd.print(TempGarage);
  Lcd.println(" *C ");
}

void TempRoom4() {
  if (pinDataGarage == 1 && Doubleclicked == 0) {
    if (WantedTempBedroom == 0) {

    } else if (WantedTempGarage + 1 > TempGarage) {
      GarageLedHeater.on();
      GarageLedVent.off();
      Blynk.virtualWrite(V4, 1);
      Serial.println("Heater on");
      Lcd.setCursor(7, 0);
      Lcd.print("Heat: On   ");
      analogWrite(lamp, Intensity);
      analogWrite(ventLamp, 0);
    } else if (WantedTempGarage < TempGarage) {
      GarageLedVent.on();
      GarageLedHeater.off();
      Blynk.virtualWrite(V4, 1);
      Serial.println("Vent on");
      Lcd.setCursor(7, 0);
      analogWrite(ventLamp, Intensity);
      analogWrite(lamp, 0);
      Lcd.print("Vent: On   ");

    } else {
      GarageLedHeater.off();
      GarageLedVent.off();
      Blynk.virtualWrite(V4, 0);
      analogWrite(lamp, 0);
      analogWrite(ventLamp, 0);
      Lcd.setCursor(7, 0);
      Lcd.print("Natural   ");
    }
  }
}

//================================================================================================================================================================

void BlynkWrites () {

  Blynk.virtualWrite(V15, TempMain);
  Blynk.virtualWrite(V16, TempKitchen);
  Blynk.virtualWrite(V17, TempBedroom);
  Blynk.virtualWrite(V18, TempGarage);

  Blynk.virtualWrite(V20, HeaterUsage);
  Blynk.virtualWrite(V21, VentUsage);

  Blynk.virtualWrite(V30, WantedTempMain);
  Blynk.virtualWrite(V31, WantedTempKitchen);
  Blynk.virtualWrite(V32, WantedTempBedroom);
  Blynk.virtualWrite(V33, WantedTempGarage);
  //Blynk.virtualWrite(V0, pinData);

  Blynk.virtualWrite(V6, Intensity);
}

void Publishers() {

  String TempMessageMain = String ((int)TempMain);
  String TempMessageKitchen = String ((int)TempKitchen);
  String TempMessageBedroom = String ((int)TempBedroom);
  String TempMessageGarage = String ((int)TempGarage);

  String HeaterUsageMessage = String ((int)HeaterUsage);
  String VentUsageMessage = String ((int)VentUsage);


  client.publish("Home/Main/temp", &TempMessageMain[0]);
  client.publish("Home/Kitchen/temp", TempMessageKitchen.c_str());
  client.publish("Home/Bedroom/temp", TempMessageBedroom.c_str());
  client.publish("Home/Garage/temp", TempMessageGarage.c_str());

  client.publish("Home/Heater/usage", HeaterUsageMessage.c_str());
  client.publish("Home/Vent/usage", VentUsageMessage.c_str());

}

//================================================================================================================================================================
void loop() {

  Publishers();
  client.loop();
  Blynk.run();
  Timer.run();

  if (MainSwitch == 1) {

    MultiEventEncoder();
    //BlynkWrites();
    SerialPrints();

    if (pinDataMenu == 1) {
      MenuChooser();
    }

    if (Menu == 1) {
      Idle();
    } else if (Menu == 2) {
      Thermostat();
    } else if (Menu == 3) {
      AddDevice();
    } else if (Menu == 4) {
      Settings();
    }

    int b2 = checkButton2();
    //if (b2 == 1) RoomclickEvent();
    //if (b2 == 2) doubleClickEvent();
    if (b2 == 3) MenuHold();



  } else {
    Lcd.setCursor(0, 0);
    Lcd.print("Home                ");
    Lcd.setCursor(0, 1);
    Lcd.print("Offline             ");

    analogWrite(lamp, 0);
    analogWrite(ventLamp , 0);
  }
}

//================================================================================================================================================================


void MenuChooser() {

  while (true) {
    //Serial.println("Choosing Menu");
    aVal = digitalRead(pinA);
    if (digitalRead(RoomPin) == LOW) {
      Serial.println("Menu Button Pressed 2");
      ButtonStateMenu2 = 0;
      pinDataMenu = 0;
      break;
    }
    else if (aVal != pinALast) { // Means the knob is rotating
      // if the knob is rotating, we need to determine direction
      // We do that by reading pin B.
      if (digitalRead(pinB) != aVal) { // Means pin A Changed first - We're Rotating Clockwise
        if (Menu <= 3) {
          Menu ++;
          Serial.print("Current menu: ");
          Serial.println(Menu);
          if (Menu == 1) {
            Lcd.setCursor(0, 0);
            Lcd.print("Choose Menu:      ");
            Lcd.setCursor(0, 1);
            Lcd.print("Idle              ");
          } else if (Menu == 2) {
            Lcd.setCursor(0, 0);
            Lcd.print("Choose Menu:      ");
            Lcd.setCursor(0, 1);
            Lcd.print("Thermostat        ");

          } else if (Menu == 3) {
            Lcd.setCursor(0, 0);
            Lcd.print("Choose Menu:      ");
            Lcd.setCursor(0, 1);
            Lcd.print("Add Device        ");

          } else if (Menu == 4) {
            Lcd.setCursor(0, 0);
            Lcd.print("Choose Menu:      ");
            Lcd.setCursor(0, 1);
            Lcd.print("Settings          ");

          }

        }
      } else {// Otherwise B changed first and we're moving CCW
        if (Menu >= 2) {
          Menu --;
          Serial.print("Current menu: ");
          Serial.println(Menu);
          if (Menu == 1) {
            Lcd.setCursor(0, 0);
            Lcd.print("Choose Menu:      ");
            Lcd.setCursor(0, 1);
            Lcd.print("Idle              ");
          } else if (Menu == 2) {
            Lcd.setCursor(0, 0);
            Lcd.print("Choose Menu:      ");
            Lcd.setCursor(0, 1);
            Lcd.print("Thermostat        ");

          } else if (Menu == 3) {
            Lcd.setCursor(0, 0);
            Lcd.print("Choose Menu:      ");
            Lcd.setCursor(0, 1);
            Lcd.print("Add Device        ");

          } else if (Menu == 4) {
            Lcd.setCursor(0, 0);
            Lcd.print("Choose Menu:      ");
            Lcd.setCursor(0, 1);
            Lcd.print("Settings          ");

          }

        }
      }
    }
    pinALast = aVal;
  }
}

//================================================================================================================================================================

void Idle() {

  Lcd.setCursor(0, 0);
  Lcd.print("VIOT Home             ");
  Lcd.setCursor(0, 1);
  Lcd.print("Home connected!       ");
}

void Thermostat () {

  sensors.requestTemperatures();
  float TempTemp = sensors.getTempCByIndex(0);
  potRead = analogRead(potPin);

  HeaterUsage = map(analogRead(HeaterUsagePin), 0, 4095, MinVolt, MaxVolt);

  VentUsage = map(analogRead(VentUsagePin), 0, 4095, MinVolt, MaxVolt);

  Intensity = map(SetIntensity, 0, 10, 0, 230);

  if (isnan(TempTemp)) {

  } else {
    TempMain = TempTemp;
  }

  
  if (isnan(PayloadTempBedroom)) {

  } else {
    TempBedroom = PayloadTempBedroom;
  }


  if (pinDataEncoderHold == 1) {
    SetUseIntensity();
  }

  if (pinDataEncoder == 1) {
    RotorEncoder ();
  }

  if (SettingIntensity == false) {

    if (Room == 1) {

      TempRoom1();
      DisplayRoom1();

    } else if (Room == 2) {

      TempRoom2();
      DisplayRoom2();

    } else if (Room == 3) {

      TempRoom3();
      DisplayRoom3();

    } else if (Room == 4) {

      TempRoom4();
      DisplayRoom4();

    }
  }

  int b = checkButton();
  if (b == 1) clickEvent();
  //if (b == 2) doubleClickEvent();
  //if (b == 3) HoldEvent();

  int b2 = checkButton2();
  if (b2 == 1) RoomclickEvent();
  else RoomClick = false;
  if (b2 == 3) MenuHold();

  int Encoderb = Encoderevent;
  if (Encoderb == 1) EncoderClick();
  //if (Encoderb == 2) doubleClickEvent();
  if (Encoderb == 3) EncoderHold();
  else SettingIntensity = false;
}

void AddDevice () {

  Lcd.setCursor(0, 0);
  Lcd.print("Add VIOT Module       ");
  Lcd.setCursor(0, 1);
  Lcd.print("Modules: ");
  Lcd.print(ModuleCount);
  Lcd.print("         ");

  int b2 = checkButton2();
  //if (b2 == 1) RoomclickEvent();
  //if (b2 == 2) doubleClickEvent();
  if (b2 == 3) MenuHold();

}

void Settings () {

  Lcd.setCursor(0, 0);
  Lcd.print("Settings       ");
  Lcd.setCursor(0, 1);
  Lcd.print("               ");

  int b2 = checkButton2();
  //if (b2 == 1) RoomclickEvent();
  //if (b2 == 2) doubleClickEvent();
  if (b2 == 3) MenuHold();
}
