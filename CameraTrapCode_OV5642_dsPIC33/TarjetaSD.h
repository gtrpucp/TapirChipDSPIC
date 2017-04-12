/* 
 * File:   TarjetaSD.h
 * Author: HENDRIX
 *
 * Created on 10 de abril de 2015, 12:49 PM
 */

#ifndef TARJETASD_H
#define	TARJETASD_H

#include <xc.h>
#include <stdint.h>
#include <string.h>

#include "pines.h"
//----- OTHER PROJECT FILES REQUIRED BY THIS SOURCE CODE FILE -----
//#include "GlobalDataTypeInit.h"
//#include "mem-ffs.h"
//#include "mem-mmcsd.h"

//#include "diskio.h"
//#include "ff.h"

typedef enum _TEST_APPLICATION_STATE {
    TA_PROCESS_WAIT_FOR_CARD,
    TA_PROCESS_CARD_INSERTED,
    TA_PROCESS_DELETE_AND_CREATE_FILE,
    TA_PROCESS_FILL_FILE,
    TA_PROCESS_CARD_OPERTATION_DONE,
    TA_PROCESS_FINISH,
    TA_PROCESS_ERROR,
} TEST_APPLICATION_STATE;

//---------------------------------------------
//----- PROCESS CARD STATE MACHINE STATES -----
//---------------------------------------------
typedef enum _FFS_PROCESS_STATE {
    FFS_PROCESS_NO_CARD,
    FFS_PROCESS_WAIT_FOR_CARD_FULLY_INSERTED,
    FFS_PROCESS_CARD_INITIALSIED
} FFS_PROCESS_STATE;

void TaskTarjetaSD(void);

//uint8_t TarjetaSD_CrearArchivo(void);
uint8_t TarjetaSD_GuardarFoto(void);

void process_mode(void);
void ffs_process(void);

#endif	/* TARJETASD_H */

