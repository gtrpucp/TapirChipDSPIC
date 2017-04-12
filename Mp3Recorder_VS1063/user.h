/* 
 * File:   user.h
 * Author: admin
 *
 * Created on 7 de noviembre de 2016, 02:39 PM
 */

#ifndef USER_H
#define	USER_H

#include <xc.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "pic_io.h"
#include "delay.h"
#include "diskio.h"
#include "ff.h"
#include "timer_tick.h"


// Comentar si no se va a usar la depuracion por puerto serial
#define DEBUG_UART

void CPU_Init();
void InicializaUART1(void);
void InicializaTIMER1(void);

uint8_t SDCard_init(void);
void InicializaIO(void);
void InicializaSPI1(void);

int Rec_CreateDir();
uint16_t Rec_SetNumFile();
char *Rec_SetNameFile();

void habilitaIntExterna();
void deshabilitaIntExterna();

#endif	/* USER_H */