#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>

#define LED_VERDE 2
#define RESET_BUTTON 13 //5
#define LONG_RESET_TIME 6000
#define SHORT_RESET_TIME 2000
#define TOMADA_1 12 //4
#define TOMADA_2 14 

int P1Status = 0;
int P2Status = 0;

// Variáveis para acessar wi-fi

// Access Point
const char* ssidAP = "ESP8266-PLUG-AP";
const char* passwordAP = "plugappasswordesp12E";

// WiFi
String ssid;
String password;

bool wificonnect = false;

IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

ESP8266WebServer server(80);

// Variáveis MQTT
const char* mqtt_server = "broker.mqtt-dashboard.com";
const int mqtt_port = 1883;

#define MQTT_SENSOR_TOPIC "trab_ubi/plug"

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastmsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void mqtt_callback(char* topic, byte* payload, unsigned int length){
  String topicString = topic;
  String msgcallback = "";
  for (int i = 0; i < length; i++) {
    msgcallback += (char)payload[i];
  }
  if(topicString.equals("trab_ubi/sub_plug1")){
    if(msgcallback.equals("ON")){
      digitalWrite(TOMADA_1, HIGH);
      P1Status = 1;
    }
    else if(msgcallback.equals("OFF")){
      digitalWrite(TOMADA_1, LOW);
      P1Status = 0;
    }
    client.publish("trab_ubi/pub_plug1_status", P1Status ? "ON" : "OFF");
    publishData();
  }
  else if(topicString.equals("trab_ubi/sub_plug2")){
    if(msgcallback.equals("ON")){
      digitalWrite(TOMADA_2, HIGH);
      P2Status = 1;
    }
    else if(msgcallback.equals("OFF")){
      digitalWrite(TOMADA_2, LOW);
      P2Status = 0;
    }
    client.publish("trab_ubi/pub_plug2_status", P2Status ? "ON" : "OFF");
    publishData();
  }
  
}

void publishData() {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["plug_trab_ubi1"] = P1Status ? "ON" : "OFF";
  root["plug_trab_ubi2"] = P2Status ? "ON" : "OFF";
  char data[200];
  root.printTo(data, root.measureLength() + 1);
  client.publish(MQTT_SENSOR_TOPIC, data, true);
  yield();
}

// mqtt Reconnect
void reconnectmqtt() {                                                       
  while (!client.connected()) {
    String clientID = "ESP-12Client-";
    clientID += String(random(0xffff, HEX));
    if (client.connect(clientID.c_str())) {                                      
      client.publish("pub_plug_connect_ubi", "Connected");    
      client.subscribe("sub_test_ubi");                
      client.subscribe("trab_ubi/sub_plug1");  
      client.subscribe("trab_ubi/sub_plug2");           
    } else {                         
      delay(5000);                                                       
    }
  }
}

// WiFi Management

int connectWifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    return 0;
    delay(1000);
  }  
  return 1;
}

void setup() {

  pinMode(RESET_BUTTON, INPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(TOMADA_1, OUTPUT);
  pinMode(TOMADA_2, OUTPUT);

  digitalWrite(TOMADA_1, LOW);
  digitalWrite(TOMADA_2, LOW);

  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ssidAP, passwordAP);
  delay(100);

  server.on("/", handle_OnConnect);
  server.on("/conectar", handle_Conectar);

  server.begin();
  
  // FIM DAS CONFIGURACOES DO OTA

  digitalWrite(LED_VERDE, HIGH);
  client.publish("trab_ubi/pub_plug1_status", P1Status ? "ON" : "OFF");
  client.publish("trab_ubi/pub_plug2_status", P2Status ? "ON" : "OFF");

}
 
void loop() {
  ArduinoOTA.handle(); // NAO RETIRAR ESTA LINHA!!!

  if(!wificonnect){
    server.handleClient();
    return;
  }
  
  if (!client.connected()) {                                             
    reconnectmqtt();
  }
  client.loop();
  
  // Main code
  
  

}

void handle_OnConnect() {

  server.send(200, "text/html", HTMLWebServer());
  
}

void handle_Conectar() {

  ssid = server.arg("network");
  password = server.arg("wifipassword");
  delay(500);

  if(connectWifi())
  { 

    wificonnect = true;
    
    randomSeed(micros());

    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(mqtt_callback);

    ArduinoOTA.begin();

    publishData();
  }
  else{
    ESP.restart();
  }
}

String HTMLWebServer() {

 String htmlweb = "<!DOCTYPE html> <html>\n";

 // Head
 htmlweb += "<head>\n";
 
 htmlweb += "<meta charset=\"UTF-8\">\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
 htmlweb += "<title>ESP-12E Smart Plug Configuration</title>\n";
 
 htmlweb += "<style>\n";
 htmlweb += "html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
 htmlweb += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
 htmlweb += ".button {display: block;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 20px;margin: 35px auto 35px;cursor: pointer;border-radius: 4px;}\n";
 htmlweb += "p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
 htmlweb += "</style>\n";
 
 htmlweb += "</head>\n";

 // Body
 htmlweb += "<body>\n";
 
 htmlweb += "<h1>Configuração de rede ESP-12E</h1>\n";

 htmlweb += "<form action=\"/conectar\">\n";
 
 htmlweb += "<p>Rede WiFi</p>\n";
 htmlweb += "<select name=\"network\" required>\n";
 htmlweb += "<option value=\"\" selected>Selecione</option>\n";
 int NNetworks = WiFi.scanNetworks();
 for(int it = 0 ; it < NNetworks ; it++)
 {
  htmlweb += "<option value=\"" + WiFi.SSID(it) + "\">" + WiFi.SSID(it) + "</option>";
 }
 htmlweb += "</select>\n";

 htmlweb += "<p>Senha</p>\n";

 htmlweb += "<input name=\"wifipassword\" type=\"password\" value>\n";

 htmlweb += "<input class=\"button\" type=\"submit\" value=\"Conectar\">\n";

 htmlweb += "</form>\n";
 
 htmlweb += "</body>\n";

 htmlweb += "</html>\n";

 return htmlweb;
 
}