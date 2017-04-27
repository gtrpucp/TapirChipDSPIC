

#ifndef Libr_slv_h
#define Libr_slv_h

#include "Arduino.h"
#include <SPI.h>
//#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <ESP8266WebServer.h>
#include <SD.h>
#include <Wire.h>
#include "DS3231.h"

/****************************************************************/
#define   DBG_OUTPUT_PORT Serial
#define   Pict_server 1
#define   PIN_SDA 5 
#define   PIN_SCL 0
#define	  TIME_OUT_PICT 60000000	 //Maximum time in us to wait for a picture
#define	  I2C_FRQ 		120000			// I2C frequency in Hz 
#define	  SLEEP_TIME 60*60
#define	  RESPONSE_TIME		10000		//Time to wait for response from master in miliseconds
//********************constants******************************///
// Remote site information
const char http_ap[] = "192.168.0.100";
const int http_port = 80;
// WiFi information
const char WIFI_SSID[] = "tpixp";
const char WIFI_PSK[] = "prueba1234";
//const int channel=11;
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
extern char* nof;

// Handlers
extern ESP8266WiFiGenericClass LeeIP;
extern WiFiEventHandler getIPEventHandler, disconnectedEventHandler;

//RTC
extern DS3231 RTC; //Create the DS3231 object
extern DateTime now;

//*************************************** pins **************************************************//
const int RX_PIN = 15;
const int input=15; // pin 0,2, deben mantenerse en High y 15 en Low durante el boot del Modulo Thing, por ello usamos el pin 16 como entrada xq tambien se encuentra libre
const int LED_PIN = 5;
const int TX_PIN = 2;
/**************************************** Time constants *****************************************/

const uint16_t rcvnametime=2000; // Time in microseconds to wait for a request of resending the imagen name to the remote node in case it was gotten corrupted

//************************************* class **************************************************//
class TRX
{
   public:
	void  	send_httpcon(char*);
	int   	transmite();
	void  	connectWiFiSTA();
	bool  	send_init();
	int 	send_p(char * ,char*);
	void 	send_info(char*);
	void  	send_c(char*);
	uint8_t	send_i(char*);
    bool  	rcv_p();
	bool	syncRTC();
	 
   private:
   
};

class Func
{
  public:
  void 		rset();
  bool 		check_connection();
  void 		initRTC();
  void		config_RTC(uint8_t);
  void		config_RTC(uint8_t,uint8_t,uint8_t);
  void		Sleep();
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
