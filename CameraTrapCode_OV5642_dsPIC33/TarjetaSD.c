
#include "TarjetaSD.h"
#include "Camara.h"

#include <stdio.h>

extern uint16_t tiemposMS[1024];
extern uint16_t tiemposMSIndex;;
extern uint32_t tiempoMSTotal;
extern float tiempoMSRes;
extern uint8_t tiempoSalida[];

extern BYTE app_state;
extern DSTATUS Stat;

BYTE ffs_card_ok = 0;
BYTE sm_ffs_process = FFS_PROCESS_NO_CARD;
volatile uint16_t ffs_10ms_timer = 0;



extern uint8_t SWITCH_1_NEW_PRESS;
extern uint8_t task_updatelcd;

//FATFS FatFs;		/* FatFs work area needed for each volume */
//FIL Fil;			/* File object needed for each open file */
//FRESULT Result;

//uint8_t useDmaNow = 0;

extern uint8_t flag_UartAck;
extern uint8_t flag_UartStart;
extern void EnviaStringUART1(uint8_t* buffer);

void TaskTarjetaSD(void) {
    //----- PROCESS MODE -----
    process_mode();
    //----- PROCESS FAT FILING SYSTEM -----
    ffs_process();

}

//**********************************
//**********************************
//********** PROCESS MODE **********
//**********************************
//**********************************




void process_mode(void) {

    //uint32_t checksum = 0;
    //static uint8_t temp = 0, temp_last = 0;
    //static uint8_t res;
    
    //uint8_t aux, aux2;
    //uint16_t port;
    
    //QSXGA - JPEG
    //static uint16_t PHOTO_WIDTH  = 800;
    //static uint16_t PHOTO_HEIGHT = 600; 
    //static uint16_t PHOTO_BYTES_PER_PIXEL = 2;
    //SVGA - YUV
    //static uint16_t PHOTO_WIDTH  = 800;
    //static uint16_t PHOTO_HEIGHT = 600; 
    //static uint16_t PHOTO_BYTES_PER_PIXEL = 2;
    //VGA - YUV
    //static uint16_t PHOTO_WIDTH  = 640;
    //static uint16_t PHOTO_HEIGHT = 480; 
    //static uint16_t PHOTO_BYTES_PER_PIXEL = 2;
    //QVGA - YUV
    //static uint16_t PHOTO_WIDTH  = 320;
    //static uint16_t PHOTO_HEIGHT = 240; 
    //static uint16_t PHOTO_BYTES_PER_PIXEL = 2;
    //CIF - YUV
    //static uint16_t PHOTO_WIDTH  = 352;
    //static uint16_t PHOTO_HEIGHT = 288; 
    //static uint16_t PHOTO_BYTES_PER_PIXEL = 2;
    //QCIF - YUV
    //static uint16_t PHOTO_WIDTH  = 176;
    //static uint16_t PHOTO_HEIGHT = 144; 
    //static uint16_t PHOTO_BYTES_PER_PIXEL = 2;
    
    /*static uint16_t height;
    static uint16_t width;
    static uint16_t bytenumber;
    static uint32_t bytecount;
    
    static uint8_t flag_fifoStart = 0;
    static uint8_t flag_fifoFilling = 0;
    
    uint16_t bw;*/

    //----- IF CARD IS REMOVED ENSURE WE RESET BACK WAITING FOR CARD TO BE INSERTED -----
    if ((app_state != TA_PROCESS_WAIT_FOR_CARD) && (!ffs_card_ok)) {
        app_state = TA_PROCESS_WAIT_FOR_CARD;
        task_updatelcd = 1;
    }
    //---------------------------------
    //----- PROCESS STATE MACHINE -----
    //---------------------------------
    switch (app_state) {
        case TA_PROCESS_WAIT_FOR_CARD:
            //-------------------------------------------
            //----- WAITING FOR CARD TO BE INSERTED -----
            //-------------------------------------------
            if (ffs_card_ok) {
                //A CARD HAS BEEN INSERTED AND IS READY TO ACCESS
                app_state = TA_PROCESS_CARD_INSERTED;
                task_updatelcd = 1;
            }
            break;

        case TA_PROCESS_CARD_INSERTED:
            //----------------------------
            //----- CARD IS INSERTED -----
            //----------------------------
            Nop();
            break;

        case TA_PROCESS_DELETE_AND_CREATE_FILE:
            /*
            //-------------------------------------------------
            //----- CREATE NEW FILES -----
            //-------------------------------------------------

            //----- CREATE NEW FILE FOR WRITING -----
            Result = f_open(&Fil, (const TCHAR*)file_CamNombre, FA_WRITE | FA_CREATE_ALWAYS);
            
            if (Result == FR_OK) {
                //----- FILE WAS SUCESSFULLY CREATED -----
                app_state = TA_PROCESS_FILL_FILE;
                task_updatelcd = 1;
                
                flag_fifoStart = 1;
                flag_fifoFilling = 1;
                
            } else {
                //----- ERROR - THE FILE COULD NOT BE CREATED -----
                app_state = TA_PROCESS_ERROR;
                task_updatelcd = 1;
                break;
                
            }
            */
            break;

        case TA_PROCESS_FILL_FILE:
            break;
            /*
            //-----------------------------------------------------------------------------
            //----- FILL FILE - LOAD DATA TO OPENED FILE FROM FIFO BUFFER -----
            //-----------------------------------------------------------------------------
            if (flag_fifoStart) {
                flag_fifoStart = 0;

                //Habilitamos el puerto de lectura del FIFO
                MEM_OE = 0; //hablita la salida
                MEM_RCLK = 0;
                MEM_RCLK = 1;                              
                
                MEM_RRST = 0;                
                MEM_RCLK = 0;                                
                MEM_RCLK = 1;
                
                MEM_RRST = 1;
                
                port = PORTB;                    
                port = port >> 8;
                aux = (uint8_t)port;

                //res = 1;
                //index_CamCapture = 0;
                //bytecount = 0;
                                    
                if (aux != 0xFF) {
                    //Tenemos que insertar el primer byte JPEG
                    buff_CamCapture[0] = 0xFF;                    
                    res = 1;
                    index_CamCapture = 1;
                    bytecount = 1;                    
                    
                } else {                
                    res = 1;
                    index_CamCapture = 0;
                    bytecount = 0;                    
                    
                }
                inicio_CamCapture = 0;

            } else if (flag_fifoFilling) {
                
                if (res) {
                  
                    do{
                    port = PORTB;                    
                    port = port >> 8;
                    aux = (uint8_t)port;

                    MEM_RCLK = 0;                    
                    MEM_RCLK = 1;
                                        
                    temp_last = temp;
                    temp = aux;
                    
                    buff_CamCapture[index_CamCapture++] = aux;
                    bytecount++; 
                    
                    //if (((temp == 0xFF) && (temp_last == 0xD8)) && (bytecount == 3)) {
                    if (((temp_last == 0xD8) && (temp == 0xFF)) && (bytecount == 3)) {
                    //if (!((temp == 0xFF) && (temp_last == 0xD8)) && (bytecount == 3)) {
                        inicio_CamCapture = 1;
                        //res = 0;
                    }
                    //} else if (index_CamCapture >= CAM_BUFF) {
                    if (index_CamCapture >= CAM_BUFF) {
                        index_CamCapture = 0;
                        //useDma = 1;
                        
                        if (inicio_CamCapture == 0){
                            res = 0;
                            width = 10;
                            height = 8;
                            break;
                        }
                        
                        Result = f_write(&Fil, buff_CamCapture, CAM_BUFF, &bw); 
                        useDma = 0;                        
                        
                        if (Result != FR_OK) {
                            res = 0;
                            width = 10;
                            height = 8;
                        }                        
                        if (bytecount == CAM_BUFF) {
                            width = 100;
                            height = 98;
                            
                            //Solo para probar que el reset del puntero de lectura está bien.
                            //#define TEST_FIFO__PUNTERO_LECTURA
#if defined TEST_FIFO__PUNTERO_LECTURA
                            flag_fifoFilling = 1;
                            flag_fifoStart = 1;
                            res = 0;
#endif
                        }
                    }  
                    //} else if ((temp == 0xD9) && (temp_last == 0xFF)) {
                    if ((temp == 0xD9) && (temp_last == 0xFF)) {
                        res = 0;                        
                        //useDma = 1;
                        f_write(&Fil, buff_CamCapture, index_CamCapture, &bw);
                        useDma = 0;                        
                        
                    }
                    
                    } while(res);
                                        
                } else {
                    //Se concluyo con toda la carga del FIFO en la SD card
                    flag_fifoFilling = 0;
                    
                    //Deshabilitamos el puerto de lectura del FIFO
                    MEM_OE = 1;
                    //pulseRCLK(1);
                    
                    //----- CLOSE THE FILE -----                    
                    Result = f_close(&Fil);			
                    
                    if ((Result != FR_OK) || (inicio_CamCapture == 0)) {
                        app_state = TA_PROCESS_ERROR; //Error - could not close
                        task_updatelcd = 1;
                        break;
                    }                  
                                        
                    app_state = TA_PROCESS_CARD_OPERTATION_DONE;
                    task_updatelcd = 1;                                                            
                }
                
            }
            break;
            */

        case TA_PROCESS_CARD_OPERTATION_DONE:
            //-----------------------------------------------------------------------------
            //----- OPERATION DONE - INDICATE SUCCESS AND WAIT FOR CARD TO BE REMOVED -----
            //-----------------------------------------------------------------------------
            break;


        case TA_PROCESS_ERROR:
            //--------------------------------------------------------------------------
            //----- ERROR OCCURED - INDICATE ERROR AND WAIT FOR CARD TO BE REMOVED -----
            //--------------------------------------------------------------------------

            //Try and close the files if they are open
            //ffs_fclose(our_file_0);

            break;


    } //switch (test_application_state)

}


//**********************************
//**********************************
//********** FFS PROCESS ***********
//**********************************
//**********************************

void ffs_process(void) {
    
    DSTATUS estado;
    uint8_t aux;
    
    switch (sm_ffs_process) {
        case FFS_PROCESS_NO_CARD:
            //-------------------------------
            //----- NO CARD IS INSERTED -----
            //-------------------------------
            ffs_card_ok = 0; //Flag that card not OK

            //Has a card has been inserted?
            estado = disk_status(0);                        
            if (estado & STA_NODISK) {
                return;/* No card in the socket */
            }
                
            //A card has been inserted
            //Pause for 500mS seconds
            ffs_10ms_timer = 500;
            sm_ffs_process = FFS_PROCESS_WAIT_FOR_CARD_FULLY_INSERTED;                        
            return;
            
        case FFS_PROCESS_WAIT_FOR_CARD_FULLY_INSERTED:
            //------------------------------------------------------------
            //----- CARD INSERTED - WAIT FOR IT TO BE FULLY INSERTED -----
            //------------------------------------------------------------
            //(To allow for users that don't insert the card in one nice quick movement)

            //Ensure card is still inserted
            estado = disk_status(0);                        
            if (estado & STA_NODISK) {
                sm_ffs_process = FFS_PROCESS_NO_CARD;
                return;/* No card in the socket */
            }
                        
            //Wait for timer to expire
            if (ffs_10ms_timer)
                return;

            //Initialise the card
            sm_ffs_process = FFS_PROCESS_CARD_INITIALSIED;

            //Actually exit this switch statement to run the card initialise procedure below (this is the only state that doesn't return)
            break;
            
        case FFS_PROCESS_CARD_INITIALSIED:
            //-----------------------------------------
            //----- CARD INSERTED AND INITIALSIED -----
            //-----------------------------------------

            //If card is still inserted then exit
            estado = disk_status(0);                        
            if (estado & STA_NODISK) {
                Nop();/* No card in the socket */
            } else {
                return;
            }

            //CARD HAS BEEN REMOVED
            ffs_card_ok = 0;

            sm_ffs_process = FFS_PROCESS_NO_CARD;
            return;
            
        default:
            
            sm_ffs_process = FFS_PROCESS_NO_CARD;
            return;
    }
    //----------------------------------------
    //----------------------------------------
    //----- INITIALISE NEW MMC / SD CARD -----
    //----------------------------------------
    //----------------------------------------
    //(The only state that exits the switch statement above is FFS_PROCESS_WAIT_FOR_CARD_RESET when it completes)
    //disk_initialize();
    aux = 10;
    do {
        Result = f_mount(&FatFs, "", 1);
    } while ((Result != FR_OK)&&(aux--));    
    
	if(Result != FR_OK) {     /* Give a work area to the default drive */
        //printf("Error mounting file: %d\n", Result);
        ffs_card_ok = 0; //Flag that the card is not OK
    }
    else {
        //puts("SD_card mounted");
        ffs_card_ok = 1; //Flag that the card is OK
    }
    
}
