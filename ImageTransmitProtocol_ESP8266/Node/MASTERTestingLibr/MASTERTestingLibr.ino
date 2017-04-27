
//#include <ESP8266WiFi.h>
//#include <WiFiClient.h>
//#include <ESP8266WebServer.h>
//#include <ESP8266mDNS.h>
/*-----RTC-----*/
#include <Wire.h>
//#include "DS3231A.h"
#define PIN_SDA 5 
#define PIN_SCL 0
#define SLEEP_TIME 60*60
//DS3231 RTC; //Create the DS3231 object
// DateTime now ;
/*--------------*/

//#include <SPI.h>
//#include <SD.h>
//#include <Wire.h>
//#include "SparkFunMPL3115A2.h"
#include <Libr_mtr.h>
#define DBG_OUTPUT_PORT Serial


//Create an instance of the object
//MPL3115A2 myPressure;

//// Only USE if nothing connected to ADC !!!!!!!!!
//// ADC_MODE(ADC_VCC);
int power=0x14; //20
const int pbuff = 2095  ; //2095
char ibuff[2048]={0};
char initbuff[2048]="HOOOLA";
// Pin definitions
//const int LED_PIN = 5;

int p_counter=0;
int altitude_t;
float altitude;

int32_t rssi=0;

char buff_si[11]={0};
int k=0,t=0;
//
//
ESP8266WebServer server_mtr(80);

WiFiClient client_mtr = server_mtr.client();
//
IPAddress apIP(192, 168, 0, 100);
//ESP8266WebServer server(http_port);

IPAddress customip(192,168,0,100);
IPAddress gateway(192,168,0,1);
IPAddress mask(255,255,255,0);
IPAddress destiny(192,168,0,99);

File thisFile;
//const int chipSelect = 4;
//Sd2Card card;
//SdVolume volume;
//SdFile root;
//static bool hasSD = false;
//File uploadFile;
char rpic[pbuff]= {0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA};

void sleep(){
  DateTime now= RTC.now();
  long wakeup = now.get()+SLEEP_TIME;
  DateTime next(wakeup); 
  func.configRTC(next.hour(),next.minute(),next.second()); //-----------------------------------
  /*
  uint8_t minute=now.minute();
  uint8_t hour=now.hour();
    if(minute>=55){
        minute-=55;
        hour+=1;
    }
  func.configRTC(hour,minute,now.second()); //-----------------------------------*/
  ESP.deepSleep(0);       
}
void rset(){  
  uint32_t sleepTimeMiliSeconds=10;
  DBG_OUTPUT_PORT.println("RESET");
  //ESP.deepSleep(sleepTimeMiliSeconds * 1000);
  //
    now = RTC.now(); //get the current date-time    
    //print only when there is a change in seconds
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
  //               
    RTC.begin();// to clean the everysecond alarm given by the reset routine
    RTC.clearINTStatus(); // to clear all interrupts at the beginning
    delay(10);
    RTC.enableInterrupts(EverySecond); 
    delay(500);
    ESP.deepSleep(0); 
}

void sync_rtc(){
   DBG_OUTPUT_PORT.print("\nSYNC_RTC\n");
   WiFiClient client_rtc = server_mtr.client();
   client_rtc.setNoDelay(true);
   k=0;
   uint8_t counterp=0;
   while (!client_rtc.available()){
      delay(100); // waits till a message comes
      DBG_OUTPUT_PORT.print("nm");
      if((counterp++)==100) rset();
    }
   t=0;

   DBG_OUTPUT_PORT.print("\nReceiving RTC from client ... "); // Starts to receive the name of the picture
   for(t=0;t<7;t++){
      if (client_rtc.available())  buff_si[t] = client_rtc.read();
      //DBG_OUTPUT_PORT.println(buff_si[t],DEC);
   }
    uint16_t year = buff_si[0]*256+ buff_si[1];
    uint8_t month = buff_si[2];
    uint8_t date= buff_si[3];
    uint8_t hour = buff_si[4];
    uint8_t minute= buff_si[5];
    uint8_t second = buff_si[6];
    
   DateTime dt(year, month, date, hour, minute , second, 0);
   RTC.adjust(dt); //Adjust date-time as defined 'dt' above 

   Serial.print("Time synced to: ");
   now = RTC.now(); //get the current date-time
   Serial.print(now.year(), DEC);
   Serial.print('/');
   Serial.print(now.month(), DEC);
   Serial.print('/');
   Serial.print(now.date(), DEC);
   Serial.print(' ');
   Serial.print(now.hour(), DEC);
   Serial.print(':');
   Serial.print(now.minute(), DEC);
   Serial.print(':');
   Serial.print(now.second(), DEC);
   Serial.println();
    
}


void rcv_p() {
   DBG_OUTPUT_PORT.print("RCV_P\n");
   WiFiClient client_init_tx = server_mtr.client();
   client_init_tx.setNoDelay(true);
   k=0;
    while (!client_init_tx.available()){
      delay(100); // waits till a message comes
      DBG_OUTPUT_PORT.print("nm");
    }
    DBG_OUTPUT_PORT.print("\nRecibiendo: "); /* Starts to receive the name of the picture */
    for(t=0;t<11;t++){
      if (client_init_tx.available())  buff_si[t] = client_init_tx.read();
      DBG_OUTPUT_PORT.print(buff_si[t]);   /*Prints the name of the picture*/
    }

    if (buff_si[0]=='N') DBG_OUTPUT_PORT.print("\nName received is OK\n");
    //**TODO** Implementar el resend
    else client_init_tx.write((char*)resendname,2);

    DBG_OUTPUT_PORT.print("\nmessage received is: ");
    DBG_OUTPUT_PORT.write((char*)buff_si, sizeof(buff_si)); // prints message received
  /*********************************************/

  unsigned long   sizep=0;
  const int       pbuff=2048;
  char            rpic[pbuff]= {0};
  char            rpic2[pbuff]= {0};
  uint32_t        times=0xFFFFFFFF;
  uint32_t        timeinit=0xFFFFFFFF;
  char *          nof= "nname.jpg";
  nof=(char*)buff_si;
  if (!strcmp(nof,"ENDOFPI.jpg")){
    sleep();
  }
  else  if (!strcmp(nof,"ERRD_SD.jpg")){
    Serial.print("\nERROR Reading SD @Trap Camera\n");
    sleep();
  }
  else  if (!strcmp(nof,"ERR_RTC.jpg")){
    Serial.print("\nERROR RTC @Trap Camera\n");
    sleep();
  }
   
  /* Read all the lines of the reply from server and print them to Serial */
  if(SD.exists((char*)nof)) SD.remove((char *)nof);
  thisFile = SD.open(nof,FILE_WRITE);
  if(!thisFile){
    DBG_OUTPUT_PORT.println(String("\nError creating ") + nof) ;
    return;
  }
  int32_t i=1,j=0;
  uint32_t m=0;
  char c=0;
  DBG_OUTPUT_PORT.print("\n------------- Receiving Pict ---------------\n ");
  while(!client_init_tx.available());
  if(client_init_tx.available()){
    timeinit=millis();
    times=millis();
    DBG_OUTPUT_PORT.println(String("\nInit Time in millis: ")+ millis());
  }
  while(i){ // No funciona con 50 000
    i++;
    if(millis()-timeinit>5000){
      DBG_OUTPUT_PORT.print("\nTIME EXCEEDED      ");
      DBG_OUTPUT_PORT.print((double)(millis()-times)/1000); 
      DBG_OUTPUT_PORT.println(" segundos");
      DBG_OUTPUT_PORT.println("Lost connection with Slave, going to sleep");
      //rset();
      sleep();
    }
    while(client_init_tx.available()){
        c = client_init_tx.read();
        //DBG_OUTPUT_PORT.print(c);
        thisFile.print(c);
        timeinit = millis();
        m = m<<8 | c;
        if (((m&0x00FFFFFF)==0xFFD9EE) || (m==0xFFD9EE00) ||  (m==0xFFD90000)||  ((m&0x00FFFFFF)==0xFFD9FF) || (m&0x00FFFFFF)==0x00FFD900   ) {
           DBG_OUTPUT_PORT.print(m,HEX);
           DBG_OUTPUT_PORT.print("\nEnd of image received.\n");
           sizep=thisFile.size();
           thisFile.close();

           //Send a confirmation to slave before transmitting image
           delay(1000);
           DBG_OUTPUT_PORT.println("Sending confirmation to slave");
           client_init_tx.write((char*)imagereceive,2);
           DBG_OUTPUT_PORT.println(String("\nEnd Time in millis:  ")+ millis());
           times=millis()-times;
           rssi= WiFi.RSSI();
           DBG_OUTPUT_PORT.print(String("RSSI: ")+ (int32_t)(rssi) + String(" dBm\n\n"));
           DBG_OUTPUT_PORT.println(String("\nTime Elapsed:    ")+ times);
           i=0;
           break;
        }
     }
     //TIME OUT: 60 sec defined at .h
     //if ((millis()-timeinit)>TIME_OUT_PICT_AP){
     //   DBG_OUTPUT_PORT.print("\nTIME EXCEEDED      ");
     //   DBG_OUTPUT_PORT.print((double)(millis()-times)/1000); 
     //   DBG_OUTPUT_PORT.print(" segundos");
     //   rset();
     //}
     
   }
   DBG_OUTPUT_PORT.print("----- End of Picture ------ "); 
   DBG_OUTPUT_PORT.println(String("\n\nImage successfully received ")); 
   DBG_OUTPUT_PORT.print(String("Tiempo: ") + (double)(times/(float)1000) + String(" seg\n"));
   DBG_OUTPUT_PORT.print(String("Tamano: ")+ sizep + String(" Bytes\n"));
   DBG_OUTPUT_PORT.print(String("Datarate: ")+ (double)(((double)sizep*(float)8/(double)times)) + String(" Kbps\n"));
   DBG_OUTPUT_PORT.print(String("RSSI: ")+ (long)(rssi) + String(" dBm\n\n"));
   
    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    thisFile = SD.open("Data.txt", FILE_WRITE);
  
    // if the file opened okay, write to it:
    if (thisFile) {
      DBG_OUTPUT_PORT.print("Writing to data.txt...\n");
      thisFile.print(String("Name of the picture: ") + nof + String("\n") );
      thisFile.print(String("Tiempo de transmision: ") + (double)((double)times/1000000) + String(" s\n"));
      thisFile.print(String("Tamano: ")+ sizep + String(" Bytes\n"));
      thisFile.print(String("Datarate: ")+ (double)((((double)sizep/(double)times)*8)*1000) + String(" Kbps\n"));
      thisFile.print(String("RSSI: ")+ (int)(rssi) + String(" dBm\n"));
      thisFile.println(" ");
      thisFile.close(); // close the file:
      DBG_OUTPUT_PORT.println("done.");
    } else {
      // if the file didn't open, print an error:
      DBG_OUTPUT_PORT.println("error creating data.txt");
      thisFile.close();   
    }   
   DBG_OUTPUT_PORT.print("End of RCV_P\n");  
  return;
}


void init_tx(){
    //read_alt();
   DBG_OUTPUT_PORT.print("Executing Init_tx \n");
   WiFiClient client_init_tx = server_mtr.client();
   client_init_tx.setNoDelay(true);
   DBG_OUTPUT_PORT.println("Sending HOLA");
   DBG_OUTPUT_PORT.write((char*)initbuff,sizeof(initbuff));
   DBG_OUTPUT_PORT.print("Enviando: ");
   client_init_tx.write((char*)initbuff,sizeof(initbuff));
   client_init_tx.flush();
    k=0;
    while (!client_init_tx.available()) {
      DBG_OUTPUT_PORT.print("Waiting information..");
      delay(20);
    } // 
    DBG_OUTPUT_PORT.print("\nRecibiendo: ");
    while (client_init_tx.available()){      
    buff_si[k++] = client_init_tx.read();
      //DBG_OUTPUT_PORT.print("\nclientavaol");
    }

    DBG_OUTPUT_PORT.println("");
    DBG_OUTPUT_PORT.write((char*)buff_si, sizeof(buff_si));
    //DBG_OUTPUT_PORT.write((char*)ibuff,sizeof(ibuff));
    DBG_OUTPUT_PORT.println("\nEND OF INIT_TX\0");
    DBG_OUTPUT_PORT.println("");

    //delay(100);
    return;
}

void setup(void){

  DBG_OUTPUT_PORT.begin(115200);  
  //DBG_OUTPUT_PORT.setDebugOutput(true);  
  trx.connectWiFiAP();
  server_mtr.on("/rset", HTTP_GET, rset);server_mtr.on("/print", HTTP_GET, init_tx);server_mtr.on("/rcv", HTTP_GET, rcv_p);server_mtr.on("/srtc", HTTP_GET, sync_rtc);
  server_mtr.begin();
  DBG_OUTPUT_PORT.println("HTTP server started");
  sdf.initSD();
  func.initRTC();  
  for (int k=0; k<=pbuff; k++){
    rpic[k]= 0x00;
  }    
  
}

void loop(void){  
  server_mtr.handleClient();
  delay(200);
}

////- See more at: http://www.esp8266.com/viewtopic.php?f=29&t=6731#sthash.YNlEDo0p.dpuf
