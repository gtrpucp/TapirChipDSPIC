#include "xc.h"
#include "stdint.h"
#include "pines.h"
#include "fuses.h"
#include "delayms.h"
#include "Camara.h"
#include "ds3231.h"
#include "user.h"

extern uint8_t flag_UartComando;
int PIR_Detect = 0; //Variable set to 1 when Interruption from PIR detected

#define ENERGY_SAVING
//#define CAMERA
#define DEBUG

void CPU_Init(void) {
    
    RCONbits.VREGS = 1;
    RCONbits.VREGSF = 1;
    
    // Sequence to set/clear IOLOCK for controlling Peripheral Pin Select (PPS))
    OSCCONL = 0x46;
    OSCCONL = 0x57;
    OSCCONbits.IOLOCK = 0; //PPS lock bit is not locked, writes to PPS are allowed

    //Configurando el system clock Fosc = 119762500Hz, Fcy = Fp = Fosc/2
    CLKDIVbits.DOZEN = 0; //Force Fcy divided by 1
    CLKDIVbits.FRCDIV = 0b000; //Internal Rast RC postcaler divided by 1
    CLKDIVbits.PLLPOST = 0; //N2 = 
    CLKDIVbits.PLLPRE = 0; //N1 = 2
    PLLFBDbits.PLLDIV = 63; //M = 65
    
    while(!OSCCONbits.LOCK);
    
    ANSELA &= ~(0x1813);
    ANSELB &= ~(0x010F);
    ANSELC &= ~(0x0807);
    TRISA |= 0x1F93;
    TRISB |= 0xFFFF;
    TRISC |= 0xBFFF;
    ANSELAbits.ANSA1 = 1;
    
}

int Read_voltage(void)
{
    if(MAX17043_ReadVoltage()<(double)MIN_BATT_V){
        TEST_LED = 1 ;
        DelayMs(300);
        TEST_LED = 0 ;
        DelayMs(300);
        TEST_LED = 1 ;
        DelayMs(300);
        printf("Battery Voltage is %f \r\n",(double)MAX17043_ReadVoltage());
        return 1;
    }else{
        printf("Battery Voltage is %f \r\n",(double)MAX17043_ReadVoltage());
        return 0;
    }
}

int main(void) {
    
    uint8_t error;
//    uint16_t a = 0;
    CPU_Init();
    Camara_Init();  
    MAX17043_Init();    //Initializes the batter voltage tester
   
//    setTime(11,36,10,AM,_24_hour_format);
//    setDate(TUE,24,1,17);

//    ClearFlagAlarm();
//    EnableAlarm();
//    setAlarm(17,2,0,19);
//    setAlarmEveryMin();
    
    PIR_IntConfig();
    
#ifdef DEBUG
    printf("Inicializo correctamente\r\n");
#endif
  
#ifdef ENERGY_SAVING  
    TAKING_PHOTO = 0;
#ifdef DEBUG
    printf("Estado del PIR: %d\r\n", PIR);
#endif
    CNPDBbits.CNPDB7 = 1;
    while(PIR);
#ifdef DEBUG
    printf("Estado del PIR: %d\r\n", PIR);
#endif
    PIR_Detect=0;
    printf("File: %s LINE: %d Me voy a dormir\r\n", __FILE__, __LINE__);
    DelayMs(100);
    AhorroEnergia();

    while(1)
    {
        //asm("PWRSAV #0x0");
        Sleep();
        if(TX_IN_PROGRESS && PIR_Detect){
        //if(PIR_Detect){
            NoAhorroEnergia();
            //TAKING_PHOTO = 1;
            if(!CamaraON()){
#ifdef DEBUG
                printf("File: %s LINE: %d Estoy despierto\r\n", __FILE__, __LINE__);
#endif
                DelayMs(2700);
//              FLASH = 1;
                if(Read_voltage() == 0){
                    error = Camara_TomarFoto();
                    DelayMs(100);
                    error = Camara_TomarFoto();
                }
    //            FLASH = 0;
            }
#ifdef DEBUG
            else {
                printf("File: %s Line: %d Error while turning on\r\n", __FILE__, __LINE__);
            }
#endif
            TAKING_PHOTO = 0;
            AhorroEnergia();
            while(PIR);
            PIR_Detect=0;
        }
    }
#endif

#ifdef CAMERA
    PIR_Detect = 0;
    while (1) {
//        if(TX_IN_PROGRESS){
            if ((TRIGGER_BUTTON == 0)) {
                PIR_Detect = 0;
                TAKING_PHOTO = 1;
//                if (TX_IN_PROGRESS) {  //To assure that the ESP8266 is not using SD 
                    error = Camara_TomarFoto();
                    error = Camara_TomarFoto();
//                    while(PIR);
                    DelayMs(100);
                    TAKING_PHOTO = 0;
//                }
            
            }
//        }
    }
#endif
    
//    TRIS_FLASH = 0;
//    while(1){
//        DelayMs(1000);
////        tiempoM();
////        DS3231_Read(0x0A);
//        MAX17043_ReadSoC();
////        FLASH = 1;
////        DelayMs(1000);
////        FLASH = 0;
//    }
//    initADC();
//    while(1)
//    {
//        a = readADC();
//        printf("lectura de la fotoresistencia: %d \r\n",a);
//        DelayMs(500);
//    }
    return 0;
}