#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const char* ssid = "Tell my WiFi I love her";
const char* password = "xxxxxxxxxx";
const char* mqtt_server = "192.168.2.109";
const char* myTemp = "home/remote5/temperature";
const char* myBat = "home/remote5/bat";
const char* myPub = "home/remote5/msg";
const char* mySub = "home/remote5/cmd";
const char* clientid = "remote5";

ADC_MODE(ADC_VCC); // enable adc to sample onboard voltage

long lastMsg = 0;
long conCnt = 0;
char msg[50];
int value = 0;
double celsius;
const int ledPin = 1; // pin for blue led on an esp-01 only works if serial is not used
boolean skipSleep = false;

WiFiClient espClient;
PubSubClient client(espClient);
OneWire oneWire(5);
DallasTemperature ds18b20(&oneWire);

void callback(char* topic, byte* payload, unsigned int length) {
  skipSleep=true; // don't go to sleep if we receive mqqt message

  //Serial.print("Message arrived [");
  //Serial.print(topic);
  //Serial.print("] ");
  
  for (int i = 0; i < length; i++) {
    //Serial.print((char)payload[i]);
  }
  //Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(ledPin, HIGH);   // Turn the LED on 
  } else { 
      if ((char)payload[0] == '0') {
        digitalWrite(ledPin, LOW);  // Turn the LED off 
    }
  }
}

void reconnect() {
  // Loop until we're reconnected, not the best approach we will get stuck here if network not available
  while (!client.connected()) {
    //Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(clientid)) {
      //Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(myPub, "Good morning!");
      // ... and resubscribe
      client.subscribe(mySub);
    } else {
      //Serial.print("failed, rc=");
      //Serial.print(client.state());
      //Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  //Serial.begin(115200);
  //Serial.println("Booting");`
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    //Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(clientid);

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    client.publish(myPub, "OTA Update Start");
    client.loop();
    //Serial.print("OTA Update");
  });
  ArduinoOTA.onEnd([]() {
    client.publish(myPub, "OTA Update Done");
    client.loop();
    //Serial.println("done!");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    //Serial.print(".");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    client.publish(myPub, "OTA Update Error");
    client.loop();
    //Serial.printf("Error[%u]: ", error);
    //if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    //else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    //else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    //else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    //else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  
  ArduinoOTA.begin(); // start listening for arduinoota updates
  
  ds18b20.begin(); // start one wire temp probe
  
  client.setServer(mqtt_server, 1883); // setup mqqt stuff
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect(); // check mqqt status
  }
  
  
  //int vBat = analogRead(A0); // read the TOUT pin 
  int  vBat = ESP.getVcc(); // internal voltage reference (Vcc)
  //float voltage = vBat * (5.545 / 1023.0); // adjust value, set 5.45 equal to your maximum expected input voltage
  
  String vStr = String(vBat);
  char myChr[8];
  vStr.toCharArray(myChr, vStr.length());
  client.publish(myBat, myChr);

  float temp = 0;
  
  ds18b20.requestTemperatures();
  byte retry = 0;
  do {
    temp = ds18b20.getTempCByIndex(0);
    retry++;
    if (retry==4) {break;}
    delay(10);
  } while (temp == 85.0 || temp == (-127.0));

  vStr = String(temp);
  vStr.toCharArray(myChr, vStr.length());
  client.publish(myTemp, myChr);
  
  client.publish(myPub, "Sleeping in 60 sec");
  
  byte cnt = 60; // 60,000 msec
  while(cnt--) {
    ArduinoOTA.handle();
    client.loop();
    delay(250);
  }

    
  if (!skipSleep) {
    client.publish(myPub, "Back in 5");
    client.loop();
  
    ESP.deepSleep(1000000 * 300, WAKE_RF_DEFAULT); // sleep for 5 min

  }
}
