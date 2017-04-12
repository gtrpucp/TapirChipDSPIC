

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "player.h"
#include "VS1063.h" 
#include "ff.h" 

#include "vs1063a-patches.plg"


#define FILE_BUFFER_SIZE        512
#define SDI_MAX_TRANSFER_SIZE   32
#define SDI_END_FILL_BYTES_FLAC 12288
#define SDI_END_FILL_BYTES      2050
#define REC_BUFFER_SIZE         512

#define SPEED_SHIFT_CHANGE      128

/* Valores entre 1 y 1-8 KiB tipico */
#define REPORT_INTERVAL 4096
#if 1
#define REPORT_ON_SCREEN
#endif

#if 1
#define PLAYER_USER_INTERFACE
#endif

#if 1
#define RECORDER_USER_INTERFACE
#endif


#define min(a,b) (((a)<(b))?(a):(b))

enum AudioFormat {
    afUnknown,
    afRiff,
    afOggVorbis,
    afMp1,
    afMp2,
    afMp3,
    afAacMp4,
    afAacAdts,
    afAacAdif,
    afFlac,
    afWma,
} audioFormat = afUnknown;

const char *afName[] = {
    "unknown",
    "RIFF",
    "Ogg",
    "MP1",
    "MP2",
    "MP3",
    "AAC MP4",
    "AAC ADTS",
    "AAC ADIF",
    "FLAC",
    "WMA",
};

/*
  Read 32-bit increasing counter value from addr.
  Because the 32-bit value can change while reading it,
  read MSB's twice and decide which is the correct one.
 */
u_int32 ReadVS10xxMem32Counter(u_int16 addr) {
    u_int16 msbV1, lsb, msbV2;
    u_int32 res;

    WriteSci(SCI_WRAMADDR, addr + 1);
    msbV1 = ReadSci(SCI_WRAM);
    WriteSci(SCI_WRAMADDR, addr);
    lsb = ReadSci(SCI_WRAM);
    msbV2 = ReadSci(SCI_WRAM);
    if (lsb < 0x8000U) {
        msbV1 = msbV2;
    }
    res = ((u_int32) msbV1 << 16) | lsb;

    return res;
}

/*
  Read 32-bit non-changing value from addr.
 */
u_int32 ReadVS10xxMem32(u_int16 addr) {
    u_int16 lsb;
    WriteSci(SCI_WRAMADDR, addr);
    lsb = ReadSci(SCI_WRAM);
    return lsb | ((u_int32) ReadSci(SCI_WRAM) << 16);
}

/*
  Read 16-bit value from addr.
 */
u_int16 ReadVS10xxMem(u_int16 addr) {
    WriteSci(SCI_WRAMADDR, addr);
    return ReadSci(SCI_WRAM);
}

/*
  Write 16-bit value to given VS10xx address
 */
void WriteVS10xxMem(u_int16 addr, u_int16 data) {
    WriteSci(SCI_WRAMADDR, addr);
    WriteSci(SCI_WRAM, data);
}

/*
  Write 32-bit value to given VS10xx address
 */
void WriteVS10xxMem32(u_int16 addr, u_int32 data) {
    WriteSci(SCI_WRAMADDR, addr);
    WriteSci(SCI_WRAM, (u_int16) data);
    WriteSci(SCI_WRAM, (u_int16) (data >> 16));
}

/*
  Loads a plugin.

  This is a slight modification of the LoadUserCode() example
  provided in many of VLSI Solution's program packages.
 */
void LoadPlugin(const u_int16 *d, u_int16 len) {
    int i = 0;

    while (i < len) {
        unsigned short addr, n, val;
        addr = d[i++];
        n = d[i++];
        if (n & 0x8000U) { /* RLE run, replicate n samples */
            n &= 0x7FFF;
            val = d[i++];
            while (n--) {
                WriteSci(addr, val);
            }
        } else { /* Copy run, copy n samples */
            while (n--) {
                val = d[i++];
                WriteSci(addr, val);
            }
        }
    }
}

enum PlayerStates {
    psPlayback = 0,
    psUserRequestedCancel,
    psCancelSentToVS10xx,
    psStopped
} playerState;


// Reproduce un archivo, Se ha usado la libreria FatFs de Elm-chan
void VS1063PlayFile(FIL *readFp) {
    static u_int8 playBuf[FILE_BUFFER_SIZE];
    UINT bytesInBuffer;     // How many bytes in buffer left  (se puede cambiar a UINT32)   
    u_int32 pos = 0;        // File position
    int endFillByte = 0;    // What byte value to send after file
    int endFillBytes = SDI_END_FILL_BYTES; // How many of those to send
    int playMode = ReadVS10xxMem(PAR_PLAY_MODE);
    long nextReportPos = 0; // File pointer where to next collect/report
    int i;
#ifdef PLAYER_USER_INTERFACE
    int volLevel = ReadSci(SCI_VOL) & 0xFF; // Assume both channels at same level
    int c;
#endif /* PLAYER_USER_INTERFACE */

#ifdef PLAYER_USER_INTERFACE
    SaveUIState();
#endif /* PLAYER_USER_INTERFACE */

    playerState = psPlayback;       // Set state to normal playback
    
    volLevel = VOLUME_DEFAULT_PLAY;    
    WriteSci(SCI_VOL, VOLUME_DEFAULT_PLAY * 0x101);  // Set volume
    WriteSci(SCI_DECODE_TIME, 0);   // Reset DECODE_TIME

    // Cambia a tipo Mono
    playMode ^= PAR_PLAY_MODE_MONO_ENA;
    
    /* Main playback loop */

    while (f_read(readFp, playBuf, FILE_BUFFER_SIZE, &bytesInBuffer) == FR_OK
            &&  bytesInBuffer > 0 && playerState != psStopped) {
        u_int8 *bufP = playBuf;
        
        // Mientras el buffer no est� vacio y el estado de reproduccion no es Stop
        while (bytesInBuffer && playerState != psStopped) {

            if (!(playMode & PAR_PLAY_MODE_PAUSE_ENA)) {
                int t = min(SDI_MAX_TRANSFER_SIZE, bytesInBuffer);

                // This is the heart of the algorithm: on the following line actual audio data gets sent to VS10xx.
                WriteSdi(bufP, t);

                bufP += t;
                bytesInBuffer -= t;
                pos += t;
            }

            /* If the user has requested cancel, set VS10xx SM_CANCEL bit */
            if (playerState == psUserRequestedCancel) {
                unsigned short oldMode;
                playerState = psCancelSentToVS10xx;
                printf("\n\rSetting SM_CANCEL at file offset %ld\n\r", pos);
                oldMode = ReadSci(SCI_MODE);
                WriteSci(SCI_MODE, oldMode | SM_CANCEL);
            }

            /* If VS10xx SM_CANCEL bit has been set, see if it has gone
               through. If it is, it is time to stop playback. */
            if (playerState == psCancelSentToVS10xx) {
                unsigned short mode = ReadSci(SCI_MODE);
                if (!(mode & SM_CANCEL)) {
                    printf("SM_CANCEL has cleared at file offset %ld\n\r", pos);
                    playerState = psStopped;
                }
            }

            /* If playback is going on as normal, see if we need to collect and
               possibly report */
            if (playerState == psPlayback && pos >= nextReportPos) {
#ifdef REPORT_ON_SCREEN
                u_int16 sampleRate;
                u_int16 hehtoBitsPerSec;
                u_int16 h1 = ReadSci(SCI_HDAT1);
#endif

                nextReportPos += REPORT_INTERVAL;
                /* It is important to collect endFillByte while still in normal
                   playback. If we need to later cancel playback or run into any
                   trouble with e.g. a broken file, we need to be able to repeatedly
                   send this byte until the decoder has been able to exit. */
                endFillByte = ReadVS10xxMem(PAR_END_FILL_BYTE);

#ifdef REPORT_ON_SCREEN
                if ((h1 & 0xFFE6) == 0xFFE2) {
                    audioFormat = afMp3;
                    endFillBytes = SDI_END_FILL_BYTES;
                } else if ((h1 & 0xFFE6) == 0xFFE4) {
                    audioFormat = afMp2;
                    endFillBytes = SDI_END_FILL_BYTES;
                } else if ((h1 & 0xFFE6) == 0xFFE6) {
                    audioFormat = afMp1;
                    endFillBytes = SDI_END_FILL_BYTES;
                } else {
                    audioFormat = afUnknown;
                    endFillBytes = SDI_END_FILL_BYTES_FLAC;
                }

                sampleRate = ReadSci(SCI_AUDATA);
                hehtoBitsPerSec = ReadVS10xxMem(PAR_BITRATE_PER_100);

                printf("\r%ldKiB "
                        "%1ds %1.1f"
                        "kb/s %dHz %s %s"
                        " %04x   ",
                        pos / 1024,
                        ReadSci(SCI_DECODE_TIME),
                        hehtoBitsPerSec * 0.1,
                        sampleRate & 0xFFFE, (sampleRate & 1) ? "stereo" : "mono",
                        afName[audioFormat], h1
                        );
#endif /* REPORT_ON_SCREEN */
            }
        } /* if (playerState == psPlayback && pos >= nextReportPos) */

#ifdef PLAYER_USER_INTERFACE
        /*  return -1 for no command and -2 for CTRL-C */
        c = GetUICommand();
        switch (c) {
                /* Volumen */
            case '-':
                if (volLevel < 255) {
                    volLevel++;
                    WriteSci(SCI_VOL, volLevel * 0x101);
                }
                break;
            case '+':
                if (volLevel) {
                    volLevel--;
                    WriteSci(SCI_VOL, volLevel * 0x101);
                }
                break;

                /* Algunis registros */
            case '_':
                printf("\n\rvol %1.1fdB, MODE %04x, ST %04x, "
                        "HDAT1 %04x HDAT0 %04x\n\r",
                        -0.5 * volLevel,
                        ReadSci(SCI_MODE),
                        ReadSci(SCI_STATUS),
                        ReadSci(SCI_HDAT1),
                        ReadSci(SCI_HDAT0));
                printf("  sampleCounter %lu", ReadVS10xxMem32Counter(PAR_SAMPLE_COUNTER));
                printf(", sdiFree %u", ReadVS10xxMem(PAR_SDI_FREE));
                printf(", audioFill %u", ReadVS10xxMem(PAR_AUDIO_FILL));
                printf("\n\r  positionMSec %lu", ReadVS10xxMem32Counter(PAR_POSITION_MSEC));
                printf(", config1 0x%04x", ReadVS10xxMem(PAR_CONFIG1));
                printf("\n\r");
                break;

                /* Ask player nicely to stop playing the song. */
            case 'q':
                if (playerState == psPlayback)
                    playerState = psUserRequestedCancel;
                break;
                
                /* Unknown commands or no command at all */
            default:
                if (c < -1) {
                    printf("Ctrl-C, aborting\n\r");
//                    fflush(stdout);
                    RestoreUIState();
                    exit(EXIT_FAILURE);
                }
                if (c >= 0) {
                    printf("\n\rUnknown char '%c' (%d)\n\r", isprint(c) ? c : '.', c);
                }
                break;
        } /* switch (c) */
#endif /* PLAYER_USER_INTERFACE */
    } /* while ((bytesInBuffer = fread(...)) > 0 && playerState != psStopped) */



#ifdef PLAYER_USER_INTERFACE
    RestoreUIState();
#endif /* PLAYER_USER_INTERFACE */

    printf("\n\rSending %d footer %d's... ", endFillBytes, endFillByte);

    /* Earlier we collected endFillByte. Now, just in case the file was
       broken, or if a cancel playback command has been given, write
       lots of endFillBytes. */
    memset(playBuf, endFillByte, sizeof (playBuf));
    for (i = 0; i < endFillBytes; i += SDI_MAX_TRANSFER_SIZE) {
        WriteSdi(playBuf, SDI_MAX_TRANSFER_SIZE);
    }

    /* If the file actually ended, and playback cancellation was not
       done earlier, do it now. */
    if (playerState == psPlayback) {
        unsigned short oldMode = ReadSci(SCI_MODE);
        WriteSci(SCI_MODE, oldMode | SM_CANCEL);
        printf("ok. Setting SM_CANCEL, waiting... ");
//        fflush(stdout);
        while (ReadSci(SCI_MODE) & SM_CANCEL)
            WriteSdi(playBuf, 2);
    }
    printf("ok\n\r");
}

/*
  This function records an audio file in Ogg, MP3.
 */
void VS1063RecordFile(FIL *writeFp, uint16_t sampleRate, uint16_t bitRate) {
    static u_int8 recBuf[REC_BUFFER_SIZE];
    u_int32 nextReportPos = 0; // File pointer where to next collect/report
    u_int32 fileSize = 0;
    UINT    bytesOutBuffer;
    int volLevel = ReadSci(SCI_VOL) & 0xFF;
    int c;

    playerState = psPlayback;

    printf("VS1063RecordFile\n\r");

    /* Initialize recording */

    /* Habilita el PLL x5, suficiente para el MP3. */
    WriteSci(SCI_CLOCKF, HZ_TO_SC_FREQ(12288000) | SC_MULT_53_50X | SC_ADD_53_00X);

    /* Example definitions for MP3 recording.
       For best quality, record at 48 kHz.
       If you must use CBR, set bitrate to at least 160 kbit/s. Avoid 128 kbit/s.
       Preferably use VBR mode, which generally gives better results for a
       given bitrate. */
    WriteSci(SCI_RECRATE, sampleRate);  
    
    /* Ajuste de ganancia 1024 = x1, 2048 = x2. Si es 0 es AGC*/
    WriteSci(SCI_RECGAIN, 0); // activa AGC 
    //WriteSci(SCI_RECMAXAUTO, 0); /* if RECGAIN = 0, define max auto gain */
    
    // Ajusta el volumen de monitoreo de la grabacion
    volLevel = VOLUME_DEFAULT_REC;    
    WriteSci(SCI_VOL, VOLUME_DEFAULT_REC * 0x101);  // Set volume
    
    // Se escoge el formato MP3 Entra del canal izquierdo(MICP))
    WriteSci(SCI_RECMODE, RM_63_FORMAT_MP3 | RM_63_ADC_MODE_LEFT);
    
    /* modo CBR */
    WriteSci(SCI_RECQUALITY, RQ_MODE_CBR | RQ_MULT_1000 | bitRate); 

    audioFormat = afMp3;
    
    // Entrada de audio seleccionado entrada MIC, tipo de funcionamiento modo ENCODER
    WriteSci(SCI_MODE, ReadSci(SCI_MODE) | SM_ENCODE);
    // Activa la grabacion
    WriteSci(SCI_AIADDR, 0x0050); 


#ifdef RECORDER_USER_INTERFACE
    SaveUIState();
#endif /* RECORDER_USER_INTERFACE */

    while (playerState != psStopped) {
        int n;

#ifdef RECORDER_USER_INTERFACE
        {
            c = GetUICommand();

            switch (c) {
                case 'q':
                    if (playerState == psPlayback) {
                        WriteSci(SCI_MODE, ReadSci(SCI_MODE) | SM_CANCEL);
                        printf("\n\rSwitching encoder off...\n\r");
                        playerState = psUserRequestedCancel;
                    }
                    break;
                case '-':
                    if (volLevel < 255) {
                        volLevel++;
                        WriteSci(SCI_VOL, volLevel * 0x101);
                    }
                    break;
                case '+':
                    if (volLevel) {
                        volLevel--;
                        WriteSci(SCI_VOL, volLevel * 0x101);
                    }
                    break;
                 default:
                    if (c < -1) {
                        printf("Ctrl-C, aborting\n\r");
//                        fflush(stdout);
                        RestoreUIState();
                        exit(EXIT_FAILURE);
                    }
                    if (c >= 0) {
                        printf("\n\rUnknown char '%c' (%d)\n\r", isprint(c) ? c : '.', c);
                    }
                    break;
            }
        }
#endif /* RECORDER_USER_INTERFACE */

        /* See if there is some data available */
        if ((n = ReadSci(SCI_RECWORDS)) > 0) {
            int i;
            u_int8 *rbp = recBuf;

            n = min(n, REC_BUFFER_SIZE / 2);
            for (i = 0; i < n; i++) {
                u_int16 w = ReadSci(SCI_RECDATA);
                *rbp++ = (u_int8) (w >> 8);
                *rbp++ = (u_int8) (w & 0xFF);
            }
            
            f_write(writeFp, recBuf, 2*n, &bytesOutBuffer);
            fileSize += 2 * n;
        } 
        else {
            /* The following read from SCI_RECWORDS may appear redundant.
               But it's not: SCI_RECWORDS needs to be rechecked AFTER we
               have seen that SM_CANCEL have cleared. */
            if (playerState != psPlayback && !(ReadSci(SCI_MODE) & SM_CANCEL)
                    && !ReadSci(SCI_RECWORDS)) {
                playerState = psStopped;
            }
        }

        if (fileSize - nextReportPos >= REPORT_INTERVAL) {
            u_int16 sampleRate = ReadSci(SCI_AUDATA);
            nextReportPos += REPORT_INTERVAL;
            printf("\r%ldKiB %lds %uHz %s %s ",
                    fileSize / 1024,
                    ReadVS10xxMem32Counter(PAR_SAMPLE_COUNTER) /
                    (sampleRate & 0xFFFE),
                    sampleRate & 0xFFFE,
                    (sampleRate & 1) ? "stereo" : "mono",
                    afName[audioFormat]
                    );
//            fflush(stdout);
        }
    } /* while (playerState != psStopped) */


#ifdef RECORDER_USER_INTERFACE
    RestoreUIState();
#endif /* RECORDER_USER_INTERFACE */

    /* We need to check whether the file had an odd length.
       That information is available in the MSB of PAR_END_FILL_BYTE.
       In that case, the 8 LSB's are the missing byte, so we'll add
       it to the output file. */
    {
        u_int16 lastByte;
        lastByte = ReadVS10xxMem(PAR_END_FILL_BYTE);
        if (lastByte & 0x8000U) {
            f_putc(lastByte & 0xFF, writeFp);
            printf("\n\rOdd length recording\n\r");
        } else {
            printf("\n\rEven length recording\n\r");
        }
    }

    f_close(writeFp);
    
    /* Finally, reset the VS10xx software, including realoading the
       patches package, to make sure everything is set up properly. */
    VSTestInitSoftware();

    printf("ok\n\r");
}

/*
  Hardware Initialization for VS1063.
 */
int VSTestInitHardware(void) {
    /* Write here your microcontroller code which puts VS10xx in hardware
       reset anc back (set xRESET to 0 for at least a few clock cycles,
       then to 1). */

    // input ports
    TRIS_VS_DREQ = 1;

    // output ports
    TRIS_VS_XCS = 0;
    TRIS_VS_XDCS = 0;
    TRIS_VS_XRESET = 0;

    spi2_init();
    VS_XRESET = 1;
    delay_ms(4);
    vs_wait();
    return 0;
}

void VSTestOffHardware(void){
    VS_XRESET = 0;
}


const u_int16 chipNumber[16] = {
    1001, 1011, 1011, 1003, 1053, 1033, 1063, 1103,
    0, 0, 0, 0, 0, 0, 0, 0
};

/*

  Software Initialization for VS1063.

  Note that you need to check whether SM_SDISHARE should be set in
  your application or not.
  
 */
int VSTestInitSoftware(void) {
    u_int16 ssVer;
    
    /* Start initialization with a dummy read, which makes sure our
       microcontoller chips selects and everything are where they
       are supposed to be and that VS10xx's SCI bus is in a known state. */
    ReadSci(SCI_MODE);

    /* First real operation is a software reset. After the software
       reset we know what the status of the IC is. You need, depending
       on your application, either set or not set SM_SDISHARE. See the
       Datasheet for details. */
    //WriteSci(SCI_MODE, SM_SDINEW | SM_SDISHARE | SM_TESTS | SM_RESET | SM_LINE1);
    WriteSci(SCI_MODE, SM_SDINEW | SM_SDISHARE | SM_TESTS | SM_RESET);

    /* A quick sanity check: write to two registers, then test if we
       get the same results. Note that if you use a too high SPI
       speed, the MSB is the most likely to fail when read again. */
    WriteSci(SCI_AICTRL1, 0xABAD);
    WriteSci(SCI_AICTRL2, 0x7E57);
    
    if (ReadSci(SCI_AICTRL1) != 0xABAD || ReadSci(SCI_AICTRL2) != 0x7E57) {
        printf("There is something wrong with VS10xx SCI registers\n\r");
        return 1;
    }
    WriteSci(SCI_AICTRL1, 0);
    WriteSci(SCI_AICTRL2, 0);

    /* Check VS10xx type */
    ssVer = ((ReadSci(SCI_STATUS) >> 4) & 15);
    if (chipNumber[ssVer]) {
        printf("Chip is VS%d\n\r", chipNumber[ssVer]);
        if (chipNumber[ssVer] != 1063) {
            printf("Incorrect chip\n\r");
            return 1;
        }
    } else {
        printf("Unknown VS10xx SCI_MODE field SS_VER = %d\n\r", ssVer);
        return 1;
    }

    /* Set the clock. Until this point we need to run SPI slow so that
       we do not exceed the maximum speeds mentioned in
       Chapter SPI Timing Diagram in the Datasheet. */
    WriteSci(SCI_CLOCKF,
            HZ_TO_SC_FREQ(12288000) | SC_MULT_53_40X | SC_ADD_53_15X);


    /* Now when we have upped the VS10xx clock speed, the microcontroller
       SPI bus can run faster. Do that before you start playing or
       recording files. */
    //VS_CLK_FAST;
    
    /* Set up other parameters. */
    WriteVS10xxMem(PAR_CONFIG1, PAR_CONFIG1_AAC_SBR_SELECTIVE_UPSAMPLE);

    /* Set volume level at -6 dB of maximum */
    WriteSci(SCI_VOL, 0x0c0c);

    /* Now it's time to load the proper patch set. */
    LoadPlugin(plugin, sizeof (plugin) / sizeof (plugin[0]));

    /* We're ready to go. */
    return 0;
}


int TaskVSRecord(char *fileName, int sr, int br)
{
    FIL File;
    FRESULT Res;
    
    Res = f_open(&File, (const TCHAR*)fileName, FA_WRITE | FA_CREATE_ALWAYS);
    printf("Record file %s\n\r", fileName);
    if (Res == FR_OK) {
        VS1063RecordFile(&File, sr, br);
    } else {
        printf("Failed opening %s for writing\n\r", fileName);
        return -1;
    }
    
    return 0;
}

int TaskVSPlayer(char *fileName)
{
    FIL File;
    FRESULT Res;
    
    Res = f_open(&File, (const TCHAR*)fileName, FA_READ | FA_OPEN_ALWAYS);
    printf("Play file %s\n\r", fileName);
    if(Res == FR_OK){
        VS1063PlayFile(&File);
    } else {
        printf("Failed opening %s for reading\n\r", fileName);
        printf("%u\n\r", Res);
        return -1;
    }
    
    return 0;
}
