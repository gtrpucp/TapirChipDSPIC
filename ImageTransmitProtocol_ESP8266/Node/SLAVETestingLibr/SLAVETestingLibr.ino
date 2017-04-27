#include "Libr_slv.h"
//#include <string.h>
//#include <ESP8266WebServer.h>
//#include <WiFiClient.h>
//#include <ESP8266WiFi.h>
/*-----RTC-----*/
//#include <Wire.h>
//#include "DS3231A.h"
//#define PIN_SDA 5 
//#define PIN_SCL 0
//DS3231 RTC; //Create the DS3231 object
//DateTime now;
/*--------------*/

WiFiClient cliente;

const char* ssid     = "tpixp";
const char* password = "prueba1234";
const char* server1 = "192.168.0.100";
uint32_t startime=0;
//const int LED_PIN = 5;
//char buffi[2055]={0};
//int32_t rssi= 0;
//char altitude=99;
char nuevafoto[12]= "N000000.JPG", fotoanterior[12]="N000000.JPG";
char *sentpic="fotoenviada.jpg";


void setup() {
  uint8_t error = 0;
  Serial.begin(115200);
  Serial.println();
  pinMode(TX_PIN,OUTPUT);
  digitalWrite(TX_PIN, HIGH);
  //pinMode(RX_PIN,INPUT); // Pin used to read the information from the camera. It has been disabled, only the camera reads the TX_PIN
  Serial.print("Connecting to ");  Serial.println(ssid);
  trx.connectWiFiSTA(); 
  Serial.println("WiFi connected");  
  Serial.print("IP address: ");  Serial.println(WiFi.localIP());
  cliente.setNoDelay(true);
  startime=millis();
  func.initRTC();
  
  //DateTime dt(__DATE__ ,__TIME__);
  //RTC.adjust(dt); //Adjust date-time as defined 'dt' above
  
  /*if(!func.check_connection()) {
    if(!func.check_connection()) {
        if(!func.check_connection()) {
            Serial.print("\nCould not stablish connection with server, going to SLEEP\n");
            func.Sleep();  //----------------------------------
         }
     }
  }*/

  error = 1;
  for(uint8_t x = 0; x < 10; x++){
    if(func.check_connection()){
      error = 0;
      break;
    }
    delay(1000);
  }
  if(error){
    Serial.println("\nCould not stablish connection with server, going to SLEEP\n");
    func.Sleep();
  }

  trx.syncRTC();

  /*if(!sdf.initSD()) {
    if(!sdf.initSD()) {
      if(!sdf.initSD()) {        
        trx.send_i("ERRD_SD.jpg");
        SPI.endTransaction();
        SPI.end();
        digitalWrite(4,HIGH); 
        func.Sleep(); //-------------------------------------------
      }
    }        
  }*/

  error = 1;
  for(uint8_t x = 0; x < 10; x++){
    if(sdf.initSD()){
      error = 0;
      break;
    }
    delay(1000);
  }
  if(error){
    trx.send_i("ERRD_SD.jpg");
    SPI.endTransaction();
    SPI.end();
    digitalWrite(4, HIGH);
    func.Sleep();
  }
  digitalWrite(TX_PIN, LOW);   Serial.println("SET PIN TX to transmit ");   // Tells camera to stop taking pictures.   
}

void loop() {
  uint8_t message = 0;
  Serial.println("\nloop"); 
  if(!func.check_connection()) {
    delay(1000);
     if(!func.check_connection()) {
        delay(1000);
         if(!func.check_connection()) {
            Serial.print("\nCould not stablish connection with server, going to SLEEP\n");
            func.Sleep();
         }
     }
  }
  digitalWrite(TX_PIN, LOW);   Serial.println("SET PIN TX to transmit ");   // Tells camera to stop taking pictures. 
  delay(700);
  //while(!trx.transmite()) delay(100); //to assure pin RX maintains the value of 0 logic
  sdf.nextname("METADATA.DAT",nuevafoto); /*Reads new picture to send*/ 
  if (!strcmp((char*)nuevafoto,(char*)fotoanterior)){
        Serial.println("\nNo new pictures to send, going back to sleep");
        trx.send_i("ENDOFPI.jpg");
        SPI.endTransaction();
        SPI.end();
        digitalWrite(4,HIGH);
        func.Sleep();            
  }
  
  switch (trx.send_i(nuevafoto)){
    case 2:
      Serial.println("\nTIMEOUT/LOST CONNECTION ... will restart \n");
      func.config_RTC(EverySecond);
      ESP.deepSleep(0);
      break;
    case 3:
      Serial.println("Master did not receive image");
      Serial.println("Going to sleep");
      func.Sleep();
    default:
        break;
  }
  strcpy(fotoanterior,nuevafoto);
  //delay(500);
  /**********SAVE TO SD*********/
  // open the file. note that only one file can be open at a time, so you have to close this one before opening another.
 /* myFile = SD.open("Data.txt", FILE_WRITE);
  // if the file opened okay, write to it:
  if (myFile) {
    DBG_OUTPUT_PORT.print("Writing to data.txt...");
    //myFile.print(String("Name of the picture: ") + nuevafoto + String("\n") );
    //thisFile.print(String("Tiempo de transmision: ") + (double)((double)times/1000000) + String(" s\n"));
    //thisFile.print(String("Tamano: ")+ sizep + String(" Bytes\n"));
    //thisFile.print(String("Datarate: ")+ (double)((((double)sizep/(double)times)*8)*1000) + String(" Kbps\n"));
    myFile.print(String("RSSI: ")+ (int)(rssi) + String(" dBm\n\n"));
    myFile.println(" ");
    myFile.close(); // close the file:
    DBG_OUTPUT_PORT.println("done.");
  } else {
    // if the file didn't open, print an error:
    DBG_OUTPUT_PORT.println("error creating data.txt");
    myFile.close();   
  }  */ 
  /********************************/
  
  //digitalWrite(TX_PIN, HIGH);   // to indicate the DSPIC that it can take photos
}

