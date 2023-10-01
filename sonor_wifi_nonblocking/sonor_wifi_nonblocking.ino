/*
 Basic MQTT example

 This sketch demonstrates the basic capabilities of the library.
 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic"
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.
 
*/

#include <SPI.h>
#include <PubSubClient.h>
// Load Wi-Fi library
#include <WiFi.h>
#include <esp_log.h>
#include <esp_wifi.h>

const char* ssid = "Hub";
const char* identifier = "ROW1";
const char* password = "848D533724";
//const char* ssid     = "gaia";
const char* mqtt_server = "192.168.86.36";//"192.168.0.106";
//const char* mqtt_server = "192.168.7.1";
byte mac[] = { 0xE0, 0x5A, 0x1B, 0xA7, 0x15, 0x34 };


///
//failed 35,34,39,36
const int trigPin7 = 23;
const int echoPin7 = 22;

const int trigPin6 = 21;
const int echoPin6 = 19;

const int trigPin5 = 5;
const int echoPin5 = 17;

const int trigPin4 = 16;
const int echoPin4 = 4;

const int trigPin3 = 32;  //*
const int echoPin3 = 33;  //*

const int trigPin2 = 25;  //*
const int echoPin2 = 26;  //*

const int trigPin1 = 27;  //*
const int echoPin1 = 14;  //*

const int trigPin0 = 12;  //*
const int echoPin0 = 13;  //*

//define sound velocity in cm/uS
#define SOUND_VELOCITY 0.034

long duration0;
long duration1;
long duration2;
long duration3;
long duration4;
long duration5;
long duration6;
long duration7;

float distanceCm0;
float distanceCm1;
float distanceCm2;
float distanceCm3;
float distanceCm4;
float distanceCm5;
float distanceCm6;
float distanceCm7;

const int detection = 100;

const int sensorStart = 0;
const int sensors = 8;

int durations[sensors];
int distancesCm[sensors];

int lastReconnectAttempt = 0;

///

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
}

boolean reconnect() {
  if (mqttClient.connect(identifier)) {
    mqttClient.subscribe("server");
  }
  return mqttClient.connected();
}


void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(WiFi.status());
  }

  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(callback);

  pinMode(trigPin0, OUTPUT);  // Sets the trigPin as an Output
  pinMode(echoPin0, INPUT);   // Sets the echoPin as an Input
  pinMode(trigPin1, OUTPUT);  // Sets the trigPin as an Output
  pinMode(echoPin1, INPUT);   // Sets the echoPin as an Input
  pinMode(trigPin2, OUTPUT);  // Sets the trigPin as an Output
  pinMode(echoPin2, INPUT);   // Sets the echoPin as an Input
  pinMode(trigPin3, OUTPUT);  // Sets the trigPin as an                     Output
  pinMode(echoPin3, INPUT);   // Sets the echoPin as an Input
  pinMode(trigPin4, OUTPUT);  // Sets the trigPin as an Output
  pinMode(echoPin4, INPUT);   // Sets the echoPin as an Input
  pinMode(trigPin5, OUTPUT);  // Sets the trigPin as an Output
  pinMode(echoPin5, INPUT);   // Sets the echoPin as an Input
  pinMode(trigPin6, OUTPUT);  // Sets the trigPin as an Output
  pinMode(echoPin6, INPUT);   // Sets the echoPin as an Input
  pinMode(trigPin7, OUTPUT);  // Sets the trigPin as an Output
  pinMode(echoPin7, INPUT);   // Sets the echoPin as an Input
  delay(1500);
  lastReconnectAttempt = 0;
}

void loop() {
  if (!mqttClient.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    mqttClient.loop();
  }
  loop_sensor();
 // for (int i = 0; i < sensors; i++) {
 //   if (distancesCm[i] < detection && distancesCm[i] > 0) {
 //     sendMessage(i, distancesCm[i]);
 //   }
 // }
}

void sendMessage(int sensor, int distanceCm){
   if (distanceCm < detection && distanceCm > 0) {
    Serial.println("let em know.....");
    int sensorNumber = sensorStart + sensor;
    String messageToSend = "{\"sensor\":\"" + String(sensorNumber) + "\", \"distance\":\"" + String(distanceCm) + "\"}";
    String topic = "/sensor/" + String(sensorNumber) + "";
    Serial.println(messageToSend);
    Serial.println(topic);
    boolean rc = mqttClient.publish(topic.c_str(), messageToSend.c_str());
    Serial.println(rc);
    delay(200);
   }
}

void loop_sensor() {

  int distance = setDistance(0, trigPin0, echoPin0);
  sendMessage(0, distance);

  distance = setDistance(1, trigPin1, echoPin1);
  sendMessage(1, distance);

  distance = setDistance(2, trigPin2, echoPin2);
  sendMessage(2, distance);

  distance = setDistance(3, trigPin3, echoPin3);
  sendMessage(3, distance);

  distance = setDistance(4, trigPin4, echoPin4);
  sendMessage(4, distance);

  distance = setDistance(5, trigPin5, echoPin5);
  sendMessage(5, distance);

  distance = setDistance(6, trigPin6, echoPin6);
  sendMessage(6, distance);

  distance = setDistance(7, trigPin7, echoPin7);
  sendMessage(7, distance);

  // Prints the distance on the Serial Monitor
  Serial.print("Distance (cm): 0: ");
  for (int i = 0; i < sensors; i++) {
    Serial.print(i);
    Serial.print("-");
    Serial.print(distancesCm[i]);
    Serial.print(" : ");
  }
  Serial.println();
}


int setDistance(int sensor, int trigPin, int echoPin) {
   Serial.println(sensor);
  digitalWrite(trigPin, LOW);
  delayMicroseconds(1);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  int duration = pulseIn(echoPin, HIGH,100000);
  durations[sensor] = duration;
  // Calculate the distance
  distancesCm[sensor] = duration * SOUND_VELOCITY / 2;
  return distancesCm[sensor];
}
