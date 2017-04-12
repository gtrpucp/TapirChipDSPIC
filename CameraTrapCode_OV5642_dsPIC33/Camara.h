/* 
 * File:   Camara.h
 * Author: HENDRIX
 *
 * Created on 21 de abril de 2016, 01:14 PM
 */

#ifndef CAMARA_H
#define	CAMARA_H

#include <xc.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "pines.h"
#include "delayms.h"
#include "diskio.h"
#include "ff.h"

#include "highlevel_i2c2.h"
#define HDByteWriteI2C  HDByteWriteI2C2
#define HDByteReadI2C   HDByteReadI2C2
#define CAM_ADDRESS 0x78

#include "highlevel_dma.h"

struct sensor_reg {
	uint16_t reg;
	uint16_t val;
};

void Camara_EnviaBytesI2C(uint16_t dir, uint8_t dat);
void Temp_Init(void);
void MetaData(void);
DWORD tiempoM(void);
void Camara_Init(void);
uint8_t CamaraON(void);
uint8_t Camara_TomarFoto(void);
//DWORD get_fattime (void);
void Camara_StartCapture(void);
void Camara_StopCapture(void);

void Camara_Set_QVGAprev_YUVmode(void);
void Camara_Set_VGAprev_YUVmode(void);
void Camara_Set_SVGAprev_YUVmode(void);
void Camara_Set_CIFprev_YUVmode(void);
void Camara_Set_QCIFprev_YUVmode(void);        
void Camara_Set_QSXGAcapt_JPEGmode(void);        
void Camara_Set_QSXGAcaptVGA_JPEGmode(void);
void Camara_Set_QSXGAcaptSXGA_JPEGmode(void);
void Camara_Set_QSXGAcaptQVGA_JPEGmode(void);        
void Camara_Set_QSXGAcaptQXGA_JPEGmode(void);
void Camara_Set_QSXGAcaptUXGA_JPEGmode(void);        
void Camara_Set_QSXGAcaptXGA_JPEGmode(void);
void CamaraSet_YUV_to_JPEG(void);
void CamaraSet_JPEG_to_YUV(void);

void Camara_Set_HighResVideo(void);
void Camara_Set_HighResVideo2(void);
void Camara_Set_HighResVideo3(void);

void DelFabricante(void);
void blanco_negro(void);
void Camara_Set_QSXGAcapt_YUVmode(void);

#endif	/* CAMARA_H */