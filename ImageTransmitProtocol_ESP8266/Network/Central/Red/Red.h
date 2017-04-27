

#ifndef Red_h
#define Red_h

#include "Arduino.h"
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <ESP8266WebServer.h>
#include <SD.h>
#include <Wire.h>
#include "DS3231A.h"
#include "MAX17043.h"

/****************************************************************/
#define   DBG_OUTPUT_PORT	Serial
#define   Pict_server 		1
#define   PIN_SDA 			5 
#define   PIN_SCL 			0
//#define   TO_P_NC 			15000000 // Maximum time in microseconds that the central node will wait for information. Initially set to 15 seconds
#define	  REG_TIME			60000//Registering time in miliseconds. 1 minuto
#define	  PICT_TIME			50000 //Time out per pict at NODE in ms
#define	  SERV_WAIT_TIME	50000//Time out per pict at SERVER in ms
#define	  I2C_FRQ			120000 // Frequency for I2C - RTC
#define	  SLEEP_TIME		60*60
/******************Constants*********************************/
// Remote site information
const char 	IP_AP[] = "192.168.4.100";
const int 	http_port = 80;
// WiFi information
const char 	WIFI_SSID[] = "tpixp";
const char 	WIFI_PSK[] = "prueba1234";
const int 	channel=1;
const WiFiPhyMode_t PhyMode=WIFI_PHY_MODE_11B; // WIFI_PHY_MODE_11B, WIFI_PHY_MODE_11G, WIFI_PHY_MODE_11N
const int 	PhyMode_Power=20; // For Maximum Power: set 20 for B, 17 for G or 14 for N
const int 	PORT=80;

/************************Variables************************/
extern IPAddress APip;
extern IPAddress APgateway;
extern IPAddress APmask;

extern int32_t rssi;

//SD
extern File myFile;
extern char buffi[2055];
extern char buff_sendi[2055];
extern const int chipSelect;
extern Sd2Card card;
extern SdVolume volume;
extern SdFile root;
extern bool hasSD;
extern int z,k;
//extern char* nof;

// Handlers
extern ESP8266WiFiGenericClass LeeIP;
extern WiFiEventHandler getIPEventHandler, disconnectedEventHandler;

//RTC
extern DS3231 RTC; //Create the DS3231 object
extern DateTime now;

//nodes
extern char node_name[6];
extern int node_number;
extern char NC[5]; // Indicates which node established connection bu changing NC[i] to 0xFF

//BatteryMonitor
extern MAX17043 batteryMonitor;

//*************************************** pins **************************************************//
const int RX_PIN = 15;
const int input=15; // pin 0,2, deben mantenerse en High y 15 en Low durante el boot del Modulo Thing, por ello usamos el pin 16 como entrada xq tambien se encuentra libre
const int LED_PIN = 20;
const int TX_PIN = 2;
/**************************************** Time constants *****************************************/

const uint16_t rcvnametime=2000; // Time in microseconds to wait for a request of resending the imagen name to the remote node in case it was gotten corrupted

/************************************* CODES ****************************************************/
const char resendname[16]={0xA5,0x01};
//************************************* class **************************************************//
class TRX
{
   public:
	bool  	send_httpcon(char*);
	int   	transmite();
	void  	connectWiFiSTA();
	void  	connectWiFiAP();
	bool  	send_init();
	int 	send_p(char * ,char*);
	void 	send_info(char*);
	void  	send_c(char*);
	uint8_t	send_i(char*);

   bool  	rcv_p();
   bool 	send_str(char *,int);
   bool   	rec_ack(char *);
   bool   	rec_buf(char *);
   
   private:
   
};

class Func
{
  public:
  void 		rset();
  bool 		check_connection();
  void		init_RTC();
  void		config_RTC(uint8_t);
  void		config_RTC(uint8_t,uint8_t,uint8_t);
  bool     	read_RTC(char *);
  bool     	set_RTC(char*);
  void 		init_BattM();
  bool		BattLevel(float);
  void		Sleep();
  //void 		ReadNode(char*);
  private:
};

class SDf
{
  public:
  bool 		initSD();
  long 		nextname(char *, char*);
  int  		confirmsent(char*);
  private:
};

extern TRX trx;
extern Func func;
extern SDf sdf;

#endif
