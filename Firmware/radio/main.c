// -*- Mode: C; c-basic-offset: 8; -*-
//
// Copyright (c) 2011 Michael Smith, All Rights Reserved
// Copyright (c) 2011 Andrew Tridgell, All Rights Reserved
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//  o Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  o Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//

///
/// @file	_start.c
///
/// Early startup code.
/// This file *must* be linked first for interrupt vector generation and main() to work.
/// XXX this may no longer be the case - it may be sufficient for the interrupt vectors
/// to be located in the same file as main()
///

#include <stdarg.h>
#include "radio.h"
#include "loop.h"
#include "timer.h"
#include "freq_hopping.h"

////////////////////////////////////////////////////////////////////////////////
/// @name	Interrupt vector prototypes
///
/// @note these *must* be placed in this file for SDCC to generate the
/// interrupt vector table correctly.
//@{

/// Serial rx/tx interrupt handler.
///
extern void	serial_interrupt(void)	__interrupt(INTERRUPT_UART0);

/// Radio event interrupt handler.
///
extern void	Receiver_ISR(void)	__interrupt(INTERRUPT_INT0);

/// Timer2 tick interrupt handler
///
extern void    T2_ISR(void)            __interrupt(INTERRUPT_TIMER2);

/// Timer3 tick interrupt handler
///
/// @todo switch this and everything it calls to use another register bank?
///
extern void    T3_ISR(void)            __interrupt(INTERRUPT_TIMER3);

//@}

__code const char g_banner_string[] = "SiK " stringify(APP_VERSION_HIGH) "." stringify(APP_VERSION_LOW) " on " BOARD_NAME;
__code const char g_version_string[] = stringify(APP_VERSION_HIGH) "." stringify(APP_VERSION_LOW);

__pdata enum BoardFrequency	g_board_frequency;	///< board info from the bootloader
__pdata uint8_t			g_board_bl_version;	///< from the bootloader

/// Configure the Si1000 for operation.
///
static void hardware_init(void);

/// Initialise the radio and bring it online.
///
static void radio_init(void);

/// statistics for radio and serial errors
__pdata struct error_counts errors;

void
main(void)
{
	// Stash board info from the bootloader before we let anything touch
	// the SFRs.
	//
	g_board_frequency = BOARD_FREQUENCY_REG;
	g_board_bl_version = BOARD_BL_VERSION_REG;

	// try to load parameters; set them to defaults if that fails.
	// this is done before hardware_init() to get the serial speed
	// XXX default parameter selection should be based on board info
	//
	if (!param_load())
		param_default();

	// Do hardware initialisation.
	hardware_init();

	// do radio initialisation
	radio_init();

	// turn on the receiver
	if (!radio_receiver_on()) {
		panic("failed to enable receiver");
	}

	serial_loop();
}

void
panic(char *fmt, ...)
{
	va_list ap;

	puts("\n**PANIC**");
	va_start(ap, fmt);
	vprintf(fmt, ap);
	puts("");

	EA = 1;
	ES0 = 1;
	
	delay_msec(1000);

	// generate a software reset
	RSTSRC |= (1 << 4);
	for (;;)
		;
}

static void
hardware_init(void)
{
	__pdata uint16_t	i;

	// Disable the watchdog timer
	PCA0MD	&= ~0x40;

	// Select the internal oscillator, prescale by 1
	FLSCL	 =  0x40;
	OSCICN	 =  0x8F;
	CLKSEL	 =  0x00;

	// Configure the VDD brown out detector
	VDM0CN	 =  0x80;
	for (i = 0; i < 350; i++);	// Wait 100us for initialization
	RSTSRC	 =  0x06;		// enable brown out and missing clock reset sources

#ifdef _BOARD_RFD900A			// Redefine port skips to override bootloader defs
	P0SKIP  =  0xCF;                // P0 UART avail on XBAR
	P1SKIP  =  0xF8;                // P1 SPI1, CEX0 avail on XBAR 
	P2SKIP  =  0x01;                // P2 CEX3 avail on XBAR, rest GPIO
#endif

	// Configure crossbar for UART
	P0MDOUT	 =  0x10;		// UART Tx push-pull
	SFRPAGE	 =  CONFIG_PAGE;
	P0DRV	 =  0x10;		// UART TX
	SFRPAGE	 =  LEGACY_PAGE;
	XBR0	 =  0x01;		// UART enable

	// SPI1
#ifdef _BOARD_RFD900A
	XBR1	|= 0x44;	// enable SPI in 3-wire mode
	P1MDOUT	|= 0xF5;	// SCK1, MOSI1, MISO1 push-pull
	P2MDOUT	|= 0xFF;	// SCK1, MOSI1, MISO1 push-pull
#else
	XBR1	|= 0x40;	// enable SPI in 3-wire mode
	P1MDOUT	|= 0xF5;	// SCK1, MOSI1, MISO1 push-pull
#endif	
	SFRPAGE	 = CONFIG_PAGE;
	P1DRV	|= 0xF5;	// SPI signals use high-current mode, LEDs and PAEN High current drive
	P2DRV	|= 0xFF;	
	SFRPAGE	 = LEGACY_PAGE;
	SPI1CFG	 = 0x40;	// master mode
	SPI1CN	 = 0x00;	// 3 wire master mode
	SPI1CKR	 = 0x00;	// Initialise SPI prescaler to divide-by-2 (12.25MHz, technically out of spec)
	SPI1CN	|= 0x01;	// enable SPI
	NSS1	 = 1;		// set NSS high

	// Clear the radio interrupt state
	IE0	 = 0;

	// initialise timers
	timer_init();

	// UART - set the configured speed
	serial_init(param_get(PARAM_SERIAL_SPEED));

	// set all interrupts to the same priority level
	IP = 0;

	// global interrupt enable
	EA = 1;

	// Turn on the 'radio running' LED and turn off the bootloader LED
	LED_RADIO = LED_ON;
	LED_BOOTLOADER = LED_OFF;

	// ADC system initialise for temp sensor
	AD0EN = 1;	// Enable ADC0
	ADC0CF = 0xF9;  // Set amp0gn=1 (1:1)
	ADC0AC = 0x00;
	ADC0MX = 0x1B;	// Set ADC0MX to temp sensor
	REF0CN = 0x07;	// Define reference and enable temp sensor

#ifdef _BOARD_RFD900A
	// PCA0, CEX0 setup and enable.
	PCA0MD = 0x88;
	PCA0PWM = 0x00;
	PCA0CPH3 = 0x80;
	PCA0CPM3 = 0x42;
	PCA0CN = 0x40;
#endif
	XBR2	 =  0x40;		// Crossbar (GPIO) enable
}

static void
radio_init(void)
{
	// Do generic PHY initialisation
	if (!radio_initialise()) {
		panic("radio_initialise failed");
	}

	// And intilise the radio with them.
	if (!radio_configure() &&
	    !radio_configure() &&
	    !radio_configure()) {
		panic("radio_configure failed");
	}
}

