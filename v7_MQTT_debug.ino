/**********************************************************************************
 MQTT envio de dados para ThingSpeak v1.0 02 DEZ 2018
 Termometro DTH11_DS18b20
 Arduino UNO EthernetShield WatchDog 8 segundos
 Mostra o endereço IP que foi obtido por DHCP na rede
 Limitacao da versao
 ID Canal e Chave Escrita devem ser preenchidos 2 x 
 No final do sketch Client.publish( "channels/IDCANAL/publish/CHAVE", msgBuffer);
 Quando define topicString ocasiona bug conteudo msgBuffer = payload da funcao grava
 ***********************************************************************************/

#include <SPI.h>
#include <PubSubClient.h>
#include <Ethernet.h>
#include "DHT.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <avr/wdt.h>  // inclui a função watchdog

// **** Sensor de Temperatura DHT11 Initialize DHT sensor ****
#define DHTPin  6
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPin, DHTTYPE);

// ***** Porta do pino de sinal do DS18B20 *****
#define ONE_WIRE_BUS 3
// Define uma instancia do oneWire para comunicacao com o sensor
OneWire oneWire(ONE_WIRE_BUS);

// Armazena temperaturas minima e maxima
float tempMin = 999;
float tempMax = 0;

DallasTemperature sensors(&oneWire);
DeviceAddress sensor1;
// ***** FIM incializacao DS18B20 *****


char mqttUserName[] = "NXTMQTT";  // Pode ser qualquer nome 
char mqttPass[] = "IJ7JNSGEHSKSMV70";  // Coloque aqui a MQTT API Key disponivel em Account > MyProfile
char writeAPIKey[] = "NVRTXD5XGGNR342Y";    // Chave de Escrita Write API Key.
long channelID = 641112; // ID do Canal

static const char alphanum[] ="0123456789"
                              "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                              "abcdefghijklmnopqrstuvwxyz";  // Gera randomico client ID.

// Servidor MQTT
//const char* server = "34.236.123.240"; 
const char* server = "mqtt.thingspeak.com"; 

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
EthernetClient ethClient;
PubSubClient Client(server, 1883, ethClient); // Initialize the PuBSubClient library.


// *************** TEMPO PARA PUBLICACAO ******************************
// ***  TEMPO EM S seguido de L 20L = 20 segundos L300 = 5 minutos  ***
//*********************************************************************
unsigned long lastConnectionTime = 0; 
const unsigned long postingInterval = 300L * 1000L; // Publica dados a cada XL seconds.

void setup() {

 Serial.begin(115200);
 wdt_enable(WDTO_8S);  // habilita o watchdog
 
 Ethernet.begin(mac);
        Serial.println("");
        Serial.println("INICIALIZACAO REDE CONCLUIDA OK");
        Serial.println("OBTENDO ENDERECO IP");
        Serial.print("Conectado na rede ");
        Serial.print("IP: ");
        Serial.println(Ethernet.localIP());

  // *** Sensor DSB ***
  sensors.begin();
  // Localiza e mostra enderecos dos sensores
  Serial.println("Localizando sensores DS18B20...");
  Serial.print("Foram encontrados ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" sensores.");
  if (!sensors.getAddress(sensor1, 0)) 
     Serial.println("Sensores DS18B20 nao encontrados !"); 
  // Mostra o endereco do sensor encontrado no barramento
  // Serial.print("Endereco sensor: ");
  // mostra_endereco_sensor(sensor1);
  Serial.println();
        
   
Client.setServer(server, 1883);   // Configura parametros do MQTT broker . 
}


void loop() {

   wdt_reset();  //  reseta o watchdog

// Reconnecta  se MQTT client is not connected.
  if (!Client.connected()) 
  {
    Serial.print("Conectando servidor MQTT ");
    Serial.println(server);
    
    reconnect();
  }

// *** Sensor DHT11 ***
  float t = dht.readTemperature(); // Read temperature from DHT sensor.
  int h = dht.readHumidity();  // Read humidity from DHT sensor.

  Serial.println();
  Serial.println("*** Sensor DHT ***"); 
  Serial.print("Temp DHT: ");
  Serial.println(t);
  Serial.print("Umi% DHT: ");
  Serial.println(h);
  
  // ***** Sensor DS18b20 *********
  sensors.requestTemperatures();
  int tempC = sensors.getTempC(sensor1);
  //Atualiza temperaturas minima e maxima
  if (tempC < tempMin)
  {
    tempMin = tempC;
  }
  if (tempC > tempMax)
  {
    tempMax = tempC;
  }
  // Mostra dados DSB18b20 no serial monitor */
  Serial.println("*** Sensor DSB ***: ");
  Serial.print("Temp DSB: ");
  Serial.println(tempC);
  Serial.println();
  
// ****** Fim dados do sensor DS18b20 *********/
  
  Client.loop();   //  Call the loop continuously to establish connection to the server.

  

  // Se o intervalo ultrapassa o tempo desde a ultima conexao, publica novamente no ThingSpeak
  if (millis() - lastConnectionTime > postingInterval) 
  {
    mqttpublish();
  }


  
}

//*********** RECONECT ***************

void reconnect() 
{
  char clientID[9];

  // Loop until reconnected.
  while (!Client.connected()) 
  {
    Serial.print("Realizando MQTT connection...");
    // Generate ClientID
    for (int i = 0; i < 8; i++) {
        clientID[i] = alphanum[random(51)];
    }
    clientID[8]='\0';
    // Connect to the MQTT broker
    if (Client.connect(clientID,mqttUserName,mqttPass)) 
    {
      Serial.print("Conectado com Client ID:  ");
      Serial.print(String(clientID));
      Serial.print(", Username: ");
      Serial.print(mqttUserName);
      Serial.print(" , Password: ");
      Serial.println(mqttPass);
    } else 
    {
      Serial.print("falha de conexao MQTT, rc=");
      // Print to know why the connection failed.
      // See https://pubsubclient.knolleary.net/api.html#state for the failure code explanation.
      Serial.print(Client.state());
      Serial.println(" tenta novamente em 5 segundos");
      delay(5000);
    }
  }
}


//*********** MQTTPUBLISH ***************


void mqttpublish() {

  // *** Sensor DHT11 ***
  int t = dht.readTemperature(); // Read temperature from DHT sensor.
  int h = dht.readHumidity();  // Read humidity from DHT sensor.

  Serial.println();
  Serial.println("*** Sensor DHT ***"); 
  Serial.print("Temp DHT: ");
  Serial.println(t);
  Serial.print("Umi% DHT: ");
  Serial.println(h);
  
  // ***** Sensor DS18b20 *********
  sensors.requestTemperatures();
  int tempC = sensors.getTempC(sensor1);
  //Atualiza temperaturas minima e maxima
  if (tempC < tempMin)
  {
    tempMin = tempC;
  }
  if (tempC > tempMax)
  {
    tempMax = tempC;
  }
  // Mostra dados DSB18b20 no serial monitor */
  Serial.println("*** Sensor DSB ***: ");
  Serial.print("Temp DSB: ");
  Serial.println(tempC);
  Serial.println();
  
  /*Serial.print(" Min : ");
  Serial.print(tempMin);
  Serial.print(" Max : ");
  Serial.println(tempMax);
  Serial.println();

// ****** Fim dados do sensor DS18b20 *********/
  
     
        // Cria a data string para enviar ao ThingSpeak campo PAYLOAD da publicacao Client.publish( TOPIC, PAYLOAD);
        //String data = String("field1=" + String(t, DEC) + "&field2=" + String(h, DEC) + "&field3=" + String(tempC, DEC) + "&field4=" + String(tempMin, DEC) + "&field5=" + String(tempMax, DEC));
        String data = String("field1=" + String(t, DEC) + "&field2=" + String(h, DEC) + "&field3=" + String(tempC, DEC) + "&field4=" + String(tempMin, DEC) + "&field5=" + String(tempMax, DEC));
        int dlength = data.length();
        char msgBuffer[dlength];
        data.toCharArray(msgBuffer, dlength + 1);
          
        //Serial.print("msgBuffer 0 = ");
        //Serial.println(msgBuffer);
 
        // Create a topic string and publish data to ThingSpeak channel feed.
        //String topicString = "channels/" + String( channelID ) + "/publish/" + String(writeAPIKey);
        
        //length = topicString.length();
        //char topicBuffer[length];
        //topicString.toCharArray(topicBuffer, length + 1);
        //Serial.print("Publish Result: ");
        //Serial.print("topicBuffer");
        //Serial.print(" , ");
        Serial.println("*** Buffer ***");
        Serial.print("msgBuffer 1 = ");
        Serial.println(msgBuffer);
        
        // ****************** THINGSPEAK ID CANAL e CHAVE ESCRITA ********************
        // ***  Client.publish( "channels/IDCANAL/publish/CAHVE", msgBuffer);      ***
        // ***************************************************************************
        
        Client.publish( "channels/641112/publish/NVRTXD5XGGNR342Y", msgBuffer);
        Serial.println("Dados publicados no MQTT server");
        Serial.println();
  
  lastConnectionTime = millis();
}
 

  


  
  


 

 



   
   
