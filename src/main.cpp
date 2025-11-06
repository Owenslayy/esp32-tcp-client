// Source : http://www.iotsharing.com/2017/05/tcp-udp-ip-with-esp32.html
// Source : https://www.dfrobot.com/blog-948.html

#include <Arduino.h>
#include "secrets.h"
#include "WiFi.h"
#include <PubSubClient.h>

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;
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

void scanNetworks()
{

  int numberOfNetworks = WiFi.scanNetworks();

  // Add delay so the terminal can catch up
  delay(3000);

  Serial.print("Number of networks found: ");
  Serial.println(numberOfNetworks);

  for (int i = 0; i < numberOfNetworks; i++)
  {

    Serial.print("Network name: ");
    Serial.println(WiFi.SSID(i));

    Serial.print("Signal strength: ");
    Serial.println(WiFi.RSSI(i));

    Serial.print("MAC address: ");
    Serial.println(WiFi.BSSIDstr(i));

    Serial.print("Encryption type: ");
    String encryptionTypeDescription = translateEncryptionType(WiFi.encryptionType(i));
    Serial.println(encryptionTypeDescription);
    Serial.println("-----------------------");
  }
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

/*void sendMessageToTCPServer() {
  Serial.println("Send TCP message to server");
  WiFiClient client;
  if (!client.connect(host, port))
  {
    Serial.println("connection failed");
    return;
  }
  client.print("My name is ESP32-Teacher");
  client.stop();
}*/

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
      client.publish("outTopic2", "ESP32 Connected!");
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
  Serial.println("\nStarting ESP32 MQTT Client");
  
  // Set up WiFi
  WiFi.mode(WIFI_STA);
  scanNetworks();
  connectToNetwork();
  
  Serial.print("Connected to WiFi, IP: ");
  Serial.println(WiFi.localIP());
  
  // Configure MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  // Initial connection attempt
  reconnect();
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
  
  // Optional: Publish status message every 30 seconds
  static unsigned long lastMsg = 0;
  if (millis() - lastMsg > 30000) {
    lastMsg = millis();
    String status = "ESP32 Uptime: " + String(millis()/1000) + "s";
    client.publish("outTopic2", status.c_str());
  }
  
  // Small delay to prevent tight looping
  delay(10);
}