
#include "user.h"
#include "ds3231.h"

//==============================================================================
//Variables estáticas
FATFS FatFs;		/* FatFs work area needed for each volume */
FIL Fil;			/* File object needed for each open file */
DIR Dir;
FRESULT Result;
uint16_t bw;
char buff_file[512];    // Buffer necesario que alamacena la data escrita o leida

uint8_t flag_UartComando = 0;
uint8_t dma0_TxIntFlag = 0;

volatile uint16_t ffs_10ms_timer;
uint8_t buff_DummyRx[2];

const char config_file[] = "cfg.dat";
const char rec_dir[] = "RECORD";
char rec_file[] = "rec00001.mp3"; 
char path_name[20]; 

const char file_audio1[] = "play1.mp3";
const char file_audio2[] = "play2.mp3";
char texto[20];

// variable a ser incrementada dentro de una interrupcion de timer de 1ms
extern ttick timerTick;

//void __attribute__((__interrupt__, no_auto_psv)) _U1RXInterrupt(void) {
//
//    uint8_t dato;
//    static uint8_t buffer[128];
//    static uint8_t index = 0;
//
//    IFS0bits.U1RXIF = 0;
//    
//    if(U1STAbits.FERR){
//        return;
//    }
//    if(U1STAbits.OERR){
//        U1STAbits.OERR = 0;
//        return;
//    }
//    
//    //Trama esperada:
//    dato = U1RXREG;
//    buffer[index++] = dato;
//    
//    if(dato == '!'){
//        if (buffer[0] == '@') {
//            flag_UartComando = 1;
//        }
//        index = 0;
//    }
//}


void __attribute__((__interrupt__, no_auto_psv)) _T1Interrupt(void) {
    
    IFS0bits.T1IF = 0; //Clear Timer1 interrupt flag
    
    timerTick.cnt++;
    //----- FAT FILING SYSTEM DRIVER TIMER -----
    disk_timerproc();
    //----- FAT FILING SYSTEM DRIVER TIMER -----
    if (ffs_10ms_timer) {
        ffs_10ms_timer--;
    }
}

void __attribute__((__interrupt__, no_auto_psv)) _INT1Interrupt(void) {

    IFS1bits.INT1IF = 0; 
}

void CPU_Init(void) {
    
    uint16_t wait = 24000;
    
    ANSELA = 0x0000;
    ANSELB = 0x0000;
    ANSELC = 0x0000;
    TRISA = 0xFFFF;
    TRISB = 0xFFFF;
    TRISC = 0xFFFF;

    OSCCONbits.IOLOCK = 0; //PPS lock bit is not locked, writes to PPS are allowed

    //Configurando el system clock Fosc = 119762500Hz, Fcy = Fp = Fosc/2
    CLKDIVbits.DOZEN = 0; //Force Fcy divided by 1
    CLKDIVbits.FRCDIV = 0b000; //Internal Rast RC postcaler divided by 1
    CLKDIVbits.PLLPOST = 0; //N2 = 
    CLKDIVbits.PLLPRE = 0; //N1 = 2
    PLLFBDbits.PLLDIV = 63; //M = 
    
    while (wait--);
    
}

void InicializaTIMER1(void) {

    //----- SETUP TIMER 1 -----
    T1CON = 0b0000000000000000;
    TMR1 = 0;
    PR1 = 59881;            // 0.99999582mseg ~ 1mseg with ~60MIPS

    IPC0bits.T1IP = 0x01;   // Set Timer 1 Interrupt Priority Level
    IFS0bits.T1IF = 0;      // Clear Timer 1 Interrupt Flag
    IEC0bits.T1IE = 1;      // Enable Timer1 interrupt
    T1CONbits.TON = 1;      // Start Timer
}

void InicializaTIMER2(void) {
    
    T2CON = 0x0020;
    TMR2 = 0;
    PR2 = 14036;        // 15mseg
    
    IFS0bits.T2IF = 0;
    IEC0bits.T2IE = 1;
    
}

void InicializaSPI1(void) {
    //*************************************************************************
    // SPI module Configuration
    // Uncomment your selection for each variable
    // See device data sheet and PIC18 Perpheral Library docs for more details
    //*************************************************************************    
    SPI1STAT = 0b0000000000000000; //enable SPI pins
    SPI1CON2 = 0b0000000000000000; //framed SPI support disabled
    //SPI1CON1 = 0b0000001000111101;  //modo 8 bits, input data sampled at end, 
    //bus mode 00, prescal 1:1, postcal 16:1
    //master mode
    SPI1CON1bits.DISSCK = 0; // Internal serial clock is enabled
    SPI1CON1bits.DISSDO = 0; // SDOx pin is controlled by the module
    SPI1CON1bits.MODE16 = 0; // Communication is word-wide (8 bits)
    SPI1CON1bits.MSTEN = 1; // Master mode enabled
    SPI1CON1bits.SMP = 1; // Input data is sampled at the end of data output time
    SPI1CON1bits.CKE = 0; // Serial output data changes on transition from
    SPI1CON1bits.CKP = 0; // Idle state for clock is a low level;
    //SPI BUS LOW SPEED (Min 100KHz, Max 400KHz)  
    //(Min speed required for ACDM41 initalisation)
    SPI1CON1bits.SPRE = 0b000; //secondary prescal 8:1
    SPI1CON1bits.PPRE = 0b00; //primary prescal 64:1    
    // active state is a high level
    SPI1STATbits.SPIEN = 1; // Enable SPI module    
    
    TRIS_SDCARD_CS = 0;
    SDCARD_CS = 1;
    
}

void InicializaIO(void) {
    
    LATA = 0;
    LATB = 0;
    LATC = 0;
    
    TRISA = 0X0200;     // 0B 0000 0010 0000 0000
    TRISB = 0X4228;        // 0B 0100 0010 0010 1000
    TRISC = 0X0000;
    
    //==========================================================================
    //Pin de test para osciloscopio
    TRIS_PROBE_TEST = 0;
    PROBE_TEST = 0;
    
    //==========================================================================
    //Pines de led monitor y boton trigger
    TRIS_BUZZER = 0;    //pin salida
    BUZZER = 0;         //pin 0v

    TRIS_TEST_BUTTON = 1;   //pin entrada
    PU_TEST_BUTTON = 1;     //enable pull up

    //==========================================================================
    //Pines del IC VS1063
    TRIS_VS_XCS = 0;
    TRIS_VS_XDCS = 0;
    TRIS_VS_DREQ = 1;
    TRIS_VS_XRESET = 0;
    
    //==========================================================================
    RPINR0bits.INT1R = 35;  //PPS INT1 --> RB3/RP35 para el pulsador
    INTCON2bits.INT1EP = 1; //flanco de bajada
    //==========================================================================
    //Interrupciones externa del pulsador
    IEC1bits.INT1IE = 0;
    IFS1bits.INT1IF = 0;
}

void habilitaIntExterna()
{
    INTCON2bits.INT1EP = 1; 
    IFS1bits.INT1IF = 0;
    IEC1bits.INT1IE = 1;
}

void deshabilitaIntExterna()
{ 
    IEC1bits.INT1IE = 0;
    IFS1bits.INT1IF = 0;
    
}

uint8_t SDCard_init(void) {

    DSTATUS estado;
    uint8_t aux;    
    
    //Ensure card is still inserted
    ffs_10ms_timer = 500;
    
    //Wait for timer to expire
    while (ffs_10ms_timer);
    
    estado = disk_status(0);                        
    if (estado & STA_NODISK) {
        //sm_ffs_process = FFS_PROCESS_NO_CARD;
        return 1;/* No card in the socket */
    }

    //----------------------------------------
    //----------------------------------------
    //----- INITIALISE NEW MMC / SD CARD -----
    //----------------------------------------
    //----------------------------------------
    aux = 10;
    do {
        Result = f_mount(&FatFs, "", 1);
    } while ((Result != FR_OK)&&(aux--));    
    
	if(Result != FR_OK) {     /* Give a work area to the default drive */
        return 1;
    }
    else {
        return 0;
    }  
}

//****************************************
// FUNCIONES PARA EL PROGRAMA PRINICIPAL


int Rec_CreateDir()
{
    FILINFO Filinfo;
//    FRESULT Res;
    
    // Revisamos si existe el directorio RECORD
    if( f_stat( rec_dir, &Filinfo) == FR_OK){
        if(f_opendir(&Dir, rec_dir) == FR_OK){
            printf("Carpeta abierta\r\n");
            return 1;
        }
    }
    else{
        f_mkdir(rec_dir);
        if(f_opendir(&Dir, rec_dir) == FR_OK){
            printf("Carpeta abierta\r\n");
            return 1;
        }
    }

    printf("Error al abrir carpeta\r\n");
    return 0;
}

uint16_t Rec_SetNumFile()
{
    uint32_t numRec;
    
    // Creamos el nombre de ruta RECORD/cfg.dat
    strcpy(path_name, rec_dir);
    strcat(path_name, (char *)"/");
    strcat(path_name, config_file);    
    
    printf("%s\r\n", path_name);
    
    // Abrimos o creamos el archivo config
    Result = f_open(&Fil, (const TCHAR*)path_name, FA_WRITE | FA_READ | FA_OPEN_ALWAYS);
    
    // Si el resultado es positivo escribimos en el archivo
    if(Result == FR_OK){    
        printf("Archivo Config abierto\n\r");
        
        // lectura de la memoria guardada
        f_read(&Fil, buff_file, 10, &bw);
        
        // Recuperamos el numero de archivo a guardar
        numRec = atol(&buff_file[4]);
        
        // Si el resultado es distinto de 0
        if(numRec && numRec < 99999){
            numRec++;
        }
        else{
            numRec = 1;
        }
        f_lseek(&Fil, 0);   // movemos el puntero del archivo
        sprintf(buff_file, "MEM=%05lu", numRec);
        f_write(&Fil, buff_file, strlen(buff_file), &bw);
        f_close(&Fil);
        printf("mem = %05lu\r\n", numRec); 
    }
    else{
        printf("Error abriendo archivo Config\n\r");
        return 0;
    }
    
    // Modifica el nombre del archivo REC
    sprintf(texto, "%05lu", numRec);
    
    rec_file[7] = texto[4];
    rec_file[6] = texto[3];
    rec_file[5] = texto[2];
    rec_file[4] = texto[1];
    rec_file[3] = texto[0];
    
    return numRec;
}

char *Rec_SetNameFile()
{
    // Creamos el nombre de ruta RECORD/RECXXX.mp3
    strcpy(path_name, rec_dir);
    strcat(path_name, (char *)"/");
    strcat(path_name, rec_file); 
    
    return path_name;
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