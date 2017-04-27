
#include "Red.h"

#ifdef Pict_server
  ESP8266WebServer server(80);
  WiFiClient client = server.client();
#endif

IPAddress APip(192,168,4,100);
IPAddress APgateway(192,168,4,1);
IPAddress APmask(255,255,255,0);

/************************Variables************************/
//SD
File myFile;
char buffi[2055]={0};
char buff_sendi[2055]={0};
const int chipSelect = 4;
Sd2Card card;
SdVolume volume;
SdFile root;
bool hasSD = false;
int z =0,k=0;
//char* nof="N100001.jpg";
int32_t rssi= 0;

// Handlers
ESP8266WiFiGenericClass LeeIP;
WiFiEventHandler getIPEventHandler, disconnectedEventHandler,testeventhandler;

//RTC
DS3231 RTC; //Create the DS3231 object
DateTime now;

//Battery
MAX17043 batteryMonitor;

//Nodes
int node_number=0;
char node_name[6]={0};
char NC[5]={0x00,0x00,0x00,0x00,0x00}; 
   
/****************************************************************
  FUNCTION NAME:send_httpcon
  FUNCTION     :Sends httpgetrequest
  INPUT        :GET REQ to send
  OUTPUT       :none
****************************************************************/
bool TRX::send_httpcon( char* REQ){
	DBG_OUTPUT_PORT.println("\n@TRx::send_httpcon\0");
   
	// Attempt to make a connection to the remote server
	if ( !client.connect(IP_AP, http_port) ){
		DBG_OUTPUT_PORT.println(String("Could not connect to") + IP_AP);
		return 0;
	}	
		
	DBG_OUTPUT_PORT.print("GET ");
	DBG_OUTPUT_PORT.print(String("/")+ REQ);
	DBG_OUTPUT_PORT.println(" HTTP/1.1");
	DBG_OUTPUT_PORT.print("Host: ");
	DBG_OUTPUT_PORT.println(IP_AP);
	DBG_OUTPUT_PORT.println("Connection: close");
	
	client.setNoDelay(true);
	// Send a request
	client.print("GET ");
	client.print(String("/")+ REQ);
	client.println(" HTTP/1.1");
	client.print("Host: ");
	client.println(IP_AP);
	client.println("Connection: close");
	client.println();    
	
	unsigned long timeout = millis();
	while (client.available() == 0) {
		Serial.println("waiting for an answer from server.");
		if (millis() - timeout > 1000) {
		  Serial.println(">>> Client Timeout !");
		  client.stop();
		  return 0;
		}
	}

	if(trx.rec_ack("ACK_OK")) return 1;
	
	/* DBG_OUTPUT_PORT.print("\n @trx::send_str \n");
  uint32_t times=0; 
  // Attempt to make a connection to the remote server
  if ( !client.connect(IP_AP, http_port) ) {
    DBG_OUTPUT_PORT.println(String("Could not connect to") + IP_AP);
	send_httpcon("rset");
	delay(250);
    return false;
  }
  
  DBG_OUTPUT_PORT.print("\nString to send is: ");DBG_OUTPUT_PORT.write((char*)"NODO1",15);
  delay(10);
  client.write((char*)"NODO1",15); // Sends the string
  client.flush();
  
  client.print(0xAA);
  client.flush(); //Waits until all outgoing characters in buffer have been sent. 
  DBG_OUTPUT_PORT.println("\nEND OF TRx::send_i\0");*/
  return 0;
  
}

/****************************************************************
  FUNCTION NAME:send_i
  FUNCTION     :Opens NAME file from the SD memory and transmits it
  INPUT        :Name of the file to transmit
  OUTPUT       :0, when error found while opening the file
				1, when suceeded
				2, when connection lost or if the picture exceeded the maximum transmission time
****************************************************************/
uint8_t TRX::send_i( char*name){
  uint8_t buff_rcv=0;
  int32_t rssi=0;
  digitalWrite(TX_PIN, 0);  // To let the DSPIC realize that the radio is about to start transmission
  DBG_OUTPUT_PORT.println("\n@TRx::send_i\0");
   
  // Attempt to make a connection to the remote server
  if ( !client.connect(IP_AP, http_port) ) {
    DBG_OUTPUT_PORT.println(String("Could not connect to") + IP_AP);
	delay(250);
    return 2;
  }

  client.setNoDelay(true);
  // Send a request
  client.println("GET /rcv HTTP/1.1");
  client.print("Host: ");
  client.println(IP_AP);
  client.println("Connection: close");
  client.println();  
  //delay(500);
	k=0;
  //wait for an answer
  //DBG_OUTPUT_PORT.print("Checkin if there is an answer:");
  /*while (client.available()){
	  buff_sendi[k++] = client.read();      
  }
  DBG_OUTPUT_PORT.write(buff_sendi, sizeof(buff_sendi)); // prints message received
  */
  //DBG_OUTPUT_PORT.print("\nName is: ");DBG_OUTPUT_PORT.write(name,11);
  delay(10);
  client.write((char*)name,11); // Sends the name of the picture 
  client.flush();
  rssi= WiFi.RSSI();  
  /*Name of image corrupted ERROR, resend*/
  uint32_t awaitrep= micros();
  /*while (micros()-awaitrep < rcvnametime ){ // evaluate if a request is received for rcvnametime microseconds
	  if (client.available()){
		  buff_rcv = client.read();
          DBG_OUTPUT_PORT.println(buff_rcv,HEX);
	  }	  
  }
  
  if (buff_rcv==0xA1) { //code for resending name of image
	client.write((char*)name,11); // Resends the name of the picture 
	client.flush();
	buff_rcv==0x00;
  }
  /*End of resending image*/
  
  
  /****************************************************/
  int i=0;
  int a=1,m=0,wr=0,n=1;
  uint16_t file[]={0};
  uint32_t times=0;
  const int pbuff = 2095  ; //2095
  char rpic[pbuff]= {0};
  unsigned long sizep=0;
  char endofimg[24]={0xFF,0xD9,0x00};
  if(!strcmp(name+2,"ENDPI.jpg"))   return 0;
  if(!strcmp(name,"ERRD_SD.jpg"))   return 0;
  if(SD.exists((char * )name)) {
	  DBG_OUTPUT_PORT.print(name); 
	  DBG_OUTPUT_PORT.print(" found and opened OK\n");
	}
  else {
		DBG_OUTPUT_PORT.write(name);DBG_OUTPUT_PORT.print(" does not exists in SD memory \n");
		client.write((char*)endofimg,3);
		client.flush();
	  }
  
  myFile = SD.open(name,FILE_READ);//read pict
  sizep= myFile.size();
  if(!myFile) DBG_OUTPUT_PORT.print("\nError opening ");   DBG_OUTPUT_PORT.println(name) ;
  times=millis();
  if (myFile) {
	  DBG_OUTPUT_PORT.println(String("\n File opened successfully: \n\r") + name ); 
      DBG_OUTPUT_PORT.println("\n------------Enviando------------");                      
      delay(500);
	  while (a){ 
			a++;
            if (m){      
              client.write((char*)rpic, sizeof(rpic));
              client.flush();         
              //DBG_OUTPUT_PORT.write((char*)rpic, sizeof(rpic));
            }      
            for(i=0;i<pbuff;i++){      // Llena primer buffer de tamano pbuff   
				
				if ((millis()-times)>PICT_TIME){
					DBG_OUTPUT_PORT.print("\nTIME EXCEEDED\n");
					rpic[0]=0xFF;rpic[1]=0xD9;rpic[2]=0x00;
					i=2;
					delay(200);			
					//send_httpcon("rset");	
					return 2;
				}
			
                rpic[i] = myFile.read();   //client.write((int)rpic[i]); 
				//DBG_OUTPUT_PORT.print(rpic[i],HEX); //client.flush();
                
				if (rpic[i]==0xFF)  {
					m=i+1;  
				}				
				
				if ((rpic[0]==0xFF)&& (rpic[1]==0xFF) && (rpic[2]==0xFF) && (rpic[3]==0xFF)) {
				//	DBG_OUTPUT_PORT.print("\n\n FIRST BYTE DIFFERENT FROM 0xFF \n");DBG_OUTPUT_PORT.println(rpic[0],HEX);DBG_OUTPUT_PORT.println(rpic[1],HEX);	DBG_OUTPUT_PORT.println(rpic[2],HEX);	DBG_OUTPUT_PORT.println(rpic[3],HEX);
					client.write((char*)endofimg,3);
					client.flush();
					//DBG_OUTPUT_PORT.println();DBG_OUTPUT_PORT.write((char*)endofimg,3);
					rpic[0]=0xFF;rpic[1]=0xD9;rpic[2]=0x00;
					i=2;
					delay(200);
				}	
				
				if ((rpic[i]==0x00)||(rpic[i]==0xFF)){
					//DBG_OUTPUT_PORT.print("\n RPIC[2] == 00 O FF  \n");
					//DBG_OUTPUT_PORT.println(i,DEC);
					if (rpic[i-1]==0xD9){
						//DBG_OUTPUT_PORT.print("\n rpic [1]=D9 \n");
						if (rpic[i-2]==0xFF){
							//DBG_OUTPUT_PORT.print("\n rpic [0] 0xFF  \n");
							client.write((char*)rpic, i+1);
							//client.flush();        
	  //                      DBG_OUTPUT_PORT.print(0xEE);
							/*client.write(0xEE);// fin de imagen*/
							//client.flush();
							/*client.write(0xEE);// fin de imagen*/
							//client.flush();
							DBG_OUTPUT_PORT.write((char*)rpic, i+1);
							DBG_OUTPUT_PORT.println("\nFINNNNNNNNNNNNNNNNNN");
							delay(2);
							//client.print(rpic[i],HEX);
							 a=0;
							 m=0;
							 break;
							 
						}
					}					
				}


				
				
				
             }// end of FOR
			 
			 
		  
          i=0;              
          delay(5);        //client.print("End of Image\n");
         }
		 
		if ((millis()-times)>PICT_TIME){
			DBG_OUTPUT_PORT.print("\nTIME EXCEEDED\n");
			return 2;
		}
  
        myFile.close();
        times=millis()-times;
        DBG_OUTPUT_PORT.print(String("Tiempo de transmision: ") + ((double)times/(double)1000)+ String(" s\n"));
		DBG_OUTPUT_PORT.print(String("Tamano: ")+ sizep + String(" Bytes\n"));
		DBG_OUTPUT_PORT.print(String("Datarate: ")+ (double)(((double)sizep*(float)8/(double)times)) + String(" Kbps\n"));
		DBG_OUTPUT_PORT.print(String("RSSI: ")+ (int32_t)(rssi) + String(" dBm\n\n"));
    }
    else {
        DBG_OUTPUT_PORT.println(String("error opening: ") + name);
        return 0;
    }
     
// open the file. note that only one file can be open at a time, so you have to close this one before opening another.
    myFile = SD.open("Data.txt", FILE_WRITE);  
    // if the file opened okay, write to it:
    if (myFile) {
      //DBG_OUTPUT_PORT.print("Writing to data.txt... ");
      myFile.print(String("Name of the picture: ") + name + String("\n") );
      myFile.print(String("Tiempo de transmision: ") + ((double)times/(double)1000000)+ String(" s\n"));
      myFile.print(String("Tamano: ")+ sizep + String(" Bytes\n"));
      myFile.print(String("Datarate: ")+ (double)(((double)sizep*(float)8000/(double)times)) + String(" Kbps\n"));
      myFile.print(String("RSSI: ")+ (int)(rssi) + String(" dBm\n"));
      //myFile.println(" ");
      myFile.close(); // close the file:
      //DBG_OUTPUT_PORT.println("done.");
    } else {
      // if the file didn't open, print an error:
      DBG_OUTPUT_PORT.println("error creating data.txt");
      myFile.close();   
    }     
  client.flush(); //Waits until all outgoing characters in buffer have been sent. 
  DBG_OUTPUT_PORT.println("\nEND OF TRx::send_i\0");
  return 1;
}

/****************************************************************
  FUNCTION NAME:trasmite
  FUNCTION     :Detects if RX pin is at 0
  INPUT        :none
  OUTPUT       :0, if PIN_RX = 1
				1, if PIN_RX = 0
****************************************************************/
int TRX::transmite(void){ // Si pin RX=0, transmitir
  DBG_OUTPUT_PORT.println("@TRx::transmite");
  int t=!(digitalRead(RX_PIN));
  return t;
}

/****************************************************************
  FUNCTION NAME:connectWiFiAP
  FUNCTION     :Attempt to connect to WiFi
  INPUT        :
  OUTPUT       :
****************************************************************/
void TRX::connectWiFiAP(){
  //pinMode(LED_PIN, OUTPUT);
  //digitalWrite(LED_PIN, LOW);
  //byte led_status = 0;  
  DBG_OUTPUT_PORT.println("\n@Connecting to Wifi -AP Mode-\n");
  
  delay(500);
  //  ESP.eraseConfig();
  delay(500);
  getIPEventHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event)
   {
     DBG_OUTPUT_PORT.println("\nEvent Handler New device connected, New IP: ");
     DBG_OUTPUT_PORT.println(WiFi.localIP());
  });

  disconnectedEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event)
  {
    DBG_OUTPUT_PORT.println("\nEvent Handler: Station disconnected\n");
  });
  
 // testeventhandler = WiFi.onSoftAPModeStationConnected()
    WiFi.setPhyMode(PhyMode);
  WiFi.setOutputPower(PhyMode_Power);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(APip,APgateway,APmask);  
  WiFi.softAP(WIFI_SSID, WIFI_PSK,channel);

  delay(600);  
  const char* phymodes[] = { "", "B", "G", "N" };
  DBG_OUTPUT_PORT.print("\nMode is: ");
  DBG_OUTPUT_PORT.println( phymodes[(int) WiFi.getPhyMode()]);  
  IPAddress myIP = WiFi.softAPIP(); 
  
  int rcount=0;
  while(!myIP){
    //digitalWrite(LED_PIN, led_status);
    //led_status ^= 0x01; 
    delay(500);
    rcount++;
    WiFi.softAPConfig(APip,APgateway,APmask);  
    if(rcount==10){
        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(APip,APgateway,APmask);  
        WiFi.softAP(WIFI_SSID, WIFI_PSK,channel);
        IPAddress myIP = WiFi.softAPIP();
        delay(1000);
    }
  }
  DBG_OUTPUT_PORT.print("AP IP address: ");
  DBG_OUTPUT_PORT.println(myIP);
  //digitalWrite(LED_PIN, HIGH);// Turn LED on when connected
  delay(600);
  //pinMode(LED_PIN, INPUT);
}

/****************************************************************
  FUNCTION NAME:connectWiFiSTA
  FUNCTION     :Attempt to connect to WiFi as STATION mode
  INPUT        :
  OUTPUT       :
****************************************************************/
void TRX::connectWiFiSTA(){  
  //pinMode(LED_PIN, OUTPUT);
  //digitalWrite(LED_PIN, LOW);
  //byte led_status = 0;  
  DBG_OUTPUT_PORT.println("\n@Connecting to Wifi -STA Mode-\n");
  getIPEventHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event)
   { DBG_OUTPUT_PORT.println("\nEvent Handler New device connected, New IP: ");
     DBG_OUTPUT_PORT.println(WiFi.localIP());
  });

  disconnectedEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event)
  { DBG_OUTPUT_PORT.println("\nEvent Handler: Station disconnected\n");
  });
  
  //------------------------------------
  // Set WiFi mode to station (client)
  WiFi.mode(WIFI_STA);
  // Initiate connection with SSID and PSK
  WiFi.begin(WIFI_SSID, WIFI_PSK,channel); 
  // Set PhyMode
  WiFi.setPhyMode(PhyMode);
  WiFi.setOutputPower(20);
  const char* phymodes[] = { "", "B", "G", "N" };
  DBG_OUTPUT_PORT.print("\nMode is: ");
  DBG_OUTPUT_PORT.println( phymodes[(int) WiFi.getPhyMode()]);
  //------------------------------------
  int c=0;
  // Blink LED while we wait for WiFi connection
  while ( WiFi.status() != WL_CONNECTED ) {
      //digitalWrite(LED_PIN, led_status);
      //led_status ^= 0x01; 
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
  DBG_OUTPUT_PORT.print("\nSTA IP address: ");
  DBG_OUTPUT_PORT.println(WiFi.localIP());
  //digitalWrite(LED_PIN, HIGH);// Turn LED on when connected
  delay(500);
  //pinMode(LED_PIN, INPUT);
}

/****************************************************************
  FUNCTION NAME:send_str
  FUNCTION     :Sends a string to destiny once stablished connection with server
  INPUT        :
  OUTPUT       : 0, when ACK not received
				 1, when ACK received
****************************************************************/
bool TRX::send_str(char * Strng, int sizeS) {
  
  DBG_OUTPUT_PORT.print("\n@trx::send_str \n");
  uint32_t times=0; 
  // Attempt to make a connection to the remote server
  /*if ( !client.connect(IP_AP, http_port) ) {
    DBG_OUTPUT_PORT.println(String("Could not connect to") + IP_AP);
	send_httpcon("rset");
	delay(250);
    return false;
  }
  */
  DBG_OUTPUT_PORT.print("\nString to send is: ");DBG_OUTPUT_PORT.write(Strng,sizeS);
  delay(10);
  client.write((char*)Strng,sizeS); // Sends the string
  client.flush();
  
  /*client.print(0xAA);
  client.flush(); //Waits until all outgoing characters in buffer have been sent. 
  DBG_OUTPUT_PORT.println("\nEND OF TRx::send_i\0");*/
  if(trx.rec_ack("ACK_OK")) return 1;
  return 0;

}

/****************************************************************
  FUNCTION NAME: rec_ack
  FUNCTION     : Expects a string STRNG from server
  INPUT        :
  OUTPUT       : 0, when ACK not received
				 1, when ACK received
****************************************************************/
bool TRX::rec_ack(char * Strng) {
  int wait=50;
  DBG_OUTPUT_PORT.print("\n@trx::rec_ack\n");
  // Attempt to make a connection to the remote server
  /*if ( !client.connect(IP_AP, http_port) ) {
    DBG_OUTPUT_PORT.println(String("Could not connect to") + IP_AP);
	send_httpcon("rset");
	delay(250);
    return false;
  }
  */
  char buffer_rec[7]={0};
  client.setNoDelay(true);
  
  while (!client.available() && (--wait>0)){      
    delay(50); // waits till a message comes
    DBG_OUTPUT_PORT.print("wt");    
  }
  
  if(!wait){
	DBG_OUTPUT_PORT.println("\nTime too long to obtain an answer");
	return 0;  
  } 
  
  for(int i=0;i<=7;i++){
	 if(client.available())  buffer_rec[i]=client.read();         
  }       
   
  DBG_OUTPUT_PORT.println(String("Buffer received is: ") + buffer_rec);
  if(!strcmp(buffer_rec,Strng)) return 1; //if buffer received is as desired	
  DBG_OUTPUT_PORT.print(Strng + String(" <- does not match the received content-> ")+ buffer_rec);
  
  /*client.print(0xAA);
  client.flush(); //Waits until all outgoing characters in buffer have been sent. 
  DBG_OUTPUT_PORT.println("\nEND OF TRx::send_i\0");*/
  return 0;

}

/****************************************************************
  FUNCTION NAME: rec_buf
  FUNCTION     : Expects a string STRNG of 6 characters from server
  INPUT        :
  OUTPUT       : Buffer received
****************************************************************/
bool TRX::rec_buf(char * buffer_rec) {
  
  DBG_OUTPUT_PORT.print("\n@trx::rec_buf \n");
  int wait=5;
  // Attempt to make a connection to the remote server
  /*if ( !client.connect(IP_AP, http_port) ) {
    DBG_OUTPUT_PORT.println(String("Could not connect to") + IP_AP);
	send_httpcon("rset");
	delay(250);
    return false;
  }
  */  
  //char buffer_rec[7]={0};
  client.setNoDelay(true);
  if ((wait--) && (!client.available())){      
    delay(1); // waits till a message comes
    DBG_OUTPUT_PORT.print("wt");    
	DBG_OUTPUT_PORT.println(wait);   
  }  
  else{
	for(int i=0;i<=7;i++){
		if(client.available())  buffer_rec[i]=client.read();         
	}
	return 1;    
  }  
   return 0;
}

/****************************************************************
  FUNCTION NAME: syncRTC
  FUNCTION     : Creates a string with the Year,month, date, hour, minute, second of the current device
  INPUT        :  None	
  OUTPUT       :  char * rtc
****************************************************************/
bool Func::read_RTC(char *rtc){
	
	Wire.begin(PIN_SDA,PIN_SCL);
	Wire.setClock(I2C_FRQ);
	RTC.begin();
	RTC.clearINTStatus(); // to clear all interrupts at the beginning
	delay(500);
	DBG_OUTPUT_PORT.print("\n@Read_RTC\n");
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
		Wire.begin(PIN_SDA,PIN_SCL);
		delay(500);
		RTC.begin();
		RTC.clearINTStatus(); // to clear all interrupts at the beginning
		RTC.enableInterrupts(EverySecond); 
		delay(500);
	}
	
	rtc[0]=(now.year()&0xFF00)>>8;
	rtc[1]=now.year()&0x00FF;
	rtc[2]=now.month();
	rtc[3]=now.date();
	rtc[4]=now.hour();
	rtc[5]=now.minute();
	rtc[6]=now.second();

	//DBG_OUTPUT_PORT.write((char*)rtc,7);
	
	return 1;
	
} 

/****************************************************************
  FUNCTION NAME: recRTC
  FUNCTION     : Receives the Year,month, date, hour, minute, second of the current device from the server and syncs this device with the time and date received
  INPUT        : char * buff_time
  OUTPUT       : none
****************************************************************/
bool Func::set_RTC(char * buff_time){
   DBG_OUTPUT_PORT.print("\nSET_RTC\n");
   uint16_t year = buff_time[0]*256+ buff_time[1];
   uint8_t month = buff_time[2];	
   uint8_t date= buff_time[3];
   uint8_t hour = buff_time[4];
   uint8_t minute= buff_time[5];
   uint8_t second = buff_time[6];
    
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

/****************************************************************
  FUNCTION NAME:check_connection
  Function called by the station only
****************************************************************/
bool Func::check_connection(){
  DBG_OUTPUT_PORT.println("\n\n@Func::check_connection\0");
   if (!client.connect(IP_AP,http_port)) {
      DBG_OUTPUT_PORT.println(String("Could not connect to") + IP_AP);
      DBG_OUTPUT_PORT.println("Client disconnected");
      DBG_OUTPUT_PORT.println();
      
	  trx.send_httpcon("rset");
	  
      // Close socket and wait for disconnect from WiFi
      client.stop();
      if ( WiFi.status() != WL_DISCONNECTED ) {
        WiFi.disconnect();
      }
      DBG_OUTPUT_PORT.println("WiFi Disconnected");
      
      // Turn off LED
      //digitalWrite(LED_PIN, LOW);
      
      // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
        if (!volume.init(card)) {
          DBG_OUTPUT_PORT.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
        }
        else {
          DBG_OUTPUT_PORT.println("\nFiles found on the card (name, date and size in bytes): ");
          //root.openRoot(volume);
          // list all files in the card with date and size
          //root.ls(LS_R | LS_DATE | LS_SIZE);
        }
      
      // Do nothing
      DBG_OUTPUT_PORT.println("Not connected. Finished connection");
	  
	  return 0;
	  
	  while(1){
		  delay(1000);
	  }

    }  
	
	DBG_OUTPUT_PORT.println("Connection OK");
	return 1;
  }
  
 /****************************************************************
  FUNCTION NAME:init_RTC
  FUNCTION     : Initializes RTC and deletes its alarms
  INPUT        :
  OUTPUT       :
****************************************************************/
void Func::init_RTC(){
	//config RTC
	pinMode(PIN_SDA,OUTPUT);
	digitalWrite(PIN_SDA,LOW);
	delay(10);
	digitalWrite(PIN_SDA,HIGH);
	delayMicroseconds(3);
	digitalWrite(PIN_SDA,LOW);
    delay(500);
	Wire.begin(PIN_SDA,PIN_SCL);
	Wire.setClock(I2C_FRQ);
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
	  Wire.begin(PIN_SDA,PIN_SCL);
	  delay(500);
	  RTC.begin();
	  RTC.clearINTStatus(); // to clear all interrupts at the beginning
	  RTC.enableInterrupts(EverySecond); 
	  delay(500);
	  ESP.deepSleep(0); 
	}
	
} 

/****************************************************************
  FUNCTION NAME:	config RTC
  FUNCTION     : 	Configs RTC to restart for a period given by everytime
  INPUT        :
  OUTPUT       :
****************************************************************/
void Func::config_RTC(uint8_t everytime){
	
	const char * Periodtime[]={"","Everysecond","EveryMinute","EveryHour"};
	DBG_OUTPUT_PORT.print("\nConfig RTC to: "); DBG_OUTPUT_PORT.println(Periodtime[(int)everytime]);
	//config RTC
	Wire.begin(PIN_SDA,PIN_SCL);
	Wire.setClock(I2C_FRQ); //200KHz
	RTC.begin();
	RTC.clearINTStatus(); // to clear all interrupts at the beginning
	delay(500);
	RTC.clearINTStatus(); // to clear all interrupts at the beginning
	RTC.enableInterrupts(everytime); 
	//end config RTC
	
	//Read initial RTC Value
	DBG_OUTPUT_PORT.print("Current Time is: ");
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
	
   if(now.year()==2165){
	  DBG_OUTPUT_PORT.println("RTC ERROR");
	  delay(500);
	  Wire.begin(PIN_SDA,PIN_SCL);
	  delay(500);
	  RTC.begin();
	  RTC.clearINTStatus(); // to clear all interrupts at the beginning
	  RTC.enableInterrupts(EverySecond); 
	  delay(500);
	  //ESP.deepSleep(0); 
	}
	
} 
void Func::config_RTC(uint8_t hour,uint8_t minute,uint8_t second){
	
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
	  Wire.begin(PIN_SDA,PIN_SCL);
	  delay(500);
	  RTC.begin();
	  RTC.clearINTStatus(); // to clear all interrupts at the beginning
	  RTC.enableInterrupts(EverySecond); 
	  delay(500);
	}
	
} 

/****************************************************************
  FUNCTION NAME:	Sleep
  FUNCTION     : 	Sets the node to Sleep
****************************************************************/
void Func::Sleep(){
	DateTime now= RTC.now();
	long wakeup = now.get()+SLEEP_TIME;
	DateTime next(wakeup); 
	func.config_RTC(next.hour(),next.minute(),next.second()); //-----------------------------------
	ESP.deepSleep(0);
	
}

/****************************************************************
  FUNCTION NAME: init_BatMntr()
  FUNCTION     : Initializes RTC and deletes its alarms
  INPUT        :
  OUTPUT       :
****************************************************************/
void Func::init_BattM(){
  /*#ifdef ESP8266
  Wire.begin(5,0); 
  #else
  Wire.begin();
  #endif
  */
  Wire.begin(PIN_SDA,PIN_SCL); 
  DBG_OUTPUT_PORT.println("MAX17043 Config");
  
  batteryMonitor.reset();
  batteryMonitor.quickStart();
  delay(1000);
  
  float cellVoltage = batteryMonitor.getVCell();
  DBG_OUTPUT_PORT.print("Voltage:\t\t");
  DBG_OUTPUT_PORT.print(cellVoltage, 4);
  DBG_OUTPUT_PORT.println("V");

  float stateOfCharge = batteryMonitor.getSoC();
  DBG_OUTPUT_PORT.print("State of charge:\t");
  DBG_OUTPUT_PORT.print(stateOfCharge);
  DBG_OUTPUT_PORT.println("%");
}  
  
/****************************************************************
  FUNCTION NAME: BattLevel()
  FUNCTION     : Initializes RTC and deletes its alarms
  INPUT        :
  OUTPUT       : 1 when Level is Higher than actual BatteryLevel;
****************************************************************/
bool Func::BattLevel(float LowLevel){
	
	float cellVoltage = batteryMonitor.getVCell();
	DBG_OUTPUT_PORT.print("Voltage:\t\t");
    DBG_OUTPUT_PORT.print(cellVoltage, 4);
	DBG_OUTPUT_PORT.println("V");

	if((cellVoltage-LowLevel)>0) return 1;
	else { 
		DBG_OUTPUT_PORT.println("LOW BATTERY, Set to Wakeup Everyhour");
		config_RTC(EveryHour); // Configs RTC to sleep for one hour		
		ESP.deepSleep(0); 
		return 0;
	}

	
}  
  
/****************************************************************
  FUNCTION NAME:rset
 Function call byt the station
****************************************************************/
void Func::rset(){
	func.config_RTC(EverySecond);
	delay(5);
	ESP.deepSleep(0);
}

/****************************************************************
  FUNCTION NAME:initSD
  FUNCTION     :Initializes SD card
  INPUT        :
  OUTPUT       :
****************************************************************/
bool SDf::initSD(){
/*Initialialize SD*/
  if (card.init(SPI_HALF_SPEED, chipSelect)){
     SD.begin(chipSelect);
     DBG_OUTPUT_PORT.println("\nSD Card initialized.");
     hasSD = true;
	 return 1;
  }
  else {
	  DBG_OUTPUT_PORT.println("\nERROR reading SD");
	  return 0;
	  
  } 
}

/****************************************************************
  FUNCTION NAME:  nextnameSD
  FUNCTION     :  Reads metadata.dat file to search for the next name that is available
  INPUT        :  name of file to search in
  OUTPUT       :  char* name of the picture 
                  0     when no new picture
****************************************************************/
long SDf::nextname(char* file, char * picname){
  DBG_OUTPUT_PORT.println("SDf::nextname");
  digitalWrite(TX_PIN, 0);

  //char picname[11]={0};
  int c=0,data=0,counter=0,readerror=0;
  long pos; //long=4Bytes
  File dataFile = SD.open(file,FILE_READ);  
  while(!dataFile){ 
	  while (readerror<15) {
		  dataFile = SD.open(file,FILE_READ);
		  readerror++;
		  DBG_OUTPUT_PORT.print("\nAttempting to read again the file\n");
	  }
	  break;
  }
  
	  if (dataFile){          // if the file is available, write to it:      
		  DBG_OUTPUT_PORT.println(file + String(" opened"));
		  data = dataFile.read(); //PRIMER CARACTER     
		  while((data!='U')&& (dataFile.available())){
				data = dataFile.read();
				//DBG_OUTPUT_PORT.println("\nHaven't find U yet.., still oo");
				//DBG_OUTPUT_PORT.write(data);
				delayMicroseconds(10);	
		  }      
		  pos= dataFile.position();
		  DBG_OUTPUT_PORT.print("Position after U: ");DBG_OUTPUT_PORT.println(pos);
		  if (data=='U') {
			DBG_OUTPUT_PORT.println("U found");
			data=dataFile.read();//reading white space
			for (c=0;c<11;c++){  //The name of the file has 11 characters            
				delay(10);
				//DBG_OUTPUT_PORT.write(dataFile.read());  
				picname[c] =dataFile.read();                                                                    
			}    
			//DBG_OUTPUT_PORT.write(picname);
			//dataFile.seek(pos-1);
			dataFile.close();    // Close file after finding the new name             
		  }  else if (data=='S') {
			DBG_OUTPUT_PORT.println("S found");
			dataFile.close();
			//return 0;
		  }
		  
		  //Write S instead of U next to the namefile
		  File dataFile = SD.open(file,FILE_WRITE);  
		  dataFile.seek(pos-1);
		  dataFile.print("S");   
		  dataFile.close();
		  //DBG_OUTPUT_PORT.println("\nEnd of nextname");
		  //return &picname[0];
	  } else { // if the file isn't open, pop up an error:
		  DBG_OUTPUT_PORT.println(String("error opening ") + file);
		 // return 0;
	  }  
		//digitalWrite(TX_PIN, 0);
}

/****************************************************************
  FUNCTION NAME:  confirmsent
  FUNCTION     :  Reads metadata.dat file to search for the next name that is available
  INPUT        :  name of picture sent
  OUTPUT       :  1 when success
                  0 when not
****************************************************************/
/* int  SDf::confirmsent(char*file,char* pictsent){
  char * readval={};
  char c=0;
  File dataFile = SD.open(file,FILE_WRITE);  
  while(readval!=pictsent){
      
      //dataFile.seek(pos-1);
      //dataFile.print("S");   
      
  }
  DBG_OUTPUT_PORT.println("pictsent name found in metadata file, success");
  dataFile.close();    
  
  
 }
 */


TRX trx;
SDf sdf;
Func func;





