/*
  EmonTx CT123 Voltage Serial Only example
  
  Part of the openenergymonitor.org project
  Licence: GNU GPL V3
  
  Author: Trystan Lea
*/

// display

#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#define OLED_WIDTH 128
#define OLED_HEIGHT 64

#define OLED_ADDR   0x3C

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT);

//mqtt

#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "mywifi";
const char* password = "mypassword";
// Add your MQTT Broker IP details

const char* mqtt_server = "";
const int mqttPort = 1883;
const char* mqttUser = "user";
const char* mqttPassword = "password";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;


//






#include "EmonLib.h"

// Create  instances for each CT channel
EnergyMonitor ct1,ct2,ct3, ct4;

// On-board emonTx LED
const int LEDpin = 9;                                                    

void setup() 
{
  Serial.begin(9600);
  // while (!Serial) {}
  // wait for serial port to connect. Needed for Leonardo only

  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();

//start wifi and mqtt

WiFi.begin(ssid, password);
 
while (WiFi.status() != WL_CONNECTED) {
delay(500);
Serial.print("Connecting to WiFi..");
}
Serial.println("Connected to the WiFi network");
  client.setServer(mqtt_server, 1883);

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
 
    if (client.connect("Enermon", mqttUser, mqttPassword )) {
 
      Serial.println("connected");  
 
    } else {
 
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
 
    }
  }

//start of enermon

  
  Serial.println("emonTX Shield CT123 Voltage Serial Only example"); 
  Serial.println("OpenEnergyMonitor.org");
 
  analogReadResolution(ADC_BITS);  //this is necessary to change to 12bit resolution for ESP32 instead of default 10
  
  // Calibration factor = CT ratio / burden resistance = (30A / 1V = 30)
  ct1.current(34, 22);
  ct2.current(35, 22);                                     
//  ct3.current(32, 22);
//  ct4.current(33, 22); 
  
  // (ADC input, calibration, phase_shift)
  ct1.voltage(39, 300.6, 1.7);                                
  ct2.voltage(39, 300.6, 1.7);                                
//  ct3.voltage(39, 300.6, 1.7);
//  ct4.voltage(39, 300.6, 1.7);
  
  // Setup indicator LED
  pinMode(LEDpin, OUTPUT);                                              
  digitalWrite(LEDpin, HIGH);                                                                                  
}

void loop() 

{
//check wifi still connected

while (WiFi.status() != WL_CONNECTED) {
Serial.print("Connecting to WiFi..");
WiFi.begin(ssid, password);
delay(5000);
}

//check mqtt connected

while (!client.connected()) {
  Serial.println("Connecting to MQTT...");
  client.setServer(mqtt_server, 1883);
  delay(2000); 
  if (client.connect("Enermon", mqttUser, mqttPassword )) {

    Serial.println("connected");  

  } else {

    Serial.print("failed with state ");
    Serial.print(client.state());
    delay(2000);

  }
}




 
  // Calculate all. No.of crossings, time-out 
  ct1.calcVI(20,2000);                                                  
  ct2.calcVI(20,2000);
//  ct3.calcVI(20,2000);
//  ct4.calcVI(20,2000);
  // Round values
  ct1.realPower = 10*round(ct1.realPower/10);  
  ct2.realPower = 10*round(ct2.realPower/10);  
  
  // Print power 
  Serial.print("CT1: "); 
  Serial.print(ct1.realPower);     
  Serial.print(" W ");
  Serial.print("CT2: ");  
  Serial.print(ct2.realPower);
  Serial.print(" W "); 
//  Serial.print(" "); 
//  Serial.print(ct3.realPower);
//  Serial.print(" "); 
//  Serial.print(ct4.realPower);
//  Serial.print(" "); 
//  Serial.print(ct1.Vrms);
  Serial.println();
    
  // Available properties: ct1.realPower, ct1.apparentPower, ct1.powerFactor, ct1.Irms and ct1.Vrms

//show values on display

  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("CT1: ");
  display.print(int(ct1.realPower));
  display.println(" W");
  display.print("CT2: ");
  display.print(int(ct2.realPower));
  display.println(" W");

  display.display();

//send values to mqtt

  client.publish("homeassistant/sensor/enermon/CT1", String(int(ct1.realPower)).c_str());  // note the .c_str() necessary after the string for mqtt
  client.publish("homeassistant/sensor/enermon/CT2", String(int(ct2.realPower)).c_str());

  delay(1000);
}
