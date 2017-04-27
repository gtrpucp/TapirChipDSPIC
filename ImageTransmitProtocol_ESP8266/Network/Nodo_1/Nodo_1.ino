#include "Red.h"
int counter=3;
char answer[7]={0};
char nuevafoto[12]= "N000000.JPG", fotoanterior[12]="N000000.JPG";

void setup() {
    Serial.begin(115200);
    DBG_OUTPUT_PORT.setDebugOutput(false);      
    trx.connectWiFiSTA();
    func.init_BattM();
    func.init_RTC(); 
    sdf.initSD();
}

void loop() {
  delay(100);
  func.BattLevel(3.7); /* sets ESP8266 to sleep if the Battery Level is less than specified */
  /*****REGISTRATION******/
  while(counter-->0){
    if(trx.send_httpcon("reg")){  //ESPERA ACK_OK
        if(trx.send_str("NODO1",5)) {
          DBG_OUTPUT_PORT.println("\nSTRING NODO 1 SENT OK\n");  
          while(1){
            if(trx.rec_buf(answer)){  /* In case something is received  */
              func.set_RTC(answer);
              break;
            }
          }
          counter=0 ;  // NODE 1 REGISTERED  
          DBG_OUTPUT_PORT.println("\nREGISTRATION OK\n");  
        }
        delay(10);
    }
  }
  /***********************/
  
  /********EXPECTING COMMMUNICATION FROM SERVER**********/
  if(trx.rec_buf(answer)){  /* In case something is received  */
    DBG_OUTPUT_PORT.println("SERVER STARTED COMMUNICATION SUCCESFULLY");
    if(!strcmp(answer,"SLEEP_")){    /*In case server sends node to SLEEP*/    
      DBG_OUTPUT_PORT.println("Set to SLEEP");
      func.Sleep(); //--------------------------------------------------------------------
    } 
    else if(!strcmp(answer,"WAKEUP")){
        DBG_OUTPUT_PORT.println("Set to Wakeup");
        digitalWrite(TX_PIN, LOW); Serial.println("SET PIN TX to transmit "); /* Tells other node to stop using SD memory */

        if(!sdf.initSD()) {
          if(!sdf.initSD()) {
            if(!sdf.initSD()) {        
              trx.send_i("ERRD_SD.jpg");
              SPI.endTransaction();
              SPI.end();
              digitalWrite(4,HIGH); 
              
            }
          }        
        }         
        sdf.nextname("METADATA.DAT",nuevafoto);        
        while(strcmp((char*)nuevafoto,(char*)fotoanterior)){ // while nuevafoto is different from fotoanterior            
            //delay(1000);                            /* time to wait in order to let the server tell everynode wether it should sleep or wakeup  */    
            //while(!trx.transmite()) delay(100); Serial.println("\nPIN RX = 0");   /* waiting for pin RX to go to 0 */            
            //while(!trx.transmite()) delay(100);                                   /* to assure pin RX maintains the value of 0 logic */
            Serial.print("\nFile to send: "); Serial.write(nuevafoto);                             
            if (trx.send_i(nuevafoto)==2){                                      /* if time_exceeded  */ 
                 Serial.println("\nTIMEOUT/LOST CONNECTION ... will restart \n"); 
                 func.config_RTC(EverySecond);ESP.deepSleep(0);                
            }
            strcpy(fotoanterior,nuevafoto);
            sdf.nextname("METADATA.DAT",nuevafoto);
            /*if(strcmp((char*)nuevafoto,(char*)fotoanterior)) trx.send_str("ININO",6);*/
            delay(500);
            digitalWrite(TX_PIN, HIGH);             /* to indicate the DSPIC that it can take photos*/          
        }
        Serial.println("\nNo new pictures to send");
        trx.send_i("N1ENDPI.jpg");
    }
    else{
        Serial.print("\nUnexpected characters - not WAKEUP nor SLEEP: ");
        Serial.println(answer);
    }
   }
  /****************************************************/
}
