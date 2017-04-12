/* 
 * File:   timer_tick.h
 * Author: Danilo-Pc
 *
 * Created on 16 de septiembre de 2015, 07:44 PM
 * 
 * Contador de tiempo. Usa el timer2 configurado para temporizacion cada 10ms
 * Habilitar interrupcion global en el programa principal
 * Agregar la funcion tTick_ISR() a la funcion de interrupcion
 */

#ifndef TIMER_TICK_H
#define	TIMER_TICK_H

#include <xc.h>
#include <stdint.h>
#include "delay.h"

#define TIME_SECOND 1000UL

// Estructura para el manejo de timers
typedef struct{
    uint16_t cnt;
    uint16_t max;
}ttick;

void ttick_ISR();
void ttick_config();
void ttick_ini(ttick *tt, uint16_t m);
uint8_t ttick_test(ttick *tt);
        
#endif	/* TIMER_TICK_H */

