
#include "delayms.h"

void DelayMs(uint16_t tms) {
    uint16_t i;
    
    for(i = 0; i < tms; i++) {
        __delay_ms(1);
    }                
}

void DelayUs(uint16_t tms) {
    uint16_t i;
    
    for(i = 0; i < tms; i++) {
        __delay_us(1);
    }                
}
