//###########################################################################
//
// FILE:   F2837xD_device.h
//
// TITLE:  F2837xD Device Definitions.
//
//###########################################################################
// $TI Release: F2837xD Support Library v3.04.00.00 $
// $Release Date: Sun Mar 25 13:26:04 CDT 2018 $
// $Copyright:
// Copyright (C) 2013-2018 Texas Instruments Incorporated - http://www.ti.com/
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions 
// are met:
// 
//   Redistributions of source code must retain the above copyright 
//   notice, this list of conditions and the following disclaimer.
// 
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the 
//   documentation and/or other materials provided with the   
//   distribution.
// 
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// $
//###########################################################################

#ifndef F2837xD_DEVICE_H
#define F2837xD_DEVICE_H

#if (!defined(CPU1) && !defined(CPU2))
#error "You must define CPU1 or CPU2 in your project properties.  Otherwise, the offsets in your header files will be inaccurate."
#endif

#if (defined(CPU1) && defined(CPU2))
#error "You have defined both CPU1 and CPU2 in your project properties.  Only a single CPU should be defined."
#endif


//*****************************************************************************
//
// Defines for pin numbers and other GPIO configuration
//
//*****************************************************************************
//
// LEDs
//
#ifdef _LAUNCHXL_F28379D
#define DEVICE_GPIO_PIN_LED1        31U  // GPIO number for LD10
#define DEVICE_GPIO_PIN_LED2        34U  // GPIO number for LD9
#define DEVICE_GPIO_CFG_LED1        GPIO_31_GPIO31  // "pinConfig" for LD10
#define DEVICE_GPIO_CFG_LED2        GPIO_34_GPIO34  // "pinConfig" for LD9
#else
#define DEVICE_GPIO_PIN_LED1        31U  // GPIO number for LD2
#define DEVICE_GPIO_PIN_LED2        34U  // GPIO number for LD3
#define DEVICE_GPIO_CFG_LED1        GPIO_31_GPIO31  // "pinConfig" for LD2
#define DEVICE_GPIO_CFG_LED2        GPIO_34_GPIO34  // "pinConfig" for LD3
#endif


//
// SCI for USB-to-UART adapter on FTDI chip
//
#ifdef _LAUNCHXL_F28379D
#define DEVICE_GPIO_PIN_SCIRXDA     43U             // GPIO number for SCI RX
#define DEVICE_GPIO_PIN_SCITXDA     42U             // GPIO number for SCI TX
#define DEVICE_GPIO_CFG_SCIRXDA     GPIO_43_SCIRXDA // "pinConfig" for SCI RX
#define DEVICE_GPIO_CFG_SCITXDA     GPIO_42_SCITXDA // "pinConfig" for SCI TX
#else
#define DEVICE_GPIO_PIN_SCIRXDA     28U             // GPIO number for SCI RX
#define DEVICE_GPIO_PIN_SCITXDA     29U             // GPIO number for SCI TX
#define DEVICE_GPIO_CFG_SCIRXDA     GPIO_28_SCIRXDA // "pinConfig" for SCI RX
#define DEVICE_GPIO_CFG_SCITXDA     GPIO_29_SCITXDA // "pinConfig" for SCI TX
#endif

//
// GPIO assignment for CAN-A and CAN-B
//
#ifdef _LAUNCHXL_F28379D
#define DEVICE_GPIO_CFG_CANRXA      GPIO_36_CANRXA  // "pinConfig" for CANA RX
#define DEVICE_GPIO_CFG_CANTXA      GPIO_37_CANTXA  // "pinConfig" for CANA TX
#define DEVICE_GPIO_CFG_CANRXB      GPIO_17_CANRXB  // "pinConfig" for CANB RX
#define DEVICE_GPIO_CFG_CANTXB      GPIO_12_CANTXB  // "pinConfig" for CANB TX
#else
#define DEVICE_GPIO_CFG_CANRXA      GPIO_30_CANRXA  // "pinConfig" for CANA RX
#define DEVICE_GPIO_CFG_CANTXA      GPIO_31_CANTXA  // "pinConfig" for CANA TX
#define DEVICE_GPIO_CFG_CANRXB      GPIO_10_CANRXB  // "pinConfig" for CANB RX
#define DEVICE_GPIO_CFG_CANTXB      GPIO_8_CANTXB   // "pinConfig" for CANB TX

//I2CA GPIO pins
#define DEVICE_GPIO_PIN_SDAA    104
#define DEVICE_GPIO_PIN_SCLA    105

#define DEVICE_GPIO_CFG_SDAA GPIO_104_SDAA
#define DEVICE_GPIO_CFG_SCLA GPIO_105_SCLA


//I2CB GPIO pins
#define DEVICE_GPIO_PIN_SDAB    40
#define DEVICE_GPIO_PIN_SCLB    41

#define DEVICE_GPIO_CFG_SDAB GPIO_40_SDAB
#define DEVICE_GPIO_CFG_SCLB GPIO_41_SCLB

#endif


// Added to add support for C2000 Launchpad XL W/ TMS320F28379D
//  If using LaunchXL with that spec, place "_LAUNCHXL_F28379D" into your
//  predefined symbols tab under project properties. - Kaden G. 07/06/2023
//****************************************************************************************
//
// Defines related to clock configuration specific to C2000 Launchpad XL W/ TMS320F28379D
//
//****************************************************************************************
//
// Launchpad Configuration
//
#ifdef _LAUNCHXL_F28379D

//
// 10MHz XTAL on LaunchPad. For use with SysCtl_getClock().
//
#define DEVICE_OSCSRC_FREQ          10000000U

//
// Define to pass to SysCtl_setClock(). Will configure the clock as follows:
// PLLSYSCLK = 10MHz (XTAL_OSC) * 40 (IMULT) * 1 (FMULT) / 2 (PLLCLK_BY_2)
//
#define DEVICE_SETCLOCK_CFG         (SYSCTL_OSCSRC_XTAL | SYSCTL_IMULT(40) |  \
                                     SYSCTL_FMULT_NONE | SYSCTL_SYSDIV(2) |   \
                                     SYSCTL_PLL_ENABLE)

//
// 200MHz SYSCLK frequency based on the above DEVICE_SETCLOCK_CFG. Update the
// code below if a different clock configuration is used!
//
#define DEVICE_SYSCLK_FREQ          ((DEVICE_OSCSRC_FREQ * 40 * 1) / 2)

//
// ControlCARD Configuration
//
#else

//
// 20MHz XTAL on controlCARD. For use with SysCtl_getClock().
//
#define DEVICE_OSCSRC_FREQ          20000000U

//
// Define to pass to SysCtl_setClock(). Will configure the clock as follows:
// PLLSYSCLK = 20MHz (XTAL_OSC) * 20 (IMULT) * 1 (FMULT) / 2 (PLLCLK_BY_2)
//
#define DEVICE_SETCLOCK_CFG         (SYSCTL_OSCSRC_XTAL | SYSCTL_IMULT(20) |  \
                                     SYSCTL_FMULT_NONE | SYSCTL_SYSDIV(2) |   \
                                     SYSCTL_PLL_ENABLE)

//
// 200MHz SYSCLK frequency based on the above DEVICE_SETCLOCK_CFG. Update the
// code below if a different clock configuration is used!
//
#define DEVICE_SYSCLK_FREQ          ((DEVICE_OSCSRC_FREQ * 20 * 1) / 2)

#endif



#ifdef __cplusplus
extern "C" {
#endif

#define   TARGET   1

//
// User To Select Target Device:
//
#define   F28_2837xD    TARGET

//
// Common CPU Definitions:
//
extern __cregister volatile unsigned int IFR;
extern __cregister volatile unsigned int IER;

#define  EINT   __asm(" clrc INTM")
#define  DINT   __asm(" setc INTM")
#define  ERTM   __asm(" clrc DBGM")
#define  DRTM   __asm(" setc DBGM")
#ifndef  EALLOW
#define  EALLOW __asm(" EALLOW")
#endif
#ifndef  EDIS
#define  EDIS   __asm(" EDIS")
#endif
#define  ESTOP0 __asm(" ESTOP0")

#define M_INT1  0x0001
#define M_INT2  0x0002
#define M_INT3  0x0004
#define M_INT4  0x0008
#define M_INT5  0x0010
#define M_INT6  0x0020
#define M_INT7  0x0040
#define M_INT8  0x0080
#define M_INT9  0x0100
#define M_INT10 0x0200
#define M_INT11 0x0400
#define M_INT12 0x0800
#define M_INT13 0x1000
#define M_INT14 0x2000
#define M_DLOG  0x4000
#define M_RTOS  0x8000

#ifndef C28X_BIT0
#define C28X_BIT0    0x00000001
#endif

#ifndef C28X_BIT1
#define C28X_BIT1    0x00000002
#endif

#ifndef C28X_BIT2
#define C28X_BIT2    0x00000004
#endif

#ifndef C28X_BIT3
#define C28X_BIT3    0x00000008
#endif

#ifndef C28X_BIT4
#define C28X_BIT4    0x00000010
#endif

#ifndef C28X_BIT5
#define C28X_BIT5    0x00000020
#endif

#ifndef C28X_BIT6
#define C28X_BIT6    0x00000040
#endif

#ifndef C28X_BIT7
#define C28X_BIT7    0x00000080
#endif

#ifndef C28X_BIT8
#define C28X_BIT8    0x00000100
#endif

#ifndef C28X_BIT9
#define C28X_BIT9    0x00000200
#endif

#ifndef C28X_BIT10
#define C28X_BIT10   0x00000400
#endif

#ifndef C28X_BIT11
#define C28X_BIT11   0x00000800
#endif

#ifndef C28X_BIT12
#define C28X_BIT12   0x00001000
#endif

#ifndef C28X_BIT13
#define C28X_BIT13   0x00002000
#endif

#ifndef C28X_BIT14
#define C28X_BIT14   0x00004000
#endif

#ifndef C28X_BIT15
#define C28X_BIT15   0x00008000
#endif

#ifndef C28X_BIT16
#define C28X_BIT16   0x00010000
#endif

#ifndef C28X_BIT17
#define C28X_BIT17   0x00020000
#endif

#ifndef C28X_BIT18
#define C28X_BIT18   0x00040000
#endif

#ifndef C28X_BIT19
#define C28X_BIT19   0x00080000
#endif

#ifndef C28X_BIT20
#define C28X_BIT20   0x00100000
#endif

#ifndef C28X_BIT21
#define C28X_BIT21   0x00200000
#endif

#ifndef C28X_BIT22
#define C28X_BIT22   0x00400000
#endif

#ifndef C28X_BIT23
#define C28X_BIT23   0x00800000
#endif

#ifndef C28X_BIT24
#define C28X_BIT24   0x01000000
#endif

#ifndef C28X_BIT25
#define C28X_BIT25   0x02000000
#endif

#ifndef C28X_BIT26
#define C28X_BIT26   0x04000000
#endif

#ifndef C28X_BIT27
#define C28X_BIT27   0x08000000
#endif

#ifndef C28X_BIT28
#define C28X_BIT28   0x10000000
#endif

#ifndef C28X_BIT29
#define C28X_BIT29   0x20000000
#endif

#ifndef C28X_BIT30
#define C28X_BIT30   0x40000000
#endif

#ifndef C28X_BIT31
#define C28X_BIT31   0x80000000
#endif

//
// For Portability, User Is Recommended To Use the C99 Standard integer types
//
#if !defined(__TMS320C28XX_CLA__)
#include <assert.h>
#include <stdarg.h>
#endif //__TMS320C28XX_CLA__
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

//
// C++ Bool Compatibility
//
#if defined(__cplusplus)
typedef bool _Bool;
#endif

//
// C99 defines boolean type to be _Bool, but this doesn't match the format of
// the other standard integer types.  bool_t has been defined to fill this gap.
//
typedef _Bool bool_t;

//
//used for a bool function return status
//
typedef _Bool status_t;

#ifndef SUCCESS
#define SUCCESS     true
#endif

#ifndef FAIL
#define FAIL        false
#endif

//
// The following data types are included for compatibility with legacy code,
// they are not recommended for use in new software.  Please use the C99
// types included above
//
#ifndef DSP28_DATA_TYPES
#define DSP28_DATA_TYPES
typedef short             	int16;
typedef long            	int32;
typedef long long			int64;
typedef unsigned short    	Uint16;
typedef unsigned long   	Uint32;
typedef unsigned long long	Uint64;
typedef float           	float32;
typedef long double     	float64;
#endif

//
// The following data types are for use with byte addressable peripherals.
// See compiler documentation on the byte_peripheral type attribute.
//
#ifndef __TMS320C28XX_CLA__
#if __TI_COMPILER_VERSION__ >= 16006000
typedef unsigned int bp_16 __attribute__((byte_peripheral));
typedef unsigned long bp_32 __attribute__((byte_peripheral));
#endif
#endif

//
// Include All Peripheral Header Files:
//
#include "F2837xD_adc.h"
#include "F2837xD_analogsubsys.h"
#include "F2837xD_cla.h"
#include "F2837xD_cmpss.h"
#include "F2837xD_cputimer.h"
#include "F2837xD_dac.h"
#include "F2837xD_dcsm.h"
#include "F2837xD_dma.h"
#include "F2837xD_ecap.h"
#include "F2837xD_emif.h"
#include "F2837xD_epwm.h"                // Enhanced PWM
#include "F2837xD_epwm_xbar.h"
#include "F2837xD_eqep.h"
#include "F2837xD_flash.h"
#include "F2837xD_gpio.h"                // General Purpose I/O Registers
#include "F2837xD_i2c.h"
#include "F2837xD_input_xbar.h"
#include "F2837xD_ipc.h"
#include "F2837xD_mcbsp.h"
#include "F2837xD_memconfig.h"
#include "F2837xD_nmiintrupt.h"          // NMI Interrupt Registers
#include "F2837xD_output_xbar.h"
#include "F2837xD_piectrl.h"             // PIE Control Registers
#include "F2837xD_pievect.h"
#include "F2837xD_sci.h"
#include "F2837xD_sdfm.h"
#include "F2837xD_spi.h"
#include "F2837xD_sysctrl.h"             // System Control/Power Modes
#include "F2837xD_upp.h"
#include "F2837xD_xbar.h"
#include "F2837xD_xint.h"                // External Interrupts

//
// byte_peripheral attribute is only supported on the C28
//
#ifndef __TMS320C28XX_CLA__
#if __TI_COMPILER_VERSION__ >= 16006000
#include "F2837xD_can.h"
#endif
#endif

#ifdef __cplusplus
}
#endif                                  // extern "C"

#endif                                  // end of F2837xD_DEVICE_H definition

//
// End of file.
//
