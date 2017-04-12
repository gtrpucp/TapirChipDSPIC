
#include <xc.h>
#include <stdio.h>
#include "pic_io.h"
#include "timer_tick.h"
#include "uart.h"

// temprizador para el button 1
ttick ttButton = {0, TIME_SECOND/10};
static uint8_t estButton = 0;

void ini_test_button(){
    estButton = 0;
}

int test_button(){

    
    switch(estButton){
        case 0:
            ttick_ini(&ttButton, 0);
            estButton++;
            break;
        case 1:
            if(ttick_test(&ttButton)){
                if(TEST_BUTTON){
//                    printf("1\r\n");
                    estButton = 2;
                }
                else{
//                    printf("0\r\n");
                    return 2;
                }
            }
            break;
        case 2:
            if(ttick_test(&ttButton)){
                if(!TEST_BUTTON){
//                    printf("2\r\n");
                    estButton = 1;
                    return 1;
                }
            }
            break;
        default: estButton = 0;    
    }
    
    return 0;
}

void SaveUIState()
{
    char dummy;
    
    while(Uart_rxAvailable()){
        dummy = Uart_getc();
    }
}

void RestoreUIState()
{
    asm("nop");
}

int GetUICommand()
{
    char data;
    
    if(test_button() == 1){
        return 'q';
    }
   
    if(Uart_rxAvailable()){
        data = Uart_getc();
        Uart_putc(data);
        return data;
    }
    
    return -1;
}

int AskUICommand()
{
    return Uart_rxAvailable();
}