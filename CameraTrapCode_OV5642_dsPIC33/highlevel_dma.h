/* 
 * File:   DMA0_highlevel.h
 * Author: HENDRIX
 *
 * Created on 11 de mayo de 2016, 10:43 AM
 */

#ifndef DMA0_HIGHLEVEL_H
#define	DMA0_HIGHLEVEL_H

#include <xc.h>
#include <stdint.h>

#include "integer.h"

void DMA0_SetConfig(const BYTE * buff);
void DMA1_SetConfig(void);
void DMA_ForceTransfer(const BYTE * buff);
void DMA_SetDisable(void);
        
#endif	/* DMA0_HIGHLEVEL_H */

