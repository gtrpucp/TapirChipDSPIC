

#ifndef PLAYER_RECORDER_H
#define PLAYER_RECORDER_H

#include "vs10xx_uc.h"

// Volumen maximo = 0, Silencio = 255, Recomendado 50
#define VOLUME_DEFAULT_PLAY  50
#define VOLUME_DEFAULT_REC   25



int VSTestInitHardware(void);
int VSTestInitSoftware(void);
void VSTestOffHardware(void);

int TaskVSRecord(char *fileName, int sr, int br);
int TaskVSPlayer(char *fileName);

void WriteSci(u_int8 addr, u_int16 data);
u_int16 ReadSci(u_int8 addr);
int WriteSdi(const u_int8 *data, u_int8 bytes);

void ini_test_button();
int test_button();
void SaveUIState(void);
void RestoreUIState(void);
int GetUICommand(void);
int AskUICommand();

#endif
