// Source : http://www.iotsharing.com/2017/05/tcp-udp-ip-with-esp32.html
// Source : https://www.dfrobot.com/blog-948.html

#include <Arduino.h>
#include "secrets.h"
#include <WiFi.h>
#include <PubSubClient.h>

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

//#define  triggerPin D5;   // change as desired
//#define ECHO_PIN  D6;  // change as desired (use a level-shift on this pin!)
const unsigned long MAX_ECHO_TIME_US = 30000UL; // 30ms => ~517cm (beyond sensor spec but safe)
// MQTT Settings
const char *mqtt_server = "maisonneuve.aws.thinger.io";  // Your broker hostname
const int mqtt_port = 1883;

// Create WiFi and MQTT client instances
WiFiClient wifiClient;
PubSubClient client(wifiClient);

void callback(char* topic, byte* payload, unsigned int length) {
  // Print received message details
  Serial.print("Topic: "); Serial.println(topic);
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  Serial.print("Payload: "); Serial.println(msg);
}
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("MQTT msg on [");
  Serial.print(topic);
  Serial.print("]: ");
  for (unsigned int i = 0; i < length; i++) Serial.write(payload[i]);
  Serial.println();

  if (String(topic) == "capter") {
    if (length == 1 && payload[0] == '1') {
      Serial.println("Turning LED ON");
      digitalWrite(LED_PIN, HIGH); // set LOW to turn off
    } else {
      Serial.println("Turning LED OFF");
      digitalWrite(LED_PIN, LOW); // set LOW to turn off
    }
  }
}
// Helper function to convert MQTT state codes to readable text
const char* mqttStateToString(int8_t state) {
  switch (state) {
    case -4: return "MQTT_CONNECTION_TIMEOUT";
    case -3: return "MQTT_CONNECTION_LOST";
    case -2: return "MQTT_CONNECT_FAILED";
    case -1: return "MQTT_DISCONNECTED";
    case 0: return "MQTT_CONNECTED";
    case 1: return "MQTT_CONNECT_BAD_PROTOCOL";
    case 2: return "MQTT_CONNECT_BAD_CLIENT_ID";
    case 3: return "MQTT_CONNECT_UNAVAILABLE";
    case 4: return "MQTT_CONNECT_BAD_CREDENTIALS";
    case 5: return "MQTT_CONNECT_UNAUTHORIZED";
    default: return "MQTT_UNKNOWN";
  }
}

String translateEncryptionType(wifi_auth_mode_t encryptionType)
{

  switch (encryptionType)
  {
  case (WIFI_AUTH_OPEN):
    return "Open";
  case (WIFI_AUTH_WEP):
    return "WEP";
  case (WIFI_AUTH_WPA_PSK):
    return "WPA_PSK";
  case (WIFI_AUTH_WPA2_PSK):
    return "WPA2_PSK";
  case (WIFI_AUTH_WPA_WPA2_PSK):
    return "WPA_WPA2_PSK";
  case (WIFI_AUTH_WPA2_ENTERPRISE):
    return "WPA2_ENTERPRISE";
  default:  
    return "UNKNOWN";
  }
  return "UNKNOWN";
}



void connectToNetwork()
{
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Establishing connection to WiFi..");
  }

  Serial.println("Connected to network");
}

void reconnect() {
  int retries = 0;
  while (!client.connected() && retries < 3) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "esp2Ow";
    clientId += String((uint32_t)ESP.getEfuseMac(), HEX);
    
    // Attempt to connect with credentials
    if (client.connect("esp2Ow", "owen2", "Certif24@")) {
      Serial.println("connected");
      // Once connected, subscribe to topics
      client.subscribe("inTopic");
      // Publish a message to show we're connected
     // client.publish("outTopic2", "ESP32 Connected!");
    } else {
      int state = client.state();
      Serial.print("failed, rc=");
      Serial.print(state);
      Serial.print(" (");
      Serial.print(mqttStateToString(state));
      Serial.println(")");
      Serial.println("Retrying in 5 seconds...");
      retries++;
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(5, OUTPUT);
  pinMode(6, INPUT);
  digitalWrite(5, LOW);
  delay(100);
  Serial.println("HC-SR04 test starting");
  Serial.println("\nStarting ESP32 MQTT Client");
  
  // Set up WiFi
  WiFi.mode(WIFI_STA);
  
  connectToNetwork();
  
  Serial.print("Connected to WiFi, IP: ");
  Serial.println(WiFi.localIP());
  
  // Configure MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  // Initial connection attempt
  reconnect();
}
long readUltrasonic_cm() {
  // Ensure trigger low
  digitalWrite(5, LOW);
  delayMicroseconds(2);

  // Send 10us pulse
  digitalWrite(5, HIGH);
  delayMicroseconds(10);
  digitalWrite(5, LOW);

  // Read echo pulse width (microseconds). timeout to avoid lock.
  unsigned long duration = pulseIn(6, HIGH, MAX_ECHO_TIME_US);
  if (duration == 0) {
    // timeout / no echo
    return -1;
  }

  // Convert to distance (cm): speed of sound ~343 m/s
  long distance_cm = (long)(duration / 58.0);
  return distance_cm;
}

void loop() {
  // Check WiFi connection first
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost, reconnecting...");
    connectToNetwork();
  }
  
  // Check MQTT connection
  if (!client.connected()) {
    reconnect();
  }
  
  // Process MQTT messages and maintain connection
  client.loop();
  long dist = readUltrasonic_cm();
char payload[64];
if (dist < 0) {
  snprintf(payload, sizeof(payload), "{\"distance_cm\":null,\"note\":\"pas_echo\"}");
} else {
  snprintf(payload, sizeof(payload), "{\"distance_cm\":%ld}", dist);
}
client.publish("outTopic2", payload);
   delay(1000);
  // Optional: Publish status message every 30 seconds
  static unsigned long lastMsg = 0;
  /*if (millis() - lastMsg > 30000) {
    lastMsg = millis();
    String status = "ESP32 Uptime: " + String(millis()/1000) + "s";
    client.publish("outTopic2", status.c_str());
  }*/
  
  // Small delay to prevent tight looping
  delay(10);
}