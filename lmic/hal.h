/*
 * Copyright (c) 2014-2016 IBM Corporation.
 * All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of the <organization> nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _hal_hpp_
#define _hal_hpp_

#include <sdk_defs.h>
#include <hw_spi.h>
#include <osal.h>
#ifdef OS_FREERTOS
#include <sys_rtc.h>
#else
#include "rtc.h"
#endif

#define HAL_LORA_SPI_NO		2

#define HAL_LORA_SPI_CLK_PORT	3
#define HAL_LORA_SPI_CLK_PIN	6
#define HAL_LORA_SPI_DI_PORT	3
#define HAL_LORA_SPI_DI_PIN	4
#define HAL_LORA_SPI_DO_PORT	3
#define HAL_LORA_SPI_DO_PIN	3
#define HAL_LORA_SPI_CS_PORT	3
#define HAL_LORA_SPI_CS_PIN	2
#define HAL_LORA_RX_PORT	3
#define HAL_LORA_RX_PIN		0
#define HAL_LORA_TX_PORT	3
#define HAL_LORA_TX_PIN		1
#define HAL_LORA_DIO0_PORT	4
#define HAL_LORA_DIO0_PIN	7
#define HAL_LORA_DIO1_PORT	4
#define HAL_LORA_DIO1_PIN	6
#define HAL_LORA_DIO2_PORT	4
#define HAL_LORA_DIO2_PIN	5

#define __DEFINE_HAL_LORA_SPI_INT(x)	HW_SPI ## x
#define __DEFINE_HAL_LORA_SPI(x)	__DEFINE_HAL_LORA_SPI_INT(x)
#define HAL_LORA_SPI			__DEFINE_HAL_LORA_SPI(HAL_LORA_SPI_NO)

/*
 * initialize hardware (IO, SPI, TIMER, IRQ).
 */
void hal_init (void);

/*
 * drive radio NSS pin (0=low, 1=high).
 */
//void hal_pin_nss (u1_t val);
#define hal_pin_nss(hi)		do {			\
		if (hi)					\
			hw_spi_set_cs_high(HW_SPI2);	\
		else					\
			hw_spi_set_cs_low(HW_SPI2);	\
	} while (0)

/*
 * drive radio RX/TX pins (0=rx, 1=tx).
 */
//void hal_pin_rxtx (u1_t val);
#define hal_pin_rxtx(tx)	do {			\
	if (tx) {					\
		hw_gpio_set_inactive(HAL_LORA_RX_PORT, HAL_LORA_RX_PIN); \
		hw_gpio_set_active(  HAL_LORA_TX_PORT, HAL_LORA_TX_PIN); \
	} else {					\
		hw_gpio_set_inactive(HAL_LORA_TX_PORT, HAL_LORA_TX_PIN); \
		hw_gpio_set_active(  HAL_LORA_RX_PORT, HAL_LORA_RX_PIN); \
	}						\
} while (0)

/*
 * control radio RST pin (0=low, 1=high, 2=floating)
 */
void hal_pin_rst (u1_t val);

/*
 * perform 8-bit SPI transaction with radio.
 *   - write given byte 'outval'
 *   - read byte and return value
 */
u1_t hal_spi (u1_t outval);

/*
 * disable all CPU interrupts.
 *   - might be invoked nested 
 *   - will be followed by matching call to hal_enableIRQs()
 */
//void hal_disableIRQs (void);
//#define hal_disableIRQs GLOBAL_INT_DISABLE
//#define hal_disableIRQs taskENTER_CRITICAL
#define hal_disableIRQs()

/*
 * enable CPU interrupts.
 */
//void hal_enableIRQs (void);
//#define hal_enableIRQs GLOBAL_INT_RESTORE
//#define hal_enableIRQs taskEXIT_CRITICAL
#define hal_enableIRQs()

/*
 * put system and CPU in low-power mode, sleep until interrupt.
 */
void hal_sleep (void);
//#define hal_sleep()	__WFI()
//#define hal_sleep()	__NOP()

/*
 * return 32-bit system time in ticks.
 */
//u4_t hal_ticks (void);
#define hal_ticks()	((u4_t)rtc_get())

/*
 * busy-wait until specified timestamp (in ticks) is reached.
 */
void hal_waitUntil (u4_t time);

/*
 * check and rewind timer for target time.
 *   - return 1 if target time is close
 *   - otherwise rewind timer for target time or full period and return 0
 */
u1_t hal_checkTimer (u4_t targettime);

/*
 * set short sleeping time
 */
void hal_setShortSleep(void);

/*
 * perform fatal failure action.
 *   - called by assertions
 *   - action could be HALT or reboot
 */
void hal_failed (void);

#endif // _hal_hpp_
