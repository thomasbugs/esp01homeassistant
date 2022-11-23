#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>

#define GPIO0 0
#define GPIO2 2

#define DHTTYPE DHT11

// Variáveis para acessar wi-fi

// Access Point
const char* ssidAP = "ESP8266-DHT11-AP";
const char* passwordAP = "dht11appasswordesp8266";

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

#define MQTT_SENSOR_TOPIC "trab_ubi/sensor"

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
  if(topicString.equals("sub_test_ubi")){
      if(msgcallback.equals("1"))
        client.publish("pub_test_ubi", "Oie");
      else if(msgcallback.equals("0"))
        client.publish("pub_test_ubi", "Tchau");
  }
  
}

// function called to publish the temperature and the humidity
void publishData(float p_temperature, float p_humidity) {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["temperature_trab_ubi"] = (String)p_temperature;
  root["humidity_trab_ubi"] = (String)p_humidity;
  char data[200];
  root.printTo(data, root.measureLength() + 1);
  client.publish(MQTT_SENSOR_TOPIC, data, true);
  yield();
}

// Variáveis para o sensor de temperatura
DHT dht(GPIO2, DHTTYPE);
const long interval = 10000;
unsigned long previousMillis = 0;
float humi = 0.0;
float temp = 0.0;

// mqtt Reconnect
void reconnectmqtt() {                                                       
  while (!client.connected()) {                  
    String clientID = "ESP-01Client-";
    clientID += String(random(0xffff, HEX));
    if (client.connect(clientID.c_str())) {                                      
      client.publish("pub_test_ubi", "Connected");    
      client.subscribe("sub_test_ubi");                         
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
  delay(50);
  dht.begin();
  delay(2000);

  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ssidAP, passwordAP);
  delay(100);

  server.on("/", handle_OnConnect);
  server.on("/conectar", handle_Conectar);

  server.begin();

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

  unsigned long currentMillis = millis();
  
  if(currentMillis - previousMillis >= interval){
    previousMillis = currentMillis;
    float t = dht.readTemperature();
    float h = dht.readHumidity();
  
    if(!isnan(t)){
      temp = t;
      snprintf(msg, MSG_BUFFER_SIZE, "%.2fºC", temp);
      publishData(temp, humi);
      delay(10);
    }
  
    if(!isnan(h)){
      humi = h;
      snprintf(msg, MSG_BUFFER_SIZE, "%.2f%%", humi);
      client.publish("humi_pub_ubiquos",msg);
      delay(10);
    }
  }
  
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
 htmlweb += "<title>ESP8266 Temperature Sensor Configuration</title>\n";
 
 htmlweb += "<style>\n";
 htmlweb += "html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
 htmlweb += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
 htmlweb += ".button {display: block;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 20px;margin: 35px auto 35px;cursor: pointer;border-radius: 4px;}\n";
 htmlweb += "p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
 htmlweb += "</style>\n";
 
 htmlweb += "</head>\n";

 // Body
 htmlweb += "<body>\n";
 
 htmlweb += "<h1>Configuração de rede ESP8266-01</h1>\n";

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