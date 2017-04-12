
#include "Libr_mtr.h"

/****************************************************************/
#define   DBG_OUTPUT_PORT Serial
#define   Pict_server 1
/****************************************************************/
//WiFiClient client; //TCP
#ifdef Pict_server
  ESP8266WebServer server(80);
  WiFiClient client = server.client();
#endif



IPAddress APip(192,168,0,100);
IPAddress APgateway(192,168,0,1);
IPAddress APmask(255,255,255,0);

IPAddress STAip(192,168,0,101);
IPAddress STAgateway(192,168,0,1);
IPAddress STAmask(255,255,255,0);

/************************Variables************************/
//SD
File myFile;
char buffi[2055]={0};
const int chipSelect = 4;
Sd2Card card;
SdVolume volume;
SdFile root;
bool hasSD = false;
int z =0;
char* nof="N100001.jpg";
//Variables
//int32_t rssi= 0;

// Handlers
ESP8266WiFiGenericClass LeeIP;
WiFiEventHandler getIPEventHandler, disconnectedEventHandler;

//RTC
DS3231 RTC; //Create the DS3231 object
DateTime now;

/****************************************************************
  FUNCTION NAME:connectWiFiAP
  FUNCTION     :Attempt to connect to WiFi
  INPUT        :
  OUTPUT       :
****************************************************************/
void TRX::connectWiFiAP(){
  DBG_OUTPUT_PORT.println("@\nConnecting to Wifi -AP Mode-");
  // Set up LED for debugging
  char smode=0;
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  byte led_status = 0;  

//  for (int i=0;i<2048;i++){
//    ibuff[i]=0xAA;
//  }
  delay(500);
//  ESP.eraseConfig();
  delay(500);
// Connect to WiFi
//  getIPEventHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event)
//   {
//     DBG_OUTPUT_PORT.println("Event Handler New device connected, New IP: ");
//     DBG_OUTPUT_PORT.println(WiFi.localIP());
//  });
//
//  disconnectedEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event)
//  {
//    DBG_OUTPUT_PORT.println("\nEvent Handler: Station disconnected\n");
//  });
  
  /**Read mode with serial input**/
 /* DBG_OUTPUT_PORT.println("\nSet the mode");
  while(!DBG_OUTPUT_PORT.available());
  smode=DBG_OUTPUT_PORT.read();
   if (smode=='b'){
	WiFi.setPhyMode(WIFI_PHY_MODE_11B);
	WiFi.setOutputPower(20);	  
  }else if (smode=='g'){
	WiFi.setPhyMode(WIFI_PHY_MODE_11G);
	WiFi.setOutputPower(17);	  
  } else if (smode=='n'){
	WiFi.setPhyMode(WIFI_PHY_MODE_11N);
	WiFi.setOutputPower(14);	  
  }else {
	DBG_OUTPUT_PORT.println("Mode not recognized, set to B");  
*/	WiFi.setPhyMode(WIFI_PHY_MODE_11G);
	WiFi.setOutputPower(20);	  
/*  } 
   /********/
   
  /**Set POWER at AP***/
/*  int powerm=20;
  DBG_OUTPUT_PORT.println("\nSet power in dBm");
  while(!DBG_OUTPUT_PORT.available());
  powerm=DBG_OUTPUT_PORT.read()-0x30;
  if(DBG_OUTPUT_PORT.available()){
	  powerm= powerm*10 +(DBG_OUTPUT_PORT.read()-0x30);	  
  }
   DBG_OUTPUT_PORT.print("\nPower is ");DBG_OUTPUT_PORT.println(powerm);  
  
  WiFi.setOutputPower(powerm);
  /*********/
  
   
  //WiFi.setPhyMode(WIFI_PHY_MODE_11B);
  //WiFi.setOutputPower(20);
  //---------------------------------------
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(APip,APgateway,APmask);    
  
  /**Set channel***/
  int channel=11;
  /*DBG_OUTPUT_PORT.println("\nSet channel");
  while(!DBG_OUTPUT_PORT.available());
  channel=DBG_OUTPUT_PORT.read()-0x30;
  if(DBG_OUTPUT_PORT.available()){
	  channel=channel*10+(DBG_OUTPUT_PORT.read()-0x30);	  
  }
  DBG_OUTPUT_PORT.print("\nChannel is  ");DBG_OUTPUT_PORT.println(channel);
  /********/
  
  
  WiFi.softAP(WIFI_SSID, WIFI_PSK,channel);
  delay(600);
  const char* phymodes[] = { "", "B", "G", "N" };
  DBG_OUTPUT_PORT.print("\nMode is: ");
  DBG_OUTPUT_PORT.println( phymodes[(int) WiFi.getPhyMode()]);  
  IPAddress myIP = WiFi.softAPIP(); 
  int rcount=0;
  while(myIP!=APip){
    rcount++;  
    WiFi.softAPConfig(APip,APgateway,APmask);  
    if(rcount==10){
        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(APip,APgateway,APmask);  
        WiFi.begin(WIFI_SSID, WIFI_PSK,channel);
        IPAddress myIP = WiFi.softAPIP();
        delay(100);
    }
  }
	DBG_OUTPUT_PORT.print("AP IP address: ");
	DBG_OUTPUT_PORT.println(myIP);
	/*END optional CONFIG*/

	digitalWrite(LED_PIN, HIGH);// Turn LED on when connected
	delay(600);
	pinMode(LED_PIN, INPUT);  
  
}

/****************************************************************
  FUNCTION NAME:connectWiFiSTA
  FUNCTION     :Attempt to connect to WiFi
  INPUT        :
  OUTPUT       :
****************************************************************/
void TRX::connectWiFiSTA(){  
  char smode=0;
  byte led_status = 0;  
  DBG_OUTPUT_PORT.println("@Connecting to Wifi -STA Mode-");
  
  /**Read mode with serial input**/
  DBG_OUTPUT_PORT.println("Set the mode");
  while(!DBG_OUTPUT_PORT.available());
  smode=DBG_OUTPUT_PORT.read();
  if (smode=='b'){
	WiFi.setPhyMode(WIFI_PHY_MODE_11B);
	WiFi.setOutputPower(20);	  
  }else if (smode=='g'){
	WiFi.setPhyMode(WIFI_PHY_MODE_11G);
	WiFi.setOutputPower(17);	  
  } else if (smode=='n'){
	WiFi.setPhyMode(WIFI_PHY_MODE_11N);
	WiFi.setOutputPower(14);	  
  }else {
	DBG_OUTPUT_PORT.println("Mode not recognized, set to B");  
	WiFi.setPhyMode(WIFI_PHY_MODE_11B);
	WiFi.setOutputPower(20);	  
  } 
  /****************************/
  
  //------------------------------------
  // Set WiFi mode to station (client)
  WiFi.mode(WIFI_STA);
  
    /**Set channel***/
  int channel=0;
  DBG_OUTPUT_PORT.println("\nSet channel");
  while(!DBG_OUTPUT_PORT.available());
  channel=DBG_OUTPUT_PORT.read()-0x30;
  DBG_OUTPUT_PORT.print(channel);
  if(DBG_OUTPUT_PORT.available()){
	  channel= channel*10 +(DBG_OUTPUT_PORT.read()-0x30);	  
  }
   DBG_OUTPUT_PORT.print("\nChannel is ");DBG_OUTPUT_PORT.println(channel);  
  /********/
  
  
  // Initiate connection with SSID and PSK
  WiFi.begin(WIFI_SSID, WIFI_PSK,channel);    
  //------------------------------------
  int c=0;
  // Blink LED while we wait for WiFi connection
  while ( WiFi.status() != WL_CONNECTED ) {
      digitalWrite(LED_PIN, led_status);
      led_status ^= 0x01; 
      delay(500);
      if (c==50){
        DBG_OUTPUT_PORT.println("\nTrying to reconnect...");
        WiFi.reconnect();
        c=0;
      }
      c++;
      DBG_OUTPUT_PORT.print(c);
  }  
  IPAddress myIP = WiFi.localIP(); 
  // Turn LED on when we are connected
  digitalWrite(LED_PIN, HIGH);
  char gmode=WiFi.getPhyMode();
  DBG_OUTPUT_PORT.print("Mode is: ");
  DBG_OUTPUT_PORT.println(gmode);
  DBG_OUTPUT_PORT.print("STA IP address: ");
  DBG_OUTPUT_PORT.println(WiFi.localIP());
}

/****************************************************************
  FUNCTION NAME:initSD
  FUNCTION     :Initializes SD card
  INPUT        :
  OUTPUT       :
****************************************************************/
void SDf::initSD(){
/*Initialialize SD*/
  if (card.init(SPI_HALF_SPEED, chipSelect)){
     SD.begin(chipSelect);
     DBG_OUTPUT_PORT.println("\nSD Card initialized.");
     hasSD = true;
  }
  else DBG_OUTPUT_PORT.println("\nERROR reading SD");
}

 /****************************************************************
  FUNCTION NAME:initRTC
  FUNCTION     : Initializes RTC and deletes its alarms
  INPUT        :
  OUTPUT       :
****************************************************************/
void Func::initRTC(){
	//config RTC
	pinMode(PIN_SDA,OUTPUT);
	digitalWrite(PIN_SDA,LOW);
	delay(10);
	digitalWrite(PIN_SDA,HIGH);
	delayMicroseconds(3);
	digitalWrite(PIN_SDA,LOW);
    delay(500);
	Wire.begin(PIN_SDA,PIN_SCL);
	Wire.setClock(200000); //200KHz
	RTC.begin();
	RTC.clearINTStatus(); // to clear all interrupts at the beginning
	delay(500);
	DBG_OUTPUT_PORT.print("\nConfig RTC\n");
	//end config RTC
	
	//Read initial RTC Value

	now = RTC.now(); //get the current date-time    
	DBG_OUTPUT_PORT.print(now.year(), DEC);
	DBG_OUTPUT_PORT.print('/');
	DBG_OUTPUT_PORT.print(now.month(), DEC);
	DBG_OUTPUT_PORT.print('/');
	DBG_OUTPUT_PORT.print(now.date(), DEC);
	DBG_OUTPUT_PORT.print(' ');
	DBG_OUTPUT_PORT.print(now.hour(), DEC);
	DBG_OUTPUT_PORT.print(':');
	DBG_OUTPUT_PORT.print(now.minute(), DEC);
	DBG_OUTPUT_PORT.print(':');
	DBG_OUTPUT_PORT.print(now.second(), DEC);
	DBG_OUTPUT_PORT.println(' ');
	//end read RTC
	
   while(now.year()==2165){
	  DBG_OUTPUT_PORT.println("RTC ERROR");
	  delay(500);
	  Wire.begin();
	  delay(500);
	  RTC.begin();
	  RTC.clearINTStatus(); // to clear all interrupts at the beginning
	  RTC.enableInterrupts(EverySecond); 
	  delay(500);
	}
	
} 

/****************************************************************
  FUNCTION NAME:	config RTC
  FUNCTION     : 	Configs RTC to restart for a period given by everytime
  INPUT        :
  OUTPUT       :
****************************************************************/
void Func::configRTC(uint8_t everytime){
	
	const char * Periodtime[]={"","Everysecond","EveryMinute","EveryHour"};
	DBG_OUTPUT_PORT.print("\nConfig RTC to: "); DBG_OUTPUT_PORT.println(Periodtime[(int)everytime]);
	//config RTC
	Wire.begin(PIN_SDA,PIN_SCL);
	Wire.setClock(200000); //200KHz
	RTC.begin();
	RTC.clearINTStatus(); // to clear all interrupts at the beginning
	delay(500);
	RTC.clearINTStatus(); // to clear all interrupts at the beginning
	RTC.enableInterrupts(everytime); 
	//end config RTC
	
	//Read initial RTC Value

	now = RTC.now(); //get the current date-time    
	DBG_OUTPUT_PORT.print(now.year(), DEC);
	DBG_OUTPUT_PORT.print('/');
	DBG_OUTPUT_PORT.print(now.month(), DEC);
	DBG_OUTPUT_PORT.print('/');
	DBG_OUTPUT_PORT.print(now.date(), DEC);
	DBG_OUTPUT_PORT.print(' ');
	DBG_OUTPUT_PORT.print(now.hour(), DEC);
	DBG_OUTPUT_PORT.print(':');
	DBG_OUTPUT_PORT.print(now.minute(), DEC);
	DBG_OUTPUT_PORT.print(':');
	DBG_OUTPUT_PORT.print(now.second(), DEC);
	DBG_OUTPUT_PORT.println(' ');
	//end read RTC
	
   while(now.year()==2165){
	  DBG_OUTPUT_PORT.println("RTC ERROR");
	  delay(500);
	  Wire.begin();
	  delay(500);
	  RTC.begin();
	  RTC.clearINTStatus(); // to clear all interrupts at the beginning
	  RTC.enableInterrupts(EverySecond); 
	  delay(500);
	}
	
} 
void Func::configRTC(uint8_t hour,uint8_t minute,uint8_t second){
	
	DBG_OUTPUT_PORT.print("\nConfig RTC to: "); DBG_OUTPUT_PORT.print(hour + String(":")); DBG_OUTPUT_PORT.print(minute + String(":")); DBG_OUTPUT_PORT.println(second);
	//config RTC
	Wire.begin(PIN_SDA,PIN_SCL);
	Wire.setClock(I2C_FRQ); //120KHz
	RTC.begin();
	RTC.clearINTStatus(); // to clear all interrupts at the beginning
	delay(500);
	RTC.clearINTStatus(); // to clear all interrupts at the beginning
	RTC.enableInterrupts(hour,minute,second); 
	//end config RTC
	
	//Read initial RTC Value

	now = RTC.now(); //get the current date-time    
	DBG_OUTPUT_PORT.print(now.year(), DEC);
	DBG_OUTPUT_PORT.print('/');
	DBG_OUTPUT_PORT.print(now.month(), DEC);
	DBG_OUTPUT_PORT.print('/');
	DBG_OUTPUT_PORT.print(now.date(), DEC);
	DBG_OUTPUT_PORT.print(' ');
	DBG_OUTPUT_PORT.print(now.hour(), DEC);
	DBG_OUTPUT_PORT.print(':');
	DBG_OUTPUT_PORT.print(now.minute(), DEC);
	DBG_OUTPUT_PORT.print(':');
	DBG_OUTPUT_PORT.print(now.second(), DEC);
	DBG_OUTPUT_PORT.println(' ');
	//end read RTC
	
   while(now.year()==2165){
	  DBG_OUTPUT_PORT.println("RTC ERROR");
	  delay(500);
	  Wire.begin();
	  delay(500);
	  RTC.begin();
	  RTC.clearINTStatus(); // to clear all interrupts at the beginning
	  RTC.enableInterrupts(EverySecond); 
	  delay(500);
	}
	
} 



TRX trx;
SDf sdf;
Func func;





