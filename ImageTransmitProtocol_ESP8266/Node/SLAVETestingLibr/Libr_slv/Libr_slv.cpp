
#include "Libr_slv.h"

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
IPAddress STAgateway(192,168,0,100);
IPAddress STAmask(255,255,255,0);

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
char* nof="N100001.jpg";
int32_t rssi= 0;

// Handlers
ESP8266WiFiGenericClass LeeIP;
WiFiEventHandler getIPEventHandler, disconnectedEventHandler;

//RTC
DS3231 RTC; //Create the DS3231 object
DateTime now;

/****************************************************************
  FUNCTION NAME:send_httpcon
  FUNCTION     :sends a GET name request to the server
  INPUT        :none
  OUTPUT       :noone
****************************************************************/
void TRX::send_httpcon( char* name){
	digitalWrite(TX_PIN, 0); 
	DBG_OUTPUT_PORT.println("\n@TRx::send_httpconn\0");
   
	// Attempt to make a connection to the remote server
	if ( !client.connect(http_ap, http_port) ) {
		DBG_OUTPUT_PORT.println(String("Could not connect to") + http_ap);
		return;
	}
	client.setNoDelay(true);
	// Send a request
	client.print("GET ");
	client.print(String("/")+ name);
	client.println(" HTTP/1.1");
	client.print("Host: ");
	client.println(http_ap);
	client.println("Connection: close");
	client.println();  
	
	DBG_OUTPUT_PORT.println("GET ");
	DBG_OUTPUT_PORT.print(String("/")+ name);
	DBG_OUTPUT_PORT.print(" HTTP/1.1");
	DBG_OUTPUT_PORT.print("Host: ");
	DBG_OUTPUT_PORT.println(http_ap);
	DBG_OUTPUT_PORT.println("Connection: close");
	DBG_OUTPUT_PORT.println();  
  
}

/****************************************************************
  FUNCTION NAME:send_i
  FUNCTION     :Abre el archivo name desde la memoria SD y lo transmite
  INPUT        :Nombre de la imagen a transmitir
  OUTPUT       :none
****************************************************************/
uint8_t TRX::send_i( char*name){
  uint8_t buff_rcv=0, error = 0;
  int32_t rssi=0;
  digitalWrite(TX_PIN, 0); 
  DBG_OUTPUT_PORT.println("\n@TRx::send_i\0");
   
  // Attempt to make a connection to the remote server
  if ( !client.connect(http_ap, http_port) ) {
    DBG_OUTPUT_PORT.println(String("Could not connect to") + http_ap);
	//send_httpcon("rset");
	delay(250);
    return 2;
  }
  client.setNoDelay(true);
  // Send a request
  client.println("GET /rcv HTTP/1.1");
  client.print("Host: ");
  client.println(http_ap);
  client.println("Connection: close");
  client.println();  
  
  k=0;
  delay(10);
  client.write((char*)name,11); // Sends the name of the picture 
  client.flush();
  
  //**TODO** Implementar el resend
  /*Name of image corrupted ERROR, resend*/
  /*uint32_t awaitrep= micros();
  
  
  while (micros()-awaitrep < rcvnametime ){ // evaluate if a request is received for rcvnametime microseconds
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
  
  rssi= WiFi.RSSI();  
  /****************************************************/
  int i=0;
  int a=1,m=0,wr=0,n=1;
  uint16_t file[]={0};
  unsigned long sizep=0;
  uint32_t times=0;
  const int pbuff = 2095  ; //2095
  char rpic[pbuff]= {0};
  char endofimg[24]={0xFF,0xD9,0x00};
  //sdf.initSD();
  if(!strcmp(name,"ENDOFPI.jpg"))   return 0;
  if(!strcmp(name,"ERRD_SD.jpg"))   return 0;
  if(SD.exists((char * )name)) {DBG_OUTPUT_PORT.print(name); DBG_OUTPUT_PORT.print(" found and opened OK\n");}
  else {
		DBG_OUTPUT_PORT.write(name);DBG_OUTPUT_PORT.print(" does not exists in SD memory \n");
		client.write((char*)endofimg,3);
		client.flush();
	  }
  
  myFile = SD.open(name,FILE_READ);//read pict
  sizep= myFile.size();
  if(!myFile) DBG_OUTPUT_PORT.print("\nError opening ");  // DBG_OUTPUT_PORT.println(name) ;
  times=micros();
  if (myFile) {
	  //DBG_OUTPUT_PORT.println(String("\nFile opened successfully: ") + name ); 
      DBG_OUTPUT_PORT.println("\n------------Enviando------------");                      
      while (a){ 
			a++;
            if (m){      
              client.write((char*)rpic, sizeof(rpic));
              client.flush();         
              //DBG_OUTPUT_PORT.write((char*)rpic, sizeof(rpic));
            }      
            for(i=0;i<pbuff;i++){      // Llena primer buffer de tamano pbuff   
				
				if ((micros()-times)>TIME_OUT_PICT){
					DBG_OUTPUT_PORT.print("\nTIME EXCEEDED\n");
					DBG_OUTPUT_PORT.println(micros()-times);
					rpic[0]=0xFF;rpic[1]=0xD9;rpic[2]=0x00;
					i=2;
					delay(200);			
					//send_httpcon("rset");	
					return 2;
				}
			
                rpic[i] = myFile.read();  
                
				if (rpic[i]==0xFF)  {
					m=i+1;  
				}				
				
				if ((rpic[0]==0xFF)&& (rpic[1]==0xFF) && (rpic[2]==0xFF) && (rpic[3]==0xFF)) {
					client.write((char*)endofimg,3);
					client.flush();
					//DBG_OUTPUT_PORT.println();DBG_OUTPUT_PORT.write((char*)endofimg,3);
					rpic[0]=0xFF;rpic[1]=0xD9;rpic[2]=0x00;
					i=2;
					delay(200);
				}						

				if((rpic[i]==0x00)||(rpic[i]==0xFF)){
					if ((rpic[i-1]==0x00)||(rpic[i-1]==0xFF)){
						if (rpic[i-2]==0xD9){
							if (rpic[i-3]==0xFF){
								client.write((char*)rpic, i+1);
								client.flush();
								//client.write(0xEE);// fin de imagen
								//client.write(0xEE);// fin de imagen
								DBG_OUTPUT_PORT.println("\nEOF");
								delay(2);
								 a=0;
								 m=0;
								 break;								 
							}
						}					
					}
				}								
             }// end of FOR		  
          i=0;              
          delay(5);  
         }
  
        myFile.close();
        times=micros()-times;
        
		//Wait for Master response for Image Complete
		uint32_t awaitrep= millis();
		error = 1;
		DBG_OUTPUT_PORT.print("Waiting for response: ");
		while(millis()-awaitrep < RESPONSE_TIME){
			delay(1);
			if(client.available()){
				buff_rcv = client.read();
				if(buff_rcv == 0xB2){
					DBG_OUTPUT_PORT.println("Image received in Master");
					error = 0;
					break;
				}
			}
		}
		
        DBG_OUTPUT_PORT.print(String("Tiempo de transmision: ") + ((double)times/(double)1000000)+ String(" s\n"));
		DBG_OUTPUT_PORT.print(String("Tamano: ")+ sizep + String(" Bytes\n"));
		DBG_OUTPUT_PORT.print(String("Datarate: ")+ (double)(((double)sizep*(float)8000/(double)times)) + String(" Kbps\n"));
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
	  if(error)
		  myFile.println("File was not received by Master");
      //myFile.println(" ");
      myFile.close(); // close the file:
      //DBG_OUTPUT_PORT.println("done.");
    } else {
      // if the file didn't open, print an error:
      DBG_OUTPUT_PORT.println("error creating data.txt");
      myFile.close();   
    }   
	
	
     
  /*  // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
    if (!volume.init(card)) {
        DBG_OUTPUT_PORT.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
        return;
    }
    else {
        DBG_OUTPUT_PORT.println("\nFiles found on the card (name, date and size in bytes): ");
        root.openRoot(volume);       // list all files in the card with date and size
        root.ls(LS_R | LS_DATE | LS_SIZE);
    }  
    */
    //digitalWrite(TX_PIN,0);
  
	/*****/
 
	//DBG_OUTPUT_PORT.println("END OF TRx::send_i\0");
	if(error)
		return 3;
	else
		return 1;
}

/****************************************************************
  FUNCTION NAME:trasmite
  FUNCTION     :Transmite si el pin RX esta en 0
  INPUT        :none
  OUTPUT       :0, si pin RX =1 
				1, si PIN RX =0
****************************************************************/
int TRX::transmite(void){ // Si pin RX=0, transmitir
  DBG_OUTPUT_PORT.println("@TRx::transmite");
  int t=(!digitalRead(RX_PIN));
  return t;
}

/****************************************************************
  FUNCTION NAME:connectWiFiSTA
  FUNCTION     :Configuration as Station. Attempt to connect to WiFi
  INPUT        :
  OUTPUT       :
****************************************************************/
void TRX::connectWiFiSTA(){  
  char smode=0;
  byte led_status = 0;  
  DBG_OUTPUT_PORT.println("@Connecting to Wifi -STA Mode-");
   
  /**Read mode with serial input**/
  /*DBG_OUTPUT_PORT.println("Set the mode");
  while(!DBG_OUTPUT_PORT.available());
  smode=DBG_OUTPUT_PORT.read();
  if (smode=='b'){
	WiFi.setPhyMode(WIFI_PHY_MODE_11B);
	WiFi.setOutputPower(80);	  
  }else if (smode=='g'){
	WiFi.setPhyMode(WIFI_PHY_MODE_11G);
	WiFi.setOutputPower(17);	  
  } else if (smode=='n'){
	WiFi.setPhyMode(WIFI_PHY_MODE_11N);
	WiFi.setOutputPower(14);	  
  }else {
	DBG_OUTPUT_PORT.println("Mode not recognized, set to B");  
*/	WiFi.setPhyMode(WIFI_PHY_MODE_11B);
	WiFi.setOutputPower(20);	  
 /* } 
  /****************************/
      
  /**Set channel***/
  int channel=11;
 /* DBG_OUTPUT_PORT.println("\nSet channel");
  while(!DBG_OUTPUT_PORT.available());
  channel=DBG_OUTPUT_PORT.read()-0x30;
  if(DBG_OUTPUT_PORT.available()){
	  channel= channel*10 +(DBG_OUTPUT_PORT.read()-0x30);	  
  }
   DBG_OUTPUT_PORT.print("\nChannel is ");DBG_OUTPUT_PORT.println(channel);  
  /********/
  
  //------------------------------------
  // Set WiFi mode to station(client)
  WiFi.mode(WIFI_STA);
  // StaticIP, gateway and subnetmask
  WiFi.config(STAip,STAgateway,STAmask);  
  // Initiate connection with SSID and PSK
  WiFi.begin(WIFI_SSID, WIFI_PSK,channel);    
  //------------------------------------
  int c=0;
  // Blink LED while we wait for WiFi connection
  while (WiFi.status() != WL_CONNECTED){
      //digitalWrite(LED_PIN, led_status);
      //led_status ^= 0x01; 
      delay(300);
      if (c==50){
        DBG_OUTPUT_PORT.println("\nTrying to reconnect...");
        WiFi.disconnect();		
		WiFi.setPhyMode(WIFI_PHY_MODE_11B);
		WiFi.setOutputPower(20);
	    WiFi.mode(WIFI_STA);
		WiFi.config(STAip,STAgateway,STAmask);  
		WiFi.begin(WIFI_SSID, WIFI_PSK,channel);  
        WiFi.reconnect();
		c=0;
      }
      c++;
      DBG_OUTPUT_PORT.print(c);
  }  
  IPAddress myIP = WiFi.localIP(); 
  // Turn LED on when we are connected
  digitalWrite(LED_PIN, HIGH);
  //WiFi.setPhyMode(WIFI_PHY_MODE_11B);
  //WiFi.setOutputPower(20);
  const char* phymodes[] = { "", "B", "G", "N" };
  char gmode=WiFi.getPhyMode();
  DBG_OUTPUT_PORT.print("\nMode is: ");
  DBG_OUTPUT_PORT.println( phymodes[(int) gmode]);
  DBG_OUTPUT_PORT.print("STA IP address: ");
  DBG_OUTPUT_PORT.println(WiFi.localIP());
}

/****************************************************************
  FUNCTION NAME:check_connection
  Function called by the station only to check that the connection to the server is ok
****************************************************************/
bool Func::check_connection(){
  DBG_OUTPUT_PORT.println("\n\n@Func::check_connection\0");
   if (!client.connect(http_ap,http_port)) {
      DBG_OUTPUT_PORT.println(String("Could not connect to") + http_ap);
      DBG_OUTPUT_PORT.println("Client disconnected from the server");
      DBG_OUTPUT_PORT.println();
      
	  /*trx.send_httpcon("rset");
	  
      // Close socket and wait for disconnect from WiFi
      client.stop();
      if ( WiFi.status() != WL_DISCONNECTED ) {
        WiFi.disconnect();
      }
      DBG_OUTPUT_PORT.println("WiFi Disconnected");
      
      // Turn off LED
      digitalWrite(LED_PIN, LOW);
      
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
	  */
	  return 0;

    }  
	
	DBG_OUTPUT_PORT.println("Connection OK");
	return 1;
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
	Wire.setClock(I2C_FRQ);
	RTC.begin();
	RTC.clearINTStatus(); // to clear all interrupts at the beginning
	delay(500);
	DBG_OUTPUT_PORT.print("\nInit RTC\n");
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
	
   if(now.year()==2165){
	  DBG_OUTPUT_PORT.println("RTC ERROR");
	  Wire.begin(PIN_SDA,PIN_SCL);
	  Wire.setClock(I2C_FRQ);
	  RTC.begin();
	  RTC.clearINTStatus(); // to clear all interrupts at the beginning
	  if(now.year()==2165){
		DBG_OUTPUT_PORT.println("RTC ERROR");
		Wire.begin(PIN_SDA,PIN_SCL);
		Wire.setClock(I2C_FRQ);
		RTC.begin();
		RTC.clearINTStatus(); // to clear all interrupts at the beginning
		if(now.year()==2165){
			trx.send_i("ERR_RTC.jpg");
			func.Sleep();
		}
	  }
   }
} 
  
/****************************************************************
  FUNCTION NAME:	config RTC
  FUNCTION     : 	Configs RTC to restart for a period given by "everytime" or "hour,minute,second"
  INPUT        :
  OUTPUT       :
****************************************************************/
void Func::config_RTC(uint8_t everytime){
	
	const char * Periodtime[]={"","Everysecond","EveryMinute","EveryHour"};
	DBG_OUTPUT_PORT.print("\nConfig RTC to wake at: "); DBG_OUTPUT_PORT.println(Periodtime[(int)everytime]);
	//config RTC
	/*Wire.begin(PIN_SDA,PIN_SCL);
	Wire.setClock(I2C_FRQ); //120KHz
	RTC.begin();
	RTC.clearINTStatus(); // to clear all interrupts at the beginning
	delay(500);*/
	//end config RTC
	RTC.clearINTStatus(); // to clear all interrupts at the beginning
	RTC.enableInterrupts(everytime); 
		
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
	  Wire.begin(PIN_SDA,PIN_SCL);
	  Wire.setClock(I2C_FRQ);
	  RTC.begin();
	  RTC.clearINTStatus(); // to clear all interrupts at the beginning
	  if(now.year()==2165){
		DBG_OUTPUT_PORT.println("RTC ERROR");
		Wire.begin(PIN_SDA,PIN_SCL);
		Wire.setClock(I2C_FRQ);
		RTC.begin();
		RTC.clearINTStatus(); // to clear all interrupts at the beginning
		if(now.year()==2165){
			trx.send_i("ERR_RTC.jpg");
			func.Sleep();
		}
	 }
   }
	
} 
void Func::config_RTC(uint8_t hour,uint8_t minute,uint8_t second){
	
	DBG_OUTPUT_PORT.print("\nConfig RTC to wake at: "); DBG_OUTPUT_PORT.print(hour + String(":")); DBG_OUTPUT_PORT.print(minute + String(":")); DBG_OUTPUT_PORT.println(second);
	/*//config RTC
	Wire.begin(PIN_SDA,PIN_SCL);
	Wire.setClock(I2C_FRQ); //120KHz
	RTC.begin();
	RTC.clearINTStatus(); // to clear all interrupts at the beginning
	delay(500);
	*/
	RTC.clearINTStatus(); // to clear all interrupts at the beginning
	RTC.enableInterrupts(hour,minute,second); 
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
	  Wire.begin(PIN_SDA,PIN_SCL);
	  Wire.setClock(I2C_FRQ);
	  RTC.begin();
	  RTC.clearINTStatus(); // to clear all interrupts at the beginning
	  if(now.year()==2165){
		DBG_OUTPUT_PORT.println("RTC ERROR");
		Wire.begin(PIN_SDA,PIN_SCL);
		Wire.setClock(I2C_FRQ);
		RTC.begin();
		RTC.clearINTStatus(); // to clear all interrupts at the beginning
		if(now.year()==2165){
			if(!trx.send_i("ERR_RTC.jpg"))	DBG_OUTPUT_PORT.println("Sending ERR_RTC.jpg error, not connected to server, going to sleep ");
			func.Sleep();
		}
	  }
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
  FUNCTION NAME: syncRTC
  FUNCTION     : Reads RTC time and send it to the Server for Synchronization
****************************************************************/
bool TRX::syncRTC(){
	
	Wire.begin(PIN_SDA,PIN_SCL);
	Wire.setClock(I2C_FRQ);
	RTC.begin();
	RTC.clearINTStatus(); // to clear all interrupts at the beginning
	delay(500);
	DBG_OUTPUT_PORT.print("\nSync RTC\n");
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
	
    if(now.year()==2165){
	  DBG_OUTPUT_PORT.println("RTC ERROR");
	  func.config_RTC(EverySecond);
	  if(now.year()==2165){
		DBG_OUTPUT_PORT.println("RTC ERROR");
		func.config_RTC(EverySecond);
		if(now.year()==2165){			
			trx.send_i("ERR_RTC.jpg");
			func.Sleep();
		}
	  }
	}
	
	send_httpcon("srtc");
	
	char rtc[7]={0};
	rtc[0]=(now.year()&0xFF00)>>8;
	rtc[1]=now.year()&0x00FF;
	rtc[2]=now.month();
	rtc[3]=now.date();
	rtc[4]=now.hour();
	rtc[5]=now.minute();
	rtc[6]=now.second();
	
	client.write((char*)rtc,7); client.flush();
	DBG_OUTPUT_PORT.write((char*)rtc,7);
	
	return 1;
	
} 

/****************************************************************
  FUNCTION NAME:rset
 Function call byt the station to reset the server
****************************************************************/
void Func::rset(){
    // Attempt to make a connection to the remote server
  if ( !client.connect(http_ap, http_port) ) {
    DBG_OUTPUT_PORT.println(String("Could not connect to") + http_ap);
    trx.connectWiFiSTA();
    return;
  }
  // Send a request
  DBG_OUTPUT_PORT.print("RESET\n");
  client.println("GET /rset HTTP/1.1");
  client.print("Host: ");
  client.println(http_ap);
  client.println("Connection: close");
  client.println();  
  delay(5000);
  WiFi.disconnect();
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
       // if(dataFile.available()){
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
      } else if (data=='S') {
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


TRX trx;
SDf sdf;
Func func;





