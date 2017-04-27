

#ifndef Libr_mtr_h
#define Libr_mtr_h

#include "Arduino.h"
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <SD.h>
#include <Wire.h>
#include "DS3231.h"

/****************************************************************/
#define   	DBG_OUTPUT_PORT Serial
#define   	Pict_server 1
#define   	PIN_SDA 5 
#define   	PIN_SCL 0
#define 	TIME_OUT_PICT_AP 60000 // Maximum duration of each photo transmitted in miliseconds.
#define		I2C_FRQ		120000
/****************************************************************/

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

//extern int32_t rssi;

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
const int LED_PIN = 25;
const int TX_PIN = 16;

/************************************* CODES ****************************************************/
const char resendname[16]={0xA5,0x01};
const char imagereceive[2]={0xB2,0x01};
  
//************************************* class **************************************************//
class TRX
{
   public:
   void  connectWiFiSTA();
   void  connectWiFiAP();
   bool  rcv_p();
    
   private:   
};

class Func
{
  public:
  void rset();
  void initRTC();
  void configRTC(uint8_t);
  void configRTC(uint8_t,uint8_t,uint8_t);
  void Sleep();
  
  private:
};

class SDf
{
  public:
  void initSD();
  
  private:
};

extern TRX trx;
extern Func func;
extern SDf sdf;

#endif
