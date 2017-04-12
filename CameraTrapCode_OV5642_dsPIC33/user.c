#include "camara.h"
#include "pines.h"
#include "xc.h"
#include "pines.h"
#include "DS3231.h"
#include <p33EP256GP504.h>
#include "user.h"

extern volatile uint8_t flag_UartComando;
volatile uint8_t dma0_TxIntFlag = 0;
extern volatile uint8_t smCamara_Capture;
extern volatile uint8_t Camara_FrameCount;
extern volatile uint8_t flag_CaptureReady;
extern volatile uint8_t time_out;
extern volatile uint16_t ffs_10ms_timer;
extern uint8_t buff_CamCapture[];
extern int PIR_Detect;
uint8_t flag_UartAck = 0;

/**
 * @brief   Uart reception interrupt
 */
void __attribute__((__interrupt__, no_auto_psv)) _U1RXInterrupt(void) {

    uint8_t dato;
    static uint8_t buffer[64];
    static uint8_t index = 0;

    IFS0bits.U1RXIF = 0;
    
    if(U1STAbits.FERR){
        return;
    }
    if(U1STAbits.OERR){
        U1STAbits.OERR = 0;
        return;
    }
    //Trama esperada:
    dato = U1RXREG;
    buffer[index++] = dato;
    
    if(dato == '!'){
        if (buffer[0] == '@') {
            //Verify if command is '01'
            if (buffer[1] == '0' && buffer[2] == '1') {
                //set flag to take picture
                flag_UartComando = 1;
            }
        }
        index = 0;
    }
}

/**
 * @brief   DMA0 Interruption
 */
void __attribute__((__interrupt__, no_auto_psv)) _DMA0Interrupt(void) {
    
    IFS0bits.DMA0IF = 0;//clear DMA interrupt flag 
    dma0_TxIntFlag = 1;
    
}

/**
 * @brief   DMA1 Interruption
 */
void __attribute__((__interrupt__, no_auto_psv)) _DMA1Interrupt(void) {
    
    IFS0bits.DMA1IF = 0;//clear DMA interrupt flag 
        
}


/**
 * @brief   ISR for PIR Detection @PIN RB7
 */
void __attribute__((__interrupt__,no_auto_psv)) _INT0Interrupt(void) {
    
    IFS0bits.INT0IF = 0;
    PIR_Detect = 1;
}

/**
 * @brief   Interruption from ORDY in FIFO Memory
 * 
 * Signal that indicates to the microprocessor that it requires
 * approximatly 15 cicles to read all the data
 */
void __attribute__((__interrupt__, no_auto_psv)) _INT1Interrupt(void) {

    IFS1bits.INT1IF = 0;
    //////////////////////////////////////
    //Reject Timeout of 5seg 
//    IFS1bits.T5IF = 0;
//    IEC1bits.T5IE = 0; // Enable Timer5 interrupt
//    T4CONbits.TON = 0; // Start 32-bit Timer
     /////////////////////////////////////
    
    switch (smCamara_Capture) {
        case 0:
            if(Camara_FrameCount-- == 0){            
                Camara_StartCapture();
                flag_UartAck = 1;
                smCamara_Capture++;            
            }
            break;
        case 1:
            Camara_StopCapture();
            smCamara_Capture = 0;
            //camara_flagCaptureReady = 1;////////////////////////////////////////
            break;
        default:
            smCamara_Capture = 0;
            break;
    }
        
}

/**
 * @brief   Interruption from VSYNC in OV5642
 * 
 * Signal that outputs the CMOS sensor to indicate that picture data
 * is being passed to the FIFO memory
 */
void __attribute__((__interrupt__, no_auto_psv)) _INT2Interrupt(void) {

    IFS1bits.INT2IF = 0;
    
    IFS1bits.T5IF = 0;
    IEC1bits.T5IE = 0; // Disable Timer5 interrupt
    T4CONbits.TON = 0; // N-Start 32-bit Timer
    
    IEC1bits.INT2IE = 0;
    flag_CaptureReady = 1;    
    time_out = 1;
}

/**
 * @brief   Timer 1 Interruption
 */
void __attribute__((__interrupt__, no_auto_psv)) _T1Interrupt(void) {

    IFS0bits.T1IF = 0; //Clear Timer1 interrupt flag
    
    //----- FAT FILING SYSTEM DRIVER TIMER -----
    disk_timerproc();
    //----- FAT FILING SYSTEM DRIVER TIMER -----
    if (ffs_10ms_timer) {
        ffs_10ms_timer--;
    }

}

/**
 * @brief   Timer 2 Interruption
 */
void __attribute__((__interrupt__, no_auto_psv)) _T2Interrupt(void) {
    
    //Configurado para interrumpir cada 15mseg
    IFS0bits.T2IF = 0;
    
}

/**
 * @brief   Timer 5 Interruption
 */
void __attribute__((__interrupt__, no_auto_psv)) _T5Interrupt(void)
{
    /* Interrupt Service Routine code goes here */
    IFS1bits.T5IF = 0; //Clear Timer5 interrupt flag
    printf("Se reinicia el timer \r\n");
    time_out = 1;
}

/**
 * @brief Sends a string over the UART port
 * 
 * @param   buffer, pointer to string
 */
void Camara_EnviaStringUART1(uint8_t *buffer) {

    while(*buffer != '\0') {
        while(U1STAbits.UTXBF);  /* wait if the buffer is full */
        U1TXREG = *buffer++;   /* transfer data byte to TX reg */
    }    
}

/**
 * @brief   Send one byte to the specified address in the OV5642 sensor
 * 
 * @param   dir     register address
 * @param   dat     data to wirte
 */
void Camara_EnviaBytesI2C(uint16_t dir, uint8_t dat) {

    HDByteWriteI2C(CAM_ADDRESS, dir, dat);

}

/**
 * @brief   Initialize the reference clock supplied to the OV5642 sensor
 * 
 * The microprocessor outputs a PWM signal to the OV5642 sensor clock input
 * the frequency is set to 7.5 Mhz
 */
void InicializaREFOCLK(void) {
    
    //Configuring the output the clock reference through the pin RP38 
    REFOCONbits.ROON = 0; //REFOCLK off
    REFOCONbits.ROSEL = 0; //Fosc is used as reference clock output
    REFOCONbits.RODIV = 0b0011; //reference divided by 8: 60MHz/8 = 7.5MHz
    REFOCONbits.ROON = 1; //REFOCLK off

    TRIS_CAM_XCLK = 0;//salida           
    RPOR2bits.RP38R = 0b110001;//RP38 --> REFCLKO   
}

/**
 * @brief Intialize Timer 1
 */
void InicializaTIMER1(void) {
    //----- SETUP TIMER 1 -----
    T1CON = 0b0000000000000000;
    TMR1 = 0;
    PR1 = 59881;            // 0.99999582mseg ~ 1mseg with ~60MIPS

    IPC0bits.T1IP = 0x01;   // Set Timer 1 Interrupt Priority Level
    IFS0bits.T1IF = 0;      // Clear Timer 1 Interrupt Flag
    IEC0bits.T1IE = 1;      // Enable Timer1 interrupt
    T1CONbits.TON = 1;      // Start Timer

}

/**
 * @brief Initialize Timer 2
 */
void InicializaTIMER2(void) {
    
    T2CON = 0x0020;
    TMR2 = 0;
    PR2 = 14036;        // Pre-escaler set to 15mseg
    
    IFS0bits.T2IF = 0;
    IEC0bits.T2IE = 1;
    
}

/**
 * @brief Initialize Timer 4 y 5 for 32 bits timer.
 */
void InicializaTIMER4_5(void){
    
    T5CONbits.TON = 0;      // Stop any 16-bit Timer5 operation
    T4CONbits.TON = 0;      // Stop any 16/32-bit Timer5 operation
    T4CONbits.T32 = 1;      // Enable 32-bit Timer mode
    T4CONbits.TCS = 0;      // Select internal instruction cycle clock
    T4CONbits.TGATE = 0;    // Disable Gated Timer mode
    T4CONbits.TCKPS = 0b01; // Select 1:1 Prescaler
    TMR5 = 0x00;            // Clear 32-bit Timer (msw)
    TMR4 = 0x00;            // Clear 32-bit Timer (lsw)
    PR5 = 0x0072;           // Load 32-bit period value (msw)
    PR4 = 0x36E4;           // Load 32-bit period value (lsw)
    
    IPC7bits.T5IP = 0x06;   // Set Timer5 Interrupt Priority Level
    IFS1bits.T5IF = 0;      // Clear Timer5 Interrupt Flag
    IEC1bits.T5IE = 1;      // Enable Timer 5 interrupt
    
    T4CONbits.TON = 1;      //Start 32-bit Timer
}

/**
 * @brief   Initialize SPI 1 pheriperal
 * 
 * Uncomment your selection for each variable. See datasheet
 * and PIC18 pheriperal for more details
 */
void InicializaSPI1(void) {
    //*************************************************************************
    // SPI module Configuration
    // Uncomment your selection for each variable
    // See device data sheet and PIC18 Perpheral Library docs for more details
    //*************************************************************************    
    SPI1STAT = 0b0000000000000000; //enable SPI pins
    SPI1CON2 = 0b0000000000000000; //framed SPI support disabled
    //SPI1CON1 = 0b0000001000111101;  //modo 8 bits, input data sampled at end, 
    //bus mode 00, prescal 1:1, postcal 16:1
    //master mode
    SPI1CON1bits.DISSCK = 0; // Internal serial clock is enabled
    SPI1CON1bits.DISSDO = 0; // SDOx pin is controlled by the module
    SPI1CON1bits.MODE16 = 0; // Communication is word-wide (8 bits)
    SPI1CON1bits.MSTEN = 1; // Master mode enabled
    SPI1CON1bits.SMP = 1; // Input data is sampled at the end of data output time
    SPI1CON1bits.CKE = 0; // Serial output data changes on transition from
    SPI1CON1bits.CKP = 0; // Idle state for clock is a low level;
    //SPI BUS LOW SPEED (Min 100KHz, Max 400KHz)  
    //(Min speed required for ACDM41 initalisation)
    SPI1CON1bits.SPRE = 0b000; //secondary prescal 8:1
    SPI1CON1bits.PPRE = 0b00; //primary prescal 64:1    
    // active state is a high level
    SPI1STATbits.SPIEN = 1; // Enable SPI module    
    
    TRIS_SDCARD_CS = 0;
    SDCARD_CS = 1;
   
}

/**
 * @brief   Initialize DMA bus for SPI 1 pheriperal
 */
void InicializaDMA_SPI(void) {
    
    DMA0_SetConfig(buff_CamCapture);
    DMA1_SetConfig();
        
}

/**
 * @brief   Initialize I2C pheriperal
 * 
 * Sets the I2C speed to 400 Khz, this setting is for
 * a cpu clock of 60 MIPS
 */
void InicializaI2C(void) {

    TRIS_CAM_SDA = 1; //RA8 input for SDA
    TRIS_CAM_SCL = 1; //RB4 input for SCL
 
    I2C2MSK = 0;
    I2C2BRG = 142;// Pre-escaler for 400KHz
    I2C2CON = 0x8000; //I2CEN = 1

    
}

/**
 * @brief   Initialize UART pheriperal
 * 
 * Sets baud rate to 115200
 */
void InicializaUART1(void) {
    
    //Configure pin RX y TX
    TRIS_TX_UART1 = 0;//TX
    TRIS_RX_UART1 = 1;//RX
    
    RPOR0bits.RP35R = 0b000001;//RP35 -> U1TX
    RPINR18bits.U1RXR = 37;//RP37 -> U1RX
    
    U1BRG = 129;    //  baudrate 115200
    U1MODE = 0x8008;
    U1STA = 0x0400;
    
    IFS0bits.U1RXIF = 0;
    IEC0bits.U1RXIE = 1; // Enable Interrupt
    
}

void initADC(void){
    ANSELA = ANSELB = ANSELC  = 0x0000;
    ANSELAbits.ANSA1 = 1;
    AD1CON1 = 0x0000;
    AD1CON2 = 0x0000;
    AD1CON3 = 0x000F;
    AD1CON4 = 0x0000;
    AD1CHS0 = 0x0001;
    AD1CHS123 = 0x0000;
    AD1CSSH = 0x0000;
    AD1CSSL = 0x0000;
    AD1CON1bits.ADON = 1;
    DelayUs(20);
}

uint16_t readADC(void){
    
    AD1CON1bits.SAMP = 1;
    DelayUs(10);
    AD1CON1bits.SAMP = 0;
    while(!AD1CON1bits.DONE);
    
    return ADC1BUF0;
}

/**
 * @brief   Initialize I/O pins
 */
void InicializaIO(void) {

    //==========================================================================
    //Test pin for Oscilloscope
    //TRIS_PROBE_TEST = 0;
    //PROBE_TEST = 0;
    
    //LED and trigger buttons
    TRIS_TEST_LED = 0; //   output
    TEST_LED = 0; //    clear pin
    
    TRIS_PIR = 1;   // input
    TRIS_FLASH = 0;
    FLASH = 0;
    
    TRIS_TEST_BUTTON = 1; //input
    PU_TEST_BUTTON = 1; //enable pull up
    
    //Camera Pins
    TRIS_CAM_HREF = 1; //input
    TRIS_CAM_VSYNC = 1; //input
    TRIS_CAM_PWDN = 0;//output
    
    //FIFO Memory pins B8-B15 are inputs
    TRISB = TRISB | 0xFF00;
    TRIS_MEM_RESET = 0; //output
    TRIS_MEM_WEN = 0; //output
    TRIS_MEM_RCLK = 0; //output
    TRIS_MEM_RRST = 0; //output
    TRIS_MEM_OE = 0; //output    
    TRIS_MEM_ORDY = 1;//input

    //Assign VSYNC pin to INT1
    RPINR0bits.INT1R = 57;  //  PPS INT1 --> RC9/RP57 para el VSYNC
    IEC1bits.INT1IE = 0;
    IFS1bits.INT1IF = 0;
    
    //Assign ORDY pin to INT2
    RPINR1bits.INT2R = 34;//PPS INT2 --> RB2/RPI34 para el ORDY
    IEC1bits.INT2IE = 0;
    IFS1bits.INT2IF = 0;
    
    //Setup initial pin states for the Camera and FIFO memory
    CAM_PWDN = 0;//power up

    MEM_RESET = 0;
    DelayMs(10);
    MEM_RESET = 1;

    
    MEM_RCLK = 1; //read port clock
    MEM_RRST = 0; //read port reset state
    MEM_OE = 1; //read port disable
    
    MEM_WEN = 0; //no capture - verify that HREF actives high
    
    // Enable Pins to communicate with ESP8266 Board
    TRIS_TAKING_PHOTO = 0;  //Output
    
}

//float percentMAX17043(void){
//
//    static uint8_t battery[2] = {0};
//    double percent1;
//    double percent;
//    
//    if(LDByteReadI2C2(MAX17043_ADDRESS, MAX17043_SOC, battery, 2))
//        printf("Error while reading I2C bus\r\n");
//    
//    printf("%x \r\n",battery[0]);
//    printf("%x \r\n",battery[1]);
//    percent = battery[0];
//    printf("%f \r\n",percent);
//    percent1 = (double)(battery[1])/256;
//    printf("%f \r\n",percent+percent1);
//    return percent+percent1;
//}

void PIR_IntConfig(void)
{
    INTCON2bits.INT0EP = 0; // 0: interrupt on positive edge, 1: iinterrupt on negative edge.
    IPC0bits.INT0IP = 7; // priority
    IFS0bits.INT0IF = 0; // reset INT0 interrupt flag
    IEC0bits.INT0IE = 1; // enable INT0 interrupt service routine
}

void NoAhorroEnergia(void)
{
    CNPDAbits.CNPDA0 = 0;
    CNPDAbits.CNPDA1 = 0;
    CNPDAbits.CNPDA2 = 0;
    CNPDAbits.CNPDA3 = 0;
    CNPDAbits.CNPDA4 = 0;
    CNPDAbits.CNPDA7 = 0;
    CNPDAbits.CNPDA8 = 0;
    CNPDAbits.CNPDA9 = 0;
    CNPDAbits.CNPDA10 = 0;
    CNPDBbits.CNPDB0 = 0;
    CNPDBbits.CNPDB2 = 0;
    CNPDBbits.CNPDB3 = 0;
    CNPDBbits.CNPDB4 = 0;
    CNPDBbits.CNPDB5 = 0;
    CNPDBbits.CNPDB6 = 0;
    //CNPDBbits.CNPDB7 = 0;
    CNPDBbits.CNPDB8 = 0;
    CNPDBbits.CNPDB10 = 0;
    CNPDBbits.CNPDB11 = 0;
    CNPDBbits.CNPDB12 = 0;
    CNPDBbits.CNPDB13 = 0;
    CNPDBbits.CNPDB14 = 0;
    CNPDBbits.CNPDB15 = 0;
    CNPDCbits.CNPDC0 = 0;
    CNPDCbits.CNPDC1 = 0;
    CNPDCbits.CNPDC2 = 0;
    CNPDCbits.CNPDC3 = 0;
    CNPDCbits.CNPDC4 = 0;
    CNPDCbits.CNPDC5 = 0;
    CNPDCbits.CNPDC6 = 0;
    CNPDCbits.CNPDC7 = 0;
    CNPDCbits.CNPDC8 = 0;
    CNPDCbits.CNPDC9 = 0;
    PMD1 = 0x0000;
    PMD2 = 0x0000;
    PMD3bits.CMPMD = 0;
    PMD3bits.CRCMD = 0;
    PMD3bits.I2C2MD = 0;
    PMD4bits.CTMUMD = 0;
    PMD4bits.REFOMD = 0;
    PMD7 = 0;
    TRIS_TAKING_PHOTO = 0;
}

void AhorroEnergia(void)
{
    TRISA = 0xFFFF;
    TRISB = 0xFFD7;
    TRISC = 0xFFFE;
    //TRISCbits.TRISC5 = 0;
    CNPDAbits.CNPDA0 = 1;
    CNPDAbits.CNPDA1 = 1;
    CNPDAbits.CNPDA2 = 1;
    CNPDAbits.CNPDA3 = 1;
    CNPDAbits.CNPDA4 = 1;
    CNPDAbits.CNPDA7 = 1;
    CNPDAbits.CNPDA8 = 1;
    CNPDAbits.CNPDA9 = 1;
    CNPDAbits.CNPDA10 = 1;
    CNPDBbits.CNPDB0 = 1;
    CNPDBbits.CNPDB2 = 1;
    CNPDBbits.CNPDB3 = 1; 
    CNPDBbits.CNPDB4 = 1;
    CNPDBbits.CNPDB5 = 1;
    CNPDBbits.CNPDB6 = 1;
    //CNPDBbits.CNPDB7 = 1;
    CNPDBbits.CNPDB8 = 1;
    CNPDBbits.CNPDB10 = 1;
    CNPDBbits.CNPDB11 = 1;
    CNPDBbits.CNPDB12 = 1;
    CNPDBbits.CNPDB13 = 1;
    CNPDBbits.CNPDB14 = 1;
    CNPDBbits.CNPDB15 = 1;
//    CNPDCbits.CNPDC0 = 1;
    CNPDCbits.CNPDC1 = 1;
    CNPDCbits.CNPDC2 = 1;
    CNPDCbits.CNPDC3 = 1;
    CNPDCbits.CNPDC4 = 1;
    CNPDCbits.CNPDC5 = 1;
    CNPDCbits.CNPDC6 = 1;
    CNPDCbits.CNPDC7 = 1;
    CNPDCbits.CNPDC8 = 1;
    CNPDCbits.CNPDC9 = 1;
    PMD1 = 0xFEFB; // Disables all Peripheral Interfaces
    PMD2 = 0x0F0F;
    PMD3bits.CMPMD = 1;
    PMD3bits.CRCMD = 1;
    PMD3bits.I2C2MD = 1;
    PMD4bits.CTMUMD = 1;
    PMD4bits.REFOMD = 1;
    PMD7 = 1;
    CAM_PWDN = 1;
}