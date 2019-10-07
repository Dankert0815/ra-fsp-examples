/***********************************************************************************************************************
 * File Name    : secure_definitions.h
 * Description  : Contains Secure code region definitions and Security operation related macros
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * DISCLAIMER
 * This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
 * other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
 * applicable laws, including copyright laws.
 * THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
 * THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
 * EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
 * SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS
 * SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
 * this software. By using this software, you agree to the additional terms and conditions found by accessing the
 * following link:
 * http://www.renesas.com/disclaimer
 *
 * Copyright (C) 2019 Renesas Electronics Corporation. All rights reserved.
 ***********************************************************************************************************************/
#ifndef SECURE_DEFINITIONS_H_
#define SECURE_DEFINITIONS_H_

#include <stdint.h>

#define _unused(x) ((void)(x))

#define SECURE_PROGRAM __attribute__((section (".secure_text")))
#define SECURE_CONST   __attribute__((section (".secure_rodata")))
#define SECURE_DATA    __attribute__((section (".secure_data")))
#define SECURE_BSS     __attribute__((section (".secure_bss")))
#define SECURE_SRAM_PROGRAM	__attribute__((section (".secure_sram_program")))

#define SECURE_DATA_CONST_VALUE         (0x53454331)  // "SEC1"
#define SECURE_DATA_VALUE               (0x53454332)  // "SEC2"
#define SECURE_DATA_WRITE_TEST_VALUE    (0x53455752)  // "SEWR"

#define DATA_CONST_VALUE        (0xCAFECAFE)
#define DATA_VALUE              (0xDEADBEEF)
#define DATA_WRITE_TEST_VALUE   (0x554E5752)  // "UNWR"

#define DUMMY_VALUE 			(0xF0F0F0F0)

#define SECURE_FLASH_WRITE_LENGTH		(128)
/*
 * secure data
 * they need to be global variables so the security of the secure regions can be verified
 */
extern SECURE_BSS volatile uint32_t s_dataBss;
extern SECURE_CONST const   uint32_t s_dataConst;
extern SECURE_DATA volatile uint32_t s_dataInit;
extern SECURE_DATA volatile uint32_t s_dataWritten;
extern SECURE_DATA uint8_t s_writeBuffer[SECURE_FLASH_WRITE_LENGTH];

/* Need to verify the exact amount of padding needed */
#define SECURE_PADDING do {\
    /* 2 instructions (4 bytes) for the pipeline */\
    __asm volatile ("   nop\n");\
    __asm volatile ("   nop\n");\
    /* 3 words (12 bytes) for the max prefetch size */\
    __asm volatile ("   nop\n");\
    __asm volatile ("   nop\n");\
    __asm volatile ("   nop\n");\
    __asm volatile ("   nop\n");\
    __asm volatile ("   nop\n");\
    __asm volatile ("   nop\n");\
    } while(0)

#endif /* SECURE_DEFINITIONS_H_ */
