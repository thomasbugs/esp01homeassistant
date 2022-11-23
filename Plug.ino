#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define LED_VERDE 2
#define RESET_BUTTON 13 //5
#define LONG_RESET_TIME 6000
#define SHORT_RESET_TIME 2000
#define TOMADA_1 12 //4
#define TOMADA_2 14 

int P1Status = 0;
int P2Status = 0;

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
  if(topicString.equals("trab_ubi/sub_plug1")){
    if(msgcallback.equals("1")){
      digitalWrite(TOMADA_1, HIGH);
      P1Status = 1;
    }
    else if(msgcallback.equals("0")){
      digitalWrite(TOMADA_1, LOW);
      P1Status = 0;
    }
    client.publish("trab_ubi/pub_plug1_status", P1Status ? "ON" : "OFF");
  }
  else if(topicString.equals("trab_ubi/sub_plug2")){
    if(msgcallback.equals("1")){
      digitalWrite(TOMADA_2, HIGH);
      P2Status = 1;
    }
    else if(msgcallback.equals("0")){
      digitalWrite(TOMADA_2, LOW);
      P2Status = 0;
    }
    client.publish("trab_ubi/pub_plug2_status", P2Status ? "ON" : "OFF");
  }
  
}

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
      client.publish("pub_plug_connect_ubi", "Connected");    
      client.subscribe("sub_test_ubi");                
      client.subscribe("trab_ubi/sub_plug1");  
      client.subscribe("trab_ubi/sub_plug2");           
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

  pinMode(RESET_BUTTON, INPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(TOMADA_1, OUTPUT);
  pinMode(TOMADA_2, OUTPUT);

  digitalWrite(TOMADA_1, LOW);
  digitalWrite(TOMADA_2, LOW);

  Serial.begin(115200);
  delay(50);
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

  digitalWrite(LED_VERDE, HIGH);
  client.publish("trab_ubi/pub_plug1_status", P1Status ? "ON" : "OFF");
  client.publish("trab_ubi/pub_plug2_status", P2Status ? "ON" : "OFF");

}
 
void loop() {
  ArduinoOTA.handle(); // NAO RETIRAR ESTA LINHA!!!
  
  if (!client.connected()) {                                             
    reconnectmqtt();
  }
  client.loop();
  
  // Main code
  
  

}