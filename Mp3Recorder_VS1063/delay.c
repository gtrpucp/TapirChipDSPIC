
#include "delay.h"  

void delay_ms(uint16_t t)
{
    while( t != 0){
        __delay_ms(1);
        t--;
    }
}

void delay_us(uint16_t t)
{
    while( t != 0){
        __delay_us(1);
        t--;
    }
}