/*
 * File:   newmainXC16.c
 * Author: HENDRIX
 *
 * Created on 22 de marzo de 2016, 04:24 PM
 */

#include <xc.h>
#include <stdint.h>
#include "pic_io.h"
#include "fuses.h"
#include "user.h"
#include "player.h"

#include "uart.h"

// 
ttick ttLed  = {0, TIME_SECOND/2};
extern char file_audio1[];
extern char file_audio2[];

uint8_t statButton;
char *pStr;

void buzzer_error();
void buzzer_ok();

int main(void) {
        
    CPU_Init();
    InicializaTIMER1();     // Para conteo de tiempo cada ~1ms
    InicializaSPI1();
    InicializaUART1();
    InicializaIO();

    delay_ms(10);

    INTCON2bits.GIE = 1; //enable all interrupts
    

    if(SDCard_init() == 1){
        printf("Error microSD\r\n");
        buzzer_error();
        while(1);
    }
    else{
        printf("SDCARD INICIALIZADA\r\n");
    }
    
    if(VSTestInitHardware() || VSTestInitSoftware()){        
        printf("Falla en inicializacion de VS1063\r\n");
        buzzer_error();
    }
    else{        
        printf("Inicializacion de VS1063 OK\r\n");
        buzzer_ok();
    }
    
    
    while (1) {  
        
        VSTestOffHardware();    // Apagamos el modulo
        habilitaIntExterna();   // Habilitamos interrupcion externa
        Sleep();                // Pasamos a modo Sleep
        deshabilitaIntExterna();    //Despertamos
        InicializaTIMER1();     // Reinicio de timer 1 
        if(VSTestInitHardware() || VSTestInitSoftware()){        
            printf("Falla en inicializacion de VS1063\r\n");
            buzzer_error();
        }
        
//        /* Reproduccion de audio */
//            buzzer_ok();
//            TaskVSPlayer(file_audio2);
        
        buzzer_ok();
        ini_test_button();
        
        Rec_CreateDir();        // Crea o abre la carpeta RECORD
        Rec_SetNumFile();       // Establece el numero de grabacion
        pStr = Rec_SetNameFile();   // Establece el nombre del archivo nuevo de grabacion          
        TaskVSRecord(pStr, 48000, 160);    // Manda a hacer una grabacion

        delay_ms(200);
        // El buzzer nos indica que se termino una grabacion
        buzzer_ok();
        delay_ms(20);
        buzzer_ok();
        delay_ms(200);
    }

    return 0;
}


void buzzer_error()
{
    BUZZER = 1;        
    delay_ms(1000);
    BUZZER = 0; 
}

void buzzer_ok()
{
    BUZZER = 1;        
    delay_ms(20);
    BUZZER = 0;
}