#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
// tp4056
#define GPIO0 0
#define GPIO2 2

#define DHTTYPE DHT11

// Serial.println("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=");

// Variáveis para acessar wi-fi

// WIFI Casa
const char* ssid = "Virus_cavalo_de_troia";
const char* password = "putygrillputygrill280642";

// Hotspot smartphone
const char* ssidHot = "Oie";
const char* passwordHot = "qqij8mh5shns6c5";


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
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String topicString = topic;
  String msgcallback = "";
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    msgcallback += (char)payload[i];
  }
  Serial.println();
  Serial.print("Mensagem: ");
  Serial.println(msgcallback);
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
  // INFO: the data must be converted into a string; a problem occurs when using floats...
  root["temperature_trab_ubi"] = (String)p_temperature;
  root["humidity_trab_ubi"] = (String)p_humidity;
  root.prettyPrintTo(Serial);
  Serial.println("");
  /*
     {
        "temperature": "23.20" ,
        "humidity": "43.70"
     }
  */
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
    Serial.print("Aguardando conexão MQTT...");
    String clientID = "ESP-01Client-";
    clientID += String(random(0xffff, HEX));
    if (client.connect(clientID.c_str())) {                               
      Serial.println("conectado");
      Serial.print("Client ID: ");
      Serial.println(clientID);
      Serial.println("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=");                                       
      client.publish("pub_test_ubi", "Connected");    
      client.subscribe("sub_test_ubi");                         
    } else {                                                             
      Serial.print("falhou, rc=");                                       
      Serial.print(client.state());                                      
      Serial.println(" tente novamente em 5s");                          
      delay(5000);                                                       
    }
  }
}

// WiFi Management

int connectWifi(int cred)
{
  WiFi.mode(WIFI_STA);
  switch (cred)
  {
    case 0:
      Serial.println(ssid);
      WiFi.begin(ssid, password);
      break;
    case 1:
      Serial.println(ssidHot);
      WiFi.begin(ssidHot, passwordHot);
      break;
  }
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    return 0;
    delay(1000);
  }
  return 1;
}

void setup() {
  Serial.begin(115200);
  delay(50);
  dht.begin();
  delay(2000);
  Serial.println("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=");
  Serial.println("Iniciando...");

  int i = 0;
  while( i < 10 ) // Tenta conectar as 2 redes descritas anteriormente 5 vezes cada
  {
    Serial.println("Tentando conexao a rede");
    if(connectWifi( i % 2 ))
    {
      Serial.println("Conectado!");
      Serial.println("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=");
      break;
    }
    else
    {
      Serial.println("Conexao falhou!");
    }
    i++;
  }
  if( i >= 10 )
  {
    Serial.println("Falha ao tentar conexão WiFi, reiniciando ESP!");
    delay(5000);
    ESP.restart();
  }

  // Porta padrao do ESP8266 para OTA eh 8266 - Voce pode mudar ser quiser, mas deixe indicado!
  // ArduinoOTA.setPort(8266);
 
  // O Hostname padrao eh esp8266-[ChipID], mas voce pode mudar com essa funcao
  // ArduinoOTA.setHostname("nome_do_meu_esp8266");
 
  // Nenhuma senha eh pedida, mas voce pode dar mais seguranca pedindo uma senha pra gravar
  // ArduinoOTA.setPassword((const char *)"123");
 
  ArduinoOTA.onStart([]() {
    Serial.println("Inicio...");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("nFim!");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progresso: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Erro [%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Autenticacao Falhou");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Falha no Inicio");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Falha na Conexao");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Falha na Recepcao");
    else if (error == OTA_END_ERROR) Serial.println("Falha no Fim");
  });
  ArduinoOTA.begin();
  Serial.println("Pronto");
  Serial.print("Endereco IP: ");
  Serial.println(WiFi.localIP());
  
  // FIM DAS CONFIGURACOES DO OTA

  randomSeed(micros());

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqtt_callback);

}
 
void loop() {
  ArduinoOTA.handle(); // NAO RETIRAR ESTA LINHA!!!
  
  if (!client.connected()) {                                             
    reconnectmqtt();
  }
  client.loop();
  unsigned long currentMillis = millis();
  
//  Serial.println("Lendo temperatura...");
  if(currentMillis - previousMillis >= interval){
    previousMillis = currentMillis;
    float t = dht.readTemperature();
    float h = dht.readHumidity();
  
    if(isnan(t)){
//    Serial.println("Erro ao ler temperatura");
      client.publish("temp_pub_ubiquos","ERROR!");
    }
    else{
      client.publish("debug_pub_ubi","TEMP");
      temp = t;
      Serial.print("Temperatura: ");
      Serial.print(temp);
      Serial.println("ºC");
      snprintf(msg, MSG_BUFFER_SIZE, "%.2fºC", temp);
      // client.publish("trab_ubi/sensorUbiquosTemp",msg);
      publishData(temp, humi);
      delay(10);
    }
  
//    Serial.println("Lendo umidade...");
    if(isnan(h)){
//    Serial.println("Erro ao ler umidade");
    }
    else{
      client.publish("debug_pub_ubi","Humi");
      humi = h;
      Serial.print("Umidade: ");
      Serial.print(humi);
      Serial.println("%");
      snprintf(msg, MSG_BUFFER_SIZE, "%.2f%%", humi);
      client.publish("humi_pub_ubiquos",msg);
      delay(10);
    }
  }
  
}
