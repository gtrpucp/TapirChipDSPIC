/* 
 * File:   globaltype.h
 * Author: HENDRIX
 *
 * Created on 9 de abril de 2015, 08:11 PM
 */

//****************************************
//****************************************
//***** GLOBAL DATA TYPE DEFINITIONS *****
//****************************************
//****************************************
#ifndef GLOBAL_DATA_TYPE_INIT				//(Include this section only once for each source file)
#define	GLOBAL_DATA_TYPE_INIT

#undef BOOL
#undef TRUE
#undef FALSE
#undef BYTE
#undef SIGNED_BYTE
#undef WORD
#undef SIGNED_WORD
#undef DWORD
#undef SIGNED_DWORD

//BOOLEAN - 1 bit:
//typedef enum _BOOL { FALSE = 0, TRUE } BOOL;
//BYTE - 8 bit unsigned:
typedef unsigned char BYTE;
//SIGNED_BYTE - 8 bit signed:
typedef signed char SIGNED_BYTE;
//WORD - 16 bit unsigned:
typedef unsigned int WORD;
//SIGNED_WORD - 16 bit signed:
typedef signed int SIGNED_WORD;
//DWORD - 32 bit unsigned:
typedef unsigned long DWORD;
//SIGNED_DWORD - 32 bit signed:
typedef signed long SIGNED_DWORD;



/* These types MUST be 16-bit or 32-bit */
typedef int				INT;
typedef unsigned int	UINT;

///* These types MUST be 16-bit */
//typedef short			SHORT;
//typedef unsigned short	WORD;
//typedef unsigned short	WCHAR;

///* These types MUST be 32-bit */
//typedef long			LONG;
//typedef unsigned long	DWORD;

/* This type MUST be 64-bit (Remove this for C89 compatibility) */
typedef unsigned long long QWORD;



//BYTE BIT ACCESS:
typedef union _BYTE_VAL
{
    struct
    {
        unsigned char b0:1;
        unsigned char b1:1;
        unsigned char b2:1;
        unsigned char b3:1;
        unsigned char b4:1;
        unsigned char b5:1;
        unsigned char b6:1;
        unsigned char b7:1;
    } bits;
    BYTE Val;
} BYTE_VAL;

//WORD ACCESS
typedef union _WORD_VAL
{
    WORD val;
    struct
    {
        BYTE LSB;
        BYTE MSB;
    } byte;
    BYTE v[2];
} WORD_VAL;
#define LSB(a)          ((a).v[0])
#define MSB(a)          ((a).v[1])

//DWORD ACCESS:
typedef union _DWORD_VAL
{
    DWORD val;
    struct
    {
        BYTE LOLSB;
        BYTE LOMSB;
        BYTE HILSB;
        BYTE HIMSB;
    } byte;
    struct
    {
        WORD LSW;
        WORD MSW;
    } word;
    BYTE v[4];
} DWORD_VAL;
#define LOWER_LSB(a)    ((a).v[0])
#define LOWER_MSB(a)    ((a).v[1])
#define UPPER_LSB(a)    ((a).v[2])
#define UPPER_MSB(a)    ((a).v[3])

//EXAMPLE OF HOW TO USE THE DATA TYPES:-
//	WORD_VAL variable_name;				//Define the variable
//	variable_name = 0xffffffff;			//Writing 32 bit value
//	variable_name.LSW = 0xffff;			//Writing 16 bit value to the lower word
//	variable_name.LOLSB = 0xff;			//Writing 8 bit value to the low word least significant byte
//	variable_name.v[0] = 0xff;			//Writing 8 bit value to byte 0 (least significant byte)




#endif		//GLOBAL_DATA_TYPE_INIT

