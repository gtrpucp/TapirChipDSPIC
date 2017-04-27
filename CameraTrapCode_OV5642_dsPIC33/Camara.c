#include <p33EP256GP504.h>

#include "Camara.h"
#include "ds3231.h"
#include "highlevel_i2c2.h"
#include "stdlib.h"
#include "user.h"

//#define DEBUG

/**
 * @brief   Estatic variables defined here
 * @{
 */
FATFS FatFs;		/* FatFs work area needed for each volume */
FIL Fil;			/* File object needed for each open file */
FIL Fil2;           /* File object needed for each open file */
FIL Fil3;           /* File object needed for each open file */
FIL Fil4;
FRESULT Result;     /* Variable to store errors while using the SD card*/
FRESULT Result1;
uint16_t tiemposMS[1024];
uint16_t tiemposMSIndex = 0;
uint32_t tiempoMSTotal = 0;
double tiempoMSRes = 0.00;

uint8_t salida[128];
char buff_name[5];
uint8_t flag_UartComando = 0;

uint8_t useDma = 0;

uint8_t buff_DummyRx[2];

//#define CAM_BUFF    512
//#define CAM_BUFF    1024
//#define CAM_BUFF    2048
#define CAM_BUFF    4096
uint8_t buff_CamCapture[CAM_BUFF] __attribute__((far,section("samplebuffer"),address(0x2000)));

uint16_t index_CamCapture;
uint16_t bw;
uint16_t bw1;
uint16_t bw2;
uint16_t bw3;
uint8_t file_CamNombre[] = "N100000.jpg";
uint8_t file_sensor[] = "N1sensor.txt";
uint8_t file_metadata[] = "METADATA.dat";
/** @}*/

/**
 * @brief   Volatile variable for interruptions
 * @{
 */
volatile uint16_t ffs_10ms_timer;
volatile uint8_t flag_CaptureReady;
volatile uint8_t smCamara_Capture = 0;
volatile uint8_t Camara_FrameCount;
volatile uint8_t time_out = 0;
/** @}*/


/**
 * @brief Get camera version
 * 
 * @return 1 if error, 0 if success
 */
static uint8_t Camara_GetVersion(void) {

    uint8_t aux1, aux2, res;

    res = HDByteReadI2C(CAM_ADDRESS, 0x300A, &aux1, 1);
    res += HDByteReadI2C(CAM_ADDRESS, 0x300B, &aux2, 1);
    if (res != 0) {
        return 1;
    }

    if (aux1 != 0x56) {
        return 1;
    }
    if (aux2 != 0x42) {
        return 1;
    }    
    return 0;
    
}

/**
 * @brief   Reset the Ov5642 sensor
 */
static void Camara_Reset(void) {

    // Use this to reset the OV5642 by software
    //uint8_t res;
    //res = HDByteWriteI2C(CAM_ADDRESS, 0x3008, 0x80);
    //DelayMs(10);
    //return res;
    
    // Use this when the OV5642 power supply is control by a MOSFET
    CAM_PWDN = 1;
    DelayMs(10);
    CAM_PWDN = 0;//power up

}

/**
 * @brief   Set the polarity of the Camera Signals
 */
static void Camara_SetSignalsPolarity(void) {

    uint8_t aux1;
    
    HDByteReadI2C(CAM_ADDRESS, 0x4740, &aux1, 1);
        
    aux1 = aux1 | 0x01;//active high VSYNC
    //aux1 = aux1 & ~(0x01);//active low VSYNC
    
    aux1 = aux1 | 0x020;//latch falling PCLK
    //aux1 = aux1 & ~(0x20);//latch rising PCLK
        
    HDByteWriteI2C(CAM_ADDRESS, 0x4740, aux1);
    
}

/**
 * Set the Camera processor clock
 */
static void Camara_SetProcessorClock(void) {
    
    // Don´t use this setting because is too fast for the DSPIC
    //HDByteWriteI2C(CAM_ADDRESS, 0x3012, 0b000);//pclk = xclk / 1
    
//    HDByteWriteI2C(CAM_ADDRESS, 0x3012, 0b000);//pclk = xclk / 1
    HDByteWriteI2C(CAM_ADDRESS, 0x3012, 0b001);//pclk = xclk / 1.5
//    HDByteWriteI2C(CAM_ADDRESS, 0x3012, 0b010);//pclk = xclk / 2
    //HDByteWriteI2C(CAM_ADDRESS, 0x3012, 0b011);//pclk = xclk / 2.5
//    HDByteWriteI2C(CAM_ADDRESS, 0x3012, 0b100);//pclk = xclk / 3 ****************
    //HDByteWriteI2C(CAM_ADDRESS, 0x3012, 0b101);//pclk = xclk / 4 
    //HDByteWriteI2C(CAM_ADDRESS, 0x3012, 0b110);//pclk = xclk / 6
    //HDByteWriteI2C(CAM_ADDRESS, 0x3012, 0b111);//pclk = xclk / 8
}

/**
 * @brief   Configure the OV5642 sensor
 */
static void Camara_SetConfig(void) {    

    //Camara_Set_QCIFprev_YUVmode();
    //Camara_Set_CIFprev_YUVmode();
    //Camara_Set_QVGAprev_YUVmode();
    Camara_Set_VGAprev_YUVmode();   // Use this option
    //Camara_Set_SVGAprev_YUVmode();
        
    //640x840 (wait ordy/high qual: fail,def qual: fail)
    //Camara_Set_QSXGAcaptQVGA_JPEGmode();
    
    //1024x768 -  (wait ordy/high qual: fail,def qual: fail)
    //Camara_Set_QSXGAcaptVGA_JPEGmode();
    
    //1024x768 -  (wait ordy/high qual: fail,def qual: fail)
    //Camara_Set_QSXGAcaptXGA_JPEGmode();
    
    //1600x1200 - 1MP (wait ordy/high qual: fail,def qual: fail)
    //Camara_Set_QSXGAcaptSXGA_JPEGmode();
    
    //1600x1200 - 1MP (wait ordy/high qual: fail,def qual: fail)
    //Camara_Set_QSXGAcaptUXGA_JPEGmode();
    
    //2048x1536 - 3MP (wait ordy/high qual: fail,def qual: fail)
    //2048x1536 - 3MP (wait ordy+delay1ordy/high qual: ,def qual: )
    //Camara_Set_QSXGAcaptQXGA_JPEGmode();
    
    //2592x1944 - 5MP (wait ordy/high qual: fail,def qual: good&bad)
    //2592x1944 - 5MP (wait ordy+delay1ordy/high qual: good,def qual: )
    Camara_Set_QSXGAcapt_JPEGmode();
    //Camara_Set_QSXGAcapt_YUVmode();
    //blanco_negro();
    Camara_SetSignalsPolarity();
    Camara_SetProcessorClock();
    
}

/**
 * @brief   Function to check if the SD card is in the slot
 * 
 * @return  0 if present, 1 if not present
 */

uint8_t Camara_SDCard(void) {

    DSTATUS estado;
    uint8_t aux;    
    
    //Ensure card is still inserted
    ffs_10ms_timer = 10;
    
    //Wait for timer to expire
#ifdef DEBUG
    printf("File: %s LINE: %d Entrando al While\r\n", __FILE__, __LINE__);
#endif
    while (ffs_10ms_timer);
#ifdef DEBUG
    printf("File: %s LINE: %d Salinedo del While\r\n", __FILE__, __LINE__);
#endif
    
    estado = disk_status(0);                        
    if (estado & STA_NODISK) {
#ifdef DEBUG
        printf("No card in SD socket\r\n");
#endif
        return 1;/* No card in the socket */
    }

    // INITIALISE NEW MMC / SD CARD
    aux = 10;
#ifdef DEBUG
    printf("File: %s LINE: %d Entrando al While\r\n", __FILE__, __LINE__);
#endif
    do {
        Result = f_mount(&FatFs, "", 1);
    } while ((Result != FR_OK)&&(aux--));
#ifdef DEBUG
    printf("File: %s LINE: %d Saliendo del While\r\n", __FILE__, __LINE__);
#endif
    
    
	if(Result != FR_OK) {     /* Give a work area to the default drive */
#ifdef DEBUG
        printf("Could not mount SD card\r\n");
#endif
        return 1;
    }
    else {
#ifdef DEBUG
        printf("SD card mounted correctly\r\n");
#endif
        return 0;
    }
    
}

/**
 * @brief   Function to Read the temperature and humidity information 
 *          and creates a file to store it.
 */
void Temp_Init(void) {
    
    static uint8_t regs[8] = {0};
    char data[125]={0};
    double Temperature = 0, Humidity = 0;
    double bateria = 0;
    double voltaje = 0;
    int len = 0;

    T1CONbits.TON = 0;
    T2CONbits.TON = 0;
    TMR2 = 0;
    T2CONbits.TON = 1;
    Temperature = HDC1000_ReadTemp();
    Humidity = HDC1000_ReadHum();
    
#ifdef DEBUG
    printf("Tiempo de uso del sensor T/H %d\r\n",TMR2);
#endif
    TMR2 = 0;
    T2CONbits.TON = 0;
    
    //Read the Time from de RTC
    DS3231_DateHour(regs);
    
    voltaje = MAX17043_ReadVoltage();
    bateria = MAX17043_ReadSoC();
    
#ifdef DEBUG
    printf("Temperture Read: %.2f C\r\n",Temperature);
    printf("Humidity Read: %.2f RH\r\n",Humidity);
    printf(" Voltaje %.2f",voltaje);
#endif
    len = sprintf(data,"%u/%u/%d %d:%d:%d Temperatura: %.2f Humedad: %.2f Bateria: %.2f Voltaje: %.2f %s Time Elapsed %.2fms\r\n",
            regs[4], regs[5], regs[6]+2000, regs[2], regs[1], regs[0], Temperature, Humidity, bateria, voltaje, file_CamNombre, tiempoMSRes);
    Result = f_open(&Fil2, (const TCHAR*)file_sensor, FA_WRITE | FA_OPEN_APPEND);    
#ifdef DEBUG
    if (Result == FR_OK) {
        //----- FILE WAS SUCESSFULLY CREATED -----
        printf("Archivo sensor.txt creado \r\n");
    }else{
        printf("No se creo sensor.txt \r\n");
    }
#endif
    
    T1CONbits.TON = 1; 
    Result = f_write(&Fil2, data, len, &bw1);
    T1CONbits.TON = 0;
    Result = f_close(&Fil2);
}

///**
// * @brief   Function to create a .dat that store the name of the .jpeg photo
// *          and a label that indicates if the file was sended or not.
// */
void MetaData(void)
{
    char metadata[14] = {0};
    char path_name[6] = {0};
    //char space = 0x0A;
    uint16_t a=0;
    T1CONbits.TON = 0;
    sprintf(metadata, "U %s\n\r",file_CamNombre);
    path_name[5] = '\0';
    path_name[4] = file_CamNombre[6];
    path_name[3] = file_CamNombre[5];
    path_name[2] = file_CamNombre[4];
    path_name[1] = file_CamNombre[3];
    path_name[0] = file_CamNombre[2];
#ifdef DEBUG
    printf("%s \r\n", metadata);
    printf("%s \r\n", path_name);
#endif
    a = atol(path_name);
#ifdef DEBUG
    printf("%i \r\n", a);
#endif
    if(a == 1)
    {
        Result = f_open(&Fil3, (const TCHAR*)file_metadata, FA_WRITE | FA_OPEN_APPEND);
#ifdef DEBUG
        if (Result == FR_OK) {
            //----- FILE WAS SUCESSFULLY CREATED -----
            printf("Nro de foto al inicio \r\n");
        }else{
            printf("No se creo nada \r\n");   
        }
#endif
        T1CONbits.TON = 1;
        path_name[5] = 0x0A;
        Result = f_write(&Fil3, path_name, sizeof path_name, &bw2);
        //Result = f_write(&Fil3, &space, sizeof space, &bw2);
        T1CONbits.TON = 0;
        Result = f_close(&Fil3);
    }
    
    Result = f_open(&Fil3, (const TCHAR*)file_metadata, FA_WRITE | FA_OPEN_APPEND);
#ifdef DEBUG
    if (Result == FR_OK) {
        //----- FILE WAS SUCESSFULLY CREATED -----
        printf("Archivo METADATA.dat creado \r\n");
    }else{
        printf("No se creo METADATA.dat \r\n");
    }
#endif
    T1CONbits.TON = 1;
    Result = f_write(&Fil3, metadata, sizeof metadata, &bw2);
    T1CONbits.TON = 0;
    Result = f_close(&Fil3);
}

///**
// * @brief   Function to Initialize all pheriperals that the OV5642
// *          requires to work properly after PWRSAV.
// */
uint8_t CamaraON()
{
    int tiempo_espera = 5000;
    Camara_Reset();
    InicializaREFOCLK();
    InicializaTIMER1();
    InicializaTIMER2();
    InicializaSPI1();
    InicializaDMA_SPI();
    InicializaI2C();
    InicializaUART1();
    InicializaIO();
    
    DelayMs(100);

    INTCON2bits.GIE = 1; //enable all interrupts

#ifdef DEBUG
    printf("File: %s LINE: %d Entrando al While\r\n", __FILE__, __LINE__);
#endif
    while (!Camara_GetVersion() && (tiempo_espera > 0)) {
        Camara_Reset();
        tiempo_espera--;
    }
#ifdef DEBUG
    printf("File: %s LINE: %d Saliendo del While\r\n", __FILE__, __LINE__);
#endif
    
    if (tiempo_espera == 0){
#ifdef DEBUG
        printf("Camera not found\r\n");
#endif
        return 1;
    }
    Camara_SetConfig();
    
    tiempo_espera = 3;
#ifdef DEBUG
    printf("File: %s LINE: %d Entrando al While\r\n", __FILE__, __LINE__);
#endif
    while (Camara_SDCard() && (tiempo_espera > 0)){
        tiempo_espera--;
    }
#ifdef DEBUG
    printf("File: %s LINE: %d Saliendo del While\r\n", __FILE__, __LINE__);
#endif
    
    if (tiempo_espera == 0){
#ifdef DEBUG
        printf("SD card not found\r\n");
#endif
        return 1;
    }
    return 0;
}

///**
// * @brief   Function to Initialize all pheriperals that the OV5642
// *          requires to work properly
//              Enables all interrupts    
// */
void Camara_Init(void) {
    
    uint8_t x;
    
    InicializaREFOCLK();
    InicializaTIMER1();
    InicializaTIMER2();
    InicializaSPI1();
    InicializaDMA_SPI();
    InicializaI2C();
    InicializaUART1();
    InicializaIO();
    //MAX17043_Init();
    
    DelayMs(100);

    INTCON2bits.GIE = 1; //enable all interrupts

#ifdef DEBUG
    printf("File: %s LINE: %d Entrando al While\r\n", __FILE__, __LINE__);
#endif
    while (Camara_GetVersion() == 1) {
        Camara_Reset();
    }
#ifdef DEBUG
    printf("File: %s LINE: %d Saliendo del While\r\n", __FILE__, __LINE__);
#endif
    Camara_SetConfig();
    
#ifdef DEBUG
    printf("File: %s LINE: %d Entrando al While\r\n", __FILE__, __LINE__);
#endif
    while (Camara_SDCard() == 1)
    {
        DelayMs(1000);
#ifdef DEBUG
        printf("no funciona el SPI \r\n");
#endif
    }
#ifdef DEBUG
    printf("File: %s LINE: %d Saliendo del While\r\n", __FILE__, __LINE__);
#endif
    TEST_LED = 1;
    DelayMs(50);
    TEST_LED = 0;
    DelayMs(50);
    TEST_LED = 1;
    DelayMs(50);
    TEST_LED = 0;
    DelayMs(50);
    TEST_LED = 1;
    DelayMs(50);
    TEST_LED = 0;
    DelayMs(50);
    
    x = 10;
#ifdef DEBUG
    printf("File: %s LINE: %d Entrando al While\r\n", __FILE__, __LINE__);
#endif
    while(x > 0){
        TEST_LED = 1;
        DelayMs(180);
        TEST_LED = 0;
        DelayMs(180);
        x--;
    }
#ifdef DEBUG
    printf("File: %s LINE: %d Saliendo del While\r\n", __FILE__, __LINE__);
#endif
}

///**
// * @brief   Set camera name
// */
static void Camara_SetFileName(void) {
    
    uint32_t num;
    
    Result = f_open(&Fil4, (const TCHAR*)file_metadata, FA_OPEN_EXISTING | FA_READ | FA_WRITE);
    // Si el resultado es positivo escribimos en el archivo
    if(Result == FR_OK){    
#ifdef DEBUG
        printf("Archivo Metadata abierto\n\r");
#endif
        // lectura de la memoria guardada
        f_read(&Fil4, buff_name, 5, &bw);
        num = atol(buff_name);
        num++;
#ifdef DEBUG
        printf("Numero de foto: %c\n\r",buff_name[0]);
        printf("Numero de foto: %c\n\r",buff_name[1]);
        printf("Numero de foto: %c\n\r",buff_name[2]);
        printf("Numero de foto: %c\n\r",buff_name[3]);
        printf("Numero de foto: %c\n\r",buff_name[4]);
#endif
        file_CamNombre[6] = buff_name[4];
        file_CamNombre[5] = buff_name[3];
        file_CamNombre[4] = buff_name[2];
        file_CamNombre[3] = buff_name[1];
        file_CamNombre[2] = buff_name[0];
        f_lseek(&Fil4, 0);
        sprintf(buff_name, "%05lu", num);
        f_write(&Fil4, buff_name, sizeof buff_name, &bw);
        f_close(&Fil4);
    }
    else{
#ifdef DEBUG
        printf("Error abriendo archivo metadata\n\r");
#endif
        TEST_LED = 1;
    }

    file_CamNombre[6]++;
    if (file_CamNombre[6] > '9') {
        file_CamNombre[6] = '0';
        file_CamNombre[5]++;
        if (file_CamNombre[5] > '9') {
            file_CamNombre[5] = '0';
            file_CamNombre[4]++;
            if (file_CamNombre[4] > '9') {
                file_CamNombre[4] = '0';
                file_CamNombre[3]++;
                if (file_CamNombre[3] > '9') {
                    file_CamNombre[3] = '0';
                    }                    
                }
            }
        }
}

/*---------------------------------------------------------*/
/* User Provided Timer Function for FatFs module           */
/*---------------------------------------------------------*/
/* This is a real time clock service to be called from     */
/* FatFs module. Any valid time must be returned even if   */
/* the system does not support a real time clock.          */
/* This is not required in read-only configuration.        */

DWORD get_fattime (void)
{
	static uint8_t regs[8] = {0};
    DS3231_DateHour(regs);
    
	return	  ((DWORD)(regs[6] + 20) << 25)
			| ((DWORD)regs[5] << 21)
			| ((DWORD)regs[4] << 16)
			| ((DWORD)regs[2] << 11)
			| ((DWORD)regs[1] << 5)
			| ((DWORD)regs[0]>> 1);
}


/**
 * @brief   Create a file to store the image
 * 
 * @return  0 if ok, 1 if error
 */
static uint8_t Camara_CrearArchivo(void) {
    
    //----- CREATE NEW FILE FOR WRITING -----
    Result = f_open(&Fil, (const TCHAR*)file_CamNombre, FA_WRITE | FA_CREATE_ALWAYS);
    
    if (Result == FR_OK) {
        //----- FILE WAS SUCESSFULLY CREATED -----
#ifdef DEBUG
        printf("File created correctly\r\n");
#endif
        return 0;
    } else {
        //----- ERROR - THE FILE COULD NOT BE CREATED -----
#ifdef DEBUG
        printf("File could not be created\r\n");
#endif
        TEST_LED = 1;
        return 1;
    }
    
}

///**
// * @brief   Function to read data from the FIFO and store it on the SD card
// * 
// * @return  0 if ok, 1 if error
// */
inline static uint8_t Camara_GuardarFoto(void) {

    uint8_t aux;
    uint16_t bw;
    uint8_t inicio_CamCapture;
    uint8_t fin_CamCapture;
    uint8_t firstCapture;
    uint16_t cantCapture;
    uint8_t timeoutinicioCapture = 0;
    uint16_t timeoutfinCapture = 0;
    uint32_t totalCapture = 0;

//    PROBE_TEST = 1;
    //Camara_EnviaStringUART1((uint8_t *)"@01,JPEG started!\r\n");
#ifdef DEBUG
    printf("File: %s Line: %d @01,JPEG started!\r\n", __FILE__, __LINE__);
#endif
    T1CONbits.TON = 0; // Stop Timer    
    tiempoMSTotal = 0;
    tiemposMSIndex = 0;
    //memset(tiemposMS, 0, sizeof (tiemposMS));
    int p=0;
    for (p=0; p < (sizeof(tiemposMS)/sizeof(tiemposMS[0])); p++) {
        tiemposMS[p] = 0;
    }
    T2CONbits.TON = 0;
    TMR2 = 0;
    T2CONbits.TON = 1;
    asm("RESET_CAMARA:");        
        
    MEM_RCLK = 0;
    MEM_RRST = 0;    
    MEM_RCLK = 1;
    DelayMs(10);
    aux = (uint8_t)(PORTB >> 8);
 
    if(aux != 0xFF) {
        buff_CamCapture[0] = 0xFF;
    } else {
        buff_CamCapture[0] = aux;
    }
    //printf("aux : %x\r\n",aux);               
    MEM_RCLK = 0;
    MEM_RRST = 1;
    
    inicio_CamCapture = 0;
    fin_CamCapture = 0;    
    firstCapture = 1;
    bw = 0;
    useDma = 0;
    cantCapture = 0;
    
    asm("MOV #0x2001, W7");
    asm("GOTO INICIO");
    
    asm("ESPERAR:");
    DelayMs(10);
#ifdef DEBUG
    printf("File: %s LINE: %d Entrando al While\r\n", __FILE__, __LINE__);
#endif
    //asm("GOTO GUARDAR");
    asm("GOTO CHECKLISTO");

    do{
        asm("INICIO: MOV PORTB, W1");
        asm("LSR W1, #8, W1"); 
        asm("MOV.B W1, [W7++]");
        
        MEM_RCLK = 1;       
        MEM_RCLK = 0;            
        
        asm("BTSC PORTB, #2");//ORDY      
        asm("GOTO ESPERAR");
        asm("CHECKLISTO: BTSS W7,#12");//para buff size 4096
        //asm("BTSS W7,#11");//para buff size 2048
        //asm("BTSS W7,#10");//para buff size 1024
        asm("GOTO INICIO");
        //======================================================================
        //SECCION DONDE GUARDAMOS LOS DATOS CAPTURADOS EN LA SD CARD
        //asm("GUARDAR:");        
        T2CONbits.TON = 0;
        tiemposMS[tiemposMSIndex++] = TMR2;
        TMR2 = 0;
        T2CONbits.TON = 1;
        //======================================================================
        //DETERMINA LA CANTIDAD DE BYTES A GUARDAR
        asm("MOV #0x2000, W1");
        asm("SUBR W1, W7, W1");
        asm("MOV W1, [W14+12]");

        //======================================================================
        //VERIFICAMOS CABECERA DEL JPEG
        if (firstCapture) {
            firstCapture = 0;
            if(++timeoutinicioCapture >= 200) { //50 milisegundos
                break;
            }
            //asm("CHECKHEADER: MOV #0xE0FF,W0");
            asm("MOV #0xE0FF,W0");
            asm("MOV #0x2002,W1");
            asm("SUBR W0,[W1],W0");
            asm("BRA NZ, RESET_CAMARA");
            inicio_CamCapture = 1;
            
        }
        //======================================================================        
        //VERIFICAMOS FINAL DEL JPEG
        asm("MOV #0x2000, W2"); // w2 tiene la dirección de memoria 0x2000        
        asm("MOV [W14+12], W0"); // Movemos a W0 los bytes escritos en el buffer
        asm("ADD W2, W0, W0"); // W0 tiene la direccion de memoria final
        
        asm("CHECKFINAL: CLR W1"); // limpiamos w1
        asm("MOV.B [W2++], W1"); // w1 = 0x2001       
        asm("SUBR W2, W0, W3");  // w3 = w0 - w2      
        asm("BRA Z, FIN_CHECKFINAL"); // si la resta es 0 salta a FIN_CHECKFINAL
        
        asm("MOV.B [W2], W3"); // w3 = contenido del buffer en la direccion w2
        asm("SL W3, #8, W3"); //
        asm("IOR W3, W1, W1");

        asm("MOV #0xD9FF, W3");
        asm("SUBR W1, W3, W3");
        asm("BRA NZ, CHECKFINAL");
        
        fin_CamCapture = 1;

        asm("FIN_CHECKFINAL: MOV #0x2000, W0");
        asm("SUBR W0, W2, W1");
        asm("MOV W1, [W14+14]");
        index_CamCapture = bw + 1;
        
        //ESCRIBIR EN LA SD CARD
        T1CONbits.TON = 1; // Start Timer 1
        
        if (fin_CamCapture) {
            /* Write data to the file */
            useDma = 1;
            f_write(&Fil, buff_CamCapture, index_CamCapture, &bw);
            useDma = 0;                        
            if ((Result != FR_OK)||(bw != index_CamCapture)) {
                break;
            }
            totalCapture += cantCapture;            
            break;            
        } else {            
            /* Write data to the file */
            useDma = 1;
            Result = f_write(&Fil, buff_CamCapture, cantCapture, &bw);
            useDma = 0;
            if ((Result != FR_OK)||(bw != cantCapture)) {
                inicio_CamCapture = 0;
                break;
            }
            totalCapture += cantCapture;
        }
        
        if(++timeoutfinCapture >= 2000) { // segundos
            fin_CamCapture = 0;
            break;
        }
        
        T1CONbits.TON = 0; // Stop Timer
        T2CONbits.TON = 0;
        tiemposMS[tiemposMSIndex++] = TMR2;
        TMR2 = 0;
        T2CONbits.TON = 1;        
        
        asm("MOV #0x2000, W7");                        

    } while(1);
#ifdef DEBUG
    printf("File: %s LINE: %d Saliendo del While\r\n", __FILE__, __LINE__);
#endif
    
    //asm("FINALIZAR:");            
    T1CONbits.TON = 0; // Stop Timer
    T2CONbits.TON = 0;
    tiemposMS[tiemposMSIndex++] = TMR2;
    //Se concluyo con toda la carga del FIFO en la SD card
//    PROBE_TEST = 0;            
    //Deshabilitamos el puerto de lectura del FIFO
    MEM_OE = 1;
    
    //----- CLOSE THE FILE -----                    
    Result = f_close(&Fil);
    
    if ((Result == FR_OK) && inicio_CamCapture && fin_CamCapture) {
        
        //Camara_EnviaStringUART1((uint8_t *)"@01,JPEG finished OK!\r\n");
#ifdef DEBUG
        printf("@01,JPEG finished OK!\r\n");
#endif
        
        for (bw = 0; bw < tiemposMSIndex; bw++) {
            tiempoMSTotal = tiempoMSTotal + (uint32_t)tiemposMS[bw];
        }
        tiempoMSRes = (float)tiempoMSTotal * 0.00106878;                
#ifdef DEBUG
        printf("@01,JPEG time elapsed: %.4f mseg!\r\n", tiempoMSRes);
#endif
        //sprintf((char *)salida, "@01,JPEG time elapsed: %.4f mseg!\r\n", tiempoMSRes);
        
        //Camara_EnviaStringUART1(salida);
        //Camara_EnviaStringUART1((uint8_t *)"@01,--------------------------------------------!\r\n");
#ifdef DEBUG
        printf("@01,--------------------------------------------!\r\n");
#endif
        
        return 0;//no error
    }    
    Camara_Reset();
#ifdef DEBUG
    //Camara_EnviaStringUART1((uint8_t *)"@01,JPEG finished ERROR!\r\n");    
    printf("@01,JPEG finished ERROR!\r\n");
    //Camara_EnviaStringUART1((uint8_t *)"@01,JPEG time elapsed: !\r\n");
    printf("@01,JPEG time elapsed: !\r\n");
    //Camara_EnviaStringUART1((uint8_t *)"@01,--------------------------------------------!\r\n");
    printf("@01,--------------------------------------------!\r\n");
#endif
    
    return 1;//theres is an error
    
}

/**
 * @brief   Reset the camera application
 */
static void Camara_ResetApp(void) {

    T1CONbits.TON = 1; // Stop Timer
    T2CONbits.TON = 0;

    MEM_RESET = 0;
    DelayMs(10);     //**************************
    MEM_RESET = 1;

            
}

/**
 * @brief   Take a picture from the camera
 * 
 * @return  0 if success, 1 if error
 */
uint8_t Camara_TomarFoto(void) {

    uint8_t error;
    Camara_SetFileName();
    error = Camara_CrearArchivo();
    FLASH = 0;
    if (error == 0) {            
#ifdef DEBUG
        Camara_EnviaStringUART1((uint8_t *)"@01,--------------------------------------------!\r\n");
        sprintf((char *)salida,"@01,Taking Photo: %s!\r\n", file_CamNombre);
        Camara_EnviaStringUART1(salida);
#endif
        
        TEST_LED = 1;
        DelayMs(50);
        TEST_LED = 0;
        
        smCamara_Capture = 0;
        flag_CaptureReady = 0;
        time_out = 0;
        //CamaraCount = 3;//wait 3 frame
        //CamaraCount = 2;//wait 2 frame
        //CamaraCount = 1;//wait 1 frame
        Camara_FrameCount = 0;//inmediatly
          
        //Interrupt for the VSYNC
        INTCON2bits.INT1EP = 1;//flanco de bajada
        IFS1bits.INT1IF = 0;
        IEC1bits.INT1IE = 1;
        
        //Timeout de VSYNC por 1 segundo
        InicializaTIMER4_5();
        
#ifdef DEBUG
        printf("File: %s Line: %d Toggleando\r\n", __FILE__, __LINE__);
#endif
        
        do {            
            asm("BTG LATC,#6");
        } while (time_out == 0);
        
        IEC1bits.T5IE = 0; // Disable Timer5 interrupt
        T4CONbits.TON = 0; // Stop 32-bit Timer
        
#ifdef DEBUG
        printf("File: %s Line: %d Time Out Acabado\r\n", __FILE__, __LINE__);
#endif
        
        if (flag_CaptureReady) {
            error = Camara_GuardarFoto();
            
        } else if (time_out == 1) {
#ifdef DEBUG
            printf("no detecte el vsync \r\n");
#endif
            error = 1;
        }
    }    
    
    //Send a beep if picture reading was succesfull
    if (error == 0) {
        TEST_LED = 1;
        DelayMs(500);
        TEST_LED = 0;                    
    //Send two tones if error while reading    
    } else {
        TEST_LED = 1;
        DelayMs(250);
        TEST_LED = 0;
        DelayMs(250);
        TEST_LED = 1;
        DelayMs(250);
        TEST_LED = 0;                
    }
    
    T2CONbits.TON = 0;
    TMR2 = 0;
    T2CONbits.TON = 1;
    MetaData();
#ifdef DEBUG
    printf("File: %s LINE: %d tiempo de creacion de metadata %d\r\n",__FILE__, __LINE__, TMR2);
#endif
    TMR2 = 0;
    T2CONbits.TON = 0;
    
    T4CONbits.T32 = 1;
    Temp_Init();
#ifdef DEBUG
    printf("TMR4 %x\r\n",TMR4);
    printf("TMR5 %x\r\n",TMR5);
#endif
    T4CONbits.T32 = 0;
    Camara_ResetApp();
    return error;//1: hubo error, 0: exito
}

///**
// * @brief   Start capturing an image
// */
void Camara_StartCapture(void) {

    MEM_WEN = 1;
    MEM_OE = 0; //hablita la salida
    
    //////////////////////////////////////
    //Timeout of 5seg 
    IFS1bits.T5IF = 0;
    IEC1bits.T5IE = 1; // Enable Timer5 interrupt
    T4CONbits.TON = 1; // Start 32-bit Timer
     /////////////////////////////////////
     
    printf("esperando ordy\r\n");
    INTCON2bits.INT2EP = 1;//flanco de bajada
    IFS1bits.INT2IF = 0;
    IEC1bits.INT2IE = 1;
    
}

///**
// * @brief   Stop the capturing an image
// */
void Camara_StopCapture(void) {
    
    MEM_WEN = 0;

    IEC1bits.INT1IE = 0;
    IEC1bits.INT2IE = 0;
    
}

void Camara_Set_QVGAprev_YUVmode(void) {

//; for the setting , 24M Mlck input and 24M Plck output
//;15fps YUV mode
Camara_EnviaBytesI2C(0x3103 ,0x93);
Camara_EnviaBytesI2C(0x3008 ,0x82);
Camara_EnviaBytesI2C(0x3017 ,0x7f);
Camara_EnviaBytesI2C(0x3018 ,0xfc);
Camara_EnviaBytesI2C(0x3810 ,0xc2);
Camara_EnviaBytesI2C(0x3615 ,0xf0);
Camara_EnviaBytesI2C(0x3000 ,0x00);
Camara_EnviaBytesI2C(0x3001 ,0x00);
Camara_EnviaBytesI2C(0x3002 ,0x5c);
Camara_EnviaBytesI2C(0x3003 ,0x00);
Camara_EnviaBytesI2C(0x3004 ,0xff);
Camara_EnviaBytesI2C(0x3005 ,0xff);
Camara_EnviaBytesI2C(0x3006 ,0x43);
Camara_EnviaBytesI2C(0x3007 ,0x37);
Camara_EnviaBytesI2C(0x3011 ,0x08);
Camara_EnviaBytesI2C(0x3010 ,0x10);
Camara_EnviaBytesI2C(0x460c ,0x22);
Camara_EnviaBytesI2C(0x3815 ,0x04);
Camara_EnviaBytesI2C(0x370c ,0xa0);
Camara_EnviaBytesI2C(0x3602 ,0xfc);
Camara_EnviaBytesI2C(0x3612 ,0xff);
Camara_EnviaBytesI2C(0x3634 ,0xc0);

Camara_EnviaBytesI2C(0x3613 ,0x00);
Camara_EnviaBytesI2C(0x3605 ,0x7c);
Camara_EnviaBytesI2C(0x3621 ,0x09);
Camara_EnviaBytesI2C(0x3622 ,0x60);
Camara_EnviaBytesI2C(0x3604 ,0x40);
Camara_EnviaBytesI2C(0x3603 ,0xa7);
Camara_EnviaBytesI2C(0x3603 ,0x27);
Camara_EnviaBytesI2C(0x4000 ,0x21);
Camara_EnviaBytesI2C(0x401d ,0x22);
Camara_EnviaBytesI2C(0x3600 ,0x54);
Camara_EnviaBytesI2C(0x3605 ,0x04);
Camara_EnviaBytesI2C(0x3606 ,0x3f);
Camara_EnviaBytesI2C(0x3c01 ,0x80);
Camara_EnviaBytesI2C(0x5000 ,0x4f);
Camara_EnviaBytesI2C(0x5020 ,0x04);
Camara_EnviaBytesI2C(0x5181 ,0x79);
Camara_EnviaBytesI2C(0x5182 ,0x00);
Camara_EnviaBytesI2C(0x5185 ,0x22);
Camara_EnviaBytesI2C(0x5197 ,0x01);
Camara_EnviaBytesI2C(0x5001 ,0xff);
Camara_EnviaBytesI2C(0x5500 ,0x0a);
Camara_EnviaBytesI2C(0x5504 ,0x00);
Camara_EnviaBytesI2C(0x5505 ,0x7f);
Camara_EnviaBytesI2C(0x5080 ,0x08);
Camara_EnviaBytesI2C(0x300e ,0x18);
Camara_EnviaBytesI2C(0x4610 ,0x00);
Camara_EnviaBytesI2C(0x471d ,0x05);
Camara_EnviaBytesI2C(0x4708 ,0x06);
Camara_EnviaBytesI2C(0x3808 ,0x02);
Camara_EnviaBytesI2C(0x3809 ,0x80);
Camara_EnviaBytesI2C(0x380a ,0x01);
Camara_EnviaBytesI2C(0x380b ,0xe0);
Camara_EnviaBytesI2C(0x380e ,0x07);
Camara_EnviaBytesI2C(0x380f ,0xd0);
Camara_EnviaBytesI2C(0x501f ,0x00);
Camara_EnviaBytesI2C(0x5000 ,0x4f);
Camara_EnviaBytesI2C(0x4300 ,0x30);
Camara_EnviaBytesI2C(0x3503 ,0x07);
Camara_EnviaBytesI2C(0x3501 ,0x73);
Camara_EnviaBytesI2C(0x3502 ,0x80);
Camara_EnviaBytesI2C(0x350b ,0x00);
Camara_EnviaBytesI2C(0x3503 ,0x07);
Camara_EnviaBytesI2C(0x3824 ,0x11);
Camara_EnviaBytesI2C(0x3501 ,0x1e);
Camara_EnviaBytesI2C(0x3502 ,0x80);
Camara_EnviaBytesI2C(0x350b ,0x7f);
Camara_EnviaBytesI2C(0x380c ,0x0c);
Camara_EnviaBytesI2C(0x380d ,0x80);

Camara_EnviaBytesI2C(0x380e ,0x03);
Camara_EnviaBytesI2C(0x380f ,0xe8);
Camara_EnviaBytesI2C(0x3a0d ,0x04);
Camara_EnviaBytesI2C(0x3a0e ,0x03);
Camara_EnviaBytesI2C(0x3818 ,0xc1);
Camara_EnviaBytesI2C(0x3705 ,0xdb);
Camara_EnviaBytesI2C(0x370a ,0x81);
Camara_EnviaBytesI2C(0x3801 ,0x80);
Camara_EnviaBytesI2C(0x3621 ,0x87);
Camara_EnviaBytesI2C(0x3801 ,0x50);
Camara_EnviaBytesI2C(0x3803 ,0x08);
Camara_EnviaBytesI2C(0x3827 ,0x08);
Camara_EnviaBytesI2C(0x3810 ,0x40);
Camara_EnviaBytesI2C(0x3804 ,0x05);
Camara_EnviaBytesI2C(0x3805 ,0x00);
Camara_EnviaBytesI2C(0x5682 ,0x05);
Camara_EnviaBytesI2C(0x5683 ,0x00);
Camara_EnviaBytesI2C(0x3806 ,0x03);
Camara_EnviaBytesI2C(0x3807 ,0xc0);
Camara_EnviaBytesI2C(0x5686 ,0x03);
Camara_EnviaBytesI2C(0x5687 ,0xbc);
Camara_EnviaBytesI2C(0x3a00 ,0x78);
Camara_EnviaBytesI2C(0x3a1a ,0x05);
Camara_EnviaBytesI2C(0x3a13 ,0x30);
Camara_EnviaBytesI2C(0x3a18 ,0x00);
Camara_EnviaBytesI2C(0x3a19 ,0x7c);
Camara_EnviaBytesI2C(0x3a08 ,0x12);
Camara_EnviaBytesI2C(0x3a09 ,0xc0);
Camara_EnviaBytesI2C(0x3a0a ,0x0f);
Camara_EnviaBytesI2C(0x3a0b ,0xa0);
Camara_EnviaBytesI2C(0x350c ,0x07);
Camara_EnviaBytesI2C(0x350d ,0xd0);
Camara_EnviaBytesI2C(0x3500 ,0x00);
Camara_EnviaBytesI2C(0x3501 ,0x00);
Camara_EnviaBytesI2C(0x3502 ,0x00);
Camara_EnviaBytesI2C(0x350a ,0x00);
Camara_EnviaBytesI2C(0x350b ,0x00);
Camara_EnviaBytesI2C(0x3503 ,0x00);
Camara_EnviaBytesI2C(0x528a ,0x02);
Camara_EnviaBytesI2C(0x528b ,0x04);
Camara_EnviaBytesI2C(0x528c ,0x08);
Camara_EnviaBytesI2C(0x528d ,0x08);
Camara_EnviaBytesI2C(0x528e ,0x08);
Camara_EnviaBytesI2C(0x528f ,0x10);
Camara_EnviaBytesI2C(0x5290 ,0x10);
Camara_EnviaBytesI2C(0x5292 ,0x00);
Camara_EnviaBytesI2C(0x5293 ,0x02);
Camara_EnviaBytesI2C(0x5294 ,0x00);

Camara_EnviaBytesI2C(0x5295 ,0x02);
Camara_EnviaBytesI2C(0x5296 ,0x00);
Camara_EnviaBytesI2C(0x5297 ,0x02);
Camara_EnviaBytesI2C(0x5298 ,0x00);
Camara_EnviaBytesI2C(0x5299 ,0x02);
Camara_EnviaBytesI2C(0x529a ,0x00);
Camara_EnviaBytesI2C(0x529b ,0x02);
Camara_EnviaBytesI2C(0x529c ,0x00);
Camara_EnviaBytesI2C(0x529d ,0x02);
Camara_EnviaBytesI2C(0x529e ,0x00);
Camara_EnviaBytesI2C(0x529f ,0x02);
Camara_EnviaBytesI2C(0x3030 ,0x2b);
Camara_EnviaBytesI2C(0x3a02 ,0x00);
Camara_EnviaBytesI2C(0x3a03 ,0x7d);
Camara_EnviaBytesI2C(0x3a04 ,0x00);
Camara_EnviaBytesI2C(0x3a14 ,0x00);
Camara_EnviaBytesI2C(0x3a15 ,0x7d);
Camara_EnviaBytesI2C(0x3a16 ,0x00);
Camara_EnviaBytesI2C(0x3a00 ,0x78);
Camara_EnviaBytesI2C(0x3a08 ,0x09);
Camara_EnviaBytesI2C(0x3a09 ,0x60);
Camara_EnviaBytesI2C(0x3a0a ,0x07);
Camara_EnviaBytesI2C(0x3a0b ,0xd0);
Camara_EnviaBytesI2C(0x3a0d ,0x08);
Camara_EnviaBytesI2C(0x3a0e ,0x06);
Camara_EnviaBytesI2C(0x5193 ,0x70);
Camara_EnviaBytesI2C(0x589b ,0x04);
Camara_EnviaBytesI2C(0x589a ,0xc5);
Camara_EnviaBytesI2C(0x401e ,0x20);
Camara_EnviaBytesI2C(0x4001 ,0x42);
Camara_EnviaBytesI2C(0x401c ,0x04);
Camara_EnviaBytesI2C(0x528a ,0x01);
Camara_EnviaBytesI2C(0x528b ,0x04);
Camara_EnviaBytesI2C(0x528c ,0x08);
Camara_EnviaBytesI2C(0x528d ,0x10);
Camara_EnviaBytesI2C(0x528e ,0x20);
Camara_EnviaBytesI2C(0x528f ,0x28);
Camara_EnviaBytesI2C(0x5290 ,0x30);
Camara_EnviaBytesI2C(0x5292 ,0x00);
Camara_EnviaBytesI2C(0x5293 ,0x01);
Camara_EnviaBytesI2C(0x5294 ,0x00);
Camara_EnviaBytesI2C(0x5295 ,0x04);
Camara_EnviaBytesI2C(0x5296 ,0x00);
Camara_EnviaBytesI2C(0x5297 ,0x08);
Camara_EnviaBytesI2C(0x5298 ,0x00);
Camara_EnviaBytesI2C(0x5299 ,0x10);
Camara_EnviaBytesI2C(0x529a ,0x00);
Camara_EnviaBytesI2C(0x529b ,0x20);

Camara_EnviaBytesI2C(0x529c ,0x00);
Camara_EnviaBytesI2C(0x529d ,0x28);
Camara_EnviaBytesI2C(0x529e ,0x00);
Camara_EnviaBytesI2C(0x529f ,0x30);
Camara_EnviaBytesI2C(0x5282 ,0x00);
Camara_EnviaBytesI2C(0x5300 ,0x00);
Camara_EnviaBytesI2C(0x5301 ,0x20);
Camara_EnviaBytesI2C(0x5302 ,0x00);
Camara_EnviaBytesI2C(0x5303 ,0x7c);
Camara_EnviaBytesI2C(0x530c ,0x00);
Camara_EnviaBytesI2C(0x530d ,0x0c);
Camara_EnviaBytesI2C(0x530e ,0x20);
Camara_EnviaBytesI2C(0x530f ,0x80);
Camara_EnviaBytesI2C(0x5310 ,0x20);
Camara_EnviaBytesI2C(0x5311 ,0x80);
Camara_EnviaBytesI2C(0x5308 ,0x20);
Camara_EnviaBytesI2C(0x5309 ,0x40);
Camara_EnviaBytesI2C(0x5304 ,0x00);
Camara_EnviaBytesI2C(0x5305 ,0x30);
Camara_EnviaBytesI2C(0x5306 ,0x00);
Camara_EnviaBytesI2C(0x5307 ,0x80);
Camara_EnviaBytesI2C(0x5314 ,0x08);
Camara_EnviaBytesI2C(0x5315 ,0x20);
Camara_EnviaBytesI2C(0x5319 ,0x30);
Camara_EnviaBytesI2C(0x5316 ,0x10);
Camara_EnviaBytesI2C(0x5317 ,0x00);
Camara_EnviaBytesI2C(0x5318 ,0x02);
Camara_EnviaBytesI2C(0x5402 ,0x3f);
Camara_EnviaBytesI2C(0x5403 ,0x00);
Camara_EnviaBytesI2C(0x3406 ,0x00);
Camara_EnviaBytesI2C(0x5180 ,0xff);
Camara_EnviaBytesI2C(0x5181 ,0x52);
Camara_EnviaBytesI2C(0x5182 ,0x11);
Camara_EnviaBytesI2C(0x5183 ,0x14);
Camara_EnviaBytesI2C(0x5184 ,0x25);
Camara_EnviaBytesI2C(0x5185 ,0x24);
Camara_EnviaBytesI2C(0x5186 ,0x06);
Camara_EnviaBytesI2C(0x5187 ,0x08);
Camara_EnviaBytesI2C(0x5188 ,0x08);
Camara_EnviaBytesI2C(0x5189 ,0x7c);
Camara_EnviaBytesI2C(0x518a ,0x60);
Camara_EnviaBytesI2C(0x518b ,0xb2);
Camara_EnviaBytesI2C(0x518c ,0xb2);
Camara_EnviaBytesI2C(0x518d ,0x44);
Camara_EnviaBytesI2C(0x518e ,0x3d);
Camara_EnviaBytesI2C(0x518f ,0x58);
Camara_EnviaBytesI2C(0x5190 ,0x46);
Camara_EnviaBytesI2C(0x5191 ,0xf8);

Camara_EnviaBytesI2C(0x5192 ,0x04);
Camara_EnviaBytesI2C(0x5193 ,0x70);
Camara_EnviaBytesI2C(0x5194 ,0xf0);
Camara_EnviaBytesI2C(0x5195 ,0xf0);
Camara_EnviaBytesI2C(0x5196 ,0x03);
Camara_EnviaBytesI2C(0x5197 ,0x01);
Camara_EnviaBytesI2C(0x5198 ,0x04);
Camara_EnviaBytesI2C(0x5199 ,0x12);
Camara_EnviaBytesI2C(0x519a ,0x04);
Camara_EnviaBytesI2C(0x519b ,0x00);
Camara_EnviaBytesI2C(0x519c ,0x06);
Camara_EnviaBytesI2C(0x519d ,0x82);
Camara_EnviaBytesI2C(0x519e ,0x00);
Camara_EnviaBytesI2C(0x5025 ,0x80);
Camara_EnviaBytesI2C(0x5583 ,0x40);
Camara_EnviaBytesI2C(0x5584 ,0x40);
Camara_EnviaBytesI2C(0x5580 ,0x02);
Camara_EnviaBytesI2C(0x5000 ,0xcf);
Camara_EnviaBytesI2C(0x3710 ,0x10);
Camara_EnviaBytesI2C(0x3632 ,0x51);
Camara_EnviaBytesI2C(0x3702 ,0x10);
Camara_EnviaBytesI2C(0x3703 ,0xb2);
Camara_EnviaBytesI2C(0x3704 ,0x18);
Camara_EnviaBytesI2C(0x370b ,0x40);
Camara_EnviaBytesI2C(0x370d ,0x03);
Camara_EnviaBytesI2C(0x3631 ,0x01);
Camara_EnviaBytesI2C(0x3632 ,0x52);
Camara_EnviaBytesI2C(0x3606 ,0x24);
Camara_EnviaBytesI2C(0x3620 ,0x96);
Camara_EnviaBytesI2C(0x5785 ,0x07);
Camara_EnviaBytesI2C(0x3a13 ,0x30);
Camara_EnviaBytesI2C(0x3600 ,0x52);
Camara_EnviaBytesI2C(0x3604 ,0x48);
Camara_EnviaBytesI2C(0x3606 ,0x1b);
Camara_EnviaBytesI2C(0x370d ,0x0b);
Camara_EnviaBytesI2C(0x370f ,0xc0);
Camara_EnviaBytesI2C(0x3709 ,0x01);
Camara_EnviaBytesI2C(0x3823 ,0x00);
Camara_EnviaBytesI2C(0x5007 ,0x00);
Camara_EnviaBytesI2C(0x5009 ,0x00);
Camara_EnviaBytesI2C(0x5011 ,0x00);
Camara_EnviaBytesI2C(0x5013 ,0x00);
Camara_EnviaBytesI2C(0x519e ,0x00);
Camara_EnviaBytesI2C(0x5086 ,0x00);
Camara_EnviaBytesI2C(0x5087 ,0x00);
Camara_EnviaBytesI2C(0x5088 ,0x00);
Camara_EnviaBytesI2C(0x5089 ,0x00);
Camara_EnviaBytesI2C(0x302b ,0x00);

Camara_EnviaBytesI2C(0x3808 ,0x01);
Camara_EnviaBytesI2C(0x3809 ,0x40);
Camara_EnviaBytesI2C(0x380a ,0x00);
Camara_EnviaBytesI2C(0x380b ,0xf0);
Camara_EnviaBytesI2C(0x3a00 ,0x78);
Camara_EnviaBytesI2C(0x5001 ,0xFF);
Camara_EnviaBytesI2C(0x5583 ,0x50);
Camara_EnviaBytesI2C(0x5584 ,0x50);
Camara_EnviaBytesI2C(0x5580 ,0x02);
Camara_EnviaBytesI2C(0x3c01 ,0x80);
Camara_EnviaBytesI2C(0x3c00 ,0x04);
//;LENS
Camara_EnviaBytesI2C(0x5800 ,0x48);
Camara_EnviaBytesI2C(0x5801 ,0x31);
Camara_EnviaBytesI2C(0x5802 ,0x21);
Camara_EnviaBytesI2C(0x5803 ,0x1b);
Camara_EnviaBytesI2C(0x5804 ,0x1a);
Camara_EnviaBytesI2C(0x5805 ,0x1e);
Camara_EnviaBytesI2C(0x5806 ,0x29);
Camara_EnviaBytesI2C(0x5807 ,0x38);
Camara_EnviaBytesI2C(0x5808 ,0x26);
Camara_EnviaBytesI2C(0x5809 ,0x17);
Camara_EnviaBytesI2C(0x580a ,0x11);
Camara_EnviaBytesI2C(0x580b ,0xe );
Camara_EnviaBytesI2C(0x580c ,0xd );
Camara_EnviaBytesI2C(0x580d ,0xe );
Camara_EnviaBytesI2C(0x580e ,0x13);
Camara_EnviaBytesI2C(0x580f ,0x1a);
Camara_EnviaBytesI2C(0x5810 ,0x15);
Camara_EnviaBytesI2C(0x5811 ,0xd );
Camara_EnviaBytesI2C(0x5812 ,0x8 );
Camara_EnviaBytesI2C(0x5813 ,0x5 );
Camara_EnviaBytesI2C(0x5814 ,0x4 );
Camara_EnviaBytesI2C(0x5815 ,0x5 );
Camara_EnviaBytesI2C(0x5816 ,0x9 );
Camara_EnviaBytesI2C(0x5817 ,0xd );
Camara_EnviaBytesI2C(0x5818 ,0x11);
Camara_EnviaBytesI2C(0x5819 ,0xa );
Camara_EnviaBytesI2C(0x581a ,0x4 );
Camara_EnviaBytesI2C(0x581b ,0x0 );
Camara_EnviaBytesI2C(0x581c ,0x0 );
Camara_EnviaBytesI2C(0x581d ,0x1 );
Camara_EnviaBytesI2C(0x581e ,0x6 );
Camara_EnviaBytesI2C(0x581f ,0x9 );
Camara_EnviaBytesI2C(0x5820 ,0x12);
Camara_EnviaBytesI2C(0x5821 ,0xb );
Camara_EnviaBytesI2C(0x5822 ,0x4 );
Camara_EnviaBytesI2C(0x5823 ,0x0 );

Camara_EnviaBytesI2C(0x5824 ,0x0 );
Camara_EnviaBytesI2C(0x5825 ,0x1 );
Camara_EnviaBytesI2C(0x5826 ,0x6 );
Camara_EnviaBytesI2C(0x5827 ,0xa );
Camara_EnviaBytesI2C(0x5828 ,0x17);
Camara_EnviaBytesI2C(0x5829 ,0xf );
Camara_EnviaBytesI2C(0x582a ,0x9 );
Camara_EnviaBytesI2C(0x582b ,0x6 );
Camara_EnviaBytesI2C(0x582c ,0x5 );
Camara_EnviaBytesI2C(0x582d ,0x6 );
Camara_EnviaBytesI2C(0x582e ,0xa );
Camara_EnviaBytesI2C(0x582f ,0xe );
Camara_EnviaBytesI2C(0x5830 ,0x28);
Camara_EnviaBytesI2C(0x5831 ,0x1a);
Camara_EnviaBytesI2C(0x5832 ,0x11);
Camara_EnviaBytesI2C(0x5833 ,0xe );
Camara_EnviaBytesI2C(0x5834 ,0xe );
Camara_EnviaBytesI2C(0x5835 ,0xf );
Camara_EnviaBytesI2C(0x5836 ,0x15);
Camara_EnviaBytesI2C(0x5837 ,0x1d);
Camara_EnviaBytesI2C(0x5838 ,0x6e);
Camara_EnviaBytesI2C(0x5839 ,0x39);
Camara_EnviaBytesI2C(0x583a ,0x27);
Camara_EnviaBytesI2C(0x583b ,0x1f);
Camara_EnviaBytesI2C(0x583c ,0x1e);
Camara_EnviaBytesI2C(0x583d ,0x23);
Camara_EnviaBytesI2C(0x583e ,0x2f);
Camara_EnviaBytesI2C(0x583f ,0x41);
Camara_EnviaBytesI2C(0x5840 ,0xe );
Camara_EnviaBytesI2C(0x5841 ,0xc );
Camara_EnviaBytesI2C(0x5842 ,0xd );
Camara_EnviaBytesI2C(0x5843 ,0xc );
Camara_EnviaBytesI2C(0x5844 ,0xc );
Camara_EnviaBytesI2C(0x5845 ,0xc );
Camara_EnviaBytesI2C(0x5846 ,0xc );
Camara_EnviaBytesI2C(0x5847 ,0xc );
Camara_EnviaBytesI2C(0x5848 ,0xd );
Camara_EnviaBytesI2C(0x5849 ,0xe );
Camara_EnviaBytesI2C(0x584a ,0xe );
Camara_EnviaBytesI2C(0x584b ,0xa );
Camara_EnviaBytesI2C(0x584c ,0xe );
Camara_EnviaBytesI2C(0x584d ,0xe );
Camara_EnviaBytesI2C(0x584e ,0x10);
Camara_EnviaBytesI2C(0x584f ,0x10);
Camara_EnviaBytesI2C(0x5850 ,0x11);
Camara_EnviaBytesI2C(0x5851 ,0xa );
Camara_EnviaBytesI2C(0x5852 ,0xf );
Camara_EnviaBytesI2C(0x5853 ,0xe );

Camara_EnviaBytesI2C(0x5854 ,0x10);
Camara_EnviaBytesI2C(0x5855 ,0x10);
Camara_EnviaBytesI2C(0x5856 ,0x10);
Camara_EnviaBytesI2C(0x5857 ,0xa );
Camara_EnviaBytesI2C(0x5858 ,0xe );
Camara_EnviaBytesI2C(0x5859 ,0xe );
Camara_EnviaBytesI2C(0x585a ,0xf );
Camara_EnviaBytesI2C(0x585b ,0xf );
Camara_EnviaBytesI2C(0x585c ,0xf );
Camara_EnviaBytesI2C(0x585d ,0xa );
Camara_EnviaBytesI2C(0x585e ,0x9 );
Camara_EnviaBytesI2C(0x585f ,0xd );
Camara_EnviaBytesI2C(0x5860 ,0xc );
Camara_EnviaBytesI2C(0x5861 ,0xb );
Camara_EnviaBytesI2C(0x5862 ,0xd );
Camara_EnviaBytesI2C(0x5863 ,0x7 );
Camara_EnviaBytesI2C(0x5864 ,0x17);
Camara_EnviaBytesI2C(0x5865 ,0x14);
Camara_EnviaBytesI2C(0x5866 ,0x18);
Camara_EnviaBytesI2C(0x5867 ,0x18);
Camara_EnviaBytesI2C(0x5868 ,0x16);
Camara_EnviaBytesI2C(0x5869 ,0x12);
Camara_EnviaBytesI2C(0x586a ,0x1b);
Camara_EnviaBytesI2C(0x586b ,0x1a);
Camara_EnviaBytesI2C(0x586c ,0x16);
Camara_EnviaBytesI2C(0x586d ,0x16);
Camara_EnviaBytesI2C(0x586e ,0x18);
Camara_EnviaBytesI2C(0x586f ,0x1f);
Camara_EnviaBytesI2C(0x5870 ,0x1c);
Camara_EnviaBytesI2C(0x5871 ,0x16);
Camara_EnviaBytesI2C(0x5872 ,0x10);
Camara_EnviaBytesI2C(0x5873 ,0xf );
Camara_EnviaBytesI2C(0x5874 ,0x13);
Camara_EnviaBytesI2C(0x5875 ,0x1c);
Camara_EnviaBytesI2C(0x5876 ,0x1e);
Camara_EnviaBytesI2C(0x5877 ,0x17);
Camara_EnviaBytesI2C(0x5878 ,0x11);
Camara_EnviaBytesI2C(0x5879 ,0x11);
Camara_EnviaBytesI2C(0x587a ,0x14);
Camara_EnviaBytesI2C(0x587b ,0x1e);
Camara_EnviaBytesI2C(0x587c ,0x1c);
Camara_EnviaBytesI2C(0x587d ,0x1c);
Camara_EnviaBytesI2C(0x587e ,0x1a);
Camara_EnviaBytesI2C(0x587f ,0x1a);
Camara_EnviaBytesI2C(0x5880 ,0x1b);
Camara_EnviaBytesI2C(0x5881 ,0x1f);
Camara_EnviaBytesI2C(0x5882 ,0x14);
Camara_EnviaBytesI2C(0x5883 ,0x1a);

Camara_EnviaBytesI2C(0x5884 ,0x1d);
Camara_EnviaBytesI2C(0x5885 ,0x1e);
Camara_EnviaBytesI2C(0x5886 ,0x1a);
Camara_EnviaBytesI2C(0x5887 ,0x1a);
//;AWB
Camara_EnviaBytesI2C(0x5180 ,0xff);
Camara_EnviaBytesI2C(0x5181 ,0x52);
Camara_EnviaBytesI2C(0x5182 ,0x11);
Camara_EnviaBytesI2C(0x5183 ,0x14);
Camara_EnviaBytesI2C(0x5184 ,0x25);
Camara_EnviaBytesI2C(0x5185 ,0x24);
Camara_EnviaBytesI2C(0x5186 ,0x14);
Camara_EnviaBytesI2C(0x5187 ,0x14);
Camara_EnviaBytesI2C(0x5188 ,0x14);
Camara_EnviaBytesI2C(0x5189 ,0x69);
Camara_EnviaBytesI2C(0x518a ,0x60);
Camara_EnviaBytesI2C(0x518b ,0xa2);
Camara_EnviaBytesI2C(0x518c ,0x9c);
Camara_EnviaBytesI2C(0x518d ,0x36);
Camara_EnviaBytesI2C(0x518e ,0x34);
Camara_EnviaBytesI2C(0x518f ,0x54);
Camara_EnviaBytesI2C(0x5190 ,0x4c);
Camara_EnviaBytesI2C(0x5191 ,0xf8);
Camara_EnviaBytesI2C(0x5192 ,0x04);
Camara_EnviaBytesI2C(0x5193 ,0x70);
Camara_EnviaBytesI2C(0x5194 ,0xf0);
Camara_EnviaBytesI2C(0x5195 ,0xf0);
Camara_EnviaBytesI2C(0x5196 ,0x03);
Camara_EnviaBytesI2C(0x5197 ,0x01);
Camara_EnviaBytesI2C(0x5198 ,0x05);
Camara_EnviaBytesI2C(0x5199 ,0x2f);
Camara_EnviaBytesI2C(0x519a ,0x04);
Camara_EnviaBytesI2C(0x519b ,0x00);
Camara_EnviaBytesI2C(0x519c ,0x06);
Camara_EnviaBytesI2C(0x519d ,0xa0);
Camara_EnviaBytesI2C(0x519e ,0xa0);
//;D/S
Camara_EnviaBytesI2C(0x528a ,0x00);
Camara_EnviaBytesI2C(0x528b ,0x01);
Camara_EnviaBytesI2C(0x528c ,0x04);
Camara_EnviaBytesI2C(0x528d ,0x08);
Camara_EnviaBytesI2C(0x528e ,0x10);
Camara_EnviaBytesI2C(0x528f ,0x20);
Camara_EnviaBytesI2C(0x5290 ,0x30);
Camara_EnviaBytesI2C(0x5292 ,0x00);
Camara_EnviaBytesI2C(0x5293 ,0x00);
Camara_EnviaBytesI2C(0x5294 ,0x00);
Camara_EnviaBytesI2C(0x5295 ,0x01);

Camara_EnviaBytesI2C(0x5296 ,0x00);
Camara_EnviaBytesI2C(0x5297 ,0x04);
Camara_EnviaBytesI2C(0x5298 ,0x00);
Camara_EnviaBytesI2C(0x5299 ,0x08);
Camara_EnviaBytesI2C(0x529a ,0x00);
Camara_EnviaBytesI2C(0x529b ,0x10);
Camara_EnviaBytesI2C(0x529c ,0x00);
Camara_EnviaBytesI2C(0x529d ,0x20);
Camara_EnviaBytesI2C(0x529e ,0x00);
Camara_EnviaBytesI2C(0x529f ,0x30);
Camara_EnviaBytesI2C(0x5282 ,0x00);
Camara_EnviaBytesI2C(0x5300 ,0x00);
Camara_EnviaBytesI2C(0x5301 ,0x20);
Camara_EnviaBytesI2C(0x5302 ,0x00);
Camara_EnviaBytesI2C(0x5303 ,0x7c);
Camara_EnviaBytesI2C(0x530c ,0x00);
Camara_EnviaBytesI2C(0x530d ,0x10);
Camara_EnviaBytesI2C(0x530e ,0x20);
Camara_EnviaBytesI2C(0x530f ,0x80);
Camara_EnviaBytesI2C(0x5310 ,0x20);
Camara_EnviaBytesI2C(0x5311 ,0x80);
Camara_EnviaBytesI2C(0x5308 ,0x20);
Camara_EnviaBytesI2C(0x5309 ,0x40);
Camara_EnviaBytesI2C(0x5304 ,0x00);
Camara_EnviaBytesI2C(0x5305 ,0x30);
Camara_EnviaBytesI2C(0x5306 ,0x00);
Camara_EnviaBytesI2C(0x5307 ,0x80);
Camara_EnviaBytesI2C(0x5314 ,0x08);
Camara_EnviaBytesI2C(0x5315 ,0x20);
Camara_EnviaBytesI2C(0x5319 ,0x30);
Camara_EnviaBytesI2C(0x5316 ,0x10);
Camara_EnviaBytesI2C(0x5317 ,0x00);
Camara_EnviaBytesI2C(0x5318 ,0x02);
//;CMX
Camara_EnviaBytesI2C(0x5380 ,0x01);
Camara_EnviaBytesI2C(0x5381 ,0x00);
Camara_EnviaBytesI2C(0x5382 ,0x00);
Camara_EnviaBytesI2C(0x5383 ,0x1f);
Camara_EnviaBytesI2C(0x5384 ,0x00);
Camara_EnviaBytesI2C(0x5385 ,0x06);
Camara_EnviaBytesI2C(0x5386 ,0x00);
Camara_EnviaBytesI2C(0x5387 ,0x00);
Camara_EnviaBytesI2C(0x5388 ,0x00);
Camara_EnviaBytesI2C(0x5389 ,0xE1);
Camara_EnviaBytesI2C(0x538A ,0x00);
Camara_EnviaBytesI2C(0x538B ,0x2B);
Camara_EnviaBytesI2C(0x538C ,0x00);
Camara_EnviaBytesI2C(0x538D ,0x00);

Camara_EnviaBytesI2C(0x538E ,0x00);
Camara_EnviaBytesI2C(0x538F ,0x10);
Camara_EnviaBytesI2C(0x5390 ,0x00);
Camara_EnviaBytesI2C(0x5391 ,0xB3);
Camara_EnviaBytesI2C(0x5392 ,0x00);
Camara_EnviaBytesI2C(0x5393 ,0xA6);
Camara_EnviaBytesI2C(0x5394 ,0x08);
//;GAMMA
Camara_EnviaBytesI2C(0x5480 ,0x0c);
Camara_EnviaBytesI2C(0x5481 ,0x18);
Camara_EnviaBytesI2C(0x5482 ,0x2f);
Camara_EnviaBytesI2C(0x5483 ,0x55);
Camara_EnviaBytesI2C(0x5484 ,0x64);
Camara_EnviaBytesI2C(0x5485 ,0x71);
Camara_EnviaBytesI2C(0x5486 ,0x7d);
Camara_EnviaBytesI2C(0x5487 ,0x87);
Camara_EnviaBytesI2C(0x5488 ,0x91);
Camara_EnviaBytesI2C(0x5489 ,0x9a);
Camara_EnviaBytesI2C(0x548A ,0xaa);
Camara_EnviaBytesI2C(0x548B ,0xb8);
Camara_EnviaBytesI2C(0x548C ,0xcd);
Camara_EnviaBytesI2C(0x548D ,0xdd);
Camara_EnviaBytesI2C(0x548E ,0xea);
Camara_EnviaBytesI2C(0x548F ,0x1d);
Camara_EnviaBytesI2C(0x5490 ,0x05);
Camara_EnviaBytesI2C(0x5491 ,0x00);
Camara_EnviaBytesI2C(0x5492 ,0x04);
Camara_EnviaBytesI2C(0x5493 ,0x20);
Camara_EnviaBytesI2C(0x5494 ,0x03);
Camara_EnviaBytesI2C(0x5495 ,0x60);
Camara_EnviaBytesI2C(0x5496 ,0x02);
Camara_EnviaBytesI2C(0x5497 ,0xB8);
Camara_EnviaBytesI2C(0x5498 ,0x02);
Camara_EnviaBytesI2C(0x5499 ,0x86);
Camara_EnviaBytesI2C(0x549A ,0x02);
Camara_EnviaBytesI2C(0x549B ,0x5B);
Camara_EnviaBytesI2C(0x549C ,0x02);
Camara_EnviaBytesI2C(0x549D ,0x3B);
Camara_EnviaBytesI2C(0x549E ,0x02);
Camara_EnviaBytesI2C(0x549F ,0x1C);
Camara_EnviaBytesI2C(0x54A0 ,0x02);
Camara_EnviaBytesI2C(0x54A1 ,0x04);
Camara_EnviaBytesI2C(0x54A2 ,0x01);
Camara_EnviaBytesI2C(0x54A3 ,0xED);
Camara_EnviaBytesI2C(0x54A4 ,0x01);
Camara_EnviaBytesI2C(0x54A5 ,0xC5);
Camara_EnviaBytesI2C(0x54A6 ,0x01);
Camara_EnviaBytesI2C(0x54A7 ,0xA5);

Camara_EnviaBytesI2C(0x54A8 ,0x01);
Camara_EnviaBytesI2C(0x54A9 ,0x6C);
Camara_EnviaBytesI2C(0x54AA ,0x01);
Camara_EnviaBytesI2C(0x54AB ,0x41);
Camara_EnviaBytesI2C(0x54AC ,0x01);
Camara_EnviaBytesI2C(0x54AD ,0x20);
Camara_EnviaBytesI2C(0x54AE ,0x00);
Camara_EnviaBytesI2C(0x54AF ,0x16);
Camara_EnviaBytesI2C(0x54B0 ,0x01);
Camara_EnviaBytesI2C(0x54B1 ,0x20);
Camara_EnviaBytesI2C(0x54B2 ,0x00);
Camara_EnviaBytesI2C(0x54B3 ,0x10);
Camara_EnviaBytesI2C(0x54B4 ,0x00);
Camara_EnviaBytesI2C(0x54B5 ,0xf0);
Camara_EnviaBytesI2C(0x54B6 ,0x00);
Camara_EnviaBytesI2C(0x54B7 ,0xDF);
Camara_EnviaBytesI2C(0x5402 ,0x3f);
Camara_EnviaBytesI2C(0x5403 ,0x00);
//;UV ADJUST
Camara_EnviaBytesI2C(0x5500 ,0x10);
Camara_EnviaBytesI2C(0x5502 ,0x00);
Camara_EnviaBytesI2C(0x5503 ,0x06);
Camara_EnviaBytesI2C(0x5504 ,0x00);
Camara_EnviaBytesI2C(0x5505 ,0x7f);
//;AE
Camara_EnviaBytesI2C(0x5025 ,0x80);
Camara_EnviaBytesI2C(0x3a0f ,0x30);
Camara_EnviaBytesI2C(0x3a10 ,0x28);
Camara_EnviaBytesI2C(0x3a1b ,0x30);
Camara_EnviaBytesI2C(0x3a1e ,0x28);
Camara_EnviaBytesI2C(0x3a11 ,0x61);
Camara_EnviaBytesI2C(0x3a1f ,0x10);
Camara_EnviaBytesI2C(0x5688 ,0xfd);
Camara_EnviaBytesI2C(0x5689 ,0xdf);
Camara_EnviaBytesI2C(0x568a ,0xfe);
Camara_EnviaBytesI2C(0x568b ,0xef);
Camara_EnviaBytesI2C(0x568c ,0xfe);
Camara_EnviaBytesI2C(0x568d ,0xef);
Camara_EnviaBytesI2C(0x568e ,0xaa);
Camara_EnviaBytesI2C(0x568f ,0xaa);

}

void Camara_Set_VGAprev_YUVmode(void) {

    //13.1.1 VGA Preview
    //; for the setting , 24M Mlck input and 24M Plck output
    //;15fps YUV mode
    Camara_EnviaBytesI2C(0x3103, 0x93);
    Camara_EnviaBytesI2C(0x3008, 0x82);
    Camara_EnviaBytesI2C(0x3017, 0x7f);
    Camara_EnviaBytesI2C(0x3018, 0xfc);
    Camara_EnviaBytesI2C(0x3810, 0xc2);
    Camara_EnviaBytesI2C(0x3615, 0xf0);
    Camara_EnviaBytesI2C(0x3000, 0x00);
    Camara_EnviaBytesI2C(0x3001, 0x00);
    Camara_EnviaBytesI2C(0x3002, 0x5c);
    Camara_EnviaBytesI2C(0x3003, 0x00);
    Camara_EnviaBytesI2C(0x3004, 0xff);
    Camara_EnviaBytesI2C(0x3005, 0xff);
    Camara_EnviaBytesI2C(0x3006, 0x43);
    Camara_EnviaBytesI2C(0x3007, 0x37);
    Camara_EnviaBytesI2C(0x3011, 0x08);
    Camara_EnviaBytesI2C(0x3010, 0x10);
    Camara_EnviaBytesI2C(0x460c, 0x22);
    Camara_EnviaBytesI2C(0x3815, 0x04);
    Camara_EnviaBytesI2C(0x370c, 0xa0);
    Camara_EnviaBytesI2C(0x3602, 0xfc);
    Camara_EnviaBytesI2C(0x3612, 0xff);
    Camara_EnviaBytesI2C(0x3634, 0xc0);
    Camara_EnviaBytesI2C(0x3613, 0x00);
    Camara_EnviaBytesI2C(0x3605, 0x7c);
    Camara_EnviaBytesI2C(0x3621, 0x09);
    Camara_EnviaBytesI2C(0x3622, 0x60);
    Camara_EnviaBytesI2C(0x3604, 0x40);
    Camara_EnviaBytesI2C(0x3603, 0xa7);
    Camara_EnviaBytesI2C(0x3603, 0x27);
    Camara_EnviaBytesI2C(0x4000, 0x21);
    Camara_EnviaBytesI2C(0x401d, 0x22);
    Camara_EnviaBytesI2C(0x3600, 0x54);
    Camara_EnviaBytesI2C(0x3605, 0x04);
    Camara_EnviaBytesI2C(0x3606, 0x3f);
    Camara_EnviaBytesI2C(0x3c01, 0x80);
    Camara_EnviaBytesI2C(0x5000, 0x4f);
    Camara_EnviaBytesI2C(0x5020, 0x04);

    Camara_EnviaBytesI2C(0x5181, 0x79);
    Camara_EnviaBytesI2C(0x5182, 0x00);
    Camara_EnviaBytesI2C(0x5185, 0x22);
    Camara_EnviaBytesI2C(0x5197, 0x01);
    Camara_EnviaBytesI2C(0x5001, 0xff);
    Camara_EnviaBytesI2C(0x5500, 0x0a);
    Camara_EnviaBytesI2C(0x5504, 0x00);
    Camara_EnviaBytesI2C(0x5505, 0x7f);
    Camara_EnviaBytesI2C(0x5080, 0x08);
    Camara_EnviaBytesI2C(0x300e, 0x18);
    Camara_EnviaBytesI2C(0x4610, 0x00);
    Camara_EnviaBytesI2C(0x471d, 0x05);
    Camara_EnviaBytesI2C(0x4708, 0x06);
    Camara_EnviaBytesI2C(0x3808, 0x02);
    Camara_EnviaBytesI2C(0x3809, 0x80);
    Camara_EnviaBytesI2C(0x380a, 0x01);
    Camara_EnviaBytesI2C(0x380b, 0xe0);
    Camara_EnviaBytesI2C(0x380e, 0x07);
    Camara_EnviaBytesI2C(0x380f, 0xd0);
    Camara_EnviaBytesI2C(0x501f, 0x00);
    Camara_EnviaBytesI2C(0x5000, 0x4f);
    Camara_EnviaBytesI2C(0x4300, 0x30);
    Camara_EnviaBytesI2C(0x3503, 0x07);
    Camara_EnviaBytesI2C(0x3501, 0x73);
    Camara_EnviaBytesI2C(0x3502, 0x80);
    Camara_EnviaBytesI2C(0x350b, 0x00);
    Camara_EnviaBytesI2C(0x3503, 0x07);
    Camara_EnviaBytesI2C(0x3824, 0x11);
    Camara_EnviaBytesI2C(0x3501, 0x1e);
    Camara_EnviaBytesI2C(0x3502, 0x80);
    Camara_EnviaBytesI2C(0x350b, 0x7f);
    Camara_EnviaBytesI2C(0x380c, 0x0c);
    Camara_EnviaBytesI2C(0x380d, 0x80);
    Camara_EnviaBytesI2C(0x380e, 0x03);
    Camara_EnviaBytesI2C(0x380f, 0xe8);
    Camara_EnviaBytesI2C(0x3a0d, 0x04);
    Camara_EnviaBytesI2C(0x3a0e, 0x03);
    Camara_EnviaBytesI2C(0x3818, 0xc1);
    Camara_EnviaBytesI2C(0x3705, 0xdb);
    Camara_EnviaBytesI2C(0x370a, 0x81);
    Camara_EnviaBytesI2C(0x3801, 0x80);
    Camara_EnviaBytesI2C(0x3621, 0x87);
    Camara_EnviaBytesI2C(0x3801, 0x50);
    Camara_EnviaBytesI2C(0x3803, 0x08);
    Camara_EnviaBytesI2C(0x3827, 0x08);
    Camara_EnviaBytesI2C(0x3810, 0x40);
    Camara_EnviaBytesI2C(0x3804, 0x05);
    Camara_EnviaBytesI2C(0x3805, 0x00);

    Camara_EnviaBytesI2C(0x5682, 0x05);
    Camara_EnviaBytesI2C(0x5683, 0x00);
    Camara_EnviaBytesI2C(0x3806, 0x03);
    Camara_EnviaBytesI2C(0x3807, 0xc0);
    Camara_EnviaBytesI2C(0x5686, 0x03);
    Camara_EnviaBytesI2C(0x5687, 0xbc);
    Camara_EnviaBytesI2C(0x3a00, 0x78);
    Camara_EnviaBytesI2C(0x3a1a, 0x05);
    Camara_EnviaBytesI2C(0x3a13, 0x30);
    Camara_EnviaBytesI2C(0x3a18, 0x00);
    Camara_EnviaBytesI2C(0x3a19, 0x7c);
    Camara_EnviaBytesI2C(0x3a08, 0x12);
    Camara_EnviaBytesI2C(0x3a09, 0xc0);
    Camara_EnviaBytesI2C(0x3a0a, 0x0f);
    Camara_EnviaBytesI2C(0x3a0b, 0xa0);
    Camara_EnviaBytesI2C(0x350c, 0x07);
    Camara_EnviaBytesI2C(0x350d, 0xd0);
    Camara_EnviaBytesI2C(0x3500, 0x00);
    Camara_EnviaBytesI2C(0x3501, 0x00);
    Camara_EnviaBytesI2C(0x3502, 0x00);
    Camara_EnviaBytesI2C(0x350a, 0x00);
    Camara_EnviaBytesI2C(0x350b, 0x00);
    Camara_EnviaBytesI2C(0x3503, 0x00);
    Camara_EnviaBytesI2C(0x528a, 0x02);
    Camara_EnviaBytesI2C(0x528b, 0x04);
    Camara_EnviaBytesI2C(0x528c, 0x08);
    Camara_EnviaBytesI2C(0x528d, 0x08);
    Camara_EnviaBytesI2C(0x528e, 0x08);
    Camara_EnviaBytesI2C(0x528f, 0x10);
    Camara_EnviaBytesI2C(0x5290, 0x10);
    Camara_EnviaBytesI2C(0x5292, 0x00);
    Camara_EnviaBytesI2C(0x5293, 0x02);
    Camara_EnviaBytesI2C(0x5294, 0x00);
    Camara_EnviaBytesI2C(0x5295, 0x02);
    Camara_EnviaBytesI2C(0x5296, 0x00);
    Camara_EnviaBytesI2C(0x5297, 0x02);
    Camara_EnviaBytesI2C(0x5298, 0x00);
    Camara_EnviaBytesI2C(0x5299, 0x02);
    Camara_EnviaBytesI2C(0x529a, 0x00);
    Camara_EnviaBytesI2C(0x529b, 0x02);
    Camara_EnviaBytesI2C(0x529c, 0x00);
    Camara_EnviaBytesI2C(0x529d, 0x02);
    Camara_EnviaBytesI2C(0x529e, 0x00);
    Camara_EnviaBytesI2C(0x529f, 0x02);
    Camara_EnviaBytesI2C(0x3030, 0x2b);
    Camara_EnviaBytesI2C(0x3a02, 0x00);
    Camara_EnviaBytesI2C(0x3a03, 0x7d);
    Camara_EnviaBytesI2C(0x3a04, 0x00);

    Camara_EnviaBytesI2C(0x3a14, 0x00);
    Camara_EnviaBytesI2C(0x3a15, 0x7d);
    Camara_EnviaBytesI2C(0x3a16, 0x00);
    Camara_EnviaBytesI2C(0x3a00, 0x78);
    Camara_EnviaBytesI2C(0x3a08, 0x09);
    Camara_EnviaBytesI2C(0x3a09, 0x60);
    Camara_EnviaBytesI2C(0x3a0a, 0x07);
    Camara_EnviaBytesI2C(0x3a0b, 0xd0);
    Camara_EnviaBytesI2C(0x3a0d, 0x08);
    Camara_EnviaBytesI2C(0x3a0e, 0x06);
    Camara_EnviaBytesI2C(0x5193, 0x70);
    Camara_EnviaBytesI2C(0x589b, 0x04);
    Camara_EnviaBytesI2C(0x589a, 0xc5);
    Camara_EnviaBytesI2C(0x401e, 0x20);
    Camara_EnviaBytesI2C(0x4001, 0x42);
    Camara_EnviaBytesI2C(0x401c, 0x04);
    Camara_EnviaBytesI2C(0x528a, 0x01);
    Camara_EnviaBytesI2C(0x528b, 0x04);
    Camara_EnviaBytesI2C(0x528c, 0x08);
    Camara_EnviaBytesI2C(0x528d, 0x10);
    Camara_EnviaBytesI2C(0x528e, 0x20);
    Camara_EnviaBytesI2C(0x528f, 0x28);
    Camara_EnviaBytesI2C(0x5290, 0x30);
    Camara_EnviaBytesI2C(0x5292, 0x00);
    Camara_EnviaBytesI2C(0x5293, 0x01);
    Camara_EnviaBytesI2C(0x5294, 0x00);
    Camara_EnviaBytesI2C(0x5295, 0x04);
    Camara_EnviaBytesI2C(0x5296, 0x00);
    Camara_EnviaBytesI2C(0x5297, 0x08);
    Camara_EnviaBytesI2C(0x5298, 0x00);
    Camara_EnviaBytesI2C(0x5299, 0x10);
    Camara_EnviaBytesI2C(0x529a, 0x00);
    Camara_EnviaBytesI2C(0x529b, 0x20);
    Camara_EnviaBytesI2C(0x529c, 0x00);
    Camara_EnviaBytesI2C(0x529d, 0x28);
    Camara_EnviaBytesI2C(0x529e, 0x00);
    Camara_EnviaBytesI2C(0x529f, 0x30);
    Camara_EnviaBytesI2C(0x5282, 0x00);
    Camara_EnviaBytesI2C(0x5300, 0x00);
    Camara_EnviaBytesI2C(0x5301, 0x20);
    Camara_EnviaBytesI2C(0x5302, 0x00);
    Camara_EnviaBytesI2C(0x5303, 0x7c);
    Camara_EnviaBytesI2C(0x530c, 0x00);
    Camara_EnviaBytesI2C(0x530d, 0x0c);
    Camara_EnviaBytesI2C(0x530e, 0x20);
    Camara_EnviaBytesI2C(0x530f, 0x80);
    Camara_EnviaBytesI2C(0x5310, 0x20);
    Camara_EnviaBytesI2C(0x5311, 0x80);

    Camara_EnviaBytesI2C(0x5308, 0x20);
    Camara_EnviaBytesI2C(0x5309, 0x40);
    Camara_EnviaBytesI2C(0x5304, 0x00);
    Camara_EnviaBytesI2C(0x5305, 0x30);
    Camara_EnviaBytesI2C(0x5306, 0x00);
    Camara_EnviaBytesI2C(0x5307, 0x80);
    Camara_EnviaBytesI2C(0x5314, 0x08);
    Camara_EnviaBytesI2C(0x5315, 0x20);
    Camara_EnviaBytesI2C(0x5319, 0x30);
    Camara_EnviaBytesI2C(0x5316, 0x10);
    Camara_EnviaBytesI2C(0x5317, 0x00);
    Camara_EnviaBytesI2C(0x5318, 0x02);
    Camara_EnviaBytesI2C(0x5402, 0x3f);
    Camara_EnviaBytesI2C(0x5403, 0x00);
    Camara_EnviaBytesI2C(0x3406, 0x00);
    Camara_EnviaBytesI2C(0x5180, 0xff);
    Camara_EnviaBytesI2C(0x5181, 0x52);
    Camara_EnviaBytesI2C(0x5182, 0x11);
    Camara_EnviaBytesI2C(0x5183, 0x14);
    Camara_EnviaBytesI2C(0x5184, 0x25);
    Camara_EnviaBytesI2C(0x5185, 0x24);
    Camara_EnviaBytesI2C(0x5186, 0x06);
    Camara_EnviaBytesI2C(0x5187, 0x08);
    Camara_EnviaBytesI2C(0x5188, 0x08);
    Camara_EnviaBytesI2C(0x5189, 0x7c);
    Camara_EnviaBytesI2C(0x518a, 0x60);
    Camara_EnviaBytesI2C(0x518b, 0xb2);
    Camara_EnviaBytesI2C(0x518c, 0xb2);
    Camara_EnviaBytesI2C(0x518d, 0x44);
    Camara_EnviaBytesI2C(0x518e, 0x3d);
    Camara_EnviaBytesI2C(0x518f, 0x58);
    Camara_EnviaBytesI2C(0x5190, 0x46);
    Camara_EnviaBytesI2C(0x5191, 0xf8);
    Camara_EnviaBytesI2C(0x5192, 0x04);
    Camara_EnviaBytesI2C(0x5193, 0x70);
    Camara_EnviaBytesI2C(0x5194, 0xf0);
    Camara_EnviaBytesI2C(0x5195, 0xf0);
    Camara_EnviaBytesI2C(0x5196, 0x03);
    Camara_EnviaBytesI2C(0x5197, 0x01);
    Camara_EnviaBytesI2C(0x5198, 0x04);
    Camara_EnviaBytesI2C(0x5199, 0x12);
    Camara_EnviaBytesI2C(0x519a, 0x04);
    Camara_EnviaBytesI2C(0x519b, 0x00);
    Camara_EnviaBytesI2C(0x519c, 0x06);
    Camara_EnviaBytesI2C(0x519d, 0x82);
    Camara_EnviaBytesI2C(0x519e, 0x00);
    Camara_EnviaBytesI2C(0x5025, 0x80);
    Camara_EnviaBytesI2C(0x5583, 0x40);

    Camara_EnviaBytesI2C(0x5584, 0x40);
    Camara_EnviaBytesI2C(0x5580, 0x02);
    Camara_EnviaBytesI2C(0x5000, 0xcf);
    Camara_EnviaBytesI2C(0x3710, 0x10);
    Camara_EnviaBytesI2C(0x3632, 0x51);
    Camara_EnviaBytesI2C(0x3702, 0x10);
    Camara_EnviaBytesI2C(0x3703, 0xb2);
    Camara_EnviaBytesI2C(0x3704, 0x18);
    Camara_EnviaBytesI2C(0x370b, 0x40);
    Camara_EnviaBytesI2C(0x370d, 0x03);
    Camara_EnviaBytesI2C(0x3631, 0x01);
    Camara_EnviaBytesI2C(0x3632, 0x52);
    Camara_EnviaBytesI2C(0x3606, 0x24);
    Camara_EnviaBytesI2C(0x3620, 0x96);
    Camara_EnviaBytesI2C(0x5785, 0x07);
    Camara_EnviaBytesI2C(0x3a13, 0x30);
    Camara_EnviaBytesI2C(0x3600, 0x52);
    Camara_EnviaBytesI2C(0x3604, 0x48);
    Camara_EnviaBytesI2C(0x3606, 0x1b);
    Camara_EnviaBytesI2C(0x370d, 0x0b);
    Camara_EnviaBytesI2C(0x370f, 0xc0);
    Camara_EnviaBytesI2C(0x3709, 0x01);
    Camara_EnviaBytesI2C(0x3823, 0x00);
    Camara_EnviaBytesI2C(0x5007, 0x00);
    Camara_EnviaBytesI2C(0x5009, 0x00);
    Camara_EnviaBytesI2C(0x5011, 0x00);
    Camara_EnviaBytesI2C(0x5013, 0x00);
    Camara_EnviaBytesI2C(0x519e, 0x00);
    Camara_EnviaBytesI2C(0x5086, 0x00);
    Camara_EnviaBytesI2C(0x5087, 0x00);
    Camara_EnviaBytesI2C(0x5088, 0x00);
    Camara_EnviaBytesI2C(0x5089, 0x00);
    Camara_EnviaBytesI2C(0x302b, 0x00);
    Camara_EnviaBytesI2C(0x5001, 0xFF);
    Camara_EnviaBytesI2C(0x5583, 0x50);
    Camara_EnviaBytesI2C(0x5584, 0x50);
    Camara_EnviaBytesI2C(0x5580, 0x02);
    Camara_EnviaBytesI2C(0x3c01, 0x80);
    Camara_EnviaBytesI2C(0x3c00, 0x04);
    //;LENS
    Camara_EnviaBytesI2C(0x5800, 0x48);
    Camara_EnviaBytesI2C(0x5801, 0x31);
    Camara_EnviaBytesI2C(0x5802, 0x21);
    Camara_EnviaBytesI2C(0x5803, 0x1b);
    Camara_EnviaBytesI2C(0x5804, 0x1a);
    Camara_EnviaBytesI2C(0x5805, 0x1e);
    Camara_EnviaBytesI2C(0x5806, 0x29);
    Camara_EnviaBytesI2C(0x5807, 0x38);

    Camara_EnviaBytesI2C(0x5808, 0x26);
    Camara_EnviaBytesI2C(0x5809, 0x17);
    Camara_EnviaBytesI2C(0x580a, 0x11);
    Camara_EnviaBytesI2C(0x580b, 0xe);
    Camara_EnviaBytesI2C(0x580c, 0xd);
    Camara_EnviaBytesI2C(0x580d, 0xe);
    Camara_EnviaBytesI2C(0x580e, 0x13);
    Camara_EnviaBytesI2C(0x580f, 0x1a);
    Camara_EnviaBytesI2C(0x5810, 0x15);
    Camara_EnviaBytesI2C(0x5811, 0xd);
    Camara_EnviaBytesI2C(0x5812, 0x8);
    Camara_EnviaBytesI2C(0x5813, 0x5);
    Camara_EnviaBytesI2C(0x5814, 0x4);
    Camara_EnviaBytesI2C(0x5815, 0x5);
    Camara_EnviaBytesI2C(0x5816, 0x9);
    Camara_EnviaBytesI2C(0x5817, 0xd);
    Camara_EnviaBytesI2C(0x5818, 0x11);
    Camara_EnviaBytesI2C(0x5819, 0xa);
    Camara_EnviaBytesI2C(0x581a, 0x4);
    Camara_EnviaBytesI2C(0x581b, 0x0);
    Camara_EnviaBytesI2C(0x581c, 0x0);
    Camara_EnviaBytesI2C(0x581d, 0x1);
    Camara_EnviaBytesI2C(0x581e, 0x6);
    Camara_EnviaBytesI2C(0x581f, 0x9);
    Camara_EnviaBytesI2C(0x5820, 0x12);
    Camara_EnviaBytesI2C(0x5821, 0xb);
    Camara_EnviaBytesI2C(0x5822, 0x4);
    Camara_EnviaBytesI2C(0x5823, 0x0);
    Camara_EnviaBytesI2C(0x5824, 0x0);
    Camara_EnviaBytesI2C(0x5825, 0x1);
    Camara_EnviaBytesI2C(0x5826, 0x6);
    Camara_EnviaBytesI2C(0x5827, 0xa);
    Camara_EnviaBytesI2C(0x5828, 0x17);
    Camara_EnviaBytesI2C(0x5829, 0xf);
    Camara_EnviaBytesI2C(0x582a, 0x9);
    Camara_EnviaBytesI2C(0x582b, 0x6);
    Camara_EnviaBytesI2C(0x582c, 0x5);
    Camara_EnviaBytesI2C(0x582d, 0x6);
    Camara_EnviaBytesI2C(0x582e, 0xa);
    Camara_EnviaBytesI2C(0x582f, 0xe);
    Camara_EnviaBytesI2C(0x5830, 0x28);
    Camara_EnviaBytesI2C(0x5831, 0x1a);
    Camara_EnviaBytesI2C(0x5832, 0x11);
    Camara_EnviaBytesI2C(0x5833, 0xe);
    Camara_EnviaBytesI2C(0x5834, 0xe);
    Camara_EnviaBytesI2C(0x5835, 0xf);
    Camara_EnviaBytesI2C(0x5836, 0x15);
    Camara_EnviaBytesI2C(0x5837, 0x1d);

    Camara_EnviaBytesI2C(0x5838, 0x6e);
    Camara_EnviaBytesI2C(0x5839, 0x39);
    Camara_EnviaBytesI2C(0x583a, 0x27);
    Camara_EnviaBytesI2C(0x583b, 0x1f);
    Camara_EnviaBytesI2C(0x583c, 0x1e);
    Camara_EnviaBytesI2C(0x583d, 0x23);
    Camara_EnviaBytesI2C(0x583e, 0x2f);
    Camara_EnviaBytesI2C(0x583f, 0x41);
    Camara_EnviaBytesI2C(0x5840, 0xe);
    Camara_EnviaBytesI2C(0x5841, 0xc);
    Camara_EnviaBytesI2C(0x5842, 0xd);
    Camara_EnviaBytesI2C(0x5843, 0xc);
    Camara_EnviaBytesI2C(0x5844, 0xc);
    Camara_EnviaBytesI2C(0x5845, 0xc);
    Camara_EnviaBytesI2C(0x5846, 0xc);
    Camara_EnviaBytesI2C(0x5847, 0xc);
    Camara_EnviaBytesI2C(0x5848, 0xd);
    Camara_EnviaBytesI2C(0x5849, 0xe);
    Camara_EnviaBytesI2C(0x584a, 0xe);
    Camara_EnviaBytesI2C(0x584b, 0xa);
    Camara_EnviaBytesI2C(0x584c, 0xe);
    Camara_EnviaBytesI2C(0x584d, 0xe);
    Camara_EnviaBytesI2C(0x584e, 0x10);
    Camara_EnviaBytesI2C(0x584f, 0x10);
    Camara_EnviaBytesI2C(0x5850, 0x11);
    Camara_EnviaBytesI2C(0x5851, 0xa);
    Camara_EnviaBytesI2C(0x5852, 0xf);
    Camara_EnviaBytesI2C(0x5853, 0xe);
    Camara_EnviaBytesI2C(0x5854, 0x10);
    Camara_EnviaBytesI2C(0x5855, 0x10);
    Camara_EnviaBytesI2C(0x5856, 0x10);
    Camara_EnviaBytesI2C(0x5857, 0xa);
    Camara_EnviaBytesI2C(0x5858, 0xe);
    Camara_EnviaBytesI2C(0x5859, 0xe);
    Camara_EnviaBytesI2C(0x585a, 0xf);
    Camara_EnviaBytesI2C(0x585b, 0xf);
    Camara_EnviaBytesI2C(0x585c, 0xf);
    Camara_EnviaBytesI2C(0x585d, 0xa);
    Camara_EnviaBytesI2C(0x585e, 0x9);
    Camara_EnviaBytesI2C(0x585f, 0xd);
    Camara_EnviaBytesI2C(0x5860, 0xc);
    Camara_EnviaBytesI2C(0x5861, 0xb);
    Camara_EnviaBytesI2C(0x5862, 0xd);
    Camara_EnviaBytesI2C(0x5863, 0x7);
    Camara_EnviaBytesI2C(0x5864, 0x17);
    Camara_EnviaBytesI2C(0x5865, 0x14);
    Camara_EnviaBytesI2C(0x5866, 0x18);
    Camara_EnviaBytesI2C(0x5867, 0x18);

    Camara_EnviaBytesI2C(0x5868, 0x16);
    Camara_EnviaBytesI2C(0x5869, 0x12);
    Camara_EnviaBytesI2C(0x586a, 0x1b);
    Camara_EnviaBytesI2C(0x586b, 0x1a);
    Camara_EnviaBytesI2C(0x586c, 0x16);
    Camara_EnviaBytesI2C(0x586d, 0x16);
    Camara_EnviaBytesI2C(0x586e, 0x18);
    Camara_EnviaBytesI2C(0x586f, 0x1f);
    Camara_EnviaBytesI2C(0x5870, 0x1c);
    Camara_EnviaBytesI2C(0x5871, 0x16);
    Camara_EnviaBytesI2C(0x5872, 0x10);
    Camara_EnviaBytesI2C(0x5873, 0xf);
    Camara_EnviaBytesI2C(0x5874, 0x13);
    Camara_EnviaBytesI2C(0x5875, 0x1c);
    Camara_EnviaBytesI2C(0x5876, 0x1e);
    Camara_EnviaBytesI2C(0x5877, 0x17);
    Camara_EnviaBytesI2C(0x5878, 0x11);
    Camara_EnviaBytesI2C(0x5879, 0x11);
    Camara_EnviaBytesI2C(0x587a, 0x14);
    Camara_EnviaBytesI2C(0x587b, 0x1e);
    Camara_EnviaBytesI2C(0x587c, 0x1c);
    Camara_EnviaBytesI2C(0x587d, 0x1c);
    Camara_EnviaBytesI2C(0x587e, 0x1a);
    Camara_EnviaBytesI2C(0x587f, 0x1a);
    Camara_EnviaBytesI2C(0x5880, 0x1b);
    Camara_EnviaBytesI2C(0x5881, 0x1f);
    Camara_EnviaBytesI2C(0x5882, 0x14);
    Camara_EnviaBytesI2C(0x5883, 0x1a);
    Camara_EnviaBytesI2C(0x5884, 0x1d);
    Camara_EnviaBytesI2C(0x5885, 0x1e);
    Camara_EnviaBytesI2C(0x5886, 0x1a);
    Camara_EnviaBytesI2C(0x5887, 0x1a);
    //;AWB
    Camara_EnviaBytesI2C(0x5180, 0xff);
    Camara_EnviaBytesI2C(0x5181, 0x52);
    Camara_EnviaBytesI2C(0x5182, 0x11);
    Camara_EnviaBytesI2C(0x5183, 0x14);
    Camara_EnviaBytesI2C(0x5184, 0x25);
    Camara_EnviaBytesI2C(0x5185, 0x24);
    Camara_EnviaBytesI2C(0x5186, 0x14);
    Camara_EnviaBytesI2C(0x5187, 0x14);
    Camara_EnviaBytesI2C(0x5188, 0x14);
    Camara_EnviaBytesI2C(0x5189, 0x69);
    Camara_EnviaBytesI2C(0x518a, 0x60);
    Camara_EnviaBytesI2C(0x518b, 0xa2);
    Camara_EnviaBytesI2C(0x518c, 0x9c);
    Camara_EnviaBytesI2C(0x518d, 0x36);
    Camara_EnviaBytesI2C(0x518e, 0x34);

    Camara_EnviaBytesI2C(0x518f, 0x54);
    Camara_EnviaBytesI2C(0x5190, 0x4c);
    Camara_EnviaBytesI2C(0x5191, 0xf8);
    Camara_EnviaBytesI2C(0x5192, 0x04);
    Camara_EnviaBytesI2C(0x5193, 0x70);
    Camara_EnviaBytesI2C(0x5194, 0xf0);
    Camara_EnviaBytesI2C(0x5195, 0xf0);
    Camara_EnviaBytesI2C(0x5196, 0x03);
    Camara_EnviaBytesI2C(0x5197, 0x01);
    Camara_EnviaBytesI2C(0x5198, 0x05);
    Camara_EnviaBytesI2C(0x5199, 0x2f);
    Camara_EnviaBytesI2C(0x519a, 0x04);
    Camara_EnviaBytesI2C(0x519b, 0x00);
    Camara_EnviaBytesI2C(0x519c, 0x06);
    Camara_EnviaBytesI2C(0x519d, 0xa0);
    Camara_EnviaBytesI2C(0x519e, 0xa0);
    //;D/S
    Camara_EnviaBytesI2C(0x528a, 0x00);
    Camara_EnviaBytesI2C(0x528b, 0x01);
    Camara_EnviaBytesI2C(0x528c, 0x04);
    Camara_EnviaBytesI2C(0x528d, 0x08);
    Camara_EnviaBytesI2C(0x528e, 0x10);
    Camara_EnviaBytesI2C(0x528f, 0x20);
    Camara_EnviaBytesI2C(0x5290, 0x30);
    Camara_EnviaBytesI2C(0x5292, 0x00);
    Camara_EnviaBytesI2C(0x5293, 0x00);
    Camara_EnviaBytesI2C(0x5294, 0x00);
    Camara_EnviaBytesI2C(0x5295, 0x01);
    Camara_EnviaBytesI2C(0x5296, 0x00);
    Camara_EnviaBytesI2C(0x5297, 0x04);
    Camara_EnviaBytesI2C(0x5298, 0x00);
    Camara_EnviaBytesI2C(0x5299, 0x08);
    Camara_EnviaBytesI2C(0x529a, 0x00);
    Camara_EnviaBytesI2C(0x529b, 0x10);
    Camara_EnviaBytesI2C(0x529c, 0x00);
    Camara_EnviaBytesI2C(0x529d, 0x20);
    Camara_EnviaBytesI2C(0x529e, 0x00);
    Camara_EnviaBytesI2C(0x529f, 0x30);
    Camara_EnviaBytesI2C(0x5282, 0x00);
    Camara_EnviaBytesI2C(0x5300, 0x00);
    Camara_EnviaBytesI2C(0x5301, 0x20);
    Camara_EnviaBytesI2C(0x5302, 0x00);
    Camara_EnviaBytesI2C(0x5303, 0x7c);
    Camara_EnviaBytesI2C(0x530c, 0x00);
    Camara_EnviaBytesI2C(0x530d, 0x10);
    Camara_EnviaBytesI2C(0x530e, 0x20);
    Camara_EnviaBytesI2C(0x530f, 0x80);
    Camara_EnviaBytesI2C(0x5310, 0x20);

    Camara_EnviaBytesI2C(0x5311, 0x80);
    Camara_EnviaBytesI2C(0x5308, 0x20);
    Camara_EnviaBytesI2C(0x5309, 0x40);
    Camara_EnviaBytesI2C(0x5304, 0x00);
    Camara_EnviaBytesI2C(0x5305, 0x30);
    Camara_EnviaBytesI2C(0x5306, 0x00);
    Camara_EnviaBytesI2C(0x5307, 0x80);
    Camara_EnviaBytesI2C(0x5314, 0x08);
    Camara_EnviaBytesI2C(0x5315, 0x20);
    Camara_EnviaBytesI2C(0x5319, 0x30);
    Camara_EnviaBytesI2C(0x5316, 0x10);
    Camara_EnviaBytesI2C(0x5317, 0x00);
    Camara_EnviaBytesI2C(0x5318, 0x02);
    //;CMX
    Camara_EnviaBytesI2C(0x5380, 0x01);
    Camara_EnviaBytesI2C(0x5381, 0x00);
    Camara_EnviaBytesI2C(0x5382, 0x00);
    Camara_EnviaBytesI2C(0x5383, 0x1f);
    Camara_EnviaBytesI2C(0x5384, 0x00);
    Camara_EnviaBytesI2C(0x5385, 0x06);
    Camara_EnviaBytesI2C(0x5386, 0x00);
    Camara_EnviaBytesI2C(0x5387, 0x00);
    Camara_EnviaBytesI2C(0x5388, 0x00);
    Camara_EnviaBytesI2C(0x5389, 0xE1);
    Camara_EnviaBytesI2C(0x538A, 0x00);
    Camara_EnviaBytesI2C(0x538B, 0x2B);
    Camara_EnviaBytesI2C(0x538C, 0x00);
    Camara_EnviaBytesI2C(0x538D, 0x00);
    Camara_EnviaBytesI2C(0x538E, 0x00);
    Camara_EnviaBytesI2C(0x538F, 0x10);
    Camara_EnviaBytesI2C(0x5390, 0x00);
    Camara_EnviaBytesI2C(0x5391, 0xB3);
    Camara_EnviaBytesI2C(0x5392, 0x00);
    Camara_EnviaBytesI2C(0x5393, 0xA6);
    Camara_EnviaBytesI2C(0x5394, 0x08);
    //;GAMMA
    Camara_EnviaBytesI2C(0x5480, 0x0c);
    Camara_EnviaBytesI2C(0x5481, 0x18);
    Camara_EnviaBytesI2C(0x5482, 0x2f);
    Camara_EnviaBytesI2C(0x5483, 0x55);
    Camara_EnviaBytesI2C(0x5484, 0x64);
    Camara_EnviaBytesI2C(0x5485, 0x71);
    Camara_EnviaBytesI2C(0x5486, 0x7d);
    Camara_EnviaBytesI2C(0x5487, 0x87);
    Camara_EnviaBytesI2C(0x5488, 0x91);
    Camara_EnviaBytesI2C(0x5489, 0x9a);
    Camara_EnviaBytesI2C(0x548A, 0xaa);
    Camara_EnviaBytesI2C(0x548B, 0xb8);

    Camara_EnviaBytesI2C(0x548C, 0xcd);
    Camara_EnviaBytesI2C(0x548D, 0xdd);
    Camara_EnviaBytesI2C(0x548E, 0xea);
    Camara_EnviaBytesI2C(0x548F, 0x1d);
    Camara_EnviaBytesI2C(0x5490, 0x05);
    Camara_EnviaBytesI2C(0x5491, 0x00);
    Camara_EnviaBytesI2C(0x5492, 0x04);
    Camara_EnviaBytesI2C(0x5493, 0x20);
    Camara_EnviaBytesI2C(0x5494, 0x03);
    Camara_EnviaBytesI2C(0x5495, 0x60);
    Camara_EnviaBytesI2C(0x5496, 0x02);
    Camara_EnviaBytesI2C(0x5497, 0xB8);
    Camara_EnviaBytesI2C(0x5498, 0x02);
    Camara_EnviaBytesI2C(0x5499, 0x86);
    Camara_EnviaBytesI2C(0x549A, 0x02);
    Camara_EnviaBytesI2C(0x549B, 0x5B);
    Camara_EnviaBytesI2C(0x549C, 0x02);
    Camara_EnviaBytesI2C(0x549D, 0x3B);
    Camara_EnviaBytesI2C(0x549E, 0x02);
    Camara_EnviaBytesI2C(0x549F, 0x1C);
    Camara_EnviaBytesI2C(0x54A0, 0x02);
    Camara_EnviaBytesI2C(0x54A1, 0x04);
    Camara_EnviaBytesI2C(0x54A2, 0x01);
    Camara_EnviaBytesI2C(0x54A3, 0xED);
    Camara_EnviaBytesI2C(0x54A4, 0x01);
    Camara_EnviaBytesI2C(0x54A5, 0xC5);
    Camara_EnviaBytesI2C(0x54A6, 0x01);
    Camara_EnviaBytesI2C(0x54A7, 0xA5);
    Camara_EnviaBytesI2C(0x54A8, 0x01);
    Camara_EnviaBytesI2C(0x54A9, 0x6C);
    Camara_EnviaBytesI2C(0x54AA, 0x01);
    Camara_EnviaBytesI2C(0x54AB, 0x41);
    Camara_EnviaBytesI2C(0x54AC, 0x01);
    Camara_EnviaBytesI2C(0x54AD, 0x20);
    Camara_EnviaBytesI2C(0x54AE, 0x00);
    Camara_EnviaBytesI2C(0x54AF, 0x16);
    Camara_EnviaBytesI2C(0x54B0, 0x01);
    Camara_EnviaBytesI2C(0x54B1, 0x20);
    Camara_EnviaBytesI2C(0x54B2, 0x00);
    Camara_EnviaBytesI2C(0x54B3, 0x10);
    Camara_EnviaBytesI2C(0x54B4, 0x00);
    Camara_EnviaBytesI2C(0x54B5, 0xf0);
    Camara_EnviaBytesI2C(0x54B6, 0x00);
    Camara_EnviaBytesI2C(0x54B7, 0xDF);
    Camara_EnviaBytesI2C(0x5402, 0x3f);
    Camara_EnviaBytesI2C(0x5403, 0x00);
    //;UV ADJUST
    Camara_EnviaBytesI2C(0x5500, 0x10);

    Camara_EnviaBytesI2C(0x5502, 0x00);
    Camara_EnviaBytesI2C(0x5503, 0x06);
    Camara_EnviaBytesI2C(0x5504, 0x00);
    Camara_EnviaBytesI2C(0x5505, 0x7f);
    //;AE
    Camara_EnviaBytesI2C(0x5025, 0x80);
    Camara_EnviaBytesI2C(0x3a0f, 0x30);
    Camara_EnviaBytesI2C(0x3a10, 0x28);
    Camara_EnviaBytesI2C(0x3a1b, 0x30);
    Camara_EnviaBytesI2C(0x3a1e, 0x28);
    Camara_EnviaBytesI2C(0x3a11, 0x61);
    Camara_EnviaBytesI2C(0x3a1f, 0x10);
    Camara_EnviaBytesI2C(0x5688, 0xfd);
    Camara_EnviaBytesI2C(0x5689, 0xdf);
    Camara_EnviaBytesI2C(0x568a, 0xfe);
    Camara_EnviaBytesI2C(0x568b, 0xef);
    Camara_EnviaBytesI2C(0x568c, 0xfe);
    Camara_EnviaBytesI2C(0x568d, 0xef);
    Camara_EnviaBytesI2C(0x568e, 0xaa);
    Camara_EnviaBytesI2C(0x568f, 0xaa);

}

void Camara_Set_SVGAprev_YUVmode(void) {
    
    Camara_Set_QVGAprev_YUVmode();
    
//3. SVGA preview
Camara_EnviaBytesI2C(0x3800 ,0x1 );
Camara_EnviaBytesI2C(0x3801 ,0x50);
Camara_EnviaBytesI2C(0x3802 ,0x0 );
Camara_EnviaBytesI2C(0x3803 ,0x8 );
Camara_EnviaBytesI2C(0x3804 ,0x5 );
Camara_EnviaBytesI2C(0x3805 ,0x0 );
Camara_EnviaBytesI2C(0x3806 ,0x3 );
Camara_EnviaBytesI2C(0x3807 ,0xc0);
Camara_EnviaBytesI2C(0x3808 ,0x3 );
Camara_EnviaBytesI2C(0x3809 ,0x20);
Camara_EnviaBytesI2C(0x380a ,0x2 );
Camara_EnviaBytesI2C(0x380b ,0x58);
Camara_EnviaBytesI2C(0x380c ,0xc );
Camara_EnviaBytesI2C(0x380d ,0x80);
Camara_EnviaBytesI2C(0x380e ,0x3 );
Camara_EnviaBytesI2C(0x380f ,0xe8);
Camara_EnviaBytesI2C(0x5001 ,0x7f);
Camara_EnviaBytesI2C(0x5680 ,0x0 );
Camara_EnviaBytesI2C(0x5681 ,0x0 );
Camara_EnviaBytesI2C(0x5682 ,0x5 );
Camara_EnviaBytesI2C(0x5683 ,0x0 );
Camara_EnviaBytesI2C(0x5684 ,0x0 );
Camara_EnviaBytesI2C(0x5685 ,0x0 );
Camara_EnviaBytesI2C(0x5686 ,0x3 );
Camara_EnviaBytesI2C(0x5687 ,0xc0);
Camara_EnviaBytesI2C(0x5687 ,0xc0);
Camara_EnviaBytesI2C(0x3815 ,0x02);    
    
}

void Camara_Set_CIFprev_YUVmode(void) {
  
    Camara_Set_QVGAprev_YUVmode();
    
//1. CIF preview
Camara_EnviaBytesI2C(0x3800 ,0x1 );

Camara_EnviaBytesI2C(0x3801 ,0x50);
Camara_EnviaBytesI2C(0x3802 ,0x0 );
Camara_EnviaBytesI2C(0x3803 ,0x8 );
Camara_EnviaBytesI2C(0x3804 ,0x4 );
Camara_EnviaBytesI2C(0x3805 ,0x96);
Camara_EnviaBytesI2C(0x3806 ,0x3 );
Camara_EnviaBytesI2C(0x3807 ,0xc0);
Camara_EnviaBytesI2C(0x3808 ,0x1 );
Camara_EnviaBytesI2C(0x3809 ,0x60);
Camara_EnviaBytesI2C(0x380a ,0x1 );
Camara_EnviaBytesI2C(0x380b ,0x20);
Camara_EnviaBytesI2C(0x380c ,0xc );
Camara_EnviaBytesI2C(0x380d ,0x80);
Camara_EnviaBytesI2C(0x380e ,0x3 );
Camara_EnviaBytesI2C(0x380f ,0xe8);
Camara_EnviaBytesI2C(0x5001 ,0x7f);
Camara_EnviaBytesI2C(0x5680 ,0x0 );
Camara_EnviaBytesI2C(0x5681 ,0x0 );
Camara_EnviaBytesI2C(0x5682 ,0x4 );
Camara_EnviaBytesI2C(0x5683 ,0x96);
Camara_EnviaBytesI2C(0x5684 ,0x0 );
Camara_EnviaBytesI2C(0x5685 ,0x0 );
Camara_EnviaBytesI2C(0x5686 ,0x3 );
Camara_EnviaBytesI2C(0x5687 ,0xc0);
    
}

void Camara_Set_QCIFprev_YUVmode(void) {
  
    Camara_Set_QVGAprev_YUVmode();
    
//2. QCIF preview
Camara_EnviaBytesI2C(0x3800 ,0x1 );
Camara_EnviaBytesI2C(0x3801 ,0x50);
Camara_EnviaBytesI2C(0x3802 ,0x0 );
Camara_EnviaBytesI2C(0x3803 ,0x8 );
Camara_EnviaBytesI2C(0x3804 ,0x4 );
Camara_EnviaBytesI2C(0x3805 ,0x96);
Camara_EnviaBytesI2C(0x3806 ,0x3 );
Camara_EnviaBytesI2C(0x3807 ,0xc0);
Camara_EnviaBytesI2C(0x3808 ,0x0 );
Camara_EnviaBytesI2C(0x3809 ,0xb0);
Camara_EnviaBytesI2C(0x380a ,0x0 );
Camara_EnviaBytesI2C(0x380b ,0x90);
Camara_EnviaBytesI2C(0x380c ,0xc );
Camara_EnviaBytesI2C(0x380d ,0x80);
Camara_EnviaBytesI2C(0x380e ,0x3 );
Camara_EnviaBytesI2C(0x380f ,0xe8);
Camara_EnviaBytesI2C(0x5001 ,0x7f);
Camara_EnviaBytesI2C(0x5680 ,0x0 );
Camara_EnviaBytesI2C(0x5681 ,0x0 );
Camara_EnviaBytesI2C(0x5682 ,0x4 );
Camara_EnviaBytesI2C(0x5683 ,0x96);
Camara_EnviaBytesI2C(0x5684 ,0x0 );
Camara_EnviaBytesI2C(0x5685 ,0x0 );

Camara_EnviaBytesI2C(0x5686 ,0x3 );
Camara_EnviaBytesI2C(0x5687 ,0xc0);
    
}

void Camara_Set_QSXGAcapt_JPEGmode(void) {

    //13.1.4 QSXGA Capture
    //; OV5642_ QSXGA _YUV7.5 fps
    //; 24 MHz input clock, 24Mhz pclk
    //; jpeg mode 7.5fps
    Camara_EnviaBytesI2C(0x3503, 0x07);
    Camara_EnviaBytesI2C(0x3000, 0x00);
    Camara_EnviaBytesI2C(0x3001, 0x00);
    Camara_EnviaBytesI2C(0x3002, 0x00);
    Camara_EnviaBytesI2C(0x3003, 0x00);
    Camara_EnviaBytesI2C(0x3005, 0xff);
    Camara_EnviaBytesI2C(0x3006, 0xff);
    Camara_EnviaBytesI2C(0x3007, 0x3f);
    Camara_EnviaBytesI2C(0x350c, 0x07);
    Camara_EnviaBytesI2C(0x350d, 0xd0);
    Camara_EnviaBytesI2C(0x3602, 0xe4);
    Camara_EnviaBytesI2C(0x3612, 0xac);
    Camara_EnviaBytesI2C(0x3613, 0x44);

    Camara_EnviaBytesI2C(0x3621, 0x27);
    Camara_EnviaBytesI2C(0x3622, 0x08);
    Camara_EnviaBytesI2C(0x3623, 0x22);
    Camara_EnviaBytesI2C(0x3604, 0x60);
    Camara_EnviaBytesI2C(0x3705, 0xda);
    Camara_EnviaBytesI2C(0x370a, 0x80);
    Camara_EnviaBytesI2C(0x3801, 0x8a);
    Camara_EnviaBytesI2C(0x3803, 0x0a);
    Camara_EnviaBytesI2C(0x3804, 0x0a);
    Camara_EnviaBytesI2C(0x3805, 0x20);
    Camara_EnviaBytesI2C(0x3806, 0x07);
    Camara_EnviaBytesI2C(0x3807, 0x98);
    Camara_EnviaBytesI2C(0x3808, 0x0a);
    Camara_EnviaBytesI2C(0x3809, 0x20);
    Camara_EnviaBytesI2C(0x380a, 0x07);
    Camara_EnviaBytesI2C(0x380b, 0x98);
    Camara_EnviaBytesI2C(0x380c, 0x0c);
    Camara_EnviaBytesI2C(0x380d, 0x80);
    Camara_EnviaBytesI2C(0x380e, 0x07);
    Camara_EnviaBytesI2C(0x380f, 0xd0);
    Camara_EnviaBytesI2C(0x3810, 0xc2);
    Camara_EnviaBytesI2C(0x3815, 0x44);
    Camara_EnviaBytesI2C(0x3818, 0xc8);
    Camara_EnviaBytesI2C(0x3824, 0x01);
    Camara_EnviaBytesI2C(0x3827, 0x0a);
    Camara_EnviaBytesI2C(0x3a00, 0x78);
    Camara_EnviaBytesI2C(0x3a0d, 0x10);
    Camara_EnviaBytesI2C(0x3a0e, 0x0d);
    Camara_EnviaBytesI2C(0x3a10, 0x32);
    Camara_EnviaBytesI2C(0x3a1b, 0x3c);
    Camara_EnviaBytesI2C(0x3a1e, 0x32);
    Camara_EnviaBytesI2C(0x3a11, 0x80);
    Camara_EnviaBytesI2C(0x3a1f, 0x20);
    Camara_EnviaBytesI2C(0x3a00, 0x78);
    Camara_EnviaBytesI2C(0x460b, 0x35);
    Camara_EnviaBytesI2C(0x471d, 0x00);
    Camara_EnviaBytesI2C(0x4713, 0x02);
    Camara_EnviaBytesI2C(0x471c, 0x50);
    Camara_EnviaBytesI2C(0x5682, 0x0a);
    Camara_EnviaBytesI2C(0x5683, 0x20);
    Camara_EnviaBytesI2C(0x5686, 0x07);
    Camara_EnviaBytesI2C(0x5687, 0x98);
    Camara_EnviaBytesI2C(0x5001, 0x4f);
    Camara_EnviaBytesI2C(0x589b, 0x00);
    Camara_EnviaBytesI2C(0x589a, 0xc0);
    
    //Low quality :
    //write_i2c(0x4407, 0x08);
    //Default quality:
    //write_i2c(0x4407 ,0x04);
    //High quality :
    //Camara_EnviaBytesI2C(0x4407, 0x02);
    
    //muy baja calidad
    Camara_EnviaBytesI2C(0x4407, 0x04);
    //
    
    Camara_EnviaBytesI2C(0x589b, 0x00);
    Camara_EnviaBytesI2C(0x589a, 0xc0);

    Camara_EnviaBytesI2C(0x3002, 0x0c);
    Camara_EnviaBytesI2C(0x3002, 0x00);
    Camara_EnviaBytesI2C(0x3503, 0x00);
    
}

void Camara_Set_QSXGAcaptVGA_JPEGmode(void) {
    
    Camara_Set_QSXGAcapt_JPEGmode();
    
//QSXGA to vga(640*480)
Camara_EnviaBytesI2C(0x3800 ,0x1 );
Camara_EnviaBytesI2C(0x3801 ,0x8A);
Camara_EnviaBytesI2C(0x3802 ,0x0 );
Camara_EnviaBytesI2C(0x3803 ,0xA );
Camara_EnviaBytesI2C(0x3804 ,0xA );
Camara_EnviaBytesI2C(0x3805 ,0x20);
Camara_EnviaBytesI2C(0x3806 ,0x7 );
Camara_EnviaBytesI2C(0x3807 ,0x98);
Camara_EnviaBytesI2C(0x3808 ,0x2 );
Camara_EnviaBytesI2C(0x3809 ,0x80);
Camara_EnviaBytesI2C(0x380a ,0x1 );
Camara_EnviaBytesI2C(0x380b ,0xe0);
Camara_EnviaBytesI2C(0x380c ,0xc );
Camara_EnviaBytesI2C(0x380d ,0x80);
Camara_EnviaBytesI2C(0x380e ,0x7 );
Camara_EnviaBytesI2C(0x380f ,0xd0);
Camara_EnviaBytesI2C(0x5001 ,0x7f);
Camara_EnviaBytesI2C(0x5680 ,0x0 );
Camara_EnviaBytesI2C(0x5681 ,0x0 );
Camara_EnviaBytesI2C(0x5682 ,0xA );
Camara_EnviaBytesI2C(0x5683 ,0x20);
Camara_EnviaBytesI2C(0x5684 ,0x0 );
Camara_EnviaBytesI2C(0x5685 ,0x0 );
Camara_EnviaBytesI2C(0x5686 ,0x7 );
Camara_EnviaBytesI2C(0x5687 ,0x98);    
        
}

void Camara_Set_QSXGAcaptSXGA_JPEGmode(void) {
    
    Camara_Set_QSXGAcapt_JPEGmode();
    
//QSXGA to sxga(1280*960)
Camara_EnviaBytesI2C(0x3800 ,0x1 );
Camara_EnviaBytesI2C(0x3801 ,0x8A);
Camara_EnviaBytesI2C(0x3802 ,0x0 );
Camara_EnviaBytesI2C(0x3803 ,0xA );
Camara_EnviaBytesI2C(0x3804 ,0xA );
Camara_EnviaBytesI2C(0x3805 ,0x20);
Camara_EnviaBytesI2C(0x3806 ,0x7 );
Camara_EnviaBytesI2C(0x3807 ,0x98);

Camara_EnviaBytesI2C(0x3808 ,0x5 );
Camara_EnviaBytesI2C(0x3809 ,0x0 );
Camara_EnviaBytesI2C(0x380a ,0x3 );
Camara_EnviaBytesI2C(0x380b ,0xc0);
Camara_EnviaBytesI2C(0x380c ,0xc );
Camara_EnviaBytesI2C(0x380d ,0x80);
Camara_EnviaBytesI2C(0x380e ,0x7 );
Camara_EnviaBytesI2C(0x380f ,0xd0);
Camara_EnviaBytesI2C(0x5001 ,0x7f);
Camara_EnviaBytesI2C(0x5680 ,0x0 );
Camara_EnviaBytesI2C(0x5681 ,0x0 );
Camara_EnviaBytesI2C(0x5682 ,0xA );
Camara_EnviaBytesI2C(0x5683 ,0x20);
Camara_EnviaBytesI2C(0x5684 ,0x0 );
Camara_EnviaBytesI2C(0x5685 ,0x0 );
Camara_EnviaBytesI2C(0x5686 ,0x7 );
Camara_EnviaBytesI2C(0x5687 ,0x98);
        
}

void Camara_Set_QSXGAcaptQVGA_JPEGmode(void) {
    
    Camara_Set_QSXGAcapt_JPEGmode();
    
//QSXGA to QVGA(320*240)
Camara_EnviaBytesI2C(0x3800 ,0x1 );
Camara_EnviaBytesI2C(0x3801 ,0x8A);
Camara_EnviaBytesI2C(0x3802 ,0x0 );
Camara_EnviaBytesI2C(0x3803 ,0xA );
Camara_EnviaBytesI2C(0x3804 ,0xA );
Camara_EnviaBytesI2C(0x3805 ,0x20);
Camara_EnviaBytesI2C(0x3806 ,0x7 );
Camara_EnviaBytesI2C(0x3807 ,0x98);
Camara_EnviaBytesI2C(0x3808 ,0x1 );
Camara_EnviaBytesI2C(0x3809 ,0x40);
Camara_EnviaBytesI2C(0x380a ,0x0 );
Camara_EnviaBytesI2C(0x380b ,0xf0);
Camara_EnviaBytesI2C(0x380c ,0xc );
Camara_EnviaBytesI2C(0x380d ,0x80);
Camara_EnviaBytesI2C(0x380e ,0x7 );
Camara_EnviaBytesI2C(0x380f ,0xd0);
Camara_EnviaBytesI2C(0x5001 ,0x7f);
Camara_EnviaBytesI2C(0x5680 ,0x0 );
Camara_EnviaBytesI2C(0x5681 ,0x0 );
Camara_EnviaBytesI2C(0x5682 ,0xA );
Camara_EnviaBytesI2C(0x5683 ,0x20);
Camara_EnviaBytesI2C(0x5684 ,0x0 );
Camara_EnviaBytesI2C(0x5685 ,0x0 );
Camara_EnviaBytesI2C(0x5686 ,0x7 );
Camara_EnviaBytesI2C(0x5687 ,0x98);    
    
}

void Camara_Set_QSXGAcaptQXGA_JPEGmode(void) {
    
    Camara_Set_QSXGAcapt_JPEGmode();
    
//QSXGA to qxga(2048*1536)
Camara_EnviaBytesI2C(0x3800 ,0x1 );
Camara_EnviaBytesI2C(0x3801 ,0x8A);
Camara_EnviaBytesI2C(0x3802 ,0x0 );
Camara_EnviaBytesI2C(0x3803 ,0xA );
Camara_EnviaBytesI2C(0x3804 ,0xA );
Camara_EnviaBytesI2C(0x3805 ,0x20);
Camara_EnviaBytesI2C(0x3806 ,0x7 );
Camara_EnviaBytesI2C(0x3807 ,0x98);
Camara_EnviaBytesI2C(0x3808 ,0x8 );
Camara_EnviaBytesI2C(0x3809 ,0x0 );
Camara_EnviaBytesI2C(0x380a ,0x6 );
Camara_EnviaBytesI2C(0x380b ,0x0 );
Camara_EnviaBytesI2C(0x380c ,0xc );
Camara_EnviaBytesI2C(0x380d ,0x80);
Camara_EnviaBytesI2C(0x380e ,0x7 );
Camara_EnviaBytesI2C(0x380f ,0xd0);
Camara_EnviaBytesI2C(0x5001 ,0x7f);
Camara_EnviaBytesI2C(0x5680 ,0x0 );
Camara_EnviaBytesI2C(0x5681 ,0x0 );
Camara_EnviaBytesI2C(0x5682 ,0xA );
Camara_EnviaBytesI2C(0x5683 ,0x20);
Camara_EnviaBytesI2C(0x5684 ,0x0 );
Camara_EnviaBytesI2C(0x5685 ,0x0 );
Camara_EnviaBytesI2C(0x5686 ,0x7 );
Camara_EnviaBytesI2C(0x5687 ,0x98);    
    
}

void Camara_Set_QSXGAcaptUXGA_JPEGmode(void) {
    
    Camara_Set_QSXGAcapt_JPEGmode();
    
//QSXGA to uxga(1600*1200)
Camara_EnviaBytesI2C(0x3800 ,0x1 );
Camara_EnviaBytesI2C(0x3801 ,0x8A);
Camara_EnviaBytesI2C(0x3802 ,0x0 );
Camara_EnviaBytesI2C(0x3803 ,0xA );
Camara_EnviaBytesI2C(0x3804 ,0xA );
Camara_EnviaBytesI2C(0x3805 ,0x20);
Camara_EnviaBytesI2C(0x3806 ,0x7 );
Camara_EnviaBytesI2C(0x3807 ,0x98);
Camara_EnviaBytesI2C(0x3808 ,0x6 );
Camara_EnviaBytesI2C(0x3809 ,0x40);
Camara_EnviaBytesI2C(0x380a ,0x4 );
Camara_EnviaBytesI2C(0x380b ,0xb0);
Camara_EnviaBytesI2C(0x380c ,0xc );
Camara_EnviaBytesI2C(0x380d ,0x80);
Camara_EnviaBytesI2C(0x380e ,0x7 );    

Camara_EnviaBytesI2C(0x380f ,0xd0);
Camara_EnviaBytesI2C(0x5001 ,0x7f);
Camara_EnviaBytesI2C(0x5680 ,0x0 );
Camara_EnviaBytesI2C(0x5681 ,0x0 );
Camara_EnviaBytesI2C(0x5682 ,0xA );
Camara_EnviaBytesI2C(0x5683 ,0x20);
Camara_EnviaBytesI2C(0x5684 ,0x0 );
Camara_EnviaBytesI2C(0x5685 ,0x0 );
Camara_EnviaBytesI2C(0x5686 ,0x7 );
Camara_EnviaBytesI2C(0x5687 ,0x98);        
    
}

void Camara_Set_QSXGAcaptXGA_JPEGmode(void) {
    
    Camara_Set_QSXGAcapt_JPEGmode();
    
//QSXGA to xga(1024*768)
Camara_EnviaBytesI2C(0x3800 ,0x1 );
Camara_EnviaBytesI2C(0x3801 ,0x8A);
Camara_EnviaBytesI2C(0x3802 ,0x0 );
Camara_EnviaBytesI2C(0x3803 ,0xA );
Camara_EnviaBytesI2C(0x3804 ,0xA );
Camara_EnviaBytesI2C(0x3805 ,0x20);
Camara_EnviaBytesI2C(0x3806 ,0x7 );
Camara_EnviaBytesI2C(0x3807 ,0x98);
Camara_EnviaBytesI2C(0x3808 ,0x4 );
Camara_EnviaBytesI2C(0x3809 ,0x0 );
Camara_EnviaBytesI2C(0x380a ,0x3 );
Camara_EnviaBytesI2C(0x380b ,0x0 );
Camara_EnviaBytesI2C(0x380c ,0xc );
Camara_EnviaBytesI2C(0x380d ,0x80);
Camara_EnviaBytesI2C(0x380e ,0x7 );
Camara_EnviaBytesI2C(0x380f ,0xd0);
Camara_EnviaBytesI2C(0x5001 ,0x7f);
Camara_EnviaBytesI2C(0x5680 ,0x0 );
Camara_EnviaBytesI2C(0x5681 ,0x0 );
Camara_EnviaBytesI2C(0x5682 ,0xA );
Camara_EnviaBytesI2C(0x5683 ,0x20);
Camara_EnviaBytesI2C(0x5684 ,0x0 );
Camara_EnviaBytesI2C(0x5685 ,0x0 );
Camara_EnviaBytesI2C(0x5686 ,0x7 );
Camara_EnviaBytesI2C(0x5687 ,0x98);    
    
}

void CamaraSet_YUV_to_JPEG(void) {

//13.1.7.1 YUV to JPEG setting
Camara_EnviaBytesI2C(0x460c ,0x02);
Camara_EnviaBytesI2C(0x460b ,0x35);
Camara_EnviaBytesI2C(0x471d ,0x00);
Camara_EnviaBytesI2C(0x3002 ,0x0c);
Camara_EnviaBytesI2C(0x3002 ,0x00);
Camara_EnviaBytesI2C(0x4713 ,0x03);//Compression mode select: mode 3
Camara_EnviaBytesI2C(0x471c ,0x50);
Camara_EnviaBytesI2C(0x3815 ,0x44);
Camara_EnviaBytesI2C(0x3818 ,0x08);//Timing control 18: thumbnail disable, compression enabled
Camara_EnviaBytesI2C(0x3006 ,0xff);
Camara_EnviaBytesI2C(0x3007 ,0x3f);    
    
}

void CamaraSet_JPEG_to_YUV(void) {
    
//13.1.7.2 JPEG to YUV setting
Camara_EnviaBytesI2C(0x460c ,0x00);
Camara_EnviaBytesI2C(0x460b ,0x37);
Camara_EnviaBytesI2C(0x471c ,0xd0);
Camara_EnviaBytesI2C(0x471d ,0x05);
Camara_EnviaBytesI2C(0x3815 ,0x01);
Camara_EnviaBytesI2C(0x3818 ,0x00);
Camara_EnviaBytesI2C(0x501f ,0x00);
Camara_EnviaBytesI2C(0x4300 ,0x30);    

Camara_EnviaBytesI2C(0x3002 ,0x1c);
        
}

void Camara_Set_HighResVideo(void) {
//13. 4 High Resolution Video
//13.4.1 1080 P
Camara_EnviaBytesI2C(0x3103 ,0x93);
Camara_EnviaBytesI2C(0x3008 ,0x82);
Camara_EnviaBytesI2C(0x3017 ,0x7f);
Camara_EnviaBytesI2C(0x3018 ,0xfc);
Camara_EnviaBytesI2C(0x3810 ,0xc2);
Camara_EnviaBytesI2C(0x3615 ,0xf0);
Camara_EnviaBytesI2C(0x3000 ,0x00);
Camara_EnviaBytesI2C(0x3001 ,0x00);
Camara_EnviaBytesI2C(0x3002 ,0x00);
Camara_EnviaBytesI2C(0x3003 ,0x00);
Camara_EnviaBytesI2C(0x3004 ,0xff);
Camara_EnviaBytesI2C(0x3030 ,0x2b);
Camara_EnviaBytesI2C(0x3011 ,0x08);
Camara_EnviaBytesI2C(0x3010 ,0x10);
Camara_EnviaBytesI2C(0x3604 ,0x60);
Camara_EnviaBytesI2C(0x3622 ,0x60);
Camara_EnviaBytesI2C(0x3621 ,0x09);
Camara_EnviaBytesI2C(0x3709 ,0x00);
Camara_EnviaBytesI2C(0x4000 ,0x21);
Camara_EnviaBytesI2C(0x401d ,0x22);
Camara_EnviaBytesI2C(0x3600 ,0x54);
Camara_EnviaBytesI2C(0x3605 ,0x04);
Camara_EnviaBytesI2C(0x3606 ,0x3f);
Camara_EnviaBytesI2C(0x3c01 ,0x80);
Camara_EnviaBytesI2C(0x300d ,0x22);
Camara_EnviaBytesI2C(0x3623 ,0x22);
Camara_EnviaBytesI2C(0x5000 ,0x4f);
Camara_EnviaBytesI2C(0x5020 ,0x04);
Camara_EnviaBytesI2C(0x5181 ,0x79);
Camara_EnviaBytesI2C(0x5182 ,0x00);
Camara_EnviaBytesI2C(0x5185 ,0x22);
Camara_EnviaBytesI2C(0x5197 ,0x01);
Camara_EnviaBytesI2C(0x5500 ,0x0a);
Camara_EnviaBytesI2C(0x5504 ,0x00);
Camara_EnviaBytesI2C(0x5505 ,0x7f);
Camara_EnviaBytesI2C(0x5080 ,0x08);
Camara_EnviaBytesI2C(0x300e ,0x18);
Camara_EnviaBytesI2C(0x4610 ,0x00);
Camara_EnviaBytesI2C(0x471d ,0x05);
Camara_EnviaBytesI2C(0x4708 ,0x06);
Camara_EnviaBytesI2C(0x370c ,0xa0);
//108 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x3808 ,0x0a);
Camara_EnviaBytesI2C(0x3809 ,0x20);
Camara_EnviaBytesI2C(0x380a ,0x07);
Camara_EnviaBytesI2C(0x380b ,0x98);
Camara_EnviaBytesI2C(0x380c ,0x0c);
Camara_EnviaBytesI2C(0x380d ,0x80);
Camara_EnviaBytesI2C(0x380e ,0x07);
Camara_EnviaBytesI2C(0x380f ,0xd0);
Camara_EnviaBytesI2C(0x5687 ,0x94);
Camara_EnviaBytesI2C(0x501f ,0x00);
Camara_EnviaBytesI2C(0x5000 ,0x4f);
Camara_EnviaBytesI2C(0x5001 ,0xcf);
Camara_EnviaBytesI2C(0x4300 ,0x30);
Camara_EnviaBytesI2C(0x4300 ,0x30);
Camara_EnviaBytesI2C(0x460b ,0x35);
Camara_EnviaBytesI2C(0x471d ,0x00);
Camara_EnviaBytesI2C(0x3002 ,0x0c);
Camara_EnviaBytesI2C(0x3002 ,0x00);
Camara_EnviaBytesI2C(0x4713 ,0x03);
Camara_EnviaBytesI2C(0x471c ,0x50);
Camara_EnviaBytesI2C(0x4721 ,0x02);
Camara_EnviaBytesI2C(0x4402 ,0x90);
Camara_EnviaBytesI2C(0x460c ,0x22);
Camara_EnviaBytesI2C(0x3815 ,0x44);
Camara_EnviaBytesI2C(0x3503 ,0x07);
Camara_EnviaBytesI2C(0x3501 ,0x73);
Camara_EnviaBytesI2C(0x3502 ,0x80);
Camara_EnviaBytesI2C(0x350b ,0x00);
Camara_EnviaBytesI2C(0x3818 ,0xc8);
Camara_EnviaBytesI2C(0x3801 ,0x88);
Camara_EnviaBytesI2C(0x3824 ,0x11);
Camara_EnviaBytesI2C(0x3a00 ,0x78);
Camara_EnviaBytesI2C(0x3a1a ,0x04);
Camara_EnviaBytesI2C(0x3a13 ,0x30);
Camara_EnviaBytesI2C(0x3a18 ,0x00);
Camara_EnviaBytesI2C(0x3a19 ,0x7c);
Camara_EnviaBytesI2C(0x3a08 ,0x12);
Camara_EnviaBytesI2C(0x3a09 ,0xc0);
Camara_EnviaBytesI2C(0x3a0a ,0x0f);
Camara_EnviaBytesI2C(0x3a0b ,0xa0);
Camara_EnviaBytesI2C(0x350c ,0x07);
Camara_EnviaBytesI2C(0x350d ,0xd0);
Camara_EnviaBytesI2C(0x3a0d ,0x08);
Camara_EnviaBytesI2C(0x3a0e ,0x06);
Camara_EnviaBytesI2C(0x3500 ,0x00);
Camara_EnviaBytesI2C(0x3501 ,0x00);
Camara_EnviaBytesI2C(0x3502 ,0x00);
Camara_EnviaBytesI2C(0x350a ,0x00);
//109 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x350b ,0x00);
Camara_EnviaBytesI2C(0x3503 ,0x00);
Camara_EnviaBytesI2C(0x3030 ,0x2b);
Camara_EnviaBytesI2C(0x3a02 ,0x00);
Camara_EnviaBytesI2C(0x3a03 ,0x7d);
Camara_EnviaBytesI2C(0x3a04 ,0x00);
Camara_EnviaBytesI2C(0x3a14 ,0x00);
Camara_EnviaBytesI2C(0x3a15 ,0x7d);
Camara_EnviaBytesI2C(0x3a16 ,0x00);
Camara_EnviaBytesI2C(0x3a00 ,0x78);
Camara_EnviaBytesI2C(0x3a08 ,0x09);
Camara_EnviaBytesI2C(0x3a09 ,0x60);
Camara_EnviaBytesI2C(0x3a0a ,0x07);
Camara_EnviaBytesI2C(0x3a0b ,0xd0);
Camara_EnviaBytesI2C(0x3a0d ,0x10);
Camara_EnviaBytesI2C(0x3a0e ,0x0d);
//******************************************
//write_i2c(0x4407 ,0x04);
//Low quality :
//write_i2c(0x4407, 0x08);
//Default quality:
//write_i2c(0x4407 ,0x04);
//High quality :
Camara_EnviaBytesI2C(0x4407, 0x02);
//******************************************
Camara_EnviaBytesI2C(0x5193 ,0x70);
Camara_EnviaBytesI2C(0x589b ,0x00);
Camara_EnviaBytesI2C(0x589a ,0xc0);
Camara_EnviaBytesI2C(0x401e ,0x20);
Camara_EnviaBytesI2C(0x4001 ,0x42);
Camara_EnviaBytesI2C(0x401c ,0x06);
Camara_EnviaBytesI2C(0x3825 ,0xac);
Camara_EnviaBytesI2C(0x3827 ,0x0c);
Camara_EnviaBytesI2C(0x5402 ,0x3f);
Camara_EnviaBytesI2C(0x5403 ,0x00);
Camara_EnviaBytesI2C(0x3406 ,0x00);
Camara_EnviaBytesI2C(0x5180 ,0xff);
Camara_EnviaBytesI2C(0x5181 ,0x52);
Camara_EnviaBytesI2C(0x5182 ,0x11);
Camara_EnviaBytesI2C(0x5183 ,0x14);
Camara_EnviaBytesI2C(0x5184 ,0x25);
Camara_EnviaBytesI2C(0x5185 ,0x24);
Camara_EnviaBytesI2C(0x5186 ,0x06);
Camara_EnviaBytesI2C(0x5187 ,0x08);
Camara_EnviaBytesI2C(0x5188 ,0x08);
Camara_EnviaBytesI2C(0x5189 ,0x7c);
Camara_EnviaBytesI2C(0x518a ,0x60);
Camara_EnviaBytesI2C(0x518b ,0xb2);
Camara_EnviaBytesI2C(0x518c ,0xb2);
Camara_EnviaBytesI2C(0x518d ,0x44);
Camara_EnviaBytesI2C(0x518e ,0x3d);
Camara_EnviaBytesI2C(0x518f ,0x58);
Camara_EnviaBytesI2C(0x5190 ,0x46);
Camara_EnviaBytesI2C(0x5191 ,0xf8);
Camara_EnviaBytesI2C(0x5192 ,0x04);
Camara_EnviaBytesI2C(0x5193 ,0x70);
//110 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x5194 ,0xf0);
Camara_EnviaBytesI2C(0x5195 ,0xf0);
Camara_EnviaBytesI2C(0x5196 ,0x03);
Camara_EnviaBytesI2C(0x5197 ,0x01);
Camara_EnviaBytesI2C(0x5198 ,0x04);
Camara_EnviaBytesI2C(0x5199 ,0x12);
Camara_EnviaBytesI2C(0x519a ,0x04);
Camara_EnviaBytesI2C(0x519b ,0x00);
Camara_EnviaBytesI2C(0x519c ,0x06);
Camara_EnviaBytesI2C(0x519d ,0x82);
Camara_EnviaBytesI2C(0x519e ,0x00);
Camara_EnviaBytesI2C(0x5025 ,0x80);
Camara_EnviaBytesI2C(0x5583 ,0x40);
Camara_EnviaBytesI2C(0x5584 ,0x40);
Camara_EnviaBytesI2C(0x5580 ,0x02);
Camara_EnviaBytesI2C(0x5000 ,0xcf);
Camara_EnviaBytesI2C(0x3710 ,0x10);
Camara_EnviaBytesI2C(0x3632 ,0x51);
Camara_EnviaBytesI2C(0x3702 ,0x10);
Camara_EnviaBytesI2C(0x3703 ,0xb2);
Camara_EnviaBytesI2C(0x3704 ,0x18);
Camara_EnviaBytesI2C(0x370b ,0x40);
Camara_EnviaBytesI2C(0x370d ,0x03);
Camara_EnviaBytesI2C(0x3631 ,0x01);
Camara_EnviaBytesI2C(0x3632 ,0x52);
Camara_EnviaBytesI2C(0x3606 ,0x24);
Camara_EnviaBytesI2C(0x3620 ,0x96);
Camara_EnviaBytesI2C(0x5785 ,0x07);
Camara_EnviaBytesI2C(0x3a13 ,0x30);
Camara_EnviaBytesI2C(0x3600 ,0x52);
Camara_EnviaBytesI2C(0x3604 ,0x48);
Camara_EnviaBytesI2C(0x3606 ,0x1b);
Camara_EnviaBytesI2C(0x370d ,0x0b);
Camara_EnviaBytesI2C(0x370f ,0xc0);
Camara_EnviaBytesI2C(0x3709 ,0x01);
Camara_EnviaBytesI2C(0x3823 ,0x00);
Camara_EnviaBytesI2C(0x5007 ,0x00);
Camara_EnviaBytesI2C(0x5009 ,0x00);
Camara_EnviaBytesI2C(0x5011 ,0x00);
Camara_EnviaBytesI2C(0x5013 ,0x00);
Camara_EnviaBytesI2C(0x519e ,0x00);
Camara_EnviaBytesI2C(0x5086 ,0x00);
Camara_EnviaBytesI2C(0x5087 ,0x00);
Camara_EnviaBytesI2C(0x5088 ,0x00);
Camara_EnviaBytesI2C(0x5089 ,0x00);
Camara_EnviaBytesI2C(0x302b ,0x00);
Camara_EnviaBytesI2C(0x3503 ,0x07);
Camara_EnviaBytesI2C(0x3011 ,0x07);
//111 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x350c ,0x04);
Camara_EnviaBytesI2C(0x350d ,0x58);
Camara_EnviaBytesI2C(0x3801 ,0x8a);
Camara_EnviaBytesI2C(0x3803 ,0x0a);
Camara_EnviaBytesI2C(0x3804 ,0x07);
Camara_EnviaBytesI2C(0x3805 ,0x80);
Camara_EnviaBytesI2C(0x3806 ,0x04);
Camara_EnviaBytesI2C(0x3807 ,0x38);
Camara_EnviaBytesI2C(0x3808 ,0x07);
Camara_EnviaBytesI2C(0x3809 ,0x80);
Camara_EnviaBytesI2C(0x380a ,0x04);
Camara_EnviaBytesI2C(0x380b ,0x38);
Camara_EnviaBytesI2C(0x380c ,0x09);
Camara_EnviaBytesI2C(0x380d ,0xd6);
Camara_EnviaBytesI2C(0x380e ,0x04);
Camara_EnviaBytesI2C(0x380f ,0x58);
Camara_EnviaBytesI2C(0x381c ,0x11);
Camara_EnviaBytesI2C(0x381d ,0xba);
Camara_EnviaBytesI2C(0x381e ,0x04);
Camara_EnviaBytesI2C(0x381f ,0x48);
Camara_EnviaBytesI2C(0x3820 ,0x04);
Camara_EnviaBytesI2C(0x3821 ,0x18);
Camara_EnviaBytesI2C(0x3a08 ,0x14);
Camara_EnviaBytesI2C(0x3a09 ,0xe0);
Camara_EnviaBytesI2C(0x3a0a ,0x11);
Camara_EnviaBytesI2C(0x3a0b ,0x60);
Camara_EnviaBytesI2C(0x3a0d ,0x04);
Camara_EnviaBytesI2C(0x3a0e ,0x03);
Camara_EnviaBytesI2C(0x5682 ,0x07);
Camara_EnviaBytesI2C(0x5683 ,0x60);
Camara_EnviaBytesI2C(0x5686 ,0x04);
Camara_EnviaBytesI2C(0x5687 ,0x1c);
Camara_EnviaBytesI2C(0x5001 ,0x7f);
Camara_EnviaBytesI2C(0x3503 ,0x00);
Camara_EnviaBytesI2C(0x3010 ,0x10);
Camara_EnviaBytesI2C(0x5001 ,0xFF);
Camara_EnviaBytesI2C(0x5583 ,0x50);
Camara_EnviaBytesI2C(0x5584 ,0x50);
Camara_EnviaBytesI2C(0x5580 ,0x02);
Camara_EnviaBytesI2C(0x3c01 ,0x80);
Camara_EnviaBytesI2C(0x3c00 ,0x04);
//;LENS
Camara_EnviaBytesI2C(0x5800 ,0x48);
Camara_EnviaBytesI2C(0x5801 ,0x31);
Camara_EnviaBytesI2C(0x5802 ,0x21);
//112 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x5803 ,0x1b);
Camara_EnviaBytesI2C(0x5804 ,0x1a);
Camara_EnviaBytesI2C(0x5805 ,0x1e);
Camara_EnviaBytesI2C(0x5806 ,0x29);
Camara_EnviaBytesI2C(0x5807 ,0x38);
Camara_EnviaBytesI2C(0x5808 ,0x26);
Camara_EnviaBytesI2C(0x5809 ,0x17);
Camara_EnviaBytesI2C(0x580a ,0x11);
Camara_EnviaBytesI2C(0x580b ,0xe );
Camara_EnviaBytesI2C(0x580c ,0xd );
Camara_EnviaBytesI2C(0x580d ,0xe );
Camara_EnviaBytesI2C(0x580e ,0x13);
Camara_EnviaBytesI2C(0x580f ,0x1a);
Camara_EnviaBytesI2C(0x5810 ,0x15);
Camara_EnviaBytesI2C(0x5811 ,0xd );
Camara_EnviaBytesI2C(0x5812 ,0x8 );
Camara_EnviaBytesI2C(0x5813 ,0x5 );
Camara_EnviaBytesI2C(0x5814 ,0x4 );
Camara_EnviaBytesI2C(0x5815 ,0x5 );
Camara_EnviaBytesI2C(0x5816 ,0x9 );
Camara_EnviaBytesI2C(0x5817 ,0xd );
Camara_EnviaBytesI2C(0x5818 ,0x11);
Camara_EnviaBytesI2C(0x5819 ,0xa );
Camara_EnviaBytesI2C(0x581a ,0x4 );
Camara_EnviaBytesI2C(0x581b ,0x0 );
Camara_EnviaBytesI2C(0x581c ,0x0 );
Camara_EnviaBytesI2C(0x581d ,0x1 );
Camara_EnviaBytesI2C(0x581e ,0x6 );
Camara_EnviaBytesI2C(0x581f ,0x9 );
Camara_EnviaBytesI2C(0x5820 ,0x12);
Camara_EnviaBytesI2C(0x5821 ,0xb );
Camara_EnviaBytesI2C(0x5822 ,0x4 );
Camara_EnviaBytesI2C(0x5823 ,0x0 );
Camara_EnviaBytesI2C(0x5824 ,0x0 );
Camara_EnviaBytesI2C(0x5825 ,0x1 );
Camara_EnviaBytesI2C(0x5826 ,0x6 );
Camara_EnviaBytesI2C(0x5827 ,0xa );
Camara_EnviaBytesI2C(0x5828 ,0x17);
Camara_EnviaBytesI2C(0x5829 ,0xf );
Camara_EnviaBytesI2C(0x582a ,0x9 );
Camara_EnviaBytesI2C(0x582b ,0x6 );
Camara_EnviaBytesI2C(0x582c ,0x5 );
Camara_EnviaBytesI2C(0x582d ,0x6 );
Camara_EnviaBytesI2C(0x582e ,0xa );
Camara_EnviaBytesI2C(0x582f ,0xe );
Camara_EnviaBytesI2C(0x5830 ,0x28);
Camara_EnviaBytesI2C(0x5831 ,0x1a);
Camara_EnviaBytesI2C(0x5832 ,0x11);
//113 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x5833 ,0xe );
Camara_EnviaBytesI2C(0x5834 ,0xe );
Camara_EnviaBytesI2C(0x5835 ,0xf );
Camara_EnviaBytesI2C(0x5836 ,0x15);
Camara_EnviaBytesI2C(0x5837 ,0x1d);
Camara_EnviaBytesI2C(0x5838 ,0x6e);
Camara_EnviaBytesI2C(0x5839 ,0x39);
Camara_EnviaBytesI2C(0x583a ,0x27);
Camara_EnviaBytesI2C(0x583b ,0x1f);
Camara_EnviaBytesI2C(0x583c ,0x1e);
Camara_EnviaBytesI2C(0x583d ,0x23);
Camara_EnviaBytesI2C(0x583e ,0x2f);
Camara_EnviaBytesI2C(0x583f ,0x41);
Camara_EnviaBytesI2C(0x5840 ,0xe );
Camara_EnviaBytesI2C(0x5841 ,0xc );
Camara_EnviaBytesI2C(0x5842 ,0xd );
Camara_EnviaBytesI2C(0x5843 ,0xc );
Camara_EnviaBytesI2C(0x5844 ,0xc );
Camara_EnviaBytesI2C(0x5845 ,0xc );
Camara_EnviaBytesI2C(0x5846 ,0xc );
Camara_EnviaBytesI2C(0x5847 ,0xc );
Camara_EnviaBytesI2C(0x5848 ,0xd );
Camara_EnviaBytesI2C(0x5849 ,0xe );
Camara_EnviaBytesI2C(0x584a ,0xe );
Camara_EnviaBytesI2C(0x584b ,0xa );
Camara_EnviaBytesI2C(0x584c ,0xe );
Camara_EnviaBytesI2C(0x584d ,0xe );
Camara_EnviaBytesI2C(0x584e ,0x10);
Camara_EnviaBytesI2C(0x584f ,0x10);
Camara_EnviaBytesI2C(0x5850 ,0x11);
Camara_EnviaBytesI2C(0x5851 ,0xa );
Camara_EnviaBytesI2C(0x5852 ,0xf );
Camara_EnviaBytesI2C(0x5853 ,0xe );
Camara_EnviaBytesI2C(0x5854 ,0x10);
Camara_EnviaBytesI2C(0x5855 ,0x10);
Camara_EnviaBytesI2C(0x5856 ,0x10);
Camara_EnviaBytesI2C(0x5857 ,0xa );
Camara_EnviaBytesI2C(0x5858 ,0xe );
Camara_EnviaBytesI2C(0x5859 ,0xe );
Camara_EnviaBytesI2C(0x585a ,0xf );
Camara_EnviaBytesI2C(0x585b ,0xf );
Camara_EnviaBytesI2C(0x585c ,0xf );
Camara_EnviaBytesI2C(0x585d ,0xa );
Camara_EnviaBytesI2C(0x585e ,0x9 );
Camara_EnviaBytesI2C(0x585f ,0xd );
Camara_EnviaBytesI2C(0x5860 ,0xc );
Camara_EnviaBytesI2C(0x5861 ,0xb );
Camara_EnviaBytesI2C(0x5862 ,0xd );
//114 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x5863 ,0x7 );
Camara_EnviaBytesI2C(0x5864 ,0x17);
Camara_EnviaBytesI2C(0x5865 ,0x14);
Camara_EnviaBytesI2C(0x5866 ,0x18);
Camara_EnviaBytesI2C(0x5867 ,0x18);
Camara_EnviaBytesI2C(0x5868 ,0x16);
Camara_EnviaBytesI2C(0x5869 ,0x12);
Camara_EnviaBytesI2C(0x586a ,0x1b);
Camara_EnviaBytesI2C(0x586b ,0x1a);
Camara_EnviaBytesI2C(0x586c ,0x16);
Camara_EnviaBytesI2C(0x586d ,0x16);
Camara_EnviaBytesI2C(0x586e ,0x18);
Camara_EnviaBytesI2C(0x586f ,0x1f);
Camara_EnviaBytesI2C(0x5870 ,0x1c);
Camara_EnviaBytesI2C(0x5871 ,0x16);
Camara_EnviaBytesI2C(0x5872 ,0x10);
Camara_EnviaBytesI2C(0x5873 ,0xf );
Camara_EnviaBytesI2C(0x5874 ,0x13);
Camara_EnviaBytesI2C(0x5875 ,0x1c);
Camara_EnviaBytesI2C(0x5876 ,0x1e);
Camara_EnviaBytesI2C(0x5877 ,0x17);
Camara_EnviaBytesI2C(0x5878 ,0x11);
Camara_EnviaBytesI2C(0x5879 ,0x11);
Camara_EnviaBytesI2C(0x587a ,0x14);
Camara_EnviaBytesI2C(0x587b ,0x1e);
Camara_EnviaBytesI2C(0x587c ,0x1c);
Camara_EnviaBytesI2C(0x587d ,0x1c);
Camara_EnviaBytesI2C(0x587e ,0x1a);
Camara_EnviaBytesI2C(0x587f ,0x1a);
Camara_EnviaBytesI2C(0x5880 ,0x1b);
Camara_EnviaBytesI2C(0x5881 ,0x1f);
Camara_EnviaBytesI2C(0x5882 ,0x14);
Camara_EnviaBytesI2C(0x5883 ,0x1a);
Camara_EnviaBytesI2C(0x5884 ,0x1d);
Camara_EnviaBytesI2C(0x5885 ,0x1e);
Camara_EnviaBytesI2C(0x5886 ,0x1a);
Camara_EnviaBytesI2C(0x5887 ,0x1a);
//;
//;AWB
Camara_EnviaBytesI2C(0x5180 ,0xff);
Camara_EnviaBytesI2C(0x5181 ,0x52);
Camara_EnviaBytesI2C(0x5182 ,0x11);
Camara_EnviaBytesI2C(0x5183 ,0x14);
Camara_EnviaBytesI2C(0x5184 ,0x25);
Camara_EnviaBytesI2C(0x5185 ,0x24);
Camara_EnviaBytesI2C(0x5186 ,0x14);
Camara_EnviaBytesI2C(0x5187 ,0x14);
Camara_EnviaBytesI2C(0x5188 ,0x14);
//115 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x5189 ,0x69);
Camara_EnviaBytesI2C(0x518a ,0x60);
Camara_EnviaBytesI2C(0x518b ,0xa2);
Camara_EnviaBytesI2C(0x518c ,0x9c);
Camara_EnviaBytesI2C(0x518d ,0x36);
Camara_EnviaBytesI2C(0x518e ,0x34);
Camara_EnviaBytesI2C(0x518f ,0x54);
Camara_EnviaBytesI2C(0x5190 ,0x4c);
Camara_EnviaBytesI2C(0x5191 ,0xf8);
Camara_EnviaBytesI2C(0x5192 ,0x04);
Camara_EnviaBytesI2C(0x5193 ,0x70);
Camara_EnviaBytesI2C(0x5194 ,0xf0);
Camara_EnviaBytesI2C(0x5195 ,0xf0);
Camara_EnviaBytesI2C(0x5196 ,0x03);
Camara_EnviaBytesI2C(0x5197 ,0x01);
Camara_EnviaBytesI2C(0x5198 ,0x05);
Camara_EnviaBytesI2C(0x5199 ,0x2f);
Camara_EnviaBytesI2C(0x519a ,0x04);
Camara_EnviaBytesI2C(0x519b ,0x00);
Camara_EnviaBytesI2C(0x519c ,0x06);
Camara_EnviaBytesI2C(0x519d ,0xa0);
Camara_EnviaBytesI2C(0x519e ,0xa0);
//;
//;D/S
Camara_EnviaBytesI2C(0x528a ,0x00);
Camara_EnviaBytesI2C(0x528b ,0x01);
Camara_EnviaBytesI2C(0x528c ,0x04);
Camara_EnviaBytesI2C(0x528d ,0x08);
Camara_EnviaBytesI2C(0x528e ,0x10);
Camara_EnviaBytesI2C(0x528f ,0x20);
Camara_EnviaBytesI2C(0x5290 ,0x30);
Camara_EnviaBytesI2C(0x5292 ,0x00);
Camara_EnviaBytesI2C(0x5293 ,0x00);
Camara_EnviaBytesI2C(0x5294 ,0x00);
Camara_EnviaBytesI2C(0x5295 ,0x01);
Camara_EnviaBytesI2C(0x5296 ,0x00);
Camara_EnviaBytesI2C(0x5297 ,0x04);
Camara_EnviaBytesI2C(0x5298 ,0x00);
Camara_EnviaBytesI2C(0x5299 ,0x08);
Camara_EnviaBytesI2C(0x529a ,0x00);
Camara_EnviaBytesI2C(0x529b ,0x10);
Camara_EnviaBytesI2C(0x529c ,0x00);
Camara_EnviaBytesI2C(0x529d ,0x20);
Camara_EnviaBytesI2C(0x529e ,0x00);
Camara_EnviaBytesI2C(0x529f ,0x30);
Camara_EnviaBytesI2C(0x5282 ,0x00);
Camara_EnviaBytesI2C(0x5300 ,0x00);
Camara_EnviaBytesI2C(0x5301 ,0x20);
//116 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x5302 ,0x00);
Camara_EnviaBytesI2C(0x5303 ,0x7c);
Camara_EnviaBytesI2C(0x530c ,0x00);
Camara_EnviaBytesI2C(0x530d ,0x10);
Camara_EnviaBytesI2C(0x530e ,0x20);
Camara_EnviaBytesI2C(0x530f ,0x80);
Camara_EnviaBytesI2C(0x5310 ,0x20);
Camara_EnviaBytesI2C(0x5311 ,0x80);
Camara_EnviaBytesI2C(0x5308 ,0x20);
Camara_EnviaBytesI2C(0x5309 ,0x40);
Camara_EnviaBytesI2C(0x5304 ,0x00);
Camara_EnviaBytesI2C(0x5305 ,0x30);
Camara_EnviaBytesI2C(0x5306 ,0x00);
Camara_EnviaBytesI2C(0x5307 ,0x80);
Camara_EnviaBytesI2C(0x5314 ,0x08);
Camara_EnviaBytesI2C(0x5315 ,0x20);
Camara_EnviaBytesI2C(0x5319 ,0x30);
Camara_EnviaBytesI2C(0x5316 ,0x10);
Camara_EnviaBytesI2C(0x5317 ,0x00);
Camara_EnviaBytesI2C(0x5318 ,0x02);
//;
//;CMX
Camara_EnviaBytesI2C(0x5380 ,0x01);
Camara_EnviaBytesI2C(0x5381 ,0x00);
Camara_EnviaBytesI2C(0x5382 ,0x00);
Camara_EnviaBytesI2C(0x5383 ,0x1f);
Camara_EnviaBytesI2C(0x5384 ,0x00);
Camara_EnviaBytesI2C(0x5385 ,0x06);
Camara_EnviaBytesI2C(0x5386 ,0x00);
Camara_EnviaBytesI2C(0x5387 ,0x00);
Camara_EnviaBytesI2C(0x5388 ,0x00);
Camara_EnviaBytesI2C(0x5389 ,0xE1);
Camara_EnviaBytesI2C(0x538A ,0x00);
Camara_EnviaBytesI2C(0x538B ,0x2B);
Camara_EnviaBytesI2C(0x538C ,0x00);
Camara_EnviaBytesI2C(0x538D ,0x00);
Camara_EnviaBytesI2C(0x538E ,0x00);
Camara_EnviaBytesI2C(0x538F ,0x10);
Camara_EnviaBytesI2C(0x5390 ,0x00);
Camara_EnviaBytesI2C(0x5391 ,0xB3);
Camara_EnviaBytesI2C(0x5392 ,0x00);
Camara_EnviaBytesI2C(0x5393 ,0xA6);
Camara_EnviaBytesI2C(0x5394 ,0x08);
//;
//;GAMMA
Camara_EnviaBytesI2C(0x5480 ,0x0c);
Camara_EnviaBytesI2C(0x5481 ,0x18);
Camara_EnviaBytesI2C(0x5482 ,0x2f);
//117 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x5483 ,0x55);
Camara_EnviaBytesI2C(0x5484 ,0x64);
Camara_EnviaBytesI2C(0x5485 ,0x71);
Camara_EnviaBytesI2C(0x5486 ,0x7d);
Camara_EnviaBytesI2C(0x5487 ,0x87);
Camara_EnviaBytesI2C(0x5488 ,0x91);
Camara_EnviaBytesI2C(0x5489 ,0x9a);
Camara_EnviaBytesI2C(0x548A ,0xaa);
Camara_EnviaBytesI2C(0x548B ,0xb8);
Camara_EnviaBytesI2C(0x548C ,0xcd);
Camara_EnviaBytesI2C(0x548D ,0xdd);
Camara_EnviaBytesI2C(0x548E ,0xea);
Camara_EnviaBytesI2C(0x548F ,0x1d);
Camara_EnviaBytesI2C(0x5490 ,0x05);
Camara_EnviaBytesI2C(0x5491 ,0x00);
Camara_EnviaBytesI2C(0x5492 ,0x04);
Camara_EnviaBytesI2C(0x5493 ,0x20);
Camara_EnviaBytesI2C(0x5494 ,0x03);
Camara_EnviaBytesI2C(0x5495 ,0x60);
Camara_EnviaBytesI2C(0x5496 ,0x02);
Camara_EnviaBytesI2C(0x5497 ,0xB8);
Camara_EnviaBytesI2C(0x5498 ,0x02);
Camara_EnviaBytesI2C(0x5499 ,0x86);
Camara_EnviaBytesI2C(0x549A ,0x02);
Camara_EnviaBytesI2C(0x549B ,0x5B);
Camara_EnviaBytesI2C(0x549C ,0x02);
Camara_EnviaBytesI2C(0x549D ,0x3B);
Camara_EnviaBytesI2C(0x549E ,0x02);
Camara_EnviaBytesI2C(0x549F ,0x1C);
Camara_EnviaBytesI2C(0x54A0 ,0x02);
Camara_EnviaBytesI2C(0x54A1 ,0x04);
Camara_EnviaBytesI2C(0x54A2 ,0x01);
Camara_EnviaBytesI2C(0x54A3 ,0xED);
Camara_EnviaBytesI2C(0x54A4 ,0x01);
Camara_EnviaBytesI2C(0x54A5 ,0xC5);
Camara_EnviaBytesI2C(0x54A6 ,0x01);
Camara_EnviaBytesI2C(0x54A7 ,0xA5);
Camara_EnviaBytesI2C(0x54A8 ,0x01);
Camara_EnviaBytesI2C(0x54A9 ,0x6C);
Camara_EnviaBytesI2C(0x54AA ,0x01);
Camara_EnviaBytesI2C(0x54AB ,0x41);
Camara_EnviaBytesI2C(0x54AC ,0x01);
Camara_EnviaBytesI2C(0x54AD ,0x20);
Camara_EnviaBytesI2C(0x54AE ,0x00);
Camara_EnviaBytesI2C(0x54AF ,0x16);
Camara_EnviaBytesI2C(0x54B0 ,0x01);
Camara_EnviaBytesI2C(0x54B1 ,0x20);
Camara_EnviaBytesI2C(0x54B2 ,0x00);
//118 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x54B3 ,0x10);
Camara_EnviaBytesI2C(0x54B4 ,0x00);
Camara_EnviaBytesI2C(0x54B5 ,0xf0);
Camara_EnviaBytesI2C(0x54B6 ,0x00);
Camara_EnviaBytesI2C(0x54B7 ,0xDF);
//;
Camara_EnviaBytesI2C(0x5402 ,0x3f);
Camara_EnviaBytesI2C(0x5403 ,0x00);
//;
//;UV ADJUST
Camara_EnviaBytesI2C(0x5500 ,0x10);
Camara_EnviaBytesI2C(0x5502 ,0x00);
Camara_EnviaBytesI2C(0x5503 ,0x06);
Camara_EnviaBytesI2C(0x5504 ,0x00);
Camara_EnviaBytesI2C(0x5505 ,0x7f);
//;AE
Camara_EnviaBytesI2C(0x5025 ,0x80);
Camara_EnviaBytesI2C(0x3a0f ,0x30);
Camara_EnviaBytesI2C(0x3a10 ,0x28);
Camara_EnviaBytesI2C(0x3a1b ,0x30);
Camara_EnviaBytesI2C(0x3a1e ,0x28);
Camara_EnviaBytesI2C(0x3a11 ,0x61);
Camara_EnviaBytesI2C(0x3a1f ,0x10);
Camara_EnviaBytesI2C(0x5688 ,0xfd);
Camara_EnviaBytesI2C(0x5689 ,0xdf);
Camara_EnviaBytesI2C(0x568a ,0xfe);
Camara_EnviaBytesI2C(0x568b ,0xef);
Camara_EnviaBytesI2C(0x568c ,0xfe);
Camara_EnviaBytesI2C(0x568d ,0xef);
Camara_EnviaBytesI2C(0x568e ,0xaa);
Camara_EnviaBytesI2C(0x568f ,0xaa);

}

void Camara_Set_HighResVideo2(void) {
    
//13. 4 High Resolution Video
//13.4.1 1080 P
Camara_EnviaBytesI2C(0x3103 ,0x93);
Camara_EnviaBytesI2C(0x3008 ,0x82);
Camara_EnviaBytesI2C(0x3017 ,0x7f);
Camara_EnviaBytesI2C(0x3018 ,0xfc);
Camara_EnviaBytesI2C(0x3810 ,0xc2);
Camara_EnviaBytesI2C(0x3615 ,0xf0);
Camara_EnviaBytesI2C(0x3000 ,0x00);
Camara_EnviaBytesI2C(0x3001 ,0x00);
Camara_EnviaBytesI2C(0x3002 ,0x00);
Camara_EnviaBytesI2C(0x3003 ,0x00);
Camara_EnviaBytesI2C(0x3004 ,0xff);
Camara_EnviaBytesI2C(0x3030 ,0x2b);
Camara_EnviaBytesI2C(0x3011 ,0x08);
Camara_EnviaBytesI2C(0x3010 ,0x10);
Camara_EnviaBytesI2C(0x3604 ,0x60);
Camara_EnviaBytesI2C(0x3622 ,0x60);
Camara_EnviaBytesI2C(0x3621 ,0x09);
Camara_EnviaBytesI2C(0x3709 ,0x00);
Camara_EnviaBytesI2C(0x4000 ,0x21);
Camara_EnviaBytesI2C(0x401d ,0x22);
Camara_EnviaBytesI2C(0x3600 ,0x54);
Camara_EnviaBytesI2C(0x3605 ,0x04);
Camara_EnviaBytesI2C(0x3606 ,0x3f);
Camara_EnviaBytesI2C(0x3c01 ,0x80);
Camara_EnviaBytesI2C(0x300d ,0x22);
Camara_EnviaBytesI2C(0x3623 ,0x22);
Camara_EnviaBytesI2C(0x5000 ,0x4f);
Camara_EnviaBytesI2C(0x5020 ,0x04);
Camara_EnviaBytesI2C(0x5181 ,0x79);
Camara_EnviaBytesI2C(0x5182 ,0x00);
Camara_EnviaBytesI2C(0x5185 ,0x22);
Camara_EnviaBytesI2C(0x5197 ,0x01);
Camara_EnviaBytesI2C(0x5500 ,0x0a);
Camara_EnviaBytesI2C(0x5504 ,0x00);
Camara_EnviaBytesI2C(0x5505 ,0x7f);
Camara_EnviaBytesI2C(0x5080 ,0x08);
Camara_EnviaBytesI2C(0x300e ,0x18);
Camara_EnviaBytesI2C(0x4610 ,0x00);
Camara_EnviaBytesI2C(0x471d ,0x05);
Camara_EnviaBytesI2C(0x4708 ,0x06);
Camara_EnviaBytesI2C(0x370c ,0xa0);
//108 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x3808 ,0x0a);
Camara_EnviaBytesI2C(0x3809 ,0x20);
Camara_EnviaBytesI2C(0x380a ,0x07);
Camara_EnviaBytesI2C(0x380b ,0x98);
Camara_EnviaBytesI2C(0x380c ,0x0c);
Camara_EnviaBytesI2C(0x380d ,0x80);
Camara_EnviaBytesI2C(0x380e ,0x07);
Camara_EnviaBytesI2C(0x380f ,0xd0);
Camara_EnviaBytesI2C(0x5687 ,0x94);
Camara_EnviaBytesI2C(0x501f ,0x00);
Camara_EnviaBytesI2C(0x5000 ,0x4f);
Camara_EnviaBytesI2C(0x5001 ,0xcf);
Camara_EnviaBytesI2C(0x4300 ,0x30);
Camara_EnviaBytesI2C(0x4300 ,0x30);
Camara_EnviaBytesI2C(0x460b ,0x35);
Camara_EnviaBytesI2C(0x471d ,0x00);
Camara_EnviaBytesI2C(0x3002 ,0x0c);
Camara_EnviaBytesI2C(0x3002 ,0x00);
Camara_EnviaBytesI2C(0x4713 ,0x03);
Camara_EnviaBytesI2C(0x471c ,0x50);
Camara_EnviaBytesI2C(0x4721 ,0x02);
Camara_EnviaBytesI2C(0x4402 ,0x90);
Camara_EnviaBytesI2C(0x460c ,0x22);
Camara_EnviaBytesI2C(0x3815 ,0x44);
Camara_EnviaBytesI2C(0x3503 ,0x07);
Camara_EnviaBytesI2C(0x3501 ,0x73);
Camara_EnviaBytesI2C(0x3502 ,0x80);
Camara_EnviaBytesI2C(0x350b ,0x00);
Camara_EnviaBytesI2C(0x3818 ,0xc8);
Camara_EnviaBytesI2C(0x3801 ,0x88);
Camara_EnviaBytesI2C(0x3824 ,0x11);
Camara_EnviaBytesI2C(0x3a00 ,0x78);
Camara_EnviaBytesI2C(0x3a1a ,0x04);
Camara_EnviaBytesI2C(0x3a13 ,0x30);
Camara_EnviaBytesI2C(0x3a18 ,0x00);
Camara_EnviaBytesI2C(0x3a19 ,0x7c);
Camara_EnviaBytesI2C(0x3a08 ,0x12);
Camara_EnviaBytesI2C(0x3a09 ,0xc0);
Camara_EnviaBytesI2C(0x3a0a ,0x0f);
Camara_EnviaBytesI2C(0x3a0b ,0xa0);
Camara_EnviaBytesI2C(0x350c ,0x07);
Camara_EnviaBytesI2C(0x350d ,0xd0);
Camara_EnviaBytesI2C(0x3a0d ,0x08);
Camara_EnviaBytesI2C(0x3a0e ,0x06);
Camara_EnviaBytesI2C(0x3500 ,0x00);
Camara_EnviaBytesI2C(0x3501 ,0x00);
Camara_EnviaBytesI2C(0x3502 ,0x00);
Camara_EnviaBytesI2C(0x350a ,0x00);
//109 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x350b ,0x00);
Camara_EnviaBytesI2C(0x3503 ,0x00);
Camara_EnviaBytesI2C(0x3030 ,0x2b);
Camara_EnviaBytesI2C(0x3a02 ,0x00);
Camara_EnviaBytesI2C(0x3a03 ,0x7d);
Camara_EnviaBytesI2C(0x3a04 ,0x00);
Camara_EnviaBytesI2C(0x3a14 ,0x00);
Camara_EnviaBytesI2C(0x3a15 ,0x7d);
Camara_EnviaBytesI2C(0x3a16 ,0x00);
Camara_EnviaBytesI2C(0x3a00 ,0x78);
Camara_EnviaBytesI2C(0x3a08 ,0x09);
Camara_EnviaBytesI2C(0x3a09 ,0x60);
Camara_EnviaBytesI2C(0x3a0a ,0x07);
Camara_EnviaBytesI2C(0x3a0b ,0xd0);
Camara_EnviaBytesI2C(0x3a0d ,0x10);
Camara_EnviaBytesI2C(0x3a0e ,0x0d);
//write_i2c(0x4407 ,0x04);
//High resolution
Camara_EnviaBytesI2C(0x4407 ,0x02);
Camara_EnviaBytesI2C(0x5193 ,0x70);
Camara_EnviaBytesI2C(0x589b ,0x00);
Camara_EnviaBytesI2C(0x589a ,0xc0);
Camara_EnviaBytesI2C(0x401e ,0x20);
Camara_EnviaBytesI2C(0x4001 ,0x42);
Camara_EnviaBytesI2C(0x401c ,0x06);
Camara_EnviaBytesI2C(0x3825 ,0xac);
Camara_EnviaBytesI2C(0x3827 ,0x0c);
Camara_EnviaBytesI2C(0x5402 ,0x3f);
Camara_EnviaBytesI2C(0x5403 ,0x00);
Camara_EnviaBytesI2C(0x3406 ,0x00);
Camara_EnviaBytesI2C(0x5180 ,0xff);
Camara_EnviaBytesI2C(0x5181 ,0x52);
Camara_EnviaBytesI2C(0x5182 ,0x11);
Camara_EnviaBytesI2C(0x5183 ,0x14);
Camara_EnviaBytesI2C(0x5184 ,0x25);
Camara_EnviaBytesI2C(0x5185 ,0x24);
Camara_EnviaBytesI2C(0x5186 ,0x06);
Camara_EnviaBytesI2C(0x5187 ,0x08);
Camara_EnviaBytesI2C(0x5188 ,0x08);
Camara_EnviaBytesI2C(0x5189 ,0x7c);
Camara_EnviaBytesI2C(0x518a ,0x60);
Camara_EnviaBytesI2C(0x518b ,0xb2);
Camara_EnviaBytesI2C(0x518c ,0xb2);
Camara_EnviaBytesI2C(0x518d ,0x44);
Camara_EnviaBytesI2C(0x518e ,0x3d);
Camara_EnviaBytesI2C(0x518f ,0x58);
Camara_EnviaBytesI2C(0x5190 ,0x46);
Camara_EnviaBytesI2C(0x5191 ,0xf8);
Camara_EnviaBytesI2C(0x5192 ,0x04);
Camara_EnviaBytesI2C(0x5193 ,0x70);
//110 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x5194 ,0xf0);
Camara_EnviaBytesI2C(0x5195 ,0xf0);
Camara_EnviaBytesI2C(0x5196 ,0x03);
Camara_EnviaBytesI2C(0x5197 ,0x01);
Camara_EnviaBytesI2C(0x5198 ,0x04);
Camara_EnviaBytesI2C(0x5199 ,0x12);
Camara_EnviaBytesI2C(0x519a ,0x04);
Camara_EnviaBytesI2C(0x519b ,0x00);
Camara_EnviaBytesI2C(0x519c ,0x06);
Camara_EnviaBytesI2C(0x519d ,0x82);
Camara_EnviaBytesI2C(0x519e ,0x00);
Camara_EnviaBytesI2C(0x5025 ,0x80);
Camara_EnviaBytesI2C(0x5583 ,0x40);
Camara_EnviaBytesI2C(0x5584 ,0x40);
Camara_EnviaBytesI2C(0x5580 ,0x02);
Camara_EnviaBytesI2C(0x5000 ,0xcf);
Camara_EnviaBytesI2C(0x3710 ,0x10);
Camara_EnviaBytesI2C(0x3632 ,0x51);
Camara_EnviaBytesI2C(0x3702 ,0x10);
Camara_EnviaBytesI2C(0x3703 ,0xb2);
Camara_EnviaBytesI2C(0x3704 ,0x18);
Camara_EnviaBytesI2C(0x370b ,0x40);
Camara_EnviaBytesI2C(0x370d ,0x03);
Camara_EnviaBytesI2C(0x3631 ,0x01);
Camara_EnviaBytesI2C(0x3632 ,0x52);
Camara_EnviaBytesI2C(0x3606 ,0x24);
Camara_EnviaBytesI2C(0x3620 ,0x96);
Camara_EnviaBytesI2C(0x5785 ,0x07);
Camara_EnviaBytesI2C(0x3a13 ,0x30);
Camara_EnviaBytesI2C(0x3600 ,0x52);
Camara_EnviaBytesI2C(0x3604 ,0x48);
Camara_EnviaBytesI2C(0x3606 ,0x1b);
Camara_EnviaBytesI2C(0x370d ,0x0b);
Camara_EnviaBytesI2C(0x370f ,0xc0);
Camara_EnviaBytesI2C(0x3709 ,0x01);
Camara_EnviaBytesI2C(0x3823 ,0x00);
Camara_EnviaBytesI2C(0x5007 ,0x00);
Camara_EnviaBytesI2C(0x5009 ,0x00);
Camara_EnviaBytesI2C(0x5011 ,0x00);
Camara_EnviaBytesI2C(0x5013 ,0x00);
Camara_EnviaBytesI2C(0x519e ,0x00);
Camara_EnviaBytesI2C(0x5086 ,0x00);
Camara_EnviaBytesI2C(0x5087 ,0x00);
Camara_EnviaBytesI2C(0x5088 ,0x00);
Camara_EnviaBytesI2C(0x5089 ,0x00);
Camara_EnviaBytesI2C(0x302b ,0x00);
Camara_EnviaBytesI2C(0x3503 ,0x07);
Camara_EnviaBytesI2C(0x3011 ,0x07);
//111 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x350c ,0x04);
Camara_EnviaBytesI2C(0x350d ,0x58);
Camara_EnviaBytesI2C(0x3801 ,0x8a);
Camara_EnviaBytesI2C(0x3803 ,0x0a);
Camara_EnviaBytesI2C(0x3804 ,0x0a);
Camara_EnviaBytesI2C(0x3805 ,0x20);
Camara_EnviaBytesI2C(0x3806 ,0x07);
Camara_EnviaBytesI2C(0x3807 ,0x98);
Camara_EnviaBytesI2C(0x3808 ,0x0a);
Camara_EnviaBytesI2C(0x3809 ,0x20);
Camara_EnviaBytesI2C(0x380a ,0x07);
Camara_EnviaBytesI2C(0x380b ,0x98);
Camara_EnviaBytesI2C(0x380c ,0x0c);
Camara_EnviaBytesI2C(0x380d ,0x80);
Camara_EnviaBytesI2C(0x380e ,0x07);
Camara_EnviaBytesI2C(0x380f ,0xd0);
Camara_EnviaBytesI2C(0x381c ,0x11);
Camara_EnviaBytesI2C(0x381d ,0xba);
Camara_EnviaBytesI2C(0x381e ,0x04);
Camara_EnviaBytesI2C(0x381f ,0x48);
Camara_EnviaBytesI2C(0x3820 ,0x04);
Camara_EnviaBytesI2C(0x3821 ,0x18);
Camara_EnviaBytesI2C(0x3a08 ,0x14);
Camara_EnviaBytesI2C(0x3a09 ,0xe0);
Camara_EnviaBytesI2C(0x3a0a ,0x11);
Camara_EnviaBytesI2C(0x3a0b ,0x60);
Camara_EnviaBytesI2C(0x3a0d ,0x04);
Camara_EnviaBytesI2C(0x3a0e ,0x03);
Camara_EnviaBytesI2C(0x5682 ,0x0a);
Camara_EnviaBytesI2C(0x5683 ,0x20);
Camara_EnviaBytesI2C(0x5686 ,0x07);
Camara_EnviaBytesI2C(0x5687 ,0x98);
Camara_EnviaBytesI2C(0x5001 ,0x7f);
Camara_EnviaBytesI2C(0x3503 ,0x00);
Camara_EnviaBytesI2C(0x3010 ,0x10);
Camara_EnviaBytesI2C(0x5001 ,0xFF);
Camara_EnviaBytesI2C(0x5583 ,0x50);
Camara_EnviaBytesI2C(0x5584 ,0x50);
Camara_EnviaBytesI2C(0x5580 ,0x02);
Camara_EnviaBytesI2C(0x3c01 ,0x80);
Camara_EnviaBytesI2C(0x3c00 ,0x04);
//;LENS
Camara_EnviaBytesI2C(0x5800 ,0x48);
Camara_EnviaBytesI2C(0x5801 ,0x31);
Camara_EnviaBytesI2C(0x5802 ,0x21);
//112 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x5803 ,0x1b);
Camara_EnviaBytesI2C(0x5804 ,0x1a);
Camara_EnviaBytesI2C(0x5805 ,0x1e);
Camara_EnviaBytesI2C(0x5806 ,0x29);
Camara_EnviaBytesI2C(0x5807 ,0x38);
Camara_EnviaBytesI2C(0x5808 ,0x26);
Camara_EnviaBytesI2C(0x5809 ,0x17);
Camara_EnviaBytesI2C(0x580a ,0x11);
Camara_EnviaBytesI2C(0x580b ,0xe );
Camara_EnviaBytesI2C(0x580c ,0xd );
Camara_EnviaBytesI2C(0x580d ,0xe );
Camara_EnviaBytesI2C(0x580e ,0x13);
Camara_EnviaBytesI2C(0x580f ,0x1a);
Camara_EnviaBytesI2C(0x5810 ,0x15);
Camara_EnviaBytesI2C(0x5811 ,0xd );
Camara_EnviaBytesI2C(0x5812 ,0x8 );
Camara_EnviaBytesI2C(0x5813 ,0x5 );
Camara_EnviaBytesI2C(0x5814 ,0x4 );
Camara_EnviaBytesI2C(0x5815 ,0x5 );
Camara_EnviaBytesI2C(0x5816 ,0x9 );
Camara_EnviaBytesI2C(0x5817 ,0xd );
Camara_EnviaBytesI2C(0x5818 ,0x11);
Camara_EnviaBytesI2C(0x5819 ,0xa );
Camara_EnviaBytesI2C(0x581a ,0x4 );
Camara_EnviaBytesI2C(0x581b ,0x0 );
Camara_EnviaBytesI2C(0x581c ,0x0 );
Camara_EnviaBytesI2C(0x581d ,0x1 );
Camara_EnviaBytesI2C(0x581e ,0x6 );
Camara_EnviaBytesI2C(0x581f ,0x9 );
Camara_EnviaBytesI2C(0x5820 ,0x12);
Camara_EnviaBytesI2C(0x5821 ,0xb );
Camara_EnviaBytesI2C(0x5822 ,0x4 );
Camara_EnviaBytesI2C(0x5823 ,0x0 );
Camara_EnviaBytesI2C(0x5824 ,0x0 );
Camara_EnviaBytesI2C(0x5825 ,0x1 );
Camara_EnviaBytesI2C(0x5826 ,0x6 );
Camara_EnviaBytesI2C(0x5827 ,0xa );
Camara_EnviaBytesI2C(0x5828 ,0x17);
Camara_EnviaBytesI2C(0x5829 ,0xf );
Camara_EnviaBytesI2C(0x582a ,0x9 );
Camara_EnviaBytesI2C(0x582b ,0x6 );
Camara_EnviaBytesI2C(0x582c ,0x5 );
Camara_EnviaBytesI2C(0x582d ,0x6 );
Camara_EnviaBytesI2C(0x582e ,0xa );
Camara_EnviaBytesI2C(0x582f ,0xe );
Camara_EnviaBytesI2C(0x5830 ,0x28);
Camara_EnviaBytesI2C(0x5831 ,0x1a);
Camara_EnviaBytesI2C(0x5832 ,0x11);
//113 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x5833 ,0xe );
Camara_EnviaBytesI2C(0x5834 ,0xe );
Camara_EnviaBytesI2C(0x5835 ,0xf );
Camara_EnviaBytesI2C(0x5836 ,0x15);
Camara_EnviaBytesI2C(0x5837 ,0x1d);
Camara_EnviaBytesI2C(0x5838 ,0x6e);
Camara_EnviaBytesI2C(0x5839 ,0x39);
Camara_EnviaBytesI2C(0x583a ,0x27);
Camara_EnviaBytesI2C(0x583b ,0x1f);
Camara_EnviaBytesI2C(0x583c ,0x1e);
Camara_EnviaBytesI2C(0x583d ,0x23);
Camara_EnviaBytesI2C(0x583e ,0x2f);
Camara_EnviaBytesI2C(0x583f ,0x41);
Camara_EnviaBytesI2C(0x5840 ,0xe );
Camara_EnviaBytesI2C(0x5841 ,0xc );
Camara_EnviaBytesI2C(0x5842 ,0xd );
Camara_EnviaBytesI2C(0x5843 ,0xc );
Camara_EnviaBytesI2C(0x5844 ,0xc );
Camara_EnviaBytesI2C(0x5845 ,0xc );
Camara_EnviaBytesI2C(0x5846 ,0xc );
Camara_EnviaBytesI2C(0x5847 ,0xc );
Camara_EnviaBytesI2C(0x5848 ,0xd );
Camara_EnviaBytesI2C(0x5849 ,0xe );
Camara_EnviaBytesI2C(0x584a ,0xe );
Camara_EnviaBytesI2C(0x584b ,0xa );
Camara_EnviaBytesI2C(0x584c ,0xe );
Camara_EnviaBytesI2C(0x584d ,0xe );
Camara_EnviaBytesI2C(0x584e ,0x10);
Camara_EnviaBytesI2C(0x584f ,0x10);
Camara_EnviaBytesI2C(0x5850 ,0x11);
Camara_EnviaBytesI2C(0x5851 ,0xa );
Camara_EnviaBytesI2C(0x5852 ,0xf );
Camara_EnviaBytesI2C(0x5853 ,0xe );
Camara_EnviaBytesI2C(0x5854 ,0x10);
Camara_EnviaBytesI2C(0x5855 ,0x10);
Camara_EnviaBytesI2C(0x5856 ,0x10);
Camara_EnviaBytesI2C(0x5857 ,0xa );
Camara_EnviaBytesI2C(0x5858 ,0xe );
Camara_EnviaBytesI2C(0x5859 ,0xe );
Camara_EnviaBytesI2C(0x585a ,0xf );
Camara_EnviaBytesI2C(0x585b ,0xf );
Camara_EnviaBytesI2C(0x585c ,0xf );
Camara_EnviaBytesI2C(0x585d ,0xa );
Camara_EnviaBytesI2C(0x585e ,0x9 );
Camara_EnviaBytesI2C(0x585f ,0xd );
Camara_EnviaBytesI2C(0x5860 ,0xc );
Camara_EnviaBytesI2C(0x5861 ,0xb );
Camara_EnviaBytesI2C(0x5862 ,0xd );
//114 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x5863 ,0x7 );
Camara_EnviaBytesI2C(0x5864 ,0x17);
Camara_EnviaBytesI2C(0x5865 ,0x14);
Camara_EnviaBytesI2C(0x5866 ,0x18);
Camara_EnviaBytesI2C(0x5867 ,0x18);
Camara_EnviaBytesI2C(0x5868 ,0x16);
Camara_EnviaBytesI2C(0x5869 ,0x12);
Camara_EnviaBytesI2C(0x586a ,0x1b);
Camara_EnviaBytesI2C(0x586b ,0x1a);
Camara_EnviaBytesI2C(0x586c ,0x16);
Camara_EnviaBytesI2C(0x586d ,0x16);
Camara_EnviaBytesI2C(0x586e ,0x18);
Camara_EnviaBytesI2C(0x586f ,0x1f);
Camara_EnviaBytesI2C(0x5870 ,0x1c);
Camara_EnviaBytesI2C(0x5871 ,0x16);
Camara_EnviaBytesI2C(0x5872 ,0x10);
Camara_EnviaBytesI2C(0x5873 ,0xf );
Camara_EnviaBytesI2C(0x5874 ,0x13);
Camara_EnviaBytesI2C(0x5875 ,0x1c);
Camara_EnviaBytesI2C(0x5876 ,0x1e);
Camara_EnviaBytesI2C(0x5877 ,0x17);
Camara_EnviaBytesI2C(0x5878 ,0x11);
Camara_EnviaBytesI2C(0x5879 ,0x11);
Camara_EnviaBytesI2C(0x587a ,0x14);
Camara_EnviaBytesI2C(0x587b ,0x1e);
Camara_EnviaBytesI2C(0x587c ,0x1c);
Camara_EnviaBytesI2C(0x587d ,0x1c);
Camara_EnviaBytesI2C(0x587e ,0x1a);
Camara_EnviaBytesI2C(0x587f ,0x1a);
Camara_EnviaBytesI2C(0x5880 ,0x1b);
Camara_EnviaBytesI2C(0x5881 ,0x1f);
Camara_EnviaBytesI2C(0x5882 ,0x14);
Camara_EnviaBytesI2C(0x5883 ,0x1a);
Camara_EnviaBytesI2C(0x5884 ,0x1d);
Camara_EnviaBytesI2C(0x5885 ,0x1e);
Camara_EnviaBytesI2C(0x5886 ,0x1a);
Camara_EnviaBytesI2C(0x5887 ,0x1a);
//;
//;AWB
Camara_EnviaBytesI2C(0x5180 ,0xff);
Camara_EnviaBytesI2C(0x5181 ,0x52);
Camara_EnviaBytesI2C(0x5182 ,0x11);
Camara_EnviaBytesI2C(0x5183 ,0x14);
Camara_EnviaBytesI2C(0x5184 ,0x25);
Camara_EnviaBytesI2C(0x5185 ,0x24);
Camara_EnviaBytesI2C(0x5186 ,0x14);
Camara_EnviaBytesI2C(0x5187 ,0x14);
Camara_EnviaBytesI2C(0x5188 ,0x14);
//115 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x5189 ,0x69);
Camara_EnviaBytesI2C(0x518a ,0x60);
Camara_EnviaBytesI2C(0x518b ,0xa2);
Camara_EnviaBytesI2C(0x518c ,0x9c);
Camara_EnviaBytesI2C(0x518d ,0x36);
Camara_EnviaBytesI2C(0x518e ,0x34);
Camara_EnviaBytesI2C(0x518f ,0x54);
Camara_EnviaBytesI2C(0x5190 ,0x4c);
Camara_EnviaBytesI2C(0x5191 ,0xf8);
Camara_EnviaBytesI2C(0x5192 ,0x04);
Camara_EnviaBytesI2C(0x5193 ,0x70);
Camara_EnviaBytesI2C(0x5194 ,0xf0);
Camara_EnviaBytesI2C(0x5195 ,0xf0);
Camara_EnviaBytesI2C(0x5196 ,0x03);
Camara_EnviaBytesI2C(0x5197 ,0x01);
Camara_EnviaBytesI2C(0x5198 ,0x05);
Camara_EnviaBytesI2C(0x5199 ,0x2f);
Camara_EnviaBytesI2C(0x519a ,0x04);
Camara_EnviaBytesI2C(0x519b ,0x00);
Camara_EnviaBytesI2C(0x519c ,0x06);
Camara_EnviaBytesI2C(0x519d ,0xa0);
Camara_EnviaBytesI2C(0x519e ,0xa0);
//;
//;D/S
Camara_EnviaBytesI2C(0x528a ,0x00);
Camara_EnviaBytesI2C(0x528b ,0x01);
Camara_EnviaBytesI2C(0x528c ,0x04);
Camara_EnviaBytesI2C(0x528d ,0x08);
Camara_EnviaBytesI2C(0x528e ,0x10);
Camara_EnviaBytesI2C(0x528f ,0x20);
Camara_EnviaBytesI2C(0x5290 ,0x30);
Camara_EnviaBytesI2C(0x5292 ,0x00);
Camara_EnviaBytesI2C(0x5293 ,0x00);
Camara_EnviaBytesI2C(0x5294 ,0x00);
Camara_EnviaBytesI2C(0x5295 ,0x01);
Camara_EnviaBytesI2C(0x5296 ,0x00);
Camara_EnviaBytesI2C(0x5297 ,0x04);
Camara_EnviaBytesI2C(0x5298 ,0x00);
Camara_EnviaBytesI2C(0x5299 ,0x08);
Camara_EnviaBytesI2C(0x529a ,0x00);
Camara_EnviaBytesI2C(0x529b ,0x10);
Camara_EnviaBytesI2C(0x529c ,0x00);
Camara_EnviaBytesI2C(0x529d ,0x20);
Camara_EnviaBytesI2C(0x529e ,0x00);
Camara_EnviaBytesI2C(0x529f ,0x30);
Camara_EnviaBytesI2C(0x5282 ,0x00);
Camara_EnviaBytesI2C(0x5300 ,0x00);
Camara_EnviaBytesI2C(0x5301 ,0x20);
//116 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x5302 ,0x00);
Camara_EnviaBytesI2C(0x5303 ,0x7c);
Camara_EnviaBytesI2C(0x530c ,0x00);
Camara_EnviaBytesI2C(0x530d ,0x10);
Camara_EnviaBytesI2C(0x530e ,0x20);
Camara_EnviaBytesI2C(0x530f ,0x80);
Camara_EnviaBytesI2C(0x5310 ,0x20);
Camara_EnviaBytesI2C(0x5311 ,0x80);
Camara_EnviaBytesI2C(0x5308 ,0x20);
Camara_EnviaBytesI2C(0x5309 ,0x40);
Camara_EnviaBytesI2C(0x5304 ,0x00);
Camara_EnviaBytesI2C(0x5305 ,0x30);
Camara_EnviaBytesI2C(0x5306 ,0x00);
Camara_EnviaBytesI2C(0x5307 ,0x80);
Camara_EnviaBytesI2C(0x5314 ,0x08);
Camara_EnviaBytesI2C(0x5315 ,0x20);
Camara_EnviaBytesI2C(0x5319 ,0x30);
Camara_EnviaBytesI2C(0x5316 ,0x10);
Camara_EnviaBytesI2C(0x5317 ,0x00);
Camara_EnviaBytesI2C(0x5318 ,0x02);
//;
//;CMX
Camara_EnviaBytesI2C(0x5380 ,0x01);
Camara_EnviaBytesI2C(0x5381 ,0x00);
Camara_EnviaBytesI2C(0x5382 ,0x00);
Camara_EnviaBytesI2C(0x5383 ,0x1f);
Camara_EnviaBytesI2C(0x5384 ,0x00);
Camara_EnviaBytesI2C(0x5385 ,0x06);
Camara_EnviaBytesI2C(0x5386 ,0x00);
Camara_EnviaBytesI2C(0x5387 ,0x00);
Camara_EnviaBytesI2C(0x5388 ,0x00);
Camara_EnviaBytesI2C(0x5389 ,0xE1);
Camara_EnviaBytesI2C(0x538A ,0x00);
Camara_EnviaBytesI2C(0x538B ,0x2B);
Camara_EnviaBytesI2C(0x538C ,0x00);
Camara_EnviaBytesI2C(0x538D ,0x00);
Camara_EnviaBytesI2C(0x538E ,0x00);
Camara_EnviaBytesI2C(0x538F ,0x10);
Camara_EnviaBytesI2C(0x5390 ,0x00);
Camara_EnviaBytesI2C(0x5391 ,0xB3);
Camara_EnviaBytesI2C(0x5392 ,0x00);
Camara_EnviaBytesI2C(0x5393 ,0xA6);
Camara_EnviaBytesI2C(0x5394 ,0x08);
//;
//;GAMMA
Camara_EnviaBytesI2C(0x5480 ,0x0c);
Camara_EnviaBytesI2C(0x5481 ,0x18);
Camara_EnviaBytesI2C(0x5482 ,0x2f);
//117 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x5483 ,0x55);
Camara_EnviaBytesI2C(0x5484 ,0x64);
Camara_EnviaBytesI2C(0x5485 ,0x71);
Camara_EnviaBytesI2C(0x5486 ,0x7d);
Camara_EnviaBytesI2C(0x5487 ,0x87);
Camara_EnviaBytesI2C(0x5488 ,0x91);
Camara_EnviaBytesI2C(0x5489 ,0x9a);
Camara_EnviaBytesI2C(0x548A ,0xaa);
Camara_EnviaBytesI2C(0x548B ,0xb8);
Camara_EnviaBytesI2C(0x548C ,0xcd);
Camara_EnviaBytesI2C(0x548D ,0xdd);
Camara_EnviaBytesI2C(0x548E ,0xea);
Camara_EnviaBytesI2C(0x548F ,0x1d);
Camara_EnviaBytesI2C(0x5490 ,0x05);
Camara_EnviaBytesI2C(0x5491 ,0x00);
Camara_EnviaBytesI2C(0x5492 ,0x04);
Camara_EnviaBytesI2C(0x5493 ,0x20);
Camara_EnviaBytesI2C(0x5494 ,0x03);
Camara_EnviaBytesI2C(0x5495 ,0x60);
Camara_EnviaBytesI2C(0x5496 ,0x02);
Camara_EnviaBytesI2C(0x5497 ,0xB8);
Camara_EnviaBytesI2C(0x5498 ,0x02);
Camara_EnviaBytesI2C(0x5499 ,0x86);
Camara_EnviaBytesI2C(0x549A ,0x02);
Camara_EnviaBytesI2C(0x549B ,0x5B);
Camara_EnviaBytesI2C(0x549C ,0x02);
Camara_EnviaBytesI2C(0x549D ,0x3B);
Camara_EnviaBytesI2C(0x549E ,0x02);
Camara_EnviaBytesI2C(0x549F ,0x1C);
Camara_EnviaBytesI2C(0x54A0 ,0x02);
Camara_EnviaBytesI2C(0x54A1 ,0x04);
Camara_EnviaBytesI2C(0x54A2 ,0x01);
Camara_EnviaBytesI2C(0x54A3 ,0xED);
Camara_EnviaBytesI2C(0x54A4 ,0x01);
Camara_EnviaBytesI2C(0x54A5 ,0xC5);
Camara_EnviaBytesI2C(0x54A6 ,0x01);
Camara_EnviaBytesI2C(0x54A7 ,0xA5);
Camara_EnviaBytesI2C(0x54A8 ,0x01);
Camara_EnviaBytesI2C(0x54A9 ,0x6C);
Camara_EnviaBytesI2C(0x54AA ,0x01);
Camara_EnviaBytesI2C(0x54AB ,0x41);
Camara_EnviaBytesI2C(0x54AC ,0x01);
Camara_EnviaBytesI2C(0x54AD ,0x20);
Camara_EnviaBytesI2C(0x54AE ,0x00);
Camara_EnviaBytesI2C(0x54AF ,0x16);
Camara_EnviaBytesI2C(0x54B0 ,0x01);
Camara_EnviaBytesI2C(0x54B1 ,0x20);
Camara_EnviaBytesI2C(0x54B2 ,0x00);
//118 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x54B3 ,0x10);
Camara_EnviaBytesI2C(0x54B4 ,0x00);
Camara_EnviaBytesI2C(0x54B5 ,0xf0);
Camara_EnviaBytesI2C(0x54B6 ,0x00);
Camara_EnviaBytesI2C(0x54B7 ,0xDF);
//;
Camara_EnviaBytesI2C(0x5402 ,0x3f);
Camara_EnviaBytesI2C(0x5403 ,0x00);
//;
//;UV ADJUST
Camara_EnviaBytesI2C(0x5500 ,0x10);
Camara_EnviaBytesI2C(0x5502 ,0x00);
Camara_EnviaBytesI2C(0x5503 ,0x06);
Camara_EnviaBytesI2C(0x5504 ,0x00);
Camara_EnviaBytesI2C(0x5505 ,0x7f);
//;AE
Camara_EnviaBytesI2C(0x5025 ,0x80);
Camara_EnviaBytesI2C(0x3a0f ,0x30);
Camara_EnviaBytesI2C(0x3a10 ,0x28);
Camara_EnviaBytesI2C(0x3a1b ,0x30);
Camara_EnviaBytesI2C(0x3a1e ,0x28);
Camara_EnviaBytesI2C(0x3a11 ,0x61);
Camara_EnviaBytesI2C(0x3a1f ,0x10);
Camara_EnviaBytesI2C(0x5688 ,0xfd);
Camara_EnviaBytesI2C(0x5689 ,0xdf);
Camara_EnviaBytesI2C(0x568a ,0xfe);
Camara_EnviaBytesI2C(0x568b ,0xef);
Camara_EnviaBytesI2C(0x568c ,0xfe);
Camara_EnviaBytesI2C(0x568d ,0xef);
Camara_EnviaBytesI2C(0x568e ,0xaa);
Camara_EnviaBytesI2C(0x568f ,0xaa);

    
}


void Camara_Set_HighResVideo3(void) {
//13. 4 High Resolution Video
//13.4.1 1080 P
Camara_EnviaBytesI2C(0x3103 ,0x93);
Camara_EnviaBytesI2C(0x3008 ,0x82);
Camara_EnviaBytesI2C(0x3017 ,0x7f);
Camara_EnviaBytesI2C(0x3018 ,0xfc);
Camara_EnviaBytesI2C(0x3810 ,0xc2);
Camara_EnviaBytesI2C(0x3615 ,0xf0);
Camara_EnviaBytesI2C(0x3000 ,0x00);
Camara_EnviaBytesI2C(0x3001 ,0x00);
Camara_EnviaBytesI2C(0x3002 ,0x00);
Camara_EnviaBytesI2C(0x3003 ,0x00);
Camara_EnviaBytesI2C(0x3004 ,0xff);
Camara_EnviaBytesI2C(0x3030 ,0x2b);
Camara_EnviaBytesI2C(0x3011 ,0x08);
Camara_EnviaBytesI2C(0x3010 ,0x10);
Camara_EnviaBytesI2C(0x3604 ,0x60);
Camara_EnviaBytesI2C(0x3622 ,0x60);
Camara_EnviaBytesI2C(0x3621 ,0x09);
Camara_EnviaBytesI2C(0x3709 ,0x00);
Camara_EnviaBytesI2C(0x4000 ,0x21);
Camara_EnviaBytesI2C(0x401d ,0x22);
Camara_EnviaBytesI2C(0x3600 ,0x54);
Camara_EnviaBytesI2C(0x3605 ,0x04);
Camara_EnviaBytesI2C(0x3606 ,0x3f);
Camara_EnviaBytesI2C(0x3c01 ,0x80);
Camara_EnviaBytesI2C(0x300d ,0x22);
Camara_EnviaBytesI2C(0x3623 ,0x22);
Camara_EnviaBytesI2C(0x5000 ,0x4f);
Camara_EnviaBytesI2C(0x5020 ,0x04);
Camara_EnviaBytesI2C(0x5181 ,0x79);
Camara_EnviaBytesI2C(0x5182 ,0x00);
Camara_EnviaBytesI2C(0x5185 ,0x22);
Camara_EnviaBytesI2C(0x5197 ,0x01);
Camara_EnviaBytesI2C(0x5500 ,0x0a);
Camara_EnviaBytesI2C(0x5504 ,0x00);
Camara_EnviaBytesI2C(0x5505 ,0x7f);
Camara_EnviaBytesI2C(0x5080 ,0x08);
Camara_EnviaBytesI2C(0x300e ,0x18);
Camara_EnviaBytesI2C(0x4610 ,0x00);
Camara_EnviaBytesI2C(0x471d ,0x05);
Camara_EnviaBytesI2C(0x4708 ,0x06);
Camara_EnviaBytesI2C(0x370c ,0xa0);
//108 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x3808 ,0x0a);
Camara_EnviaBytesI2C(0x3809 ,0x20);
Camara_EnviaBytesI2C(0x380a ,0x07);
Camara_EnviaBytesI2C(0x380b ,0x98);
Camara_EnviaBytesI2C(0x380c ,0x0c);
Camara_EnviaBytesI2C(0x380d ,0x80);
Camara_EnviaBytesI2C(0x380e ,0x07);
Camara_EnviaBytesI2C(0x380f ,0xd0);
Camara_EnviaBytesI2C(0x5687 ,0x98);
Camara_EnviaBytesI2C(0x501f ,0x00);
Camara_EnviaBytesI2C(0x5000 ,0x4f);
Camara_EnviaBytesI2C(0x5001 ,0xcf);
Camara_EnviaBytesI2C(0x4300 ,0x30);
Camara_EnviaBytesI2C(0x4300 ,0x30);
Camara_EnviaBytesI2C(0x460b ,0x35);
Camara_EnviaBytesI2C(0x471d ,0x00);
Camara_EnviaBytesI2C(0x3002 ,0x0c);
Camara_EnviaBytesI2C(0x3002 ,0x00);
Camara_EnviaBytesI2C(0x4713 ,0x03);
Camara_EnviaBytesI2C(0x471c ,0x50);
Camara_EnviaBytesI2C(0x4721 ,0x02);
Camara_EnviaBytesI2C(0x4402 ,0x90);
Camara_EnviaBytesI2C(0x460c ,0x22);
Camara_EnviaBytesI2C(0x3815 ,0x44);
Camara_EnviaBytesI2C(0x3503 ,0x07);
Camara_EnviaBytesI2C(0x3501 ,0x73);
Camara_EnviaBytesI2C(0x3502 ,0x80);
Camara_EnviaBytesI2C(0x350b ,0x00);
Camara_EnviaBytesI2C(0x3818 ,0xc8);
Camara_EnviaBytesI2C(0x3801 ,0x88);
Camara_EnviaBytesI2C(0x3824 ,0x11);
Camara_EnviaBytesI2C(0x3a00 ,0x78);
Camara_EnviaBytesI2C(0x3a1a ,0x04);
Camara_EnviaBytesI2C(0x3a13 ,0x30);
Camara_EnviaBytesI2C(0x3a18 ,0x00);
Camara_EnviaBytesI2C(0x3a19 ,0x7c);
Camara_EnviaBytesI2C(0x3a08 ,0x12);
Camara_EnviaBytesI2C(0x3a09 ,0xc0);
Camara_EnviaBytesI2C(0x3a0a ,0x0f);
Camara_EnviaBytesI2C(0x3a0b ,0xa0);
Camara_EnviaBytesI2C(0x350c ,0x07);
Camara_EnviaBytesI2C(0x350d ,0xd0);
Camara_EnviaBytesI2C(0x3a0d ,0x08);
Camara_EnviaBytesI2C(0x3a0e ,0x06);
Camara_EnviaBytesI2C(0x3500 ,0x00);
Camara_EnviaBytesI2C(0x3501 ,0x00);
Camara_EnviaBytesI2C(0x3502 ,0x00);
Camara_EnviaBytesI2C(0x350a ,0x00);
//109 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x350b ,0x00);
Camara_EnviaBytesI2C(0x3503 ,0x00);
Camara_EnviaBytesI2C(0x3030 ,0x2b);
Camara_EnviaBytesI2C(0x3a02 ,0x00);
Camara_EnviaBytesI2C(0x3a03 ,0x7d);
Camara_EnviaBytesI2C(0x3a04 ,0x00);
Camara_EnviaBytesI2C(0x3a14 ,0x00);
Camara_EnviaBytesI2C(0x3a15 ,0x7d);
Camara_EnviaBytesI2C(0x3a16 ,0x00);
Camara_EnviaBytesI2C(0x3a00 ,0x78);
Camara_EnviaBytesI2C(0x3a08 ,0x09);
Camara_EnviaBytesI2C(0x3a09 ,0x60);
Camara_EnviaBytesI2C(0x3a0a ,0x07);
Camara_EnviaBytesI2C(0x3a0b ,0xd0);
Camara_EnviaBytesI2C(0x3a0d ,0x10);
Camara_EnviaBytesI2C(0x3a0e ,0x0d);
//write_i2c(0x4407 ,0x04);
//High resolution
Camara_EnviaBytesI2C(0x4407 ,0x02);
Camara_EnviaBytesI2C(0x5193 ,0x70);
Camara_EnviaBytesI2C(0x589b ,0x00);
Camara_EnviaBytesI2C(0x589a ,0xc0);
Camara_EnviaBytesI2C(0x401e ,0x20);
Camara_EnviaBytesI2C(0x4001 ,0x42);
Camara_EnviaBytesI2C(0x401c ,0x06);
Camara_EnviaBytesI2C(0x3825 ,0xac);
Camara_EnviaBytesI2C(0x3827 ,0x0c);
Camara_EnviaBytesI2C(0x5402 ,0x3f);
Camara_EnviaBytesI2C(0x5403 ,0x00);
Camara_EnviaBytesI2C(0x3406 ,0x00);
Camara_EnviaBytesI2C(0x5180 ,0xff);
Camara_EnviaBytesI2C(0x5181 ,0x52);
Camara_EnviaBytesI2C(0x5182 ,0x11);
Camara_EnviaBytesI2C(0x5183 ,0x14);
Camara_EnviaBytesI2C(0x5184 ,0x25);
Camara_EnviaBytesI2C(0x5185 ,0x24);
Camara_EnviaBytesI2C(0x5186 ,0x06);
Camara_EnviaBytesI2C(0x5187 ,0x08);
Camara_EnviaBytesI2C(0x5188 ,0x08);
Camara_EnviaBytesI2C(0x5189 ,0x7c);
Camara_EnviaBytesI2C(0x518a ,0x60);
Camara_EnviaBytesI2C(0x518b ,0xb2);
Camara_EnviaBytesI2C(0x518c ,0xb2);
Camara_EnviaBytesI2C(0x518d ,0x44);
Camara_EnviaBytesI2C(0x518e ,0x3d);
Camara_EnviaBytesI2C(0x518f ,0x58);
Camara_EnviaBytesI2C(0x5190 ,0x46);
Camara_EnviaBytesI2C(0x5191 ,0xf8);
Camara_EnviaBytesI2C(0x5192 ,0x04);
Camara_EnviaBytesI2C(0x5193 ,0x70);
//110 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x5194 ,0xf0);
Camara_EnviaBytesI2C(0x5195 ,0xf0);
Camara_EnviaBytesI2C(0x5196 ,0x03);
Camara_EnviaBytesI2C(0x5197 ,0x01);
Camara_EnviaBytesI2C(0x5198 ,0x04);
Camara_EnviaBytesI2C(0x5199 ,0x12);
Camara_EnviaBytesI2C(0x519a ,0x04);
Camara_EnviaBytesI2C(0x519b ,0x00);
Camara_EnviaBytesI2C(0x519c ,0x06);
Camara_EnviaBytesI2C(0x519d ,0x82);
Camara_EnviaBytesI2C(0x519e ,0x00);
Camara_EnviaBytesI2C(0x5025 ,0x80);
Camara_EnviaBytesI2C(0x5583 ,0x40);
Camara_EnviaBytesI2C(0x5584 ,0x40);
Camara_EnviaBytesI2C(0x5580 ,0x02);
Camara_EnviaBytesI2C(0x5000 ,0xcf);
Camara_EnviaBytesI2C(0x3710 ,0x10);
Camara_EnviaBytesI2C(0x3632 ,0x51);
Camara_EnviaBytesI2C(0x3702 ,0x10);
Camara_EnviaBytesI2C(0x3703 ,0xb2);
Camara_EnviaBytesI2C(0x3704 ,0x18);
Camara_EnviaBytesI2C(0x370b ,0x40);
Camara_EnviaBytesI2C(0x370d ,0x03);
Camara_EnviaBytesI2C(0x3631 ,0x01);
Camara_EnviaBytesI2C(0x3632 ,0x52);
Camara_EnviaBytesI2C(0x3606 ,0x24);
Camara_EnviaBytesI2C(0x3620 ,0x96);
Camara_EnviaBytesI2C(0x5785 ,0x07);
Camara_EnviaBytesI2C(0x3a13 ,0x30);
Camara_EnviaBytesI2C(0x3600 ,0x52);
Camara_EnviaBytesI2C(0x3604 ,0x48);
Camara_EnviaBytesI2C(0x3606 ,0x1b);
Camara_EnviaBytesI2C(0x370d ,0x0b);
Camara_EnviaBytesI2C(0x370f ,0xc0);
Camara_EnviaBytesI2C(0x3709 ,0x01);
Camara_EnviaBytesI2C(0x3823 ,0x00);
Camara_EnviaBytesI2C(0x5007 ,0x00);
Camara_EnviaBytesI2C(0x5009 ,0x00);
Camara_EnviaBytesI2C(0x5011 ,0x00);
Camara_EnviaBytesI2C(0x5013 ,0x00);
Camara_EnviaBytesI2C(0x519e ,0x00);
Camara_EnviaBytesI2C(0x5086 ,0x00);
Camara_EnviaBytesI2C(0x5087 ,0x00);
Camara_EnviaBytesI2C(0x5088 ,0x00);
Camara_EnviaBytesI2C(0x5089 ,0x00);
Camara_EnviaBytesI2C(0x302b ,0x00);
Camara_EnviaBytesI2C(0x3503 ,0x07);
Camara_EnviaBytesI2C(0x3011 ,0x07);
//111 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x350c ,0x04);
Camara_EnviaBytesI2C(0x350d ,0x58);
Camara_EnviaBytesI2C(0x3801 ,0x8a);
Camara_EnviaBytesI2C(0x3803 ,0x0a);
Camara_EnviaBytesI2C(0x3804 ,0x0a);
Camara_EnviaBytesI2C(0x3805 ,0x20);
Camara_EnviaBytesI2C(0x3806 ,0x07);
Camara_EnviaBytesI2C(0x3807 ,0x98);
Camara_EnviaBytesI2C(0x3808 ,0x0a);
Camara_EnviaBytesI2C(0x3809 ,0x20);
Camara_EnviaBytesI2C(0x380a ,0x07);
Camara_EnviaBytesI2C(0x380b ,0x98);
Camara_EnviaBytesI2C(0x380c ,0x0c);
Camara_EnviaBytesI2C(0x380d ,0x80);
Camara_EnviaBytesI2C(0x380e ,0x07);
Camara_EnviaBytesI2C(0x380f ,0xd0);
Camara_EnviaBytesI2C(0x381c ,0x11);
Camara_EnviaBytesI2C(0x381d ,0xba);
Camara_EnviaBytesI2C(0x381e ,0x04);
Camara_EnviaBytesI2C(0x381f ,0x48);
Camara_EnviaBytesI2C(0x3820 ,0x04);
Camara_EnviaBytesI2C(0x3821 ,0x18);
Camara_EnviaBytesI2C(0x3a08 ,0x14);
Camara_EnviaBytesI2C(0x3a09 ,0xe0);
Camara_EnviaBytesI2C(0x3a0a ,0x11);
Camara_EnviaBytesI2C(0x3a0b ,0x60);
Camara_EnviaBytesI2C(0x3a0d ,0x04);
Camara_EnviaBytesI2C(0x3a0e ,0x03);
Camara_EnviaBytesI2C(0x5680, 0x00);
Camara_EnviaBytesI2C(0x5681 ,0x00);
Camara_EnviaBytesI2C(0x5682 ,0x0a);
Camara_EnviaBytesI2C(0x5683 ,0x20);
Camara_EnviaBytesI2C(0x5684 ,0x00);
Camara_EnviaBytesI2C(0x5685 ,0x00);
Camara_EnviaBytesI2C(0x5686 ,0x07);
Camara_EnviaBytesI2C(0x5687 ,0x98);
Camara_EnviaBytesI2C(0x5001 ,0x7f);
Camara_EnviaBytesI2C(0x3503 ,0x00);
Camara_EnviaBytesI2C(0x3010 ,0x10);
Camara_EnviaBytesI2C(0x5001 ,0xFF);
Camara_EnviaBytesI2C(0x5583 ,0x50);
Camara_EnviaBytesI2C(0x5584 ,0x50);
Camara_EnviaBytesI2C(0x5580 ,0x02);
Camara_EnviaBytesI2C(0x3c01 ,0x80);
Camara_EnviaBytesI2C(0x3c00 ,0x04);
//;LENS
Camara_EnviaBytesI2C(0x5800 ,0x48);
Camara_EnviaBytesI2C(0x5801 ,0x31);
Camara_EnviaBytesI2C(0x5802 ,0x21);
//112 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x5803 ,0x1b);
Camara_EnviaBytesI2C(0x5804 ,0x1a);
Camara_EnviaBytesI2C(0x5805 ,0x1e);
Camara_EnviaBytesI2C(0x5806 ,0x29);
Camara_EnviaBytesI2C(0x5807 ,0x38);
Camara_EnviaBytesI2C(0x5808 ,0x26);
Camara_EnviaBytesI2C(0x5809 ,0x17);
Camara_EnviaBytesI2C(0x580a ,0x11);
Camara_EnviaBytesI2C(0x580b ,0xe );
Camara_EnviaBytesI2C(0x580c ,0xd );
Camara_EnviaBytesI2C(0x580d ,0xe );
Camara_EnviaBytesI2C(0x580e ,0x13);
Camara_EnviaBytesI2C(0x580f ,0x1a);
Camara_EnviaBytesI2C(0x5810 ,0x15);
Camara_EnviaBytesI2C(0x5811 ,0xd );
Camara_EnviaBytesI2C(0x5812 ,0x8 );
Camara_EnviaBytesI2C(0x5813 ,0x5 );
Camara_EnviaBytesI2C(0x5814 ,0x4 );
Camara_EnviaBytesI2C(0x5815 ,0x5 );
Camara_EnviaBytesI2C(0x5816 ,0x9 );
Camara_EnviaBytesI2C(0x5817 ,0xd );
Camara_EnviaBytesI2C(0x5818 ,0x11);
Camara_EnviaBytesI2C(0x5819 ,0xa );
Camara_EnviaBytesI2C(0x581a ,0x4 );
Camara_EnviaBytesI2C(0x581b ,0x0 );
Camara_EnviaBytesI2C(0x581c ,0x0 );
Camara_EnviaBytesI2C(0x581d ,0x1 );
Camara_EnviaBytesI2C(0x581e ,0x6 );
Camara_EnviaBytesI2C(0x581f ,0x9 );
Camara_EnviaBytesI2C(0x5820 ,0x12);
Camara_EnviaBytesI2C(0x5821 ,0xb );
Camara_EnviaBytesI2C(0x5822 ,0x4 );
Camara_EnviaBytesI2C(0x5823 ,0x0 );
Camara_EnviaBytesI2C(0x5824 ,0x0 );
Camara_EnviaBytesI2C(0x5825 ,0x1 );
Camara_EnviaBytesI2C(0x5826 ,0x6 );
Camara_EnviaBytesI2C(0x5827 ,0xa );
Camara_EnviaBytesI2C(0x5828 ,0x17);
Camara_EnviaBytesI2C(0x5829 ,0xf );
Camara_EnviaBytesI2C(0x582a ,0x9 );
Camara_EnviaBytesI2C(0x582b ,0x6 );
Camara_EnviaBytesI2C(0x582c ,0x5 );
Camara_EnviaBytesI2C(0x582d ,0x6 );
Camara_EnviaBytesI2C(0x582e ,0xa );
Camara_EnviaBytesI2C(0x582f ,0xe );
Camara_EnviaBytesI2C(0x5830 ,0x28);
Camara_EnviaBytesI2C(0x5831 ,0x1a);
Camara_EnviaBytesI2C(0x5832 ,0x11);
//113 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x5833 ,0xe );
Camara_EnviaBytesI2C(0x5834 ,0xe );
Camara_EnviaBytesI2C(0x5835 ,0xf );
Camara_EnviaBytesI2C(0x5836 ,0x15);
Camara_EnviaBytesI2C(0x5837 ,0x1d);
Camara_EnviaBytesI2C(0x5838 ,0x6e);
Camara_EnviaBytesI2C(0x5839 ,0x39);
Camara_EnviaBytesI2C(0x583a ,0x27);
Camara_EnviaBytesI2C(0x583b ,0x1f);
Camara_EnviaBytesI2C(0x583c ,0x1e);
Camara_EnviaBytesI2C(0x583d ,0x23);
Camara_EnviaBytesI2C(0x583e ,0x2f);
Camara_EnviaBytesI2C(0x583f ,0x41);
Camara_EnviaBytesI2C(0x5840 ,0xe );
Camara_EnviaBytesI2C(0x5841 ,0xc );
Camara_EnviaBytesI2C(0x5842 ,0xd );
Camara_EnviaBytesI2C(0x5843 ,0xc );
Camara_EnviaBytesI2C(0x5844 ,0xc );
Camara_EnviaBytesI2C(0x5845 ,0xc );
Camara_EnviaBytesI2C(0x5846 ,0xc );
Camara_EnviaBytesI2C(0x5847 ,0xc );
Camara_EnviaBytesI2C(0x5848 ,0xd );
Camara_EnviaBytesI2C(0x5849 ,0xe );
Camara_EnviaBytesI2C(0x584a ,0xe );
Camara_EnviaBytesI2C(0x584b ,0xa );
Camara_EnviaBytesI2C(0x584c ,0xe );
Camara_EnviaBytesI2C(0x584d ,0xe );
Camara_EnviaBytesI2C(0x584e ,0x10);
Camara_EnviaBytesI2C(0x584f ,0x10);
Camara_EnviaBytesI2C(0x5850 ,0x11);
Camara_EnviaBytesI2C(0x5851 ,0xa );
Camara_EnviaBytesI2C(0x5852 ,0xf );
Camara_EnviaBytesI2C(0x5853 ,0xe );
Camara_EnviaBytesI2C(0x5854 ,0x10);
Camara_EnviaBytesI2C(0x5855 ,0x10);
Camara_EnviaBytesI2C(0x5856 ,0x10);
Camara_EnviaBytesI2C(0x5857 ,0xa );
Camara_EnviaBytesI2C(0x5858 ,0xe );
Camara_EnviaBytesI2C(0x5859 ,0xe );
Camara_EnviaBytesI2C(0x585a ,0xf );
Camara_EnviaBytesI2C(0x585b ,0xf );
Camara_EnviaBytesI2C(0x585c ,0xf );
Camara_EnviaBytesI2C(0x585d ,0xa );
Camara_EnviaBytesI2C(0x585e ,0x9 );
Camara_EnviaBytesI2C(0x585f ,0xd );
Camara_EnviaBytesI2C(0x5860 ,0xc );
Camara_EnviaBytesI2C(0x5861 ,0xb );
Camara_EnviaBytesI2C(0x5862 ,0xd );
//114 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x5863 ,0x7 );
Camara_EnviaBytesI2C(0x5864 ,0x17);
Camara_EnviaBytesI2C(0x5865 ,0x14);
Camara_EnviaBytesI2C(0x5866 ,0x18);
Camara_EnviaBytesI2C(0x5867 ,0x18);
Camara_EnviaBytesI2C(0x5868 ,0x16);
Camara_EnviaBytesI2C(0x5869 ,0x12);
Camara_EnviaBytesI2C(0x586a ,0x1b);
Camara_EnviaBytesI2C(0x586b ,0x1a);
Camara_EnviaBytesI2C(0x586c ,0x16);
Camara_EnviaBytesI2C(0x586d ,0x16);
Camara_EnviaBytesI2C(0x586e ,0x18);
Camara_EnviaBytesI2C(0x586f ,0x1f);
Camara_EnviaBytesI2C(0x5870 ,0x1c);
Camara_EnviaBytesI2C(0x5871 ,0x16);
Camara_EnviaBytesI2C(0x5872 ,0x10);
Camara_EnviaBytesI2C(0x5873 ,0xf );
Camara_EnviaBytesI2C(0x5874 ,0x13);
Camara_EnviaBytesI2C(0x5875 ,0x1c);
Camara_EnviaBytesI2C(0x5876 ,0x1e);
Camara_EnviaBytesI2C(0x5877 ,0x17);
Camara_EnviaBytesI2C(0x5878 ,0x11);
Camara_EnviaBytesI2C(0x5879 ,0x11);
Camara_EnviaBytesI2C(0x587a ,0x14);
Camara_EnviaBytesI2C(0x587b ,0x1e);
Camara_EnviaBytesI2C(0x587c ,0x1c);
Camara_EnviaBytesI2C(0x587d ,0x1c);
Camara_EnviaBytesI2C(0x587e ,0x1a);
Camara_EnviaBytesI2C(0x587f ,0x1a);
Camara_EnviaBytesI2C(0x5880 ,0x1b);
Camara_EnviaBytesI2C(0x5881 ,0x1f);
Camara_EnviaBytesI2C(0x5882 ,0x14);
Camara_EnviaBytesI2C(0x5883 ,0x1a);
Camara_EnviaBytesI2C(0x5884 ,0x1d);
Camara_EnviaBytesI2C(0x5885 ,0x1e);
Camara_EnviaBytesI2C(0x5886 ,0x1a);
Camara_EnviaBytesI2C(0x5887 ,0x1a);
//;
//;AWB
Camara_EnviaBytesI2C(0x5180 ,0xff);
Camara_EnviaBytesI2C(0x5181 ,0x52);
Camara_EnviaBytesI2C(0x5182 ,0x11);
Camara_EnviaBytesI2C(0x5183 ,0x14);
Camara_EnviaBytesI2C(0x5184 ,0x25);
Camara_EnviaBytesI2C(0x5185 ,0x24);
Camara_EnviaBytesI2C(0x5186 ,0x14);
Camara_EnviaBytesI2C(0x5187 ,0x14);
Camara_EnviaBytesI2C(0x5188 ,0x14);
//115 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x5189 ,0x69);
Camara_EnviaBytesI2C(0x518a ,0x60);
Camara_EnviaBytesI2C(0x518b ,0xa2);
Camara_EnviaBytesI2C(0x518c ,0x9c);
Camara_EnviaBytesI2C(0x518d ,0x36);
Camara_EnviaBytesI2C(0x518e ,0x34);
Camara_EnviaBytesI2C(0x518f ,0x54);
Camara_EnviaBytesI2C(0x5190 ,0x4c);
Camara_EnviaBytesI2C(0x5191 ,0xf8);
Camara_EnviaBytesI2C(0x5192 ,0x04);
Camara_EnviaBytesI2C(0x5193 ,0x70);
Camara_EnviaBytesI2C(0x5194 ,0xf0);
Camara_EnviaBytesI2C(0x5195 ,0xf0);
Camara_EnviaBytesI2C(0x5196 ,0x03);
Camara_EnviaBytesI2C(0x5197 ,0x01);
Camara_EnviaBytesI2C(0x5198 ,0x05);
Camara_EnviaBytesI2C(0x5199 ,0x2f);
Camara_EnviaBytesI2C(0x519a ,0x04);
Camara_EnviaBytesI2C(0x519b ,0x00);
Camara_EnviaBytesI2C(0x519c ,0x06);
Camara_EnviaBytesI2C(0x519d ,0xa0);
Camara_EnviaBytesI2C(0x519e ,0xa0);
//;
//;D/S
Camara_EnviaBytesI2C(0x528a ,0x00);
Camara_EnviaBytesI2C(0x528b ,0x01);
Camara_EnviaBytesI2C(0x528c ,0x04);
Camara_EnviaBytesI2C(0x528d ,0x08);
Camara_EnviaBytesI2C(0x528e ,0x10);
Camara_EnviaBytesI2C(0x528f ,0x20);
Camara_EnviaBytesI2C(0x5290 ,0x30);
Camara_EnviaBytesI2C(0x5292 ,0x00);
Camara_EnviaBytesI2C(0x5293 ,0x00);
Camara_EnviaBytesI2C(0x5294 ,0x00);
Camara_EnviaBytesI2C(0x5295 ,0x01);
Camara_EnviaBytesI2C(0x5296 ,0x00);
Camara_EnviaBytesI2C(0x5297 ,0x04);
Camara_EnviaBytesI2C(0x5298 ,0x00);
Camara_EnviaBytesI2C(0x5299 ,0x08);
Camara_EnviaBytesI2C(0x529a ,0x00);
Camara_EnviaBytesI2C(0x529b ,0x10);
Camara_EnviaBytesI2C(0x529c ,0x00);
Camara_EnviaBytesI2C(0x529d ,0x20);
Camara_EnviaBytesI2C(0x529e ,0x00);
Camara_EnviaBytesI2C(0x529f ,0x30);
Camara_EnviaBytesI2C(0x5282 ,0x00);
Camara_EnviaBytesI2C(0x5300 ,0x00);
Camara_EnviaBytesI2C(0x5301 ,0x20);
//116 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x5302 ,0x00);
Camara_EnviaBytesI2C(0x5303 ,0x7c);
Camara_EnviaBytesI2C(0x530c ,0x00);
Camara_EnviaBytesI2C(0x530d ,0x10);
Camara_EnviaBytesI2C(0x530e ,0x20);
Camara_EnviaBytesI2C(0x530f ,0x80);
Camara_EnviaBytesI2C(0x5310 ,0x20);
Camara_EnviaBytesI2C(0x5311 ,0x80);
Camara_EnviaBytesI2C(0x5308 ,0x20);
Camara_EnviaBytesI2C(0x5309 ,0x40);
Camara_EnviaBytesI2C(0x5304 ,0x00);
Camara_EnviaBytesI2C(0x5305 ,0x30);
Camara_EnviaBytesI2C(0x5306 ,0x00);
Camara_EnviaBytesI2C(0x5307 ,0x80);
Camara_EnviaBytesI2C(0x5314 ,0x08);
Camara_EnviaBytesI2C(0x5315 ,0x20);
Camara_EnviaBytesI2C(0x5319 ,0x30);
Camara_EnviaBytesI2C(0x5316 ,0x10);
Camara_EnviaBytesI2C(0x5317 ,0x00);
Camara_EnviaBytesI2C(0x5318 ,0x02);
//;
//;CMX
Camara_EnviaBytesI2C(0x5380 ,0x01);
Camara_EnviaBytesI2C(0x5381 ,0x00);
Camara_EnviaBytesI2C(0x5382 ,0x00);
Camara_EnviaBytesI2C(0x5383 ,0x1f);
Camara_EnviaBytesI2C(0x5384 ,0x00);
Camara_EnviaBytesI2C(0x5385 ,0x06);
Camara_EnviaBytesI2C(0x5386 ,0x00);
Camara_EnviaBytesI2C(0x5387 ,0x00);
Camara_EnviaBytesI2C(0x5388 ,0x00);
Camara_EnviaBytesI2C(0x5389 ,0xE1);
Camara_EnviaBytesI2C(0x538A ,0x00);
Camara_EnviaBytesI2C(0x538B ,0x2B);
Camara_EnviaBytesI2C(0x538C ,0x00);
Camara_EnviaBytesI2C(0x538D ,0x00);
Camara_EnviaBytesI2C(0x538E ,0x00);
Camara_EnviaBytesI2C(0x538F ,0x10);
Camara_EnviaBytesI2C(0x5390 ,0x00);
Camara_EnviaBytesI2C(0x5391 ,0xB3);
Camara_EnviaBytesI2C(0x5392 ,0x00);
Camara_EnviaBytesI2C(0x5393 ,0xA6);
Camara_EnviaBytesI2C(0x5394 ,0x08);
//;
//;GAMMA
Camara_EnviaBytesI2C(0x5480 ,0x0c);
Camara_EnviaBytesI2C(0x5481 ,0x18);
Camara_EnviaBytesI2C(0x5482 ,0x2f);
//117 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x5483 ,0x55);
Camara_EnviaBytesI2C(0x5484 ,0x64);
Camara_EnviaBytesI2C(0x5485 ,0x71);
Camara_EnviaBytesI2C(0x5486 ,0x7d);
Camara_EnviaBytesI2C(0x5487 ,0x87);
Camara_EnviaBytesI2C(0x5488 ,0x91);
Camara_EnviaBytesI2C(0x5489 ,0x9a);
Camara_EnviaBytesI2C(0x548A ,0xaa);
Camara_EnviaBytesI2C(0x548B ,0xb8);
Camara_EnviaBytesI2C(0x548C ,0xcd);
Camara_EnviaBytesI2C(0x548D ,0xdd);
Camara_EnviaBytesI2C(0x548E ,0xea);
Camara_EnviaBytesI2C(0x548F ,0x1d);
Camara_EnviaBytesI2C(0x5490 ,0x05);
Camara_EnviaBytesI2C(0x5491 ,0x00);
Camara_EnviaBytesI2C(0x5492 ,0x04);
Camara_EnviaBytesI2C(0x5493 ,0x20);
Camara_EnviaBytesI2C(0x5494 ,0x03);
Camara_EnviaBytesI2C(0x5495 ,0x60);
Camara_EnviaBytesI2C(0x5496 ,0x02);
Camara_EnviaBytesI2C(0x5497 ,0xB8);
Camara_EnviaBytesI2C(0x5498 ,0x02);
Camara_EnviaBytesI2C(0x5499 ,0x86);
Camara_EnviaBytesI2C(0x549A ,0x02);
Camara_EnviaBytesI2C(0x549B ,0x5B);
Camara_EnviaBytesI2C(0x549C ,0x02);
Camara_EnviaBytesI2C(0x549D ,0x3B);
Camara_EnviaBytesI2C(0x549E ,0x02);
Camara_EnviaBytesI2C(0x549F ,0x1C);
Camara_EnviaBytesI2C(0x54A0 ,0x02);
Camara_EnviaBytesI2C(0x54A1 ,0x04);
Camara_EnviaBytesI2C(0x54A2 ,0x01);
Camara_EnviaBytesI2C(0x54A3 ,0xED);
Camara_EnviaBytesI2C(0x54A4 ,0x01);
Camara_EnviaBytesI2C(0x54A5 ,0xC5);
Camara_EnviaBytesI2C(0x54A6 ,0x01);
Camara_EnviaBytesI2C(0x54A7 ,0xA5);
Camara_EnviaBytesI2C(0x54A8 ,0x01);
Camara_EnviaBytesI2C(0x54A9 ,0x6C);
Camara_EnviaBytesI2C(0x54AA ,0x01);
Camara_EnviaBytesI2C(0x54AB ,0x41);
Camara_EnviaBytesI2C(0x54AC ,0x01);
Camara_EnviaBytesI2C(0x54AD ,0x20);
Camara_EnviaBytesI2C(0x54AE ,0x00);
Camara_EnviaBytesI2C(0x54AF ,0x16);
Camara_EnviaBytesI2C(0x54B0 ,0x01);
Camara_EnviaBytesI2C(0x54B1 ,0x20);
Camara_EnviaBytesI2C(0x54B2 ,0x00);
//118 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x54B3 ,0x10);
Camara_EnviaBytesI2C(0x54B4 ,0x00);
Camara_EnviaBytesI2C(0x54B5 ,0xf0);
Camara_EnviaBytesI2C(0x54B6 ,0x00);
Camara_EnviaBytesI2C(0x54B7 ,0xDF);
//;
Camara_EnviaBytesI2C(0x5402 ,0x3f);
Camara_EnviaBytesI2C(0x5403 ,0x00);
//;
//;UV ADJUST
Camara_EnviaBytesI2C(0x5500 ,0x10);
Camara_EnviaBytesI2C(0x5502 ,0x00);
Camara_EnviaBytesI2C(0x5503 ,0x06);
Camara_EnviaBytesI2C(0x5504 ,0x00);
Camara_EnviaBytesI2C(0x5505 ,0x7f);
//;AE
Camara_EnviaBytesI2C(0x5025 ,0x80);
Camara_EnviaBytesI2C(0x3a0f ,0x30);
Camara_EnviaBytesI2C(0x3a10 ,0x28);
Camara_EnviaBytesI2C(0x3a1b ,0x30);
Camara_EnviaBytesI2C(0x3a1e ,0x28);
Camara_EnviaBytesI2C(0x3a11 ,0x61);
Camara_EnviaBytesI2C(0x3a1f ,0x10);
Camara_EnviaBytesI2C(0x5688 ,0xff);
Camara_EnviaBytesI2C(0x5689 ,0xff);
Camara_EnviaBytesI2C(0x568a ,0xff);
Camara_EnviaBytesI2C(0x568b ,0xff);
Camara_EnviaBytesI2C(0x568c ,0xff);
Camara_EnviaBytesI2C(0x568d ,0xff);
Camara_EnviaBytesI2C(0x568e ,0xff);
Camara_EnviaBytesI2C(0x568f ,0xff);

}


void DelFabricante(void) {
    
Camara_EnviaBytesI2C(0x3103,0x93);
Camara_EnviaBytesI2C(0x3008,0x82);
Camara_EnviaBytesI2C(0x3017,0x7f);
Camara_EnviaBytesI2C(0x3018,0xfc);
Camara_EnviaBytesI2C(0x3810,0xc2);
Camara_EnviaBytesI2C(0x3615,0xf0);
Camara_EnviaBytesI2C(0x3000,0x00);
Camara_EnviaBytesI2C(0x3001,0x00);
Camara_EnviaBytesI2C(0x3002,0x00);
Camara_EnviaBytesI2C(0x3003,0x00);
Camara_EnviaBytesI2C(0x3004,0xff);
Camara_EnviaBytesI2C(0x3030,0x2b);
Camara_EnviaBytesI2C(0x3011,0x08);
Camara_EnviaBytesI2C(0x3010,0x10);
Camara_EnviaBytesI2C(0x3604,0x60);
Camara_EnviaBytesI2C(0x3622,0x60);
Camara_EnviaBytesI2C(0x3621,0x09);
Camara_EnviaBytesI2C(0x3705,0xd9);
Camara_EnviaBytesI2C(0x3709,0x00);
Camara_EnviaBytesI2C(0x4000,0x21);
Camara_EnviaBytesI2C(0x401d,0x22);
Camara_EnviaBytesI2C(0x3600,0x54);
Camara_EnviaBytesI2C(0x3605,0x04);
Camara_EnviaBytesI2C(0x3606,0x3f);
Camara_EnviaBytesI2C(0x3c01,0x80);
Camara_EnviaBytesI2C(0x300d,0x22);
Camara_EnviaBytesI2C(0x3623,0x22);
Camara_EnviaBytesI2C(0x5000,0x4f);
Camara_EnviaBytesI2C(0x5020,0x04);
Camara_EnviaBytesI2C(0x5181,0x79);
Camara_EnviaBytesI2C(0x5182,0x00);
Camara_EnviaBytesI2C(0x5185,0x22);
Camara_EnviaBytesI2C(0x5197,0x01);
Camara_EnviaBytesI2C(0x5500,0x0a);
Camara_EnviaBytesI2C(0x5504,0x00);
Camara_EnviaBytesI2C(0x5505,0x7f);
Camara_EnviaBytesI2C(0x5080,0x08);
Camara_EnviaBytesI2C(0x300e,0x18);
Camara_EnviaBytesI2C(0x4610,0x00);
Camara_EnviaBytesI2C(0x471d,0x05);
Camara_EnviaBytesI2C(0x4708,0x06);
Camara_EnviaBytesI2C(0x370c,0xa0);
Camara_EnviaBytesI2C(0x3808,0x0a);
Camara_EnviaBytesI2C(0x3809,0x20);
Camara_EnviaBytesI2C(0x380a,0x07);
Camara_EnviaBytesI2C(0x380b,0x98);
Camara_EnviaBytesI2C(0x380c,0x0c);
Camara_EnviaBytesI2C(0x380d,0x80);
Camara_EnviaBytesI2C(0x380e,0x07);
Camara_EnviaBytesI2C(0x380f,0xd0);
Camara_EnviaBytesI2C(0x5687,0x94);
Camara_EnviaBytesI2C(0x501f,0x00);
Camara_EnviaBytesI2C(0x5000,0x4f);
Camara_EnviaBytesI2C(0x5001,0xcf);
Camara_EnviaBytesI2C(0x4300,0x30);
Camara_EnviaBytesI2C(0x4300,0x30);
Camara_EnviaBytesI2C(0x460b,0x35);
Camara_EnviaBytesI2C(0x471d,0x00);
Camara_EnviaBytesI2C(0x3002,0x0c);
Camara_EnviaBytesI2C(0x3002,0x00);
Camara_EnviaBytesI2C(0x4713,0x03);
Camara_EnviaBytesI2C(0x471c,0x50);
Camara_EnviaBytesI2C(0x4721,0x02);
Camara_EnviaBytesI2C(0x4402,0x90);
Camara_EnviaBytesI2C(0x460c,0x22);
Camara_EnviaBytesI2C(0x3815,0x44);
Camara_EnviaBytesI2C(0x3503,0x07);
Camara_EnviaBytesI2C(0x3501,0x73);
Camara_EnviaBytesI2C(0x3502,0x80);
Camara_EnviaBytesI2C(0x350b,0x00);
Camara_EnviaBytesI2C(0x3818,0xc8);
Camara_EnviaBytesI2C(0x3801,0x88);
Camara_EnviaBytesI2C(0x3824,0x11);
Camara_EnviaBytesI2C(0x3a00,0x78);
Camara_EnviaBytesI2C(0x3a1a,0x04);
Camara_EnviaBytesI2C(0x3a13,0x30);
Camara_EnviaBytesI2C(0x3a18,0x00);
Camara_EnviaBytesI2C(0x3a19,0x7c);
Camara_EnviaBytesI2C(0x3a08,0x12);
Camara_EnviaBytesI2C(0x3a09,0xc0);
Camara_EnviaBytesI2C(0x3a0a,0x0f);
Camara_EnviaBytesI2C(0x3a0b,0xa0);
Camara_EnviaBytesI2C(0x350c,0x07);
Camara_EnviaBytesI2C(0x350d,0xd0);
Camara_EnviaBytesI2C(0x3a0d,0x08);
Camara_EnviaBytesI2C(0x3a0e,0x06);
Camara_EnviaBytesI2C(0x3500,0x00);
Camara_EnviaBytesI2C(0x3501,0x00);
Camara_EnviaBytesI2C(0x3502,0x00);
Camara_EnviaBytesI2C(0x350a,0x00);
Camara_EnviaBytesI2C(0x350b,0x00);
Camara_EnviaBytesI2C(0x3503,0x00);
Camara_EnviaBytesI2C(0x3a0f,0x3c);
Camara_EnviaBytesI2C(0x3a10,0x32);
Camara_EnviaBytesI2C(0x3a1b,0x3c);
Camara_EnviaBytesI2C(0x3a1e,0x32);
Camara_EnviaBytesI2C(0x3a11,0x80);
Camara_EnviaBytesI2C(0x3a1f,0x20);
Camara_EnviaBytesI2C(0x3030,0x0b);
Camara_EnviaBytesI2C(0x3a02,0x00);
Camara_EnviaBytesI2C(0x3a03,0x7d);
Camara_EnviaBytesI2C(0x3a04,0x00);
Camara_EnviaBytesI2C(0x3a14,0x00);
Camara_EnviaBytesI2C(0x3a15,0x7d);
Camara_EnviaBytesI2C(0x3a16,0x00);
Camara_EnviaBytesI2C(0x3a00,0x78);
Camara_EnviaBytesI2C(0x3a08,0x09);
Camara_EnviaBytesI2C(0x3a09,0x60);
Camara_EnviaBytesI2C(0x3a0a,0x07);
Camara_EnviaBytesI2C(0x3a0b,0xd0);
Camara_EnviaBytesI2C(0x3a0d,0x10);
Camara_EnviaBytesI2C(0x3a0e,0x0d);


//write_i2c(0x4407,0x04);
Camara_EnviaBytesI2C(0x4407,0x08);


Camara_EnviaBytesI2C(0x5193,0x70);
Camara_EnviaBytesI2C(0x589b,0x00);
Camara_EnviaBytesI2C(0x589a,0xc0);
Camara_EnviaBytesI2C(0x401e,0x20);
Camara_EnviaBytesI2C(0x4001,0x42);
Camara_EnviaBytesI2C(0x4002,0x02);
Camara_EnviaBytesI2C(0x401c,0x06);
Camara_EnviaBytesI2C(0x3825,0xac);
Camara_EnviaBytesI2C(0x3827,0x0c);
Camara_EnviaBytesI2C(0x528a,0x01);
Camara_EnviaBytesI2C(0x528b,0x04);
Camara_EnviaBytesI2C(0x528c,0x08);
Camara_EnviaBytesI2C(0x528d,0x10);
Camara_EnviaBytesI2C(0x528e,0x20);
Camara_EnviaBytesI2C(0x528f,0x28);
Camara_EnviaBytesI2C(0x5290,0x30);
Camara_EnviaBytesI2C(0x5292,0x00);
Camara_EnviaBytesI2C(0x5293,0x01);
Camara_EnviaBytesI2C(0x5294,0x00);
Camara_EnviaBytesI2C(0x5295,0x04);
Camara_EnviaBytesI2C(0x5296,0x00);
Camara_EnviaBytesI2C(0x5297,0x08);
Camara_EnviaBytesI2C(0x5298,0x00);
Camara_EnviaBytesI2C(0x5299,0x10);
Camara_EnviaBytesI2C(0x529a,0x00);
Camara_EnviaBytesI2C(0x529b,0x20);
Camara_EnviaBytesI2C(0x529c,0x00);
Camara_EnviaBytesI2C(0x529d,0x28);
Camara_EnviaBytesI2C(0x529e,0x00);
Camara_EnviaBytesI2C(0x529f,0x30);
Camara_EnviaBytesI2C(0x5282,0x00);
Camara_EnviaBytesI2C(0x5300,0x00);
Camara_EnviaBytesI2C(0x5301,0x20);
Camara_EnviaBytesI2C(0x5302,0x00);
Camara_EnviaBytesI2C(0x5303,0x7c);
Camara_EnviaBytesI2C(0x530c,0x00);
Camara_EnviaBytesI2C(0x530d,0x0c);
Camara_EnviaBytesI2C(0x530e,0x20);
Camara_EnviaBytesI2C(0x530f,0x80);
Camara_EnviaBytesI2C(0x5310,0x20);
Camara_EnviaBytesI2C(0x5311,0x80);
Camara_EnviaBytesI2C(0x5308,0x20);
Camara_EnviaBytesI2C(0x5309,0x40);
Camara_EnviaBytesI2C(0x5304,0x00);
Camara_EnviaBytesI2C(0x5305,0x30);
Camara_EnviaBytesI2C(0x5306,0x00);
Camara_EnviaBytesI2C(0x5307,0x80);
Camara_EnviaBytesI2C(0x5314,0x08);
Camara_EnviaBytesI2C(0x5315,0x20);
Camara_EnviaBytesI2C(0x5319,0x30);
Camara_EnviaBytesI2C(0x5316,0x10);
Camara_EnviaBytesI2C(0x5317,0x00);
Camara_EnviaBytesI2C(0x5318,0x02);
Camara_EnviaBytesI2C(0x5380,0x01);
Camara_EnviaBytesI2C(0x5381,0x00);
Camara_EnviaBytesI2C(0x5382,0x00);
Camara_EnviaBytesI2C(0x5383,0x4e);
Camara_EnviaBytesI2C(0x5384,0x00);
Camara_EnviaBytesI2C(0x5385,0x0f);
Camara_EnviaBytesI2C(0x5386,0x00);
Camara_EnviaBytesI2C(0x5387,0x00);
Camara_EnviaBytesI2C(0x5388,0x01);
Camara_EnviaBytesI2C(0x5389,0x15);
Camara_EnviaBytesI2C(0x538a,0x00);
Camara_EnviaBytesI2C(0x538b,0x31);
Camara_EnviaBytesI2C(0x538c,0x00);
Camara_EnviaBytesI2C(0x538d,0x00);
Camara_EnviaBytesI2C(0x538e,0x00);
Camara_EnviaBytesI2C(0x538f,0x0f);
Camara_EnviaBytesI2C(0x5390,0x00);
Camara_EnviaBytesI2C(0x5391,0xab);
Camara_EnviaBytesI2C(0x5392,0x00);
Camara_EnviaBytesI2C(0x5393,0xa2);
Camara_EnviaBytesI2C(0x5394,0x08);
Camara_EnviaBytesI2C(0x5480,0x14);
Camara_EnviaBytesI2C(0x5481,0x21);
Camara_EnviaBytesI2C(0x5482,0x36);
Camara_EnviaBytesI2C(0x5483,0x57);
Camara_EnviaBytesI2C(0x5484,0x65);
Camara_EnviaBytesI2C(0x5485,0x71);
Camara_EnviaBytesI2C(0x5486,0x7d);
Camara_EnviaBytesI2C(0x5487,0x87);
Camara_EnviaBytesI2C(0x5488,0x91);
Camara_EnviaBytesI2C(0x5489,0x9a);
Camara_EnviaBytesI2C(0x548a,0xaa);
Camara_EnviaBytesI2C(0x548b,0xb8);
Camara_EnviaBytesI2C(0x548c,0xcd);
Camara_EnviaBytesI2C(0x548d,0xdd);
Camara_EnviaBytesI2C(0x548e,0xea);
Camara_EnviaBytesI2C(0x548f,0x1d);
Camara_EnviaBytesI2C(0x5490,0x05);
Camara_EnviaBytesI2C(0x5491,0x00);
Camara_EnviaBytesI2C(0x5492,0x04);
Camara_EnviaBytesI2C(0x5493,0x20);
Camara_EnviaBytesI2C(0x5494,0x03);
Camara_EnviaBytesI2C(0x5495,0x60);
Camara_EnviaBytesI2C(0x5496,0x02);
Camara_EnviaBytesI2C(0x5497,0xb8);
Camara_EnviaBytesI2C(0x5498,0x02);
Camara_EnviaBytesI2C(0x5499,0x86);
Camara_EnviaBytesI2C(0x549a,0x02);
Camara_EnviaBytesI2C(0x549b,0x5b);
Camara_EnviaBytesI2C(0x549c,0x02);
Camara_EnviaBytesI2C(0x549d,0x3b);
Camara_EnviaBytesI2C(0x549e,0x02);
Camara_EnviaBytesI2C(0x549f,0x1c);
Camara_EnviaBytesI2C(0x54a0,0x02);
Camara_EnviaBytesI2C(0x54a1,0x04);
Camara_EnviaBytesI2C(0x54a2,0x01);
Camara_EnviaBytesI2C(0x54a3,0xed);
Camara_EnviaBytesI2C(0x54a4,0x01);
Camara_EnviaBytesI2C(0x54a5,0xc5);
Camara_EnviaBytesI2C(0x54a6,0x01);
Camara_EnviaBytesI2C(0x54a7,0xa5);
Camara_EnviaBytesI2C(0x54a8,0x01);
Camara_EnviaBytesI2C(0x54a9,0x6c);
Camara_EnviaBytesI2C(0x54aa,0x01);
Camara_EnviaBytesI2C(0x54ab,0x41);
Camara_EnviaBytesI2C(0x54ac,0x01);
Camara_EnviaBytesI2C(0x54ad,0x20);
Camara_EnviaBytesI2C(0x54ae,0x00);
Camara_EnviaBytesI2C(0x54af,0x16);
Camara_EnviaBytesI2C(0x54b0,0x01);
Camara_EnviaBytesI2C(0x54b1,0x20);
Camara_EnviaBytesI2C(0x54b2,0x00);
Camara_EnviaBytesI2C(0x54b3,0x10);
Camara_EnviaBytesI2C(0x54b4,0x00);
Camara_EnviaBytesI2C(0x54b5,0xf0);
Camara_EnviaBytesI2C(0x54b6,0x00);
Camara_EnviaBytesI2C(0x54b7,0xdf);
Camara_EnviaBytesI2C(0x5402,0x3f);
Camara_EnviaBytesI2C(0x5403,0x00);
Camara_EnviaBytesI2C(0x3406,0x00);
Camara_EnviaBytesI2C(0x5180,0xff);
Camara_EnviaBytesI2C(0x5181,0x52);
Camara_EnviaBytesI2C(0x5182,0x11);
Camara_EnviaBytesI2C(0x5183,0x14);
Camara_EnviaBytesI2C(0x5184,0x25);
Camara_EnviaBytesI2C(0x5185,0x24);
Camara_EnviaBytesI2C(0x5186,0x06);
Camara_EnviaBytesI2C(0x5187,0x08);
Camara_EnviaBytesI2C(0x5188,0x08);
Camara_EnviaBytesI2C(0x5189,0x7c);
Camara_EnviaBytesI2C(0x518a,0x60);
Camara_EnviaBytesI2C(0x518b,0xb2);
Camara_EnviaBytesI2C(0x518c,0xb2);
Camara_EnviaBytesI2C(0x518d,0x44);
Camara_EnviaBytesI2C(0x518e,0x3d);
Camara_EnviaBytesI2C(0x518f,0x58);
Camara_EnviaBytesI2C(0x5190,0x46);
Camara_EnviaBytesI2C(0x5191,0xf8);
Camara_EnviaBytesI2C(0x5192,0x04);
Camara_EnviaBytesI2C(0x5193,0x70);
Camara_EnviaBytesI2C(0x5194,0xf0);
Camara_EnviaBytesI2C(0x5195,0xf0);
Camara_EnviaBytesI2C(0x5196,0x03);
Camara_EnviaBytesI2C(0x5197,0x01);
Camara_EnviaBytesI2C(0x5198,0x04);
Camara_EnviaBytesI2C(0x5199,0x12);
Camara_EnviaBytesI2C(0x519a,0x04);
Camara_EnviaBytesI2C(0x519b,0x00);
Camara_EnviaBytesI2C(0x519c,0x06);
Camara_EnviaBytesI2C(0x519d,0x82);
Camara_EnviaBytesI2C(0x519e,0x00);
Camara_EnviaBytesI2C(0x5025,0x80);
Camara_EnviaBytesI2C(0x3a0f,0x38);
Camara_EnviaBytesI2C(0x3a10,0x30);
Camara_EnviaBytesI2C(0x3a1b,0x3a);
Camara_EnviaBytesI2C(0x3a1e,0x2e);
Camara_EnviaBytesI2C(0x3a11,0x60);
Camara_EnviaBytesI2C(0x3a1f,0x10);
Camara_EnviaBytesI2C(0x5688,0xa6);
Camara_EnviaBytesI2C(0x5689,0x6a);
Camara_EnviaBytesI2C(0x568a,0xea);
Camara_EnviaBytesI2C(0x568b,0xae);
Camara_EnviaBytesI2C(0x568c,0xa6);
Camara_EnviaBytesI2C(0x568d,0x6a);
Camara_EnviaBytesI2C(0x568e,0x62);
Camara_EnviaBytesI2C(0x568f,0x26);
Camara_EnviaBytesI2C(0x5583,0x40);
Camara_EnviaBytesI2C(0x5584,0x40);
Camara_EnviaBytesI2C(0x5580,0x02);
Camara_EnviaBytesI2C(0x5000,0xcf);
Camara_EnviaBytesI2C(0x5800,0x27);
Camara_EnviaBytesI2C(0x5801,0x19);
Camara_EnviaBytesI2C(0x5802,0x12);
Camara_EnviaBytesI2C(0x5803,0x0f);
Camara_EnviaBytesI2C(0x5804,0x10);
Camara_EnviaBytesI2C(0x5805,0x15);
Camara_EnviaBytesI2C(0x5806,0x1e);
Camara_EnviaBytesI2C(0x5807,0x2f);
Camara_EnviaBytesI2C(0x5808,0x15);
Camara_EnviaBytesI2C(0x5809,0x0d);
Camara_EnviaBytesI2C(0x580a,0x0a);
Camara_EnviaBytesI2C(0x580b,0x09);
Camara_EnviaBytesI2C(0x580c,0x0a);
Camara_EnviaBytesI2C(0x580d,0x0c);
Camara_EnviaBytesI2C(0x580e,0x12);
Camara_EnviaBytesI2C(0x580f,0x19);
Camara_EnviaBytesI2C(0x5810,0x0b);
Camara_EnviaBytesI2C(0x5811,0x07);
Camara_EnviaBytesI2C(0x5812,0x04);
Camara_EnviaBytesI2C(0x5813,0x03);
Camara_EnviaBytesI2C(0x5814,0x03);
Camara_EnviaBytesI2C(0x5815,0x06);
Camara_EnviaBytesI2C(0x5816,0x0a);
Camara_EnviaBytesI2C(0x5817,0x0f);
Camara_EnviaBytesI2C(0x5818,0x0a);
Camara_EnviaBytesI2C(0x5819,0x05);
Camara_EnviaBytesI2C(0x581a,0x01);
Camara_EnviaBytesI2C(0x581b,0x00);
Camara_EnviaBytesI2C(0x581c,0x00);
Camara_EnviaBytesI2C(0x581d,0x03);
Camara_EnviaBytesI2C(0x581e,0x08);
Camara_EnviaBytesI2C(0x581f,0x0c);
Camara_EnviaBytesI2C(0x5820,0x0a);
Camara_EnviaBytesI2C(0x5821,0x05);
Camara_EnviaBytesI2C(0x5822,0x01);
Camara_EnviaBytesI2C(0x5823,0x00);
Camara_EnviaBytesI2C(0x5824,0x00);
Camara_EnviaBytesI2C(0x5825,0x03);
Camara_EnviaBytesI2C(0x5826,0x08);
Camara_EnviaBytesI2C(0x5827,0x0c);
Camara_EnviaBytesI2C(0x5828,0x0e);
Camara_EnviaBytesI2C(0x5829,0x08);
Camara_EnviaBytesI2C(0x582a,0x06);
Camara_EnviaBytesI2C(0x582b,0x04);
Camara_EnviaBytesI2C(0x582c,0x05);
Camara_EnviaBytesI2C(0x582d,0x07);
Camara_EnviaBytesI2C(0x582e,0x0b);
Camara_EnviaBytesI2C(0x582f,0x12);
Camara_EnviaBytesI2C(0x5830,0x18);
Camara_EnviaBytesI2C(0x5831,0x10);
Camara_EnviaBytesI2C(0x5832,0x0c);
Camara_EnviaBytesI2C(0x5833,0x0a);
Camara_EnviaBytesI2C(0x5834,0x0b);
Camara_EnviaBytesI2C(0x5835,0x0e);
Camara_EnviaBytesI2C(0x5836,0x15);
Camara_EnviaBytesI2C(0x5837,0x19);
Camara_EnviaBytesI2C(0x5838,0x32);
Camara_EnviaBytesI2C(0x5839,0x1f);
Camara_EnviaBytesI2C(0x583a,0x18);
Camara_EnviaBytesI2C(0x583b,0x16);
Camara_EnviaBytesI2C(0x583c,0x17);
Camara_EnviaBytesI2C(0x583d,0x1e);
Camara_EnviaBytesI2C(0x583e,0x26);
Camara_EnviaBytesI2C(0x583f,0x53);
Camara_EnviaBytesI2C(0x5840,0x10);
Camara_EnviaBytesI2C(0x5841,0x0f);
Camara_EnviaBytesI2C(0x5842,0x0d);
Camara_EnviaBytesI2C(0x5843,0x0c);
Camara_EnviaBytesI2C(0x5844,0x0e);
Camara_EnviaBytesI2C(0x5845,0x09);
Camara_EnviaBytesI2C(0x5846,0x11);
Camara_EnviaBytesI2C(0x5847,0x10);
Camara_EnviaBytesI2C(0x5848,0x10);
Camara_EnviaBytesI2C(0x5849,0x10);
Camara_EnviaBytesI2C(0x584a,0x10);
Camara_EnviaBytesI2C(0x584b,0x0e);
Camara_EnviaBytesI2C(0x584c,0x10);
Camara_EnviaBytesI2C(0x584d,0x10);
Camara_EnviaBytesI2C(0x584e,0x11);
Camara_EnviaBytesI2C(0x584f,0x10);
Camara_EnviaBytesI2C(0x5850,0x0f);
Camara_EnviaBytesI2C(0x5851,0x0c);
Camara_EnviaBytesI2C(0x5852,0x0f);
Camara_EnviaBytesI2C(0x5853,0x10);
Camara_EnviaBytesI2C(0x5854,0x10);
Camara_EnviaBytesI2C(0x5855,0x0f);
Camara_EnviaBytesI2C(0x5856,0x0e);
Camara_EnviaBytesI2C(0x5857,0x0b);
Camara_EnviaBytesI2C(0x5858,0x10);
Camara_EnviaBytesI2C(0x5859,0x0d);
Camara_EnviaBytesI2C(0x585a,0x0d);
Camara_EnviaBytesI2C(0x585b,0x0c);
Camara_EnviaBytesI2C(0x585c,0x0c);
Camara_EnviaBytesI2C(0x585d,0x0c);
Camara_EnviaBytesI2C(0x585e,0x0b);
Camara_EnviaBytesI2C(0x585f,0x0c);
Camara_EnviaBytesI2C(0x5860,0x0c);
Camara_EnviaBytesI2C(0x5861,0x0c);
Camara_EnviaBytesI2C(0x5862,0x0d);
Camara_EnviaBytesI2C(0x5863,0x08);
Camara_EnviaBytesI2C(0x5864,0x11);
Camara_EnviaBytesI2C(0x5865,0x18);
Camara_EnviaBytesI2C(0x5866,0x18);
Camara_EnviaBytesI2C(0x5867,0x19);
Camara_EnviaBytesI2C(0x5868,0x17);
Camara_EnviaBytesI2C(0x5869,0x19);
Camara_EnviaBytesI2C(0x586a,0x16);
Camara_EnviaBytesI2C(0x586b,0x13);
Camara_EnviaBytesI2C(0x586c,0x13);
Camara_EnviaBytesI2C(0x586d,0x12);
Camara_EnviaBytesI2C(0x586e,0x13);
Camara_EnviaBytesI2C(0x586f,0x16);
Camara_EnviaBytesI2C(0x5870,0x14);
Camara_EnviaBytesI2C(0x5871,0x12);
Camara_EnviaBytesI2C(0x5872,0x10);
Camara_EnviaBytesI2C(0x5873,0x11);
Camara_EnviaBytesI2C(0x5874,0x11);
Camara_EnviaBytesI2C(0x5875,0x16);
Camara_EnviaBytesI2C(0x5876,0x14);
Camara_EnviaBytesI2C(0x5877,0x11);
Camara_EnviaBytesI2C(0x5878,0x10);
Camara_EnviaBytesI2C(0x5879,0x0f);
Camara_EnviaBytesI2C(0x587a,0x10);
Camara_EnviaBytesI2C(0x587b,0x14);
Camara_EnviaBytesI2C(0x587c,0x13);
Camara_EnviaBytesI2C(0x587d,0x12);
Camara_EnviaBytesI2C(0x587e,0x11);
Camara_EnviaBytesI2C(0x587f,0x11);
Camara_EnviaBytesI2C(0x5880,0x12);
Camara_EnviaBytesI2C(0x5881,0x15);
Camara_EnviaBytesI2C(0x5882,0x14);
Camara_EnviaBytesI2C(0x5883,0x15);
Camara_EnviaBytesI2C(0x5884,0x15);
Camara_EnviaBytesI2C(0x5885,0x15);
Camara_EnviaBytesI2C(0x5886,0x13);
Camara_EnviaBytesI2C(0x5887,0x17);
Camara_EnviaBytesI2C(0x3710,0x10);
Camara_EnviaBytesI2C(0x3632,0x51);
Camara_EnviaBytesI2C(0x3702,0x10);
Camara_EnviaBytesI2C(0x3703,0xb2);
Camara_EnviaBytesI2C(0x3704,0x18);
Camara_EnviaBytesI2C(0x370b,0x40);
Camara_EnviaBytesI2C(0x370d,0x03);
Camara_EnviaBytesI2C(0x3631,0x01);
Camara_EnviaBytesI2C(0x3632,0x52);
Camara_EnviaBytesI2C(0x3606,0x24);
Camara_EnviaBytesI2C(0x3620,0x96);
Camara_EnviaBytesI2C(0x5785,0x07);
Camara_EnviaBytesI2C(0x3a13,0x30);
Camara_EnviaBytesI2C(0x3600,0x52);
Camara_EnviaBytesI2C(0x3604,0x48);
Camara_EnviaBytesI2C(0x3606,0x1b);
Camara_EnviaBytesI2C(0x370d,0x0b);
Camara_EnviaBytesI2C(0x370f,0xc0);
Camara_EnviaBytesI2C(0x3709,0x01);
Camara_EnviaBytesI2C(0x3823,0x00);
Camara_EnviaBytesI2C(0x5007,0x00);
Camara_EnviaBytesI2C(0x5009,0x00);
Camara_EnviaBytesI2C(0x5011,0x00);
Camara_EnviaBytesI2C(0x5013,0x00);
Camara_EnviaBytesI2C(0x519e,0x00);
Camara_EnviaBytesI2C(0x5086,0x00);
Camara_EnviaBytesI2C(0x5087,0x00);
Camara_EnviaBytesI2C(0x5088,0x00);
Camara_EnviaBytesI2C(0x5089,0x00);
Camara_EnviaBytesI2C(0x302b,0x00);
Camara_EnviaBytesI2C(0x4300,0x30);
Camara_EnviaBytesI2C(0x460b,0x35);
Camara_EnviaBytesI2C(0x3002,0x0c);
Camara_EnviaBytesI2C(0x3002,0x00);
Camara_EnviaBytesI2C(0x4713,0x04);
Camara_EnviaBytesI2C(0x4600,0x80);
Camara_EnviaBytesI2C(0x4721,0x02);
Camara_EnviaBytesI2C(0x471c,0x50);
Camara_EnviaBytesI2C(0x4408,0x00);
Camara_EnviaBytesI2C(0x460c,0x22);
Camara_EnviaBytesI2C(0x3815,0x44);
Camara_EnviaBytesI2C(0x4402,0x90);
Camara_EnviaBytesI2C(0x4602,0x05);
//HSize 
Camara_EnviaBytesI2C(0x4603,0x00);
Camara_EnviaBytesI2C(0x4604,0x06);
Camara_EnviaBytesI2C(0x4605,0x00);
    
    
}


void Camara_Set_HighResVideo_V2(void) {
//13. 4 High Resolution Video
//13.4.1 1080 P
Camara_EnviaBytesI2C(0x3103 ,0x93);
Camara_EnviaBytesI2C(0x3008 ,0x82);
Camara_EnviaBytesI2C(0x3017 ,0x7f);
Camara_EnviaBytesI2C(0x3018 ,0xfc);
Camara_EnviaBytesI2C(0x3810 ,0xc2);
Camara_EnviaBytesI2C(0x3615 ,0xf0);
Camara_EnviaBytesI2C(0x3000 ,0x00);
Camara_EnviaBytesI2C(0x3001 ,0x00);
Camara_EnviaBytesI2C(0x3002 ,0x00);
Camara_EnviaBytesI2C(0x3003 ,0x00);
Camara_EnviaBytesI2C(0x3004 ,0xff);
Camara_EnviaBytesI2C(0x3030 ,0x2b);
Camara_EnviaBytesI2C(0x3011 ,0x08);
Camara_EnviaBytesI2C(0x3010 ,0x10);
Camara_EnviaBytesI2C(0x3604 ,0x60);
Camara_EnviaBytesI2C(0x3622 ,0x60);
Camara_EnviaBytesI2C(0x3621 ,0x09);
Camara_EnviaBytesI2C(0x3709 ,0x00);
Camara_EnviaBytesI2C(0x4000 ,0x21);
Camara_EnviaBytesI2C(0x401d ,0x22);
Camara_EnviaBytesI2C(0x3600 ,0x54);
Camara_EnviaBytesI2C(0x3605 ,0x04);
Camara_EnviaBytesI2C(0x3606 ,0x3f);
Camara_EnviaBytesI2C(0x3c01 ,0x80);
Camara_EnviaBytesI2C(0x300d ,0x22);
Camara_EnviaBytesI2C(0x3623 ,0x22);
Camara_EnviaBytesI2C(0x5000 ,0x4f);
Camara_EnviaBytesI2C(0x5020 ,0x04);
Camara_EnviaBytesI2C(0x5181 ,0x79);
Camara_EnviaBytesI2C(0x5182 ,0x00);
Camara_EnviaBytesI2C(0x5185 ,0x22);
Camara_EnviaBytesI2C(0x5197 ,0x01);
Camara_EnviaBytesI2C(0x5500 ,0x0a);
Camara_EnviaBytesI2C(0x5504 ,0x00);
Camara_EnviaBytesI2C(0x5505 ,0x7f);
Camara_EnviaBytesI2C(0x5080 ,0x08);
Camara_EnviaBytesI2C(0x300e ,0x18);
Camara_EnviaBytesI2C(0x4610 ,0x00);
Camara_EnviaBytesI2C(0x471d ,0x05);
Camara_EnviaBytesI2C(0x4708 ,0x06);
Camara_EnviaBytesI2C(0x370c ,0xa0);
//108 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x3808 ,0x0a);
Camara_EnviaBytesI2C(0x3809 ,0x20);
Camara_EnviaBytesI2C(0x380a ,0x07);
Camara_EnviaBytesI2C(0x380b ,0x98);
Camara_EnviaBytesI2C(0x380c ,0x0c);
Camara_EnviaBytesI2C(0x380d ,0x80);
Camara_EnviaBytesI2C(0x380e ,0x07);
Camara_EnviaBytesI2C(0x380f ,0xd0);
Camara_EnviaBytesI2C(0x5687 ,0x98);
Camara_EnviaBytesI2C(0x501f ,0x00);
Camara_EnviaBytesI2C(0x5000 ,0x4f);
Camara_EnviaBytesI2C(0x5001 ,0xcf);
Camara_EnviaBytesI2C(0x4300 ,0x30);
Camara_EnviaBytesI2C(0x4300 ,0x30);
Camara_EnviaBytesI2C(0x460b ,0x35);
Camara_EnviaBytesI2C(0x471d ,0x00);
Camara_EnviaBytesI2C(0x3002 ,0x0c);
Camara_EnviaBytesI2C(0x3002 ,0x00);
Camara_EnviaBytesI2C(0x4713 ,0x03);
Camara_EnviaBytesI2C(0x471c ,0x50);
Camara_EnviaBytesI2C(0x4721 ,0x02);
Camara_EnviaBytesI2C(0x4402 ,0x90);
Camara_EnviaBytesI2C(0x460c ,0x22);
Camara_EnviaBytesI2C(0x3815 ,0x44);
Camara_EnviaBytesI2C(0x3503 ,0x07);
Camara_EnviaBytesI2C(0x3501 ,0x73);
Camara_EnviaBytesI2C(0x3502 ,0x80);
Camara_EnviaBytesI2C(0x350b ,0x00);
Camara_EnviaBytesI2C(0x3818 ,0xc8);
Camara_EnviaBytesI2C(0x3801 ,0x8a);
Camara_EnviaBytesI2C(0x3824 ,0x11);
Camara_EnviaBytesI2C(0x3a00 ,0x78);
Camara_EnviaBytesI2C(0x3a1a ,0x04);
Camara_EnviaBytesI2C(0x3a13 ,0x30);
Camara_EnviaBytesI2C(0x3a18 ,0x00);
Camara_EnviaBytesI2C(0x3a19 ,0x7c);
Camara_EnviaBytesI2C(0x3a08 ,0x12);
Camara_EnviaBytesI2C(0x3a09 ,0xc0);
Camara_EnviaBytesI2C(0x3a0a ,0x0f);
Camara_EnviaBytesI2C(0x3a0b ,0xa0);
Camara_EnviaBytesI2C(0x350c ,0x07);
Camara_EnviaBytesI2C(0x350d ,0xd0);
Camara_EnviaBytesI2C(0x3a0d ,0x08);
Camara_EnviaBytesI2C(0x3a0e ,0x06);
Camara_EnviaBytesI2C(0x3500 ,0x00);
Camara_EnviaBytesI2C(0x3501 ,0x00);
Camara_EnviaBytesI2C(0x3502 ,0x00);
Camara_EnviaBytesI2C(0x350a ,0x00);
//109 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x350b ,0x00);
Camara_EnviaBytesI2C(0x3503 ,0x00);
Camara_EnviaBytesI2C(0x3030 ,0x2b);
Camara_EnviaBytesI2C(0x3a02 ,0x00);
Camara_EnviaBytesI2C(0x3a03 ,0x7d);
Camara_EnviaBytesI2C(0x3a04 ,0x00);
Camara_EnviaBytesI2C(0x3a14 ,0x00);
Camara_EnviaBytesI2C(0x3a15 ,0x7d);
Camara_EnviaBytesI2C(0x3a16 ,0x00);
Camara_EnviaBytesI2C(0x3a00 ,0x78);
Camara_EnviaBytesI2C(0x3a08 ,0x09);
Camara_EnviaBytesI2C(0x3a09 ,0x60);
Camara_EnviaBytesI2C(0x3a0a ,0x07);
Camara_EnviaBytesI2C(0x3a0b ,0xd0);
Camara_EnviaBytesI2C(0x3a0d ,0x10);
Camara_EnviaBytesI2C(0x3a0e ,0x0d);
//write_i2c(0x4407 ,0x04);
//High resolution
Camara_EnviaBytesI2C(0x4407 ,0x02);
Camara_EnviaBytesI2C(0x5193 ,0x70);
Camara_EnviaBytesI2C(0x589b ,0x00);
Camara_EnviaBytesI2C(0x589a ,0xc0);
Camara_EnviaBytesI2C(0x401e ,0x20);
Camara_EnviaBytesI2C(0x4001 ,0x42);
Camara_EnviaBytesI2C(0x401c ,0x06);
Camara_EnviaBytesI2C(0x3825 ,0xac);
Camara_EnviaBytesI2C(0x3827 ,0x0c);
Camara_EnviaBytesI2C(0x5402 ,0x3f);
Camara_EnviaBytesI2C(0x5403 ,0x00);
Camara_EnviaBytesI2C(0x3406 ,0x00);
Camara_EnviaBytesI2C(0x5180 ,0xff);
Camara_EnviaBytesI2C(0x5181 ,0x52);
Camara_EnviaBytesI2C(0x5182 ,0x11);
Camara_EnviaBytesI2C(0x5183 ,0x14);
Camara_EnviaBytesI2C(0x5184 ,0x25);
Camara_EnviaBytesI2C(0x5185 ,0x24);
Camara_EnviaBytesI2C(0x5186 ,0x1c);
Camara_EnviaBytesI2C(0x5187 ,0x18);
Camara_EnviaBytesI2C(0x5188 ,0x18);
Camara_EnviaBytesI2C(0x5189 ,0x6e);
Camara_EnviaBytesI2C(0x518a ,0x68);
Camara_EnviaBytesI2C(0x518b ,0xc0);
Camara_EnviaBytesI2C(0x518c ,0xbd);
Camara_EnviaBytesI2C(0x518d ,0x3d);
Camara_EnviaBytesI2C(0x518e ,0x3d);
Camara_EnviaBytesI2C(0x518f ,0x54);
Camara_EnviaBytesI2C(0x5190 ,0x54);
Camara_EnviaBytesI2C(0x5191 ,0xf8);
Camara_EnviaBytesI2C(0x5192 ,0x04);
Camara_EnviaBytesI2C(0x5193 ,0x70);
//110 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x5194 ,0xf0);
Camara_EnviaBytesI2C(0x5195 ,0xf0);
Camara_EnviaBytesI2C(0x5196 ,0x03);
Camara_EnviaBytesI2C(0x5197 ,0x01);
Camara_EnviaBytesI2C(0x5198 ,0x04);
Camara_EnviaBytesI2C(0x5199 ,0x12);
Camara_EnviaBytesI2C(0x519a ,0x04);
Camara_EnviaBytesI2C(0x519b ,0x00);
Camara_EnviaBytesI2C(0x519c ,0x06);
Camara_EnviaBytesI2C(0x519d ,0x82);
Camara_EnviaBytesI2C(0x519e ,0x00);
Camara_EnviaBytesI2C(0x5025 ,0x80);
Camara_EnviaBytesI2C(0x5583 ,0x40);
Camara_EnviaBytesI2C(0x5584 ,0x40);
Camara_EnviaBytesI2C(0x5580 ,0x02);
Camara_EnviaBytesI2C(0x5000 ,0xcf);
Camara_EnviaBytesI2C(0x3710 ,0x10);
Camara_EnviaBytesI2C(0x3632 ,0x51);
Camara_EnviaBytesI2C(0x3702 ,0x10);
Camara_EnviaBytesI2C(0x3703 ,0xb2);
Camara_EnviaBytesI2C(0x3704 ,0x18);
Camara_EnviaBytesI2C(0x370b ,0x40);
Camara_EnviaBytesI2C(0x370d ,0x03);
Camara_EnviaBytesI2C(0x3631 ,0x01);
Camara_EnviaBytesI2C(0x3632 ,0x52);
Camara_EnviaBytesI2C(0x3606 ,0x24);
Camara_EnviaBytesI2C(0x3620 ,0x96);
Camara_EnviaBytesI2C(0x5785 ,0x07);
Camara_EnviaBytesI2C(0x3a13 ,0x30);
Camara_EnviaBytesI2C(0x3600 ,0x52);
Camara_EnviaBytesI2C(0x3604 ,0x48);
Camara_EnviaBytesI2C(0x3606 ,0x1b);
Camara_EnviaBytesI2C(0x370d ,0x0b);
Camara_EnviaBytesI2C(0x370f ,0xc0);
Camara_EnviaBytesI2C(0x3709 ,0x01);
Camara_EnviaBytesI2C(0x3823 ,0x00);
Camara_EnviaBytesI2C(0x5007 ,0x00);
Camara_EnviaBytesI2C(0x5009 ,0x00);
Camara_EnviaBytesI2C(0x5011 ,0x00);
Camara_EnviaBytesI2C(0x5013 ,0x00);
Camara_EnviaBytesI2C(0x519e ,0x00);
Camara_EnviaBytesI2C(0x5086 ,0x00);
Camara_EnviaBytesI2C(0x5087 ,0x00);
Camara_EnviaBytesI2C(0x5088 ,0x00);
Camara_EnviaBytesI2C(0x5089 ,0x00);
Camara_EnviaBytesI2C(0x302b ,0x00);
Camara_EnviaBytesI2C(0x3503 ,0x07);
Camara_EnviaBytesI2C(0x3011 ,0x07);
//111 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x350c ,0x04);
Camara_EnviaBytesI2C(0x350d ,0x58);
Camara_EnviaBytesI2C(0x3801 ,0x8a);
Camara_EnviaBytesI2C(0x3803 ,0x0a);
Camara_EnviaBytesI2C(0x3804 ,0x0a);
Camara_EnviaBytesI2C(0x3805 ,0x20);
Camara_EnviaBytesI2C(0x3806 ,0x07);
Camara_EnviaBytesI2C(0x3807 ,0x98);
Camara_EnviaBytesI2C(0x3808 ,0x0a);
Camara_EnviaBytesI2C(0x3809 ,0x20);
Camara_EnviaBytesI2C(0x380a ,0x07);
Camara_EnviaBytesI2C(0x380b ,0x98);
Camara_EnviaBytesI2C(0x380c ,0x0c);
Camara_EnviaBytesI2C(0x380d ,0x80);
Camara_EnviaBytesI2C(0x380e ,0x07);
Camara_EnviaBytesI2C(0x380f ,0xd0);
Camara_EnviaBytesI2C(0x381c ,0x11);
Camara_EnviaBytesI2C(0x381d ,0xba);
Camara_EnviaBytesI2C(0x381e ,0x04);
Camara_EnviaBytesI2C(0x381f ,0x48);
Camara_EnviaBytesI2C(0x3820 ,0x04);
Camara_EnviaBytesI2C(0x3821 ,0x18);
Camara_EnviaBytesI2C(0x3a08 ,0x14);
Camara_EnviaBytesI2C(0x3a09 ,0xe0);
Camara_EnviaBytesI2C(0x3a0a ,0x11);
Camara_EnviaBytesI2C(0x3a0b ,0x60);
Camara_EnviaBytesI2C(0x3a0d ,0x04);
Camara_EnviaBytesI2C(0x3a0e ,0x03);
Camara_EnviaBytesI2C(0x5680, 0x00);
Camara_EnviaBytesI2C(0x5681 ,0x00);
Camara_EnviaBytesI2C(0x5682 ,0x0a);
Camara_EnviaBytesI2C(0x5683 ,0x20);
Camara_EnviaBytesI2C(0x5684 ,0x00);
Camara_EnviaBytesI2C(0x5685 ,0x00);
Camara_EnviaBytesI2C(0x5686 ,0x07);
Camara_EnviaBytesI2C(0x5687 ,0x98);
Camara_EnviaBytesI2C(0x5001 ,0x7f);
Camara_EnviaBytesI2C(0x3503 ,0x00);
Camara_EnviaBytesI2C(0x3010 ,0x10);
Camara_EnviaBytesI2C(0x5001 ,0xFF);
Camara_EnviaBytesI2C(0x5583 ,0x50);
Camara_EnviaBytesI2C(0x5584 ,0x50);
Camara_EnviaBytesI2C(0x5580 ,0x02);
Camara_EnviaBytesI2C(0x3c01 ,0x80);
Camara_EnviaBytesI2C(0x3c00 ,0x04);
//;LENS
Camara_EnviaBytesI2C(0x5800 ,0x48);
Camara_EnviaBytesI2C(0x5801 ,0x31);
Camara_EnviaBytesI2C(0x5802 ,0x21);
//112 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x5803 ,0x1b);
Camara_EnviaBytesI2C(0x5804 ,0x1a);
Camara_EnviaBytesI2C(0x5805 ,0x1e);
Camara_EnviaBytesI2C(0x5806 ,0x29);
Camara_EnviaBytesI2C(0x5807 ,0x38);
Camara_EnviaBytesI2C(0x5808 ,0x26);
Camara_EnviaBytesI2C(0x5809 ,0x17);
Camara_EnviaBytesI2C(0x580a ,0x11);
Camara_EnviaBytesI2C(0x580b ,0xe );
Camara_EnviaBytesI2C(0x580c ,0xd );
Camara_EnviaBytesI2C(0x580d ,0xe );
Camara_EnviaBytesI2C(0x580e ,0x13);
Camara_EnviaBytesI2C(0x580f ,0x1a);
Camara_EnviaBytesI2C(0x5810 ,0x15);
Camara_EnviaBytesI2C(0x5811 ,0xd );
Camara_EnviaBytesI2C(0x5812 ,0x8 );
Camara_EnviaBytesI2C(0x5813 ,0x5 );
Camara_EnviaBytesI2C(0x5814 ,0x4 );
Camara_EnviaBytesI2C(0x5815 ,0x5 );
Camara_EnviaBytesI2C(0x5816 ,0x9 );
Camara_EnviaBytesI2C(0x5817 ,0xd );
Camara_EnviaBytesI2C(0x5818 ,0x11);
Camara_EnviaBytesI2C(0x5819 ,0xa );
Camara_EnviaBytesI2C(0x581a ,0x4 );
Camara_EnviaBytesI2C(0x581b ,0x0 );
Camara_EnviaBytesI2C(0x581c ,0x0 );
Camara_EnviaBytesI2C(0x581d ,0x1 );
Camara_EnviaBytesI2C(0x581e ,0x6 );
Camara_EnviaBytesI2C(0x581f ,0x9 );
Camara_EnviaBytesI2C(0x5820 ,0x12);
Camara_EnviaBytesI2C(0x5821 ,0xb );
Camara_EnviaBytesI2C(0x5822 ,0x4 );
Camara_EnviaBytesI2C(0x5823 ,0x0 );
Camara_EnviaBytesI2C(0x5824 ,0x0 );
Camara_EnviaBytesI2C(0x5825 ,0x1 );
Camara_EnviaBytesI2C(0x5826 ,0x6 );
Camara_EnviaBytesI2C(0x5827 ,0xa );
Camara_EnviaBytesI2C(0x5828 ,0x17);
Camara_EnviaBytesI2C(0x5829 ,0xf );
Camara_EnviaBytesI2C(0x582a ,0x9 );
Camara_EnviaBytesI2C(0x582b ,0x6 );
Camara_EnviaBytesI2C(0x582c ,0x5 );
Camara_EnviaBytesI2C(0x582d ,0x6 );
Camara_EnviaBytesI2C(0x582e ,0xa );
Camara_EnviaBytesI2C(0x582f ,0xe );
Camara_EnviaBytesI2C(0x5830 ,0x28);
Camara_EnviaBytesI2C(0x5831 ,0x1a);
Camara_EnviaBytesI2C(0x5832 ,0x11);
//113 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x5833 ,0xe );
Camara_EnviaBytesI2C(0x5834 ,0xe );
Camara_EnviaBytesI2C(0x5835 ,0xf );
Camara_EnviaBytesI2C(0x5836 ,0x15);
Camara_EnviaBytesI2C(0x5837 ,0x1d);
Camara_EnviaBytesI2C(0x5838 ,0x6e);
Camara_EnviaBytesI2C(0x5839 ,0x39);
Camara_EnviaBytesI2C(0x583a ,0x27);
Camara_EnviaBytesI2C(0x583b ,0x1f);
Camara_EnviaBytesI2C(0x583c ,0x1e);
Camara_EnviaBytesI2C(0x583d ,0x23);
Camara_EnviaBytesI2C(0x583e ,0x2f);
Camara_EnviaBytesI2C(0x583f ,0x41);
Camara_EnviaBytesI2C(0x5840 ,0xe );
Camara_EnviaBytesI2C(0x5841 ,0xc );
Camara_EnviaBytesI2C(0x5842 ,0xd );
Camara_EnviaBytesI2C(0x5843 ,0xc );
Camara_EnviaBytesI2C(0x5844 ,0xc );
Camara_EnviaBytesI2C(0x5845 ,0xc );
Camara_EnviaBytesI2C(0x5846 ,0xc );
Camara_EnviaBytesI2C(0x5847 ,0xc );
Camara_EnviaBytesI2C(0x5848 ,0xd );
Camara_EnviaBytesI2C(0x5849 ,0xe );
Camara_EnviaBytesI2C(0x584a ,0xe );
Camara_EnviaBytesI2C(0x584b ,0xa );
Camara_EnviaBytesI2C(0x584c ,0xe );
Camara_EnviaBytesI2C(0x584d ,0xe );
Camara_EnviaBytesI2C(0x584e ,0x10);
Camara_EnviaBytesI2C(0x584f ,0x10);
Camara_EnviaBytesI2C(0x5850 ,0x11);
Camara_EnviaBytesI2C(0x5851 ,0xa );
Camara_EnviaBytesI2C(0x5852 ,0xf );
Camara_EnviaBytesI2C(0x5853 ,0xe );
Camara_EnviaBytesI2C(0x5854 ,0x10);
Camara_EnviaBytesI2C(0x5855 ,0x10);
Camara_EnviaBytesI2C(0x5856 ,0x10);
Camara_EnviaBytesI2C(0x5857 ,0xa );
Camara_EnviaBytesI2C(0x5858 ,0xe );
Camara_EnviaBytesI2C(0x5859 ,0xe );
Camara_EnviaBytesI2C(0x585a ,0xf );
Camara_EnviaBytesI2C(0x585b ,0xf );
Camara_EnviaBytesI2C(0x585c ,0xf );
Camara_EnviaBytesI2C(0x585d ,0xa );
Camara_EnviaBytesI2C(0x585e ,0x9 );
Camara_EnviaBytesI2C(0x585f ,0xd );
Camara_EnviaBytesI2C(0x5860 ,0xc );
Camara_EnviaBytesI2C(0x5861 ,0xb );
Camara_EnviaBytesI2C(0x5862 ,0xd );
//114 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x5863 ,0x7 );
Camara_EnviaBytesI2C(0x5864 ,0x17);
Camara_EnviaBytesI2C(0x5865 ,0x14);
Camara_EnviaBytesI2C(0x5866 ,0x18);
Camara_EnviaBytesI2C(0x5867 ,0x18);
Camara_EnviaBytesI2C(0x5868 ,0x16);
Camara_EnviaBytesI2C(0x5869 ,0x12);
Camara_EnviaBytesI2C(0x586a ,0x1b);
Camara_EnviaBytesI2C(0x586b ,0x1a);
Camara_EnviaBytesI2C(0x586c ,0x16);
Camara_EnviaBytesI2C(0x586d ,0x16);
Camara_EnviaBytesI2C(0x586e ,0x18);
Camara_EnviaBytesI2C(0x586f ,0x1f);
Camara_EnviaBytesI2C(0x5870 ,0x1c);
Camara_EnviaBytesI2C(0x5871 ,0x16);
Camara_EnviaBytesI2C(0x5872 ,0x10);
Camara_EnviaBytesI2C(0x5873 ,0xf );
Camara_EnviaBytesI2C(0x5874 ,0x13);
Camara_EnviaBytesI2C(0x5875 ,0x1c);
Camara_EnviaBytesI2C(0x5876 ,0x1e);
Camara_EnviaBytesI2C(0x5877 ,0x17);
Camara_EnviaBytesI2C(0x5878 ,0x11);
Camara_EnviaBytesI2C(0x5879 ,0x11);
Camara_EnviaBytesI2C(0x587a ,0x14);
Camara_EnviaBytesI2C(0x587b ,0x1e);
Camara_EnviaBytesI2C(0x587c ,0x1c);
Camara_EnviaBytesI2C(0x587d ,0x1c);
Camara_EnviaBytesI2C(0x587e ,0x1a);
Camara_EnviaBytesI2C(0x587f ,0x1a);
Camara_EnviaBytesI2C(0x5880 ,0x1b);
Camara_EnviaBytesI2C(0x5881 ,0x1f);
Camara_EnviaBytesI2C(0x5882 ,0x14);
Camara_EnviaBytesI2C(0x5883 ,0x1a);
Camara_EnviaBytesI2C(0x5884 ,0x1d);
Camara_EnviaBytesI2C(0x5885 ,0x1e);
Camara_EnviaBytesI2C(0x5886 ,0x1a);
Camara_EnviaBytesI2C(0x5887 ,0x1a);
//;
//;AWB
Camara_EnviaBytesI2C(0x5180 ,0xff);
Camara_EnviaBytesI2C(0x5181 ,0x52);
Camara_EnviaBytesI2C(0x5182 ,0x11);
Camara_EnviaBytesI2C(0x5183 ,0x14);
Camara_EnviaBytesI2C(0x5184 ,0x25);
Camara_EnviaBytesI2C(0x5185 ,0x24);
Camara_EnviaBytesI2C(0x5186 ,0x14);
Camara_EnviaBytesI2C(0x5187 ,0x14);
Camara_EnviaBytesI2C(0x5188 ,0x14);
//115 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x5189 ,0x69);
Camara_EnviaBytesI2C(0x518a ,0x60);
Camara_EnviaBytesI2C(0x518b ,0xa2);
Camara_EnviaBytesI2C(0x518c ,0x9c);
Camara_EnviaBytesI2C(0x518d ,0x36);
Camara_EnviaBytesI2C(0x518e ,0x34);
Camara_EnviaBytesI2C(0x518f ,0x54);
Camara_EnviaBytesI2C(0x5190 ,0x4c);
Camara_EnviaBytesI2C(0x5191 ,0xf8);
Camara_EnviaBytesI2C(0x5192 ,0x04);
Camara_EnviaBytesI2C(0x5193 ,0x70);
Camara_EnviaBytesI2C(0x5194 ,0xf0);
Camara_EnviaBytesI2C(0x5195 ,0xf0);
Camara_EnviaBytesI2C(0x5196 ,0x03);
Camara_EnviaBytesI2C(0x5197 ,0x01);
Camara_EnviaBytesI2C(0x5198 ,0x05);
Camara_EnviaBytesI2C(0x5199 ,0x2f);
Camara_EnviaBytesI2C(0x519a ,0x04);
Camara_EnviaBytesI2C(0x519b ,0x00);
Camara_EnviaBytesI2C(0x519c ,0x06);
Camara_EnviaBytesI2C(0x519d ,0xa0);
Camara_EnviaBytesI2C(0x519e ,0xa0);
//;
//;D/S
Camara_EnviaBytesI2C(0x528a ,0x00);
Camara_EnviaBytesI2C(0x528b ,0x01);
Camara_EnviaBytesI2C(0x528c ,0x04);
Camara_EnviaBytesI2C(0x528d ,0x08);
Camara_EnviaBytesI2C(0x528e ,0x10);
Camara_EnviaBytesI2C(0x528f ,0x20);
Camara_EnviaBytesI2C(0x5290 ,0x30);
Camara_EnviaBytesI2C(0x5292 ,0x00);
Camara_EnviaBytesI2C(0x5293 ,0x00);
Camara_EnviaBytesI2C(0x5294 ,0x00);
Camara_EnviaBytesI2C(0x5295 ,0x01);
Camara_EnviaBytesI2C(0x5296 ,0x00);
Camara_EnviaBytesI2C(0x5297 ,0x04);
Camara_EnviaBytesI2C(0x5298 ,0x00);
Camara_EnviaBytesI2C(0x5299 ,0x08);
Camara_EnviaBytesI2C(0x529a ,0x00);
Camara_EnviaBytesI2C(0x529b ,0x10);
Camara_EnviaBytesI2C(0x529c ,0x00);
Camara_EnviaBytesI2C(0x529d ,0x20);
Camara_EnviaBytesI2C(0x529e ,0x00);
Camara_EnviaBytesI2C(0x529f ,0x30);
Camara_EnviaBytesI2C(0x5282 ,0x00);
Camara_EnviaBytesI2C(0x5300 ,0x00);
Camara_EnviaBytesI2C(0x5301 ,0x20);
//116 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x5302 ,0x00);
Camara_EnviaBytesI2C(0x5303 ,0x7c);
Camara_EnviaBytesI2C(0x530c ,0x00);
Camara_EnviaBytesI2C(0x530d ,0x10);
Camara_EnviaBytesI2C(0x530e ,0x20);
Camara_EnviaBytesI2C(0x530f ,0x80);
Camara_EnviaBytesI2C(0x5310 ,0x20);
Camara_EnviaBytesI2C(0x5311 ,0x80);
Camara_EnviaBytesI2C(0x5308 ,0x20);
Camara_EnviaBytesI2C(0x5309 ,0x40);
Camara_EnviaBytesI2C(0x5304 ,0x00);
Camara_EnviaBytesI2C(0x5305 ,0x30);
Camara_EnviaBytesI2C(0x5306 ,0x00);
Camara_EnviaBytesI2C(0x5307 ,0x80);
Camara_EnviaBytesI2C(0x5314 ,0x08);
Camara_EnviaBytesI2C(0x5315 ,0x20);
Camara_EnviaBytesI2C(0x5319 ,0x30);
Camara_EnviaBytesI2C(0x5316 ,0x10);
Camara_EnviaBytesI2C(0x5317 ,0x00);
Camara_EnviaBytesI2C(0x5318 ,0x02);
//;
//;CMX
Camara_EnviaBytesI2C(0x5380 ,0x01);
Camara_EnviaBytesI2C(0x5381 ,0x00);
Camara_EnviaBytesI2C(0x5382 ,0x00);
Camara_EnviaBytesI2C(0x5383 ,0x1f);
Camara_EnviaBytesI2C(0x5384 ,0x00);
Camara_EnviaBytesI2C(0x5385 ,0x06);
Camara_EnviaBytesI2C(0x5386 ,0x00);
Camara_EnviaBytesI2C(0x5387 ,0x00);
Camara_EnviaBytesI2C(0x5388 ,0x00);
Camara_EnviaBytesI2C(0x5389 ,0xE1);
Camara_EnviaBytesI2C(0x538A ,0x00);
Camara_EnviaBytesI2C(0x538B ,0x2B);
Camara_EnviaBytesI2C(0x538C ,0x00);
Camara_EnviaBytesI2C(0x538D ,0x00);
Camara_EnviaBytesI2C(0x538E ,0x00);
Camara_EnviaBytesI2C(0x538F ,0x10);
Camara_EnviaBytesI2C(0x5390 ,0x00);
Camara_EnviaBytesI2C(0x5391 ,0xB3);
Camara_EnviaBytesI2C(0x5392 ,0x00);
Camara_EnviaBytesI2C(0x5393 ,0xA6);
Camara_EnviaBytesI2C(0x5394 ,0x08);
//;
//;GAMMA
Camara_EnviaBytesI2C(0x5480 ,0x0c);
Camara_EnviaBytesI2C(0x5481 ,0x18);
Camara_EnviaBytesI2C(0x5482 ,0x2f);
//117 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x5483 ,0x55);
Camara_EnviaBytesI2C(0x5484 ,0x64);
Camara_EnviaBytesI2C(0x5485 ,0x71);
Camara_EnviaBytesI2C(0x5486 ,0x7d);
Camara_EnviaBytesI2C(0x5487 ,0x87);
Camara_EnviaBytesI2C(0x5488 ,0x91);
Camara_EnviaBytesI2C(0x5489 ,0x9a);
Camara_EnviaBytesI2C(0x548A ,0xaa);
Camara_EnviaBytesI2C(0x548B ,0xb8);
Camara_EnviaBytesI2C(0x548C ,0xcd);
Camara_EnviaBytesI2C(0x548D ,0xdd);
Camara_EnviaBytesI2C(0x548E ,0xea);
Camara_EnviaBytesI2C(0x548F ,0x1d);
Camara_EnviaBytesI2C(0x5490 ,0x05);
Camara_EnviaBytesI2C(0x5491 ,0x00);
Camara_EnviaBytesI2C(0x5492 ,0x04);
Camara_EnviaBytesI2C(0x5493 ,0x20);
Camara_EnviaBytesI2C(0x5494 ,0x03);
Camara_EnviaBytesI2C(0x5495 ,0x60);
Camara_EnviaBytesI2C(0x5496 ,0x02);
Camara_EnviaBytesI2C(0x5497 ,0xB8);
Camara_EnviaBytesI2C(0x5498 ,0x02);
Camara_EnviaBytesI2C(0x5499 ,0x86);
Camara_EnviaBytesI2C(0x549A ,0x02);
Camara_EnviaBytesI2C(0x549B ,0x5B);
Camara_EnviaBytesI2C(0x549C ,0x02);
Camara_EnviaBytesI2C(0x549D ,0x3B);
Camara_EnviaBytesI2C(0x549E ,0x02);
Camara_EnviaBytesI2C(0x549F ,0x1C);
Camara_EnviaBytesI2C(0x54A0 ,0x02);
Camara_EnviaBytesI2C(0x54A1 ,0x04);
Camara_EnviaBytesI2C(0x54A2 ,0x01);
Camara_EnviaBytesI2C(0x54A3 ,0xED);
Camara_EnviaBytesI2C(0x54A4 ,0x01);
Camara_EnviaBytesI2C(0x54A5 ,0xC5);
Camara_EnviaBytesI2C(0x54A6 ,0x01);
Camara_EnviaBytesI2C(0x54A7 ,0xA5);
Camara_EnviaBytesI2C(0x54A8 ,0x01);
Camara_EnviaBytesI2C(0x54A9 ,0x6C);
Camara_EnviaBytesI2C(0x54AA ,0x01);
Camara_EnviaBytesI2C(0x54AB ,0x41);
Camara_EnviaBytesI2C(0x54AC ,0x01);
Camara_EnviaBytesI2C(0x54AD ,0x20);
Camara_EnviaBytesI2C(0x54AE ,0x00);
Camara_EnviaBytesI2C(0x54AF ,0x16);
Camara_EnviaBytesI2C(0x54B0 ,0x01);
Camara_EnviaBytesI2C(0x54B1 ,0x20);
Camara_EnviaBytesI2C(0x54B2 ,0x00);
//118 Company Confidential
//OV5642 Camera Module Software Application Notes
Camara_EnviaBytesI2C(0x54B3 ,0x10);
Camara_EnviaBytesI2C(0x54B4 ,0x00);
Camara_EnviaBytesI2C(0x54B5 ,0xf0);
Camara_EnviaBytesI2C(0x54B6 ,0x00);
Camara_EnviaBytesI2C(0x54B7 ,0xDF);
//;
Camara_EnviaBytesI2C(0x5402 ,0x3f);
Camara_EnviaBytesI2C(0x5403 ,0x00);
//;
//;UV ADJUST
Camara_EnviaBytesI2C(0x5500 ,0x10);
Camara_EnviaBytesI2C(0x5502 ,0x00);
Camara_EnviaBytesI2C(0x5503 ,0x06);
Camara_EnviaBytesI2C(0x5504 ,0x00);
Camara_EnviaBytesI2C(0x5505 ,0x7f);
//;AE
Camara_EnviaBytesI2C(0x5025 ,0x80);
Camara_EnviaBytesI2C(0x3a0f ,0x30);
Camara_EnviaBytesI2C(0x3a10 ,0x28);
Camara_EnviaBytesI2C(0x3a1b ,0x30);
Camara_EnviaBytesI2C(0x3a1e ,0x28);
Camara_EnviaBytesI2C(0x3a11 ,0x61);
Camara_EnviaBytesI2C(0x3a1f ,0x10);
Camara_EnviaBytesI2C(0x5688 ,0xff);
Camara_EnviaBytesI2C(0x5689 ,0xff);
Camara_EnviaBytesI2C(0x568a ,0xff);
Camara_EnviaBytesI2C(0x568b ,0xff);
Camara_EnviaBytesI2C(0x568c ,0xff);
Camara_EnviaBytesI2C(0x568d ,0xff);
Camara_EnviaBytesI2C(0x568e ,0xff);
Camara_EnviaBytesI2C(0x568f ,0xff);

}

void DelFabricante2(void) {
    
Camara_EnviaBytesI2C(0x3103,0x93);
Camara_EnviaBytesI2C(0x3008,0x82);
Camara_EnviaBytesI2C(0x3017,0x7f);
Camara_EnviaBytesI2C(0x3018,0xfc);
Camara_EnviaBytesI2C(0x3810,0xc2);
Camara_EnviaBytesI2C(0x3615,0xf0);
Camara_EnviaBytesI2C(0x3000,0x00);
Camara_EnviaBytesI2C(0x3001,0x00);
Camara_EnviaBytesI2C(0x3002,0x00);
Camara_EnviaBytesI2C(0x3003,0x00);
Camara_EnviaBytesI2C(0x3004,0xff);
Camara_EnviaBytesI2C(0x3030,0x2b);
Camara_EnviaBytesI2C(0x3011,0x08);
Camara_EnviaBytesI2C(0x3010,0x10);
Camara_EnviaBytesI2C(0x3604,0x60);
Camara_EnviaBytesI2C(0x3622,0x60);
Camara_EnviaBytesI2C(0x3621,0x09);
Camara_EnviaBytesI2C(0x3705,0xd9);
Camara_EnviaBytesI2C(0x3709,0x00);
Camara_EnviaBytesI2C(0x4000,0x21);
Camara_EnviaBytesI2C(0x401d,0x22);
Camara_EnviaBytesI2C(0x3600,0x54);
Camara_EnviaBytesI2C(0x3605,0x04);
Camara_EnviaBytesI2C(0x3606,0x3f);
Camara_EnviaBytesI2C(0x3c01,0x80);
Camara_EnviaBytesI2C(0x300d,0x22);
Camara_EnviaBytesI2C(0x3623,0x22);
Camara_EnviaBytesI2C(0x5000,0x4f);
Camara_EnviaBytesI2C(0x5020,0x04);
Camara_EnviaBytesI2C(0x5181,0x79);
Camara_EnviaBytesI2C(0x5182,0x00);
Camara_EnviaBytesI2C(0x5185,0x22);
Camara_EnviaBytesI2C(0x5197,0x01);
Camara_EnviaBytesI2C(0x5500,0x0a);
Camara_EnviaBytesI2C(0x5504,0x00);
Camara_EnviaBytesI2C(0x5505,0x7f);
Camara_EnviaBytesI2C(0x5080,0x08);
Camara_EnviaBytesI2C(0x300e,0x18);
Camara_EnviaBytesI2C(0x4610,0x00);
Camara_EnviaBytesI2C(0x471d,0x05);
Camara_EnviaBytesI2C(0x4708,0x06);
Camara_EnviaBytesI2C(0x370c,0xa0);
Camara_EnviaBytesI2C(0x3808,0x0a);
Camara_EnviaBytesI2C(0x3809,0x20);
Camara_EnviaBytesI2C(0x380a,0x07);
Camara_EnviaBytesI2C(0x380b,0x98);
Camara_EnviaBytesI2C(0x380c,0x0c);
Camara_EnviaBytesI2C(0x380d,0x80);
Camara_EnviaBytesI2C(0x380e,0x07);
Camara_EnviaBytesI2C(0x380f,0xd0);
Camara_EnviaBytesI2C(0x5687,0x94);
Camara_EnviaBytesI2C(0x501f,0x00);
Camara_EnviaBytesI2C(0x5000,0x4f);
Camara_EnviaBytesI2C(0x5001,0xcf);
Camara_EnviaBytesI2C(0x4300,0x30);
Camara_EnviaBytesI2C(0x4300,0x30);
Camara_EnviaBytesI2C(0x460b,0x35);
Camara_EnviaBytesI2C(0x471d,0x00);
Camara_EnviaBytesI2C(0x3002,0x0c);
Camara_EnviaBytesI2C(0x3002,0x00);
Camara_EnviaBytesI2C(0x4713,0x03);
Camara_EnviaBytesI2C(0x471c,0x50);
Camara_EnviaBytesI2C(0x4721,0x02);
Camara_EnviaBytesI2C(0x4402,0x90);
Camara_EnviaBytesI2C(0x460c,0x22);
Camara_EnviaBytesI2C(0x3815,0x44);
Camara_EnviaBytesI2C(0x3503,0x07);
Camara_EnviaBytesI2C(0x3501,0x73);
Camara_EnviaBytesI2C(0x3502,0x80);
Camara_EnviaBytesI2C(0x350b,0x00);
Camara_EnviaBytesI2C(0x3818,0xc8);
Camara_EnviaBytesI2C(0x3801,0x88);
Camara_EnviaBytesI2C(0x3824,0x11);
Camara_EnviaBytesI2C(0x3a00,0x78);
Camara_EnviaBytesI2C(0x3a1a,0x04);
Camara_EnviaBytesI2C(0x3a13,0x30);
Camara_EnviaBytesI2C(0x3a18,0x00);
Camara_EnviaBytesI2C(0x3a19,0x7c);
Camara_EnviaBytesI2C(0x3a08,0x12);
Camara_EnviaBytesI2C(0x3a09,0xc0);
Camara_EnviaBytesI2C(0x3a0a,0x0f);
Camara_EnviaBytesI2C(0x3a0b,0xa0);
Camara_EnviaBytesI2C(0x350c,0x07);
Camara_EnviaBytesI2C(0x350d,0xd0);
Camara_EnviaBytesI2C(0x3a0d,0x08);
Camara_EnviaBytesI2C(0x3a0e,0x06);
Camara_EnviaBytesI2C(0x3500,0x00);
Camara_EnviaBytesI2C(0x3501,0x00);
Camara_EnviaBytesI2C(0x3502,0x00);
Camara_EnviaBytesI2C(0x350a,0x00);
Camara_EnviaBytesI2C(0x350b,0x00);
Camara_EnviaBytesI2C(0x3503,0x00);
Camara_EnviaBytesI2C(0x3a0f,0x3c);
Camara_EnviaBytesI2C(0x3a10,0x32);
Camara_EnviaBytesI2C(0x3a1b,0x3c);
Camara_EnviaBytesI2C(0x3a1e,0x32);
Camara_EnviaBytesI2C(0x3a11,0x80);
Camara_EnviaBytesI2C(0x3a1f,0x20);
Camara_EnviaBytesI2C(0x3030,0x0b);
Camara_EnviaBytesI2C(0x3a02,0x00);
Camara_EnviaBytesI2C(0x3a03,0x7d);
Camara_EnviaBytesI2C(0x3a04,0x00);
Camara_EnviaBytesI2C(0x3a14,0x00);
Camara_EnviaBytesI2C(0x3a15,0x7d);
Camara_EnviaBytesI2C(0x3a16,0x00);
Camara_EnviaBytesI2C(0x3a00,0x78);
Camara_EnviaBytesI2C(0x3a08,0x09);
Camara_EnviaBytesI2C(0x3a09,0x60);
Camara_EnviaBytesI2C(0x3a0a,0x07);
Camara_EnviaBytesI2C(0x3a0b,0xd0);
Camara_EnviaBytesI2C(0x3a0d,0x10);
Camara_EnviaBytesI2C(0x3a0e,0x0d);

//Low quality :
//write_i2c(0x4407, 0x08);
//Default quality:
//write_i2c(0x4407 ,0x04);
//High quality :
Camara_EnviaBytesI2C(0x4407, 0x02);

Camara_EnviaBytesI2C(0x5193,0x70);
Camara_EnviaBytesI2C(0x589b,0x00);
Camara_EnviaBytesI2C(0x589a,0xc0);
Camara_EnviaBytesI2C(0x401e,0x20);
Camara_EnviaBytesI2C(0x4001,0x42);
Camara_EnviaBytesI2C(0x4002,0x02);
Camara_EnviaBytesI2C(0x401c,0x06);
Camara_EnviaBytesI2C(0x3825,0xac);
Camara_EnviaBytesI2C(0x3827,0x0c);
Camara_EnviaBytesI2C(0x528a,0x01);
Camara_EnviaBytesI2C(0x528b,0x04);
Camara_EnviaBytesI2C(0x528c,0x08);
Camara_EnviaBytesI2C(0x528d,0x10);
Camara_EnviaBytesI2C(0x528e,0x20);
Camara_EnviaBytesI2C(0x528f,0x28);
Camara_EnviaBytesI2C(0x5290,0x30);
Camara_EnviaBytesI2C(0x5292,0x00);
Camara_EnviaBytesI2C(0x5293,0x01);
Camara_EnviaBytesI2C(0x5294,0x00);
Camara_EnviaBytesI2C(0x5295,0x04);
Camara_EnviaBytesI2C(0x5296,0x00);
Camara_EnviaBytesI2C(0x5297,0x08);
Camara_EnviaBytesI2C(0x5298,0x00);
Camara_EnviaBytesI2C(0x5299,0x10);
Camara_EnviaBytesI2C(0x529a,0x00);
Camara_EnviaBytesI2C(0x529b,0x20);
Camara_EnviaBytesI2C(0x529c,0x00);
Camara_EnviaBytesI2C(0x529d,0x28);
Camara_EnviaBytesI2C(0x529e,0x00);
Camara_EnviaBytesI2C(0x529f,0x30);
Camara_EnviaBytesI2C(0x5282,0x00);
Camara_EnviaBytesI2C(0x5300,0x00);
Camara_EnviaBytesI2C(0x5301,0x20);
Camara_EnviaBytesI2C(0x5302,0x00);
Camara_EnviaBytesI2C(0x5303,0x7c);
Camara_EnviaBytesI2C(0x530c,0x00);
Camara_EnviaBytesI2C(0x530d,0x0c);
Camara_EnviaBytesI2C(0x530e,0x20);
Camara_EnviaBytesI2C(0x530f,0x80);
Camara_EnviaBytesI2C(0x5310,0x20);
Camara_EnviaBytesI2C(0x5311,0x80);
Camara_EnviaBytesI2C(0x5308,0x20);
Camara_EnviaBytesI2C(0x5309,0x40);
Camara_EnviaBytesI2C(0x5304,0x00);
Camara_EnviaBytesI2C(0x5305,0x30);
Camara_EnviaBytesI2C(0x5306,0x00);
Camara_EnviaBytesI2C(0x5307,0x80);
Camara_EnviaBytesI2C(0x5314,0x08);
Camara_EnviaBytesI2C(0x5315,0x20);
Camara_EnviaBytesI2C(0x5319,0x30);
Camara_EnviaBytesI2C(0x5316,0x10);
Camara_EnviaBytesI2C(0x5317,0x00);
Camara_EnviaBytesI2C(0x5318,0x02);
Camara_EnviaBytesI2C(0x5380,0x01);
Camara_EnviaBytesI2C(0x5381,0x00);
Camara_EnviaBytesI2C(0x5382,0x00);
Camara_EnviaBytesI2C(0x5383,0x4e);
Camara_EnviaBytesI2C(0x5384,0x00);
Camara_EnviaBytesI2C(0x5385,0x0f);
Camara_EnviaBytesI2C(0x5386,0x00);
Camara_EnviaBytesI2C(0x5387,0x00);
Camara_EnviaBytesI2C(0x5388,0x01);
Camara_EnviaBytesI2C(0x5389,0x15);
Camara_EnviaBytesI2C(0x538a,0x00);
Camara_EnviaBytesI2C(0x538b,0x31);
Camara_EnviaBytesI2C(0x538c,0x00);
Camara_EnviaBytesI2C(0x538d,0x00);
Camara_EnviaBytesI2C(0x538e,0x00);
Camara_EnviaBytesI2C(0x538f,0x0f);
Camara_EnviaBytesI2C(0x5390,0x00);
Camara_EnviaBytesI2C(0x5391,0xab);
Camara_EnviaBytesI2C(0x5392,0x00);
Camara_EnviaBytesI2C(0x5393,0xa2);
Camara_EnviaBytesI2C(0x5394,0x08);
Camara_EnviaBytesI2C(0x5480,0x14);
Camara_EnviaBytesI2C(0x5481,0x21);
Camara_EnviaBytesI2C(0x5482,0x36);
Camara_EnviaBytesI2C(0x5483,0x57);
Camara_EnviaBytesI2C(0x5484,0x65);
Camara_EnviaBytesI2C(0x5485,0x71);
Camara_EnviaBytesI2C(0x5486,0x7d);
Camara_EnviaBytesI2C(0x5487,0x87);
Camara_EnviaBytesI2C(0x5488,0x91);
Camara_EnviaBytesI2C(0x5489,0x9a);
Camara_EnviaBytesI2C(0x548a,0xaa);
Camara_EnviaBytesI2C(0x548b,0xb8);
Camara_EnviaBytesI2C(0x548c,0xcd);
Camara_EnviaBytesI2C(0x548d,0xdd);
Camara_EnviaBytesI2C(0x548e,0xea);
Camara_EnviaBytesI2C(0x548f,0x1d);
Camara_EnviaBytesI2C(0x5490,0x05);
Camara_EnviaBytesI2C(0x5491,0x00);
Camara_EnviaBytesI2C(0x5492,0x04);
Camara_EnviaBytesI2C(0x5493,0x20);
Camara_EnviaBytesI2C(0x5494,0x03);
Camara_EnviaBytesI2C(0x5495,0x60);
Camara_EnviaBytesI2C(0x5496,0x02);
Camara_EnviaBytesI2C(0x5497,0xb8);
Camara_EnviaBytesI2C(0x5498,0x02);
Camara_EnviaBytesI2C(0x5499,0x86);
Camara_EnviaBytesI2C(0x549a,0x02);
Camara_EnviaBytesI2C(0x549b,0x5b);
Camara_EnviaBytesI2C(0x549c,0x02);
Camara_EnviaBytesI2C(0x549d,0x3b);
Camara_EnviaBytesI2C(0x549e,0x02);
Camara_EnviaBytesI2C(0x549f,0x1c);
Camara_EnviaBytesI2C(0x54a0,0x02);
Camara_EnviaBytesI2C(0x54a1,0x04);
Camara_EnviaBytesI2C(0x54a2,0x01);
Camara_EnviaBytesI2C(0x54a3,0xed);
Camara_EnviaBytesI2C(0x54a4,0x01);
Camara_EnviaBytesI2C(0x54a5,0xc5);
Camara_EnviaBytesI2C(0x54a6,0x01);
Camara_EnviaBytesI2C(0x54a7,0xa5);
Camara_EnviaBytesI2C(0x54a8,0x01);
Camara_EnviaBytesI2C(0x54a9,0x6c);
Camara_EnviaBytesI2C(0x54aa,0x01);
Camara_EnviaBytesI2C(0x54ab,0x41);
Camara_EnviaBytesI2C(0x54ac,0x01);
Camara_EnviaBytesI2C(0x54ad,0x20);
Camara_EnviaBytesI2C(0x54ae,0x00);
Camara_EnviaBytesI2C(0x54af,0x16);
Camara_EnviaBytesI2C(0x54b0,0x01);
Camara_EnviaBytesI2C(0x54b1,0x20);
Camara_EnviaBytesI2C(0x54b2,0x00);
Camara_EnviaBytesI2C(0x54b3,0x10);
Camara_EnviaBytesI2C(0x54b4,0x00);
Camara_EnviaBytesI2C(0x54b5,0xf0);
Camara_EnviaBytesI2C(0x54b6,0x00);
Camara_EnviaBytesI2C(0x54b7,0xdf);
Camara_EnviaBytesI2C(0x5402,0x3f);
Camara_EnviaBytesI2C(0x5403,0x00);
Camara_EnviaBytesI2C(0x3406,0x00);
Camara_EnviaBytesI2C(0x5180,0xff);
Camara_EnviaBytesI2C(0x5181,0x52);
Camara_EnviaBytesI2C(0x5182,0x11);
Camara_EnviaBytesI2C(0x5183,0x14);
Camara_EnviaBytesI2C(0x5184,0x25);
Camara_EnviaBytesI2C(0x5185,0x24);
Camara_EnviaBytesI2C(0x5186,0x06);
Camara_EnviaBytesI2C(0x5187,0x08);
Camara_EnviaBytesI2C(0x5188,0x08);
Camara_EnviaBytesI2C(0x5189,0x7c);
Camara_EnviaBytesI2C(0x518a,0x60);
Camara_EnviaBytesI2C(0x518b,0xb2);
Camara_EnviaBytesI2C(0x518c,0xb2);
Camara_EnviaBytesI2C(0x518d,0x44);
Camara_EnviaBytesI2C(0x518e,0x3d);
Camara_EnviaBytesI2C(0x518f,0x58);
Camara_EnviaBytesI2C(0x5190,0x46);
Camara_EnviaBytesI2C(0x5191,0xf8);
Camara_EnviaBytesI2C(0x5192,0x04);
Camara_EnviaBytesI2C(0x5193,0x70);
Camara_EnviaBytesI2C(0x5194,0xf0);
Camara_EnviaBytesI2C(0x5195,0xf0);
Camara_EnviaBytesI2C(0x5196,0x03);
Camara_EnviaBytesI2C(0x5197,0x01);
Camara_EnviaBytesI2C(0x5198,0x04);
Camara_EnviaBytesI2C(0x5199,0x12);
Camara_EnviaBytesI2C(0x519a,0x04);
Camara_EnviaBytesI2C(0x519b,0x00);
Camara_EnviaBytesI2C(0x519c,0x06);
Camara_EnviaBytesI2C(0x519d,0x82);
Camara_EnviaBytesI2C(0x519e,0x00);
Camara_EnviaBytesI2C(0x5025,0x80);
Camara_EnviaBytesI2C(0x3a0f,0x38);
Camara_EnviaBytesI2C(0x3a10,0x30);
Camara_EnviaBytesI2C(0x3a1b,0x3a);
Camara_EnviaBytesI2C(0x3a1e,0x2e);
Camara_EnviaBytesI2C(0x3a11,0x60);
Camara_EnviaBytesI2C(0x3a1f,0x10);
Camara_EnviaBytesI2C(0x5688,0xa6);
Camara_EnviaBytesI2C(0x5689,0x6a);
Camara_EnviaBytesI2C(0x568a,0xea);
Camara_EnviaBytesI2C(0x568b,0xae);
Camara_EnviaBytesI2C(0x568c,0xa6);
Camara_EnviaBytesI2C(0x568d,0x6a);
Camara_EnviaBytesI2C(0x568e,0x62);
Camara_EnviaBytesI2C(0x568f,0x26);
Camara_EnviaBytesI2C(0x5583,0x40);
Camara_EnviaBytesI2C(0x5584,0x40);
Camara_EnviaBytesI2C(0x5580,0x02);
Camara_EnviaBytesI2C(0x5000,0xcf);
Camara_EnviaBytesI2C(0x5800,0x27);
Camara_EnviaBytesI2C(0x5801,0x19);
Camara_EnviaBytesI2C(0x5802,0x12);
Camara_EnviaBytesI2C(0x5803,0x0f);
Camara_EnviaBytesI2C(0x5804,0x10);
Camara_EnviaBytesI2C(0x5805,0x15);
Camara_EnviaBytesI2C(0x5806,0x1e);
Camara_EnviaBytesI2C(0x5807,0x2f);
Camara_EnviaBytesI2C(0x5808,0x15);
Camara_EnviaBytesI2C(0x5809,0x0d);
Camara_EnviaBytesI2C(0x580a,0x0a);
Camara_EnviaBytesI2C(0x580b,0x09);
Camara_EnviaBytesI2C(0x580c,0x0a);
Camara_EnviaBytesI2C(0x580d,0x0c);
Camara_EnviaBytesI2C(0x580e,0x12);
Camara_EnviaBytesI2C(0x580f,0x19);
Camara_EnviaBytesI2C(0x5810,0x0b);
Camara_EnviaBytesI2C(0x5811,0x07);
Camara_EnviaBytesI2C(0x5812,0x04);
Camara_EnviaBytesI2C(0x5813,0x03);
Camara_EnviaBytesI2C(0x5814,0x03);
Camara_EnviaBytesI2C(0x5815,0x06);
Camara_EnviaBytesI2C(0x5816,0x0a);
Camara_EnviaBytesI2C(0x5817,0x0f);
Camara_EnviaBytesI2C(0x5818,0x0a);
Camara_EnviaBytesI2C(0x5819,0x05);
Camara_EnviaBytesI2C(0x581a,0x01);
Camara_EnviaBytesI2C(0x581b,0x00);
Camara_EnviaBytesI2C(0x581c,0x00);
Camara_EnviaBytesI2C(0x581d,0x03);
Camara_EnviaBytesI2C(0x581e,0x08);
Camara_EnviaBytesI2C(0x581f,0x0c);
Camara_EnviaBytesI2C(0x5820,0x0a);
Camara_EnviaBytesI2C(0x5821,0x05);
Camara_EnviaBytesI2C(0x5822,0x01);
Camara_EnviaBytesI2C(0x5823,0x00);
Camara_EnviaBytesI2C(0x5824,0x00);
Camara_EnviaBytesI2C(0x5825,0x03);
Camara_EnviaBytesI2C(0x5826,0x08);
Camara_EnviaBytesI2C(0x5827,0x0c);
Camara_EnviaBytesI2C(0x5828,0x0e);
Camara_EnviaBytesI2C(0x5829,0x08);
Camara_EnviaBytesI2C(0x582a,0x06);
Camara_EnviaBytesI2C(0x582b,0x04);
Camara_EnviaBytesI2C(0x582c,0x05);
Camara_EnviaBytesI2C(0x582d,0x07);
Camara_EnviaBytesI2C(0x582e,0x0b);
Camara_EnviaBytesI2C(0x582f,0x12);
Camara_EnviaBytesI2C(0x5830,0x18);
Camara_EnviaBytesI2C(0x5831,0x10);
Camara_EnviaBytesI2C(0x5832,0x0c);
Camara_EnviaBytesI2C(0x5833,0x0a);
Camara_EnviaBytesI2C(0x5834,0x0b);
Camara_EnviaBytesI2C(0x5835,0x0e);
Camara_EnviaBytesI2C(0x5836,0x15);
Camara_EnviaBytesI2C(0x5837,0x19);
Camara_EnviaBytesI2C(0x5838,0x32);
Camara_EnviaBytesI2C(0x5839,0x1f);
Camara_EnviaBytesI2C(0x583a,0x18);
Camara_EnviaBytesI2C(0x583b,0x16);
Camara_EnviaBytesI2C(0x583c,0x17);
Camara_EnviaBytesI2C(0x583d,0x1e);
Camara_EnviaBytesI2C(0x583e,0x26);
Camara_EnviaBytesI2C(0x583f,0x53);
Camara_EnviaBytesI2C(0x5840,0x10);
Camara_EnviaBytesI2C(0x5841,0x0f);
Camara_EnviaBytesI2C(0x5842,0x0d);
Camara_EnviaBytesI2C(0x5843,0x0c);
Camara_EnviaBytesI2C(0x5844,0x0e);
Camara_EnviaBytesI2C(0x5845,0x09);
Camara_EnviaBytesI2C(0x5846,0x11);
Camara_EnviaBytesI2C(0x5847,0x10);
Camara_EnviaBytesI2C(0x5848,0x10);
Camara_EnviaBytesI2C(0x5849,0x10);
Camara_EnviaBytesI2C(0x584a,0x10);
Camara_EnviaBytesI2C(0x584b,0x0e);
Camara_EnviaBytesI2C(0x584c,0x10);
Camara_EnviaBytesI2C(0x584d,0x10);
Camara_EnviaBytesI2C(0x584e,0x11);
Camara_EnviaBytesI2C(0x584f,0x10);
Camara_EnviaBytesI2C(0x5850,0x0f);
Camara_EnviaBytesI2C(0x5851,0x0c);
Camara_EnviaBytesI2C(0x5852,0x0f);
Camara_EnviaBytesI2C(0x5853,0x10);
Camara_EnviaBytesI2C(0x5854,0x10);
Camara_EnviaBytesI2C(0x5855,0x0f);
Camara_EnviaBytesI2C(0x5856,0x0e);
Camara_EnviaBytesI2C(0x5857,0x0b);
Camara_EnviaBytesI2C(0x5858,0x10);
Camara_EnviaBytesI2C(0x5859,0x0d);
Camara_EnviaBytesI2C(0x585a,0x0d);
Camara_EnviaBytesI2C(0x585b,0x0c);
Camara_EnviaBytesI2C(0x585c,0x0c);
Camara_EnviaBytesI2C(0x585d,0x0c);
Camara_EnviaBytesI2C(0x585e,0x0b);
Camara_EnviaBytesI2C(0x585f,0x0c);
Camara_EnviaBytesI2C(0x5860,0x0c);
Camara_EnviaBytesI2C(0x5861,0x0c);
Camara_EnviaBytesI2C(0x5862,0x0d);
Camara_EnviaBytesI2C(0x5863,0x08);
Camara_EnviaBytesI2C(0x5864,0x11);
Camara_EnviaBytesI2C(0x5865,0x18);
Camara_EnviaBytesI2C(0x5866,0x18);
Camara_EnviaBytesI2C(0x5867,0x19);
Camara_EnviaBytesI2C(0x5868,0x17);
Camara_EnviaBytesI2C(0x5869,0x19);
Camara_EnviaBytesI2C(0x586a,0x16);
Camara_EnviaBytesI2C(0x586b,0x13);
Camara_EnviaBytesI2C(0x586c,0x13);
Camara_EnviaBytesI2C(0x586d,0x12);
Camara_EnviaBytesI2C(0x586e,0x13);
Camara_EnviaBytesI2C(0x586f,0x16);
Camara_EnviaBytesI2C(0x5870,0x14);
Camara_EnviaBytesI2C(0x5871,0x12);
Camara_EnviaBytesI2C(0x5872,0x10);
Camara_EnviaBytesI2C(0x5873,0x11);
Camara_EnviaBytesI2C(0x5874,0x11);
Camara_EnviaBytesI2C(0x5875,0x16);
Camara_EnviaBytesI2C(0x5876,0x14);
Camara_EnviaBytesI2C(0x5877,0x11);
Camara_EnviaBytesI2C(0x5878,0x10);
Camara_EnviaBytesI2C(0x5879,0x0f);
Camara_EnviaBytesI2C(0x587a,0x10);
Camara_EnviaBytesI2C(0x587b,0x14);
Camara_EnviaBytesI2C(0x587c,0x13);
Camara_EnviaBytesI2C(0x587d,0x12);
Camara_EnviaBytesI2C(0x587e,0x11);
Camara_EnviaBytesI2C(0x587f,0x11);
Camara_EnviaBytesI2C(0x5880,0x12);
Camara_EnviaBytesI2C(0x5881,0x15);
Camara_EnviaBytesI2C(0x5882,0x14);
Camara_EnviaBytesI2C(0x5883,0x15);
Camara_EnviaBytesI2C(0x5884,0x15);
Camara_EnviaBytesI2C(0x5885,0x15);
Camara_EnviaBytesI2C(0x5886,0x13);
Camara_EnviaBytesI2C(0x5887,0x17);
Camara_EnviaBytesI2C(0x3710,0x10);
Camara_EnviaBytesI2C(0x3632,0x51);
Camara_EnviaBytesI2C(0x3702,0x10);
Camara_EnviaBytesI2C(0x3703,0xb2);
Camara_EnviaBytesI2C(0x3704,0x18);
Camara_EnviaBytesI2C(0x370b,0x40);
Camara_EnviaBytesI2C(0x370d,0x03);
Camara_EnviaBytesI2C(0x3631,0x01);
Camara_EnviaBytesI2C(0x3632,0x52);
Camara_EnviaBytesI2C(0x3606,0x24);
Camara_EnviaBytesI2C(0x3620,0x96);
Camara_EnviaBytesI2C(0x5785,0x07);
Camara_EnviaBytesI2C(0x3a13,0x30);
Camara_EnviaBytesI2C(0x3600,0x52);
Camara_EnviaBytesI2C(0x3604,0x48);
Camara_EnviaBytesI2C(0x3606,0x1b);
Camara_EnviaBytesI2C(0x370d,0x0b);
Camara_EnviaBytesI2C(0x370f,0xc0);
Camara_EnviaBytesI2C(0x3709,0x01);
Camara_EnviaBytesI2C(0x3823,0x00);
Camara_EnviaBytesI2C(0x5007,0x00);
Camara_EnviaBytesI2C(0x5009,0x00);
Camara_EnviaBytesI2C(0x5011,0x00);
Camara_EnviaBytesI2C(0x5013,0x00);
Camara_EnviaBytesI2C(0x519e,0x00);
Camara_EnviaBytesI2C(0x5086,0x00);
Camara_EnviaBytesI2C(0x5087,0x00);
Camara_EnviaBytesI2C(0x5088,0x00);
Camara_EnviaBytesI2C(0x5089,0x00);
Camara_EnviaBytesI2C(0x302b,0x00);
Camara_EnviaBytesI2C(0x3503,0x07);
Camara_EnviaBytesI2C(0x3011,0x07);
Camara_EnviaBytesI2C(0x350c,0x04);
Camara_EnviaBytesI2C(0x350d,0x58);
Camara_EnviaBytesI2C(0x3621,0x09);
Camara_EnviaBytesI2C(0x370a,0x80);
Camara_EnviaBytesI2C(0x3803,0x0a);
Camara_EnviaBytesI2C(0x3804,0x07);
Camara_EnviaBytesI2C(0x3805,0x80);
Camara_EnviaBytesI2C(0x3806,0x04);
Camara_EnviaBytesI2C(0x3807,0x38);
Camara_EnviaBytesI2C(0x3808,0x07);
Camara_EnviaBytesI2C(0x3809,0x80);
Camara_EnviaBytesI2C(0x380a,0x04);
Camara_EnviaBytesI2C(0x380b,0x38);
Camara_EnviaBytesI2C(0x380c,0x09);
Camara_EnviaBytesI2C(0x380d,0xd6);
Camara_EnviaBytesI2C(0x380e,0x04);
Camara_EnviaBytesI2C(0x380f,0x58);
Camara_EnviaBytesI2C(0x3810,0xc2);
Camara_EnviaBytesI2C(0x3818,0xc8);
Camara_EnviaBytesI2C(0x381c,0x11);
Camara_EnviaBytesI2C(0x381d,0xba);
Camara_EnviaBytesI2C(0x381e,0x04);
Camara_EnviaBytesI2C(0x381f,0x48);
Camara_EnviaBytesI2C(0x3820,0x04);
Camara_EnviaBytesI2C(0x3821,0x18);
Camara_EnviaBytesI2C(0x3824,0x11);
Camara_EnviaBytesI2C(0x3a08,0x14);
Camara_EnviaBytesI2C(0x3a09,0xe0);
Camara_EnviaBytesI2C(0x3a0a,0x11);
Camara_EnviaBytesI2C(0x3a0b,0x60);
Camara_EnviaBytesI2C(0x3a0d,0x04);
Camara_EnviaBytesI2C(0x3a0e,0x03);
Camara_EnviaBytesI2C(0x401c,0x06);
Camara_EnviaBytesI2C(0x5682,0x07);
Camara_EnviaBytesI2C(0x5683,0x60);
Camara_EnviaBytesI2C(0x5686,0x04);
Camara_EnviaBytesI2C(0x5687,0x1c);
Camara_EnviaBytesI2C(0x5001,0x7f);
Camara_EnviaBytesI2C(0x589b,0x00);
Camara_EnviaBytesI2C(0x589a,0xc0);
Camara_EnviaBytesI2C(0x3503,0x00);
Camara_EnviaBytesI2C(0x3010,0x10);
  
}

void Camara_Set_QSXGAcapt_YUVmode(void)
{
    Camara_EnviaBytesI2C(0x3503 , 0x7 );
    Camara_EnviaBytesI2C(0x3000 , 0x0 );
    Camara_EnviaBytesI2C(0x3001 , 0x0 );
    Camara_EnviaBytesI2C(0x3002 , 0x0 );
    Camara_EnviaBytesI2C(0x3003 , 0x0 );
    Camara_EnviaBytesI2C(0x3005 , 0xff);
    Camara_EnviaBytesI2C(0x3006 , 0xff);
    Camara_EnviaBytesI2C(0x3007 , 0x3f);
    Camara_EnviaBytesI2C(0x350c , 0x7 );
    Camara_EnviaBytesI2C(0x350d , 0xd0);
    Camara_EnviaBytesI2C(0x3602 , 0xe4);
    Camara_EnviaBytesI2C(0x3612 , 0xac);
    Camara_EnviaBytesI2C(0x3613 , 0x44);
    Camara_EnviaBytesI2C(0x3621 , 0x27);
    Camara_EnviaBytesI2C(0x3622 , 0x8 );
    Camara_EnviaBytesI2C(0x3623 , 0x22);
    Camara_EnviaBytesI2C(0x3604 , 0x60);
    Camara_EnviaBytesI2C(0x3705 , 0xda);
    Camara_EnviaBytesI2C(0x370a , 0x80);
    Camara_EnviaBytesI2C(0x3801 , 0x8a);
    Camara_EnviaBytesI2C(0x3803 , 0xa );
    Camara_EnviaBytesI2C(0x3804 , 0xa );
    Camara_EnviaBytesI2C(0x3805 , 0x20);
    Camara_EnviaBytesI2C(0x3806 , 0x7 );
    Camara_EnviaBytesI2C(0x3807 , 0x98);
    Camara_EnviaBytesI2C(0x3808 , 0xa );
    Camara_EnviaBytesI2C(0x3809 , 0x20);
    Camara_EnviaBytesI2C(0x380a , 0x7 );
    Camara_EnviaBytesI2C(0x380b , 0x98);
    Camara_EnviaBytesI2C(0x380c , 0xc );
    Camara_EnviaBytesI2C(0x380d , 0x80);
    Camara_EnviaBytesI2C(0x380e , 0x7 );
    Camara_EnviaBytesI2C(0x380f , 0xd0);
    Camara_EnviaBytesI2C(0x3810 , 0xc2);
    Camara_EnviaBytesI2C(0x3815 , 0x1 );
    Camara_EnviaBytesI2C(0x3818 , 0xc0);
    Camara_EnviaBytesI2C(0x3824 , 0x1 );
    Camara_EnviaBytesI2C(0x3827 , 0xa );
    Camara_EnviaBytesI2C(0x3a00 , 0x78);
    Camara_EnviaBytesI2C(0x3a0d , 0x10);
    Camara_EnviaBytesI2C(0x3a0e , 0xd );
    Camara_EnviaBytesI2C(0x3a10 , 0x32);
    Camara_EnviaBytesI2C(0x3a1b , 0x40);
    Camara_EnviaBytesI2C(0x3a1e , 0x2e);
    Camara_EnviaBytesI2C(0x3a11 , 0xd0);
    Camara_EnviaBytesI2C(0x3a1f , 0x40);
    Camara_EnviaBytesI2C(0x3a00 , 0x78);
    Camara_EnviaBytesI2C(0x460b , 0x37);
    Camara_EnviaBytesI2C(0x471d , 0x5 );
    Camara_EnviaBytesI2C(0x4713 , 0x2 );
    Camara_EnviaBytesI2C(0x471c , 0xd0);
    Camara_EnviaBytesI2C(0x5682 , 0xa );
    Camara_EnviaBytesI2C(0x5683 , 0x20);
    Camara_EnviaBytesI2C(0x5686 , 0x7 );
    Camara_EnviaBytesI2C(0x5687 , 0x98);
    Camara_EnviaBytesI2C(0x5001 , 0x1 );
    Camara_EnviaBytesI2C(0x589b , 0x0 );
    Camara_EnviaBytesI2C(0x589a , 0xc0);
    Camara_EnviaBytesI2C(0x4407 , 0xc );
    Camara_EnviaBytesI2C(0x589b , 0x0 );
    Camara_EnviaBytesI2C(0x589a , 0xc0);
    Camara_EnviaBytesI2C(0x3002 , 0x0 );
    Camara_EnviaBytesI2C(0x3002 , 0x0 );
    Camara_EnviaBytesI2C(0x3503 , 0x0 );
    Camara_EnviaBytesI2C(0x3010 , 0x10);
    Camara_EnviaBytesI2C(0x3009 , 0x1 );
    Camara_EnviaBytesI2C(0x300a , 0x56);
}


void blanco_negro(void)
{
    /*NIGHT MODE CAMERA*/
//    Camara_EnviaBytesI2C(0x3011 ,0x08);
//    Camara_EnviaBytesI2C(0x3012 ,0x00);
//    Camara_EnviaBytesI2C(0x3010 ,0x70);
//    Camara_EnviaBytesI2C(0x460c ,0x22);
//    Camara_EnviaBytesI2C(0x380c ,0x0c);
//    Camara_EnviaBytesI2C(0x380d ,0x80);
//    Camara_EnviaBytesI2C(0x3a00 ,0x78);
//    Camara_EnviaBytesI2C(0x3a08 ,0x09);
//    Camara_EnviaBytesI2C(0x3a09 ,0x60);
//    Camara_EnviaBytesI2C(0x3a0a ,0x07);
//    Camara_EnviaBytesI2C(0x3a0b ,0xd0);
//    Camara_EnviaBytesI2C(0x3a0d ,0x08);
//    Camara_EnviaBytesI2C(0x3a0e ,0x06);
    
    /*NIGHT MODE CAMERA AUTO*/
//    Camara_EnviaBytesI2C(0x3011 ,0x08);
//    Camara_EnviaBytesI2C(0x3012 ,0x00);
//    Camara_EnviaBytesI2C(0x3010 ,0x10);
//    Camara_EnviaBytesI2C(0x460c ,0x22);
//    Camara_EnviaBytesI2C(0x380c ,0x0c);
//    Camara_EnviaBytesI2C(0x380d ,0x80);
//    Camara_EnviaBytesI2C(0x3a00 ,0x7c);
//    Camara_EnviaBytesI2C(0x3a08 ,0x09);
//    Camara_EnviaBytesI2C(0x3a09 ,0x60);
//    Camara_EnviaBytesI2C(0x3a0a ,0x07);
//    Camara_EnviaBytesI2C(0x3a0b ,0xd0);
//    Camara_EnviaBytesI2C(0x3a0d ,0x08);
//    Camara_EnviaBytesI2C(0x3a0e ,0x06);
//    Camara_EnviaBytesI2C(0x3a03 ,0xfa);
    
    /*BLANCO Y NEGRO*/
    Camara_EnviaBytesI2C(0x5001 ,0xff);
    Camara_EnviaBytesI2C(0x5580 ,0x18);
    Camara_EnviaBytesI2C(0x5585 ,0x80);
    Camara_EnviaBytesI2C(0x5586 ,0x80);
    
//    /*SATURACION A BLANCO Y NEGRO*/
//    Camara_EnviaBytesI2C(0x5001 ,0xff);
//    Camara_EnviaBytesI2C(0x5584 ,0x00);
//    Camara_EnviaBytesI2C(0x5580 ,0x02);
    
    /*MAYOR APERTURA DEL LENTE*/
    Camara_EnviaBytesI2C(0x3a0f ,0x60);
    Camara_EnviaBytesI2C(0x3a10 ,0x58);
    Camara_EnviaBytesI2C(0x3a11 ,0xa0);
    Camara_EnviaBytesI2C(0x3a1b ,0x60);
    Camara_EnviaBytesI2C(0x3a1e ,0x58);
    Camara_EnviaBytesI2C(0x3a1f ,0x20);
}

