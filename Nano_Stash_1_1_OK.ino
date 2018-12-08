// Handle returning code and reset Ethernet module if needed
// 2013-10-22 hneiraf@gmail.com

// Modifing so that it works on my setup for www.thingspeak.com.
// Arduino uno 3, ETH modul on SPI with CS on pin 8.
// Also added a few changes found on various forums.
// 2015-11-09 dani.lomajhenic@gmail.com

// Renamed original <res> var into <counts>, to make the code more readable
// 2016-03-11 marc@6foot7.nl

#include <EtherCard.h>

// change these settings to match your own setup
#define APIKEY "01DTIBWT1NQLC2OU" // put your key here 
//#define ethCSpin 8 // put your CS/SS pin here.

// Ethernet interface mac address, must be unique on the LAN
static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };
const char website[] PROGMEM = "api.thingspeak.com";
byte Ethernet::buffer[700];
uint32_t timer;
Stash stash;
byte session;

//timing variable
int counts = 50;


// ********** SETUP ********** 
void setup () {
 Serial.begin(115200);
 Serial.println("\n[ThingSpeak transmissao por Stash NANO ETH SHIELD]");

 //Initialize Ethernet
 initialize_ethernet();
}

// ********** ROTINA LOOP **********
void loop () {
 //if correct answer is not received then re-initialize Ethernet module, so in this case it is (110-100)*200ms = 2 sec for the reply to happen.
   if (counts > 110) {
   initialize_ethernet(); 
  }
 
 counts = counts + 1;
 ether.packetLoop(ether.packetReceive());
 //100 counts = 20 segundos (200ms cada count)

 if (counts == 100) {

  // generate random value for field1 as payload - by using a separate stash,
  // we can determine the size of the generated  message ahead of time
  
  Serial.println("Gerando Random");
  
  byte sd = stash.create();
  stash.print("field1=");
  stash.print(random(100));
  stash.save();

  // generate the header with payload - note that the stash size is used,
  // and that a "stash descriptor" is passed in as argument using "$H"
 Stash::prepare(PSTR("POST /update HTTP/1.0" "\r\n"
  "Host: $F" "\r\n"
  "Connection: close" "\r\n"
  "X-THINGSPEAKAPIKEY: $F" "\r\n"
  "Content-Type: application/x-www-form-urlencoded" "\r\n"
  "Content-Length: $D" "\r\n"
  "\r\n"
  "$H"),
  website, PSTR(APIKEY), stash.size(), sd);

  Serial.print("millis ");
  Serial.println(millis());

  //Serial.print("lastConnectionTime ");
 // Serial.print(lastConnectionTime);
  
  // Se o intervalo ultrapassa o tempo desde a ultima conexao, publica novamente no ThingSpeak
 // if (millis() - lastConnectionTime > postingInterval) 
//  {

  // envia o pacote - isso também libera todo o buffer do stash uma vez processado
  session = ether.tcpSend(); 
  Serial.println("Pacote Enviado");
  Serial.println(" ");
   
  //}
  
//  lastConnectionTime = millis();

  //Serial.print("lastConnectionTime ");
  //Serial.print(lastConnectionTime);

  // added from: http://jeelabs.net/boards/7/topics/2241
  int freeCount = stash.freeCount();
  if (freeCount <= 3) { Stash::initMap(56); } 
 }
 
 const char* reply = ether.tcpReply(session);
 
 if (reply != 0) {
  counts = 0;
  Serial.println(" ");
  Serial.println("************************* ");
  Serial.println(F(" >>>RESPOSTA recebida...."));
  Serial.println(reply);
 }
 delay(200); //milliseconds per count
}

// ********** ROTINA INICALIZA ETHERNET **********
void initialize_ethernet(void){ 
 for(;;){ // keep trying until you succeed 
  //Reinitialize Ethernet module
  if (ether.begin(sizeof Ethernet::buffer, mymac, SS) == 0){ 
   Serial.println( "Failed to access Ethernet controller");
   continue;
  }
 
  if (!ether.dhcpSetup()){
   Serial.println("DHCP failed");
   continue;
  }

  ether.printIp("IP: ", ether.myip);
  ether.printIp("GW: ", ether.gwip); 
  ether.printIp("DNS: ", ether.dnsip); 

  if (!ether.dnsLookup(website))
   Serial.println("DNS failed");

  ether.printIp("SRV: ", ether.hisip);

  //reset init value
  counts = 90;
  break;
 }
}
