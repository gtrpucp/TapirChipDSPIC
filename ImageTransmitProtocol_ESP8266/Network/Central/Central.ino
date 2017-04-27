#include "Red.h"
ESP8266WebServer server_central(PORT);
//WiFiServer server_central_wifi(PORT);

WiFiClient client1,client2,client3,client4;
uint32_t init_time=0;
bool node_connected=0;
const int pbuff = 2095  ; //2095
char TS[5]={0,0,0,0,0};
char val[7]={0};
uint8_t c=1;
char buff_si[11]={0};
int t=0;
File thisFile;

void rset(){  
  DBG_OUTPUT_PORT.println("RESET");
  //func.init_RTC();
  func.config_RTC(EverySecond); 
  delay(500);
  ESP.deepSleep(0);
}

void rcv_p() {
   unsigned long sizep=0;
   int32_t rssi=0;
   char rpic[pbuff]= {0};
   unsigned long timeinit=0;
   int32_t times=0;
   char nof[11]= "N1001.jpg";
   int32_t i=1,j=0;
   uint32_t m=0;
   char c=0;
   
   DBG_OUTPUT_PORT.print("RCV_P\n");
   WiFiClient client_init_tx = server_central.client();
   client_init_tx.setNoDelay(true);
   k=0;
   while (!client_init_tx.available()){
      delay(100); // waits till a message comes
      DBG_OUTPUT_PORT.print("nm");
   }
   DBG_OUTPUT_PORT.print("\nRecibiendo: "); // Starts to receive the name of the picture

   for(t=0;t<11;t++){
      if (client_init_tx.available())  buff_si[t] = client_init_tx.read();
      DBG_OUTPUT_PORT.print(buff_si[t]);
   }
    
   if (buff_si[0]=='N') DBG_OUTPUT_PORT.print("\nName received is OK\n");   
   else client_init_tx.write((char*)resendname,2);
   node_number= buff_si[1]- 0x30;
   DBG_OUTPUT_PORT.println(String("\nNode number is") + node_number);
   switch (node_number){
    case 1:   client1 = server_central.client();
    break;
    case 2:   client2 = server_central.client();
    break;
    case 3:   client3 = server_central.client();
    break;
    case 4:   client4 = server_central.client();
    break;
    default:
    DBG_OUTPUT_PORT.print("\nNode number not read accurately\n");
   }  
   DBG_OUTPUT_PORT.print("\nmessage received is: "); DBG_OUTPUT_PORT.write((char*)buff_si, sizeof(buff_si)); // prints message received
  /*********************************************/
   strcpy(nof,buff_si);
   if(!strcmp(nof+2,"ENDPI.jpg")){
    DBG_OUTPUT_PORT.print(String("\nTransmission of pictures from @NODE ")+node_number+String( " complete\n"));
    node_connected=0;
    return;
    }

   if(!strcmp(nof,"ERRD_SD.jpg")){
      DBG_OUTPUT_PORT.println(String("\nERROR READ SD @NODE")+node_number);
      node_connected=0;
      return;
    }
      
   // Read all the lines of the reply from server and print them to Serial
   if(SD.exists((char * )nof)) SD.remove((char *)nof);
   thisFile = SD.open(nof,FILE_WRITE);
   if(!thisFile){
      DBG_OUTPUT_PORT.println(String("\nError creating ") + nof) ;
      return;
   }
   DBG_OUTPUT_PORT.print("\n------------- Receiving Pict ---------------\n ");
   //rssi= WiFi.RSSI();
   while(!client_init_tx.available());
   if(client_init_tx.available()){
     timeinit=millis();
     DBG_OUTPUT_PORT.println(String("\nInit Time in miliseconds: ")+ millis());
   }
   while(i){ // No funciona con 50 000
     i++;    
       while(client_init_tx.available()){                       
          c = client_init_tx.read();        //DBG_OUTPUT_PORT.print(c);  
          thisFile.print(c);  
          m = m<<8 | c;
          if (((m&0x00FFFFFF)==0xFFD9EE) || (m==0xFFD9EE00) ||  (m==0xFFD90000)||  ((m&0x00FFFFFF)==0xFFD9FF) || (m&0x00FFFFFF)==0x00FFD900   ) {
               DBG_OUTPUT_PORT.print(m,HEX);
               DBG_OUTPUT_PORT.print("\nEnd of image received.\n");
               sizep=thisFile.size();
               thisFile.close();            
               DBG_OUTPUT_PORT.println(String("\nEnd Time in micros:  ")+ micros());
               times=millis()-timeinit;
               DBG_OUTPUT_PORT.println(String("\nTime Elapsed:    ")+ times);
               i=0;
               break;          
          }  
     }

     if (millis()-timeinit>SERV_WAIT_TIME){
          DBG_OUTPUT_PORT.print("\nCONNECTION TIME EXCEEDED      ");
          DBG_OUTPUT_PORT.println(millis()-timeinit);  
          return;
     }          
   }
   
   DBG_OUTPUT_PORT.print("----- End of Picture ------ "); 
   DBG_OUTPUT_PORT.println(String("\n\nImage successfully received ")); 
   DBG_OUTPUT_PORT.print(String("Tiempo: ") + (double)((float)times/(float)1000) + String(" seg\n"));
   DBG_OUTPUT_PORT.print(String("Tamano: ")+ (float)sizep/1000.0 + String(" KBytes\n"));
   DBG_OUTPUT_PORT.print(String("Datarate: ")+ (double)(((double)sizep*(float)8000/(double)times)) + String(" Kbps\n"));
   DBG_OUTPUT_PORT.print(String("RSSI: ")+ (int)(rssi) + String(" dBm\n\n"));
   
   thisFile = SD.open("Data.txt", FILE_WRITE);
   // if the file opened okay, write to it:
   if (thisFile) {
      DBG_OUTPUT_PORT.print("Writing to data.txt...\n");
      thisFile.print(String("Name of the picture: ") + nof + String("\n") );
      thisFile.print(String("Tiempo de transmision: ") + (double)((double)times/10000000) + String(" s\n"));
      thisFile.print(String("Tamano: ")+ sizep + String(" Bytes\n"));
      thisFile.print(String("Datarate: ")+ (double)((((double)sizep/(double)times)*8)*1000) + String(" Kbps\n"));
      thisFile.print(String("RSSI= ")+ (int)(rssi) + String(" dBm\n\n"));
      thisFile.println(" ");
      thisFile.close(); // close the file:
      DBG_OUTPUT_PORT.println("done.");
    } else {           // if the file didn't open, print an error:
      DBG_OUTPUT_PORT.println("error creating data.txt");
      thisFile.close();   
    }   
   
   DBG_OUTPUT_PORT.print("End of RCV_P\n");  
   return;
}

void reg_node(){
   DBG_OUTPUT_PORT.print("REG_NODE\n");
   char   buff_rec[11]={0};
   int     t=0;  
   char    time_arr[7]={0};
   WiFiClient client_init_tx = server_central.client();
   client_init_tx.setNoDelay(true);
   /*** SEND RECEPTION ACK****/
   client_init_tx.write((char*)"ACK_OK",7); 
   /********************/     
   while (!client_init_tx.available()){
      delay(50); // waits till a message comes
      DBG_OUTPUT_PORT.print("nm");
   }
   DBG_OUTPUT_PORT.print("\nRecibiendo: "); 
   
   /*------Receive Buffer buff_rec: Name of Node------*/
   for(t=0;t<11;t++){
     if (client_init_tx.available())  buff_rec[t] = client_init_tx.read(); /*Receives Node Name*/
      /*DBG_OUTPUT_PORT.print(buff_rec[t]);*/
   }
   delay(150);
   client_init_tx.write((char*)"ACK_OK",7);
   client_init_tx.flush();
   /*----------------------------------*/
   
   //DBG_OUTPUT_PORT.print("\nNode recognized is: "); DBG_OUTPUT_PORT.write((char*)buff_rec, sizeof(buff_rec)); DBG_OUTPUT_PORT.println();   
   strcpy(node_name,buff_rec);
   if (!(strcmp(node_name,"NODO1"))){
        client1 = server_central.client();
        NC[1]=0xFF;
        TS[1]=0xFF;
        DBG_OUTPUT_PORT.print("\nNODO 1 SAVED\n");
        //delay(50); // Time needed for the slave to process
        func.read_RTC(time_arr);
        client1.write((char*)time_arr,7);
        client1.flush();
        
   }
   else if (!(strcmp(node_name,"NODO2"))){
        client2 = server_central.client();   
        NC[2]=0xFF;
        TS[2]=0xFF;
        DBG_OUTPUT_PORT.print("\nNODO 2 SAVED\n");   
        //delay(150);
        func.read_RTC(time_arr);
        client2.write((char*)time_arr,7);
        client2.flush();
   }
   else if (!(strcmp(node_name,"NODO3"))){
        client3 = server_central.client();
        NC[3]=0xFF;
        TS[3]=0xFF;
        DBG_OUTPUT_PORT.print("\nNODO 3 SAVED\n");
        //delay(100);
        func.read_RTC(time_arr);
        client3.write((char*)time_arr,7);
        client3.flush();
   }
   else if (!(strcmp(node_name,"NODO4"))){
        client4 = server_central.client();
        NC[4]=0xFF;
        TS[4]=0xFF;
        DBG_OUTPUT_PORT.print("\nNODO 4 SAVED\n");
        //delay(100);
        func.read_RTC(time_arr);
        client4.write((char*)time_arr,7);
        client4.flush();
   }
  /* 
   for(int i=1; i<=4;i++){
    DBG_OUTPUT_PORT.print(String("NC")+ (i) + String(": "));
    DBG_OUTPUT_PORT.println(NC[i],DEC);
   }
*/
   //strcpy(TS,NC);
   return;
}


/****************************************************************
  FUNCTION NAME: connect_node
  FUNCTION     : Sends a string VALUE to the client DESTINY
  INPUT        :
  OUTPUT       : 0, when ACK not received
         1, when ACK received
****************************************************************/
bool connect_node(char* destiny, char * value) {
  DBG_OUTPUT_PORT.print("@trx::connect_node \n");
  if(!(strcmp(destiny,"client1"))){
      client1.setNoDelay(true);
      client1.write((char *)value,7); // Write to client1
      client1.flush();
      DBG_OUTPUT_PORT.println(String("sent to")+val);
      return 1;
  } else if(!(strcmp(destiny,"client2"))){
      client2.setNoDelay(true);
      client2.write((char *)value,7); // Write to client2
      client2.flush();
      DBG_OUTPUT_PORT.println(String("sent to")+val);
      return 1;  
  } else if(!(strcmp(destiny,"client3"))){
      client3.setNoDelay(true);
      client3.write((char *)value,7); // Write to client3  
      client3.flush();
      DBG_OUTPUT_PORT.println(String("sent to")+val);
      return 1;
  }
  else if(!(strcmp(destiny,"client4"))){
      client4.setNoDelay(true);
      client4.write((char *)value,7); // Write to client4    
      client4.flush(); 
      DBG_OUTPUT_PORT.println(String("sent to")+val);
      return 1;
  }
  return 0;
}




void setup() {
  
    Serial.begin(115200);
    DBG_OUTPUT_PORT.setDebugOutput(true);  
    sdf.initSD();
    trx.connectWiFiAP();
    func.init_RTC();
    //DateTime dt(2017,03,13,0,0,0,0);
    //RTC.adjust(dt); //Adjust date-time as defined 'dt' above   
    server_central.on("/rset", HTTP_GET, rset);
    server_central.on("/reg", HTTP_GET, reg_node);
    server_central.on("/rcv", HTTP_GET, rcv_p);
    server_central.begin();
    //server_central_wifi.begin();
    DBG_OUTPUT_PORT.println("HTTP server started");
    init_time=millis();
}

void loop() {  
  server_central.handleClient();  /* server_central_wifi.handleClient();*/
  delay(300);
  if(REG_TIME < millis() - init_time){  // Registering time finished
    //Scan the nodes that were registered
    if(!node_connected){                 // If there is no node connected      
      if(NC[c]== 0xFF){                  // Node[c] registered?
         snprintf(val,8,"client%i",(c));
         connect_node(val,"WAKEUP");     // send to node c WAKEUP
         node_connected=1;
         NC[c]=0x00;                     // Unregister the node 
      }
      if (c==5){
        delay(1000);
        Serial.println(String("\n TRANSMISSION WITH NODES COMPLETED, SENDING ALL TO SLEEP")+TS);
        for (int n=1;n<=4;n++){ 
          if(TS[n]==0xFF){
            snprintf(val,8,"client%i",(n));
            Serial.println(String("Sending to sleep to node: ")+val);
            connect_node(val,"SLEEP_"); 
          }
          delay(100);
        }
        
        func.Sleep();//---------------------------------------------------------------        
      }
      else c++;
    }
  }                  
}
