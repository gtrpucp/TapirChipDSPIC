
#include "timer_tick.h"

// Variable que puede ser modificada en interrupcion
volatile ttick timerTick;

/*
void tTick_ISR()
{
    if(PIR1bits.TMR2IF && PIE1bits.TMR2IE){
       timerTick.cnt++;
       PIR1bits.TMR2IF = 0;
    }
}
*/

// Configurar para obtener una temporizacion de 1ms
void ttick_config()
{
    
}
void ttick_ini(ttick *tt, uint16_t m)
{
    if(m != 0)
        tt->max = m;
    tt->cnt = timerTick.cnt;
}

uint8_t ttick_test(ttick *tt)
{
    ttick ttAct;
   
   ttAct.cnt=timerTick.cnt;
   
   if((ttAct.cnt - tt->cnt >= tt->max))
   {
      tt->cnt=ttAct.cnt;
      return 1;
   }
   else
      return 0;
}
