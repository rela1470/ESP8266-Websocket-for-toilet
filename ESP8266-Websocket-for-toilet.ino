/**
 * ESP8266-Websocket-for-toilet
 * 
 * require ESP8266-Websocket
 * https://github.com/morrissinger/ESP8266-Websocket
 * 
 * based on https://github.com/morrissinger/ESP8266-Websocket/blob/master/examples/WebSocketClient_Demo/WebSocketClient_Demo.ino
 */
#include <ESP8266WiFi.h>
#include <WebSocketClient.h>

//settings
//----------------------------------------------------------------------------
const int DOOR_NAME_MAX = 10;
const int DOOR_MAX      = 4;
char DOOR_NAME[DOOR_MAX][DOOR_NAME_MAX] = {"6F_man", "6F_woman", "7F_man", "7F_woman"};
int DOOR_PORT[DOOR_MAX] = {-1, -1, 14, 12}; //-1 empty
int LED_PORT[DOOR_MAX]  = {3, 4, 1, 5}; //-1 empty

char* WEBSOCKET_PATH = "/";
char* WEBSOCKET_HOST = "echo.websocket.org";

char* WIFI_SSID      = "your-sshd";
char* WIFI_PASSWORD  = "your-pass";
//----------------------------------------------------------------------------

char DOOR_STATUS_NAME[2][6] = {"full", "empty"};
int doorState[DOOR_MAX];
unsigned long doorTimer[DOOR_MAX];
WebSocketClient webSocketClient;

// Use WiFiClient class to create TCP connections
WiFiClient client;

void restart()
{
  Serial.println("Restart...");
  for (int i = 0; i < DOOR_MAX; i++) {
    if (LED_PORT[i] >= 0) digitalWrite(LED_PORT[i], HIGH);
    doorState[i] = doorTimer[i] = 0;
  }
  
  setup();
  
  for (int i = 0; i < DOOR_MAX; i++) {
    if (LED_PORT[i] >= 0) digitalWrite(LED_PORT[i], LOW);
  }
  Serial.println("Restart End!");
  return;
}

void setup() {
  Serial.begin(115200);
  delay(10);

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  delay(5000);
  
  // Connect to the websocket server
  if (client.connect(WEBSOCKET_HOST, 8484)) {
    Serial.println("Connected");
  }
  else {
    Serial.println("Connection failed.");
    return restart();
  }

  // Handshake with the server
  webSocketClient.path = WEBSOCKET_PATH;
  webSocketClient.host = WEBSOCKET_HOST;
  if (webSocketClient.handshake(client)) {
    Serial.println("Handshake successful");
  }
  else {
    Serial.println("Handshake failed.");
    return restart();
  }

  //Port Setup
  for (int i = 0; i < DOOR_MAX; i++) {
    if (DOOR_PORT[i] >= 0) pinMode(DOOR_PORT[i], INPUT_PULLUP);
    if (LED_PORT[i]  >= 0) pinMode(LED_PORT[i],  OUTPUT);
  }
}

int getMin(unsigned long now = 0)
{
  return int((millis() - now )/ 60000);
}

void loop() {
  String data;

  if (client.connected()) {

  //PORT UPDATE
  for (int i = 0; i < DOOR_MAX; i++) {
    if (DOOR_PORT[i] >= 0 && doorState[i] != digitalRead(DOOR_PORT[i])) {
      doorState[i] = digitalRead(DOOR_PORT[i]);
      doorTimer[i] = millis();
      if (LED_PORT[i] >= 0) digitalWrite(LED_PORT[i], !doorState[i]);
      Serial.print("PortUpdate:" + String(i) + "=" + String(doorState[i]) + ",");
      webSocketClient.sendData(String(DOOR_NAME[i]) + " " + String(DOOR_STATUS_NAME[doorState[i]]) + " " + String(getMin(doorTimer[i])));
    }
  }
    
    webSocketClient.getData(data);

    if (data.length() > 0) {
      Serial.print("Received data: ");
      Serial.println(data);

      if (data == "getState") {
        Serial.print("getStateCheck:");
        for (int i = 0; i < DOOR_MAX; i++) {
          if (DOOR_PORT[i] >= 0) {
            webSocketClient.sendData(String(DOOR_NAME[i]) + " " + String(DOOR_STATUS_NAME[doorState[i]]) + " " + String(getMin(doorTimer[i])));
          }
        }
      }
      else {
        //other Door Received
        for (int i = 0; i < DOOR_MAX; i++) {
          if (DOOR_PORT[i] == -1 && data.indexOf(DOOR_NAME[i]) > -1) {
            Serial.println(String(DOOR_NAME[i]) + "is Recieved!");
            if (LED_PORT[i] >= 0) digitalWrite(LED_PORT[i], data.indexOf(DOOR_STATUS_NAME[0]) > -1);
          }
        }
      }
    }
  }
  else {
    Serial.println("Client disconnected.");
    // Hang on disconnect.
    return restart();
  }
  
  // wait to fully let the client disconnect
  delay(3000);
}
