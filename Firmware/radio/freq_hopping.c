// -*- Mode: C; c-basic-offset: 8; -*-
//
// Copyright (c) 2012 Andrew Tridgell, All Rights Reserved
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
/// @file	freq_hopping.c
///
/// Frequency hop managerment
///

#include <stdarg.h>
#include "radio.h"
#include "freq_hopping.h"

/*
  Derived from Australian RFM69 Register codes from: https://github.com/dekay/DavisRFM69
  conversion as such:
  freq = (msb << 16 + mid << 8 + lsb) * 61.03515625
 */
static __code uint32_t frequencies[51] = {
	918075988UL, 921050964UL, 924496948UL, 921991943UL, 919328979UL, 925436950UL, 923085998UL, 920111999UL, 
	923712951UL, 921520996UL, 918546997UL, 922617980UL, 924963989UL, 920581970UL, 918859985UL, 922304992UL, 
	924028991UL, 919642944UL, 925750000UL, 921365966UL, 918389953UL, 922774963UL, 924653991UL, 920268981UL, 
	925591979UL, 919174987UL, 921833984UL, 923400939UL, 925122985UL, 918233947UL, 920738952UL, 924182983UL, 
	922146972UL, 919486999UL, 922931945UL, 925905944UL, 923869995UL, 919955993UL, 921207946UL, 923242980UL, 
	918703979UL, 924809997UL, 922461975UL, 920424987UL, 923556945UL, 919016967UL, 924339965UL, 919799987UL, 
	921677978UL, 925278991UL, 920895996UL
};

__pdata static volatile uint8_t receive_channel;

// tell the loop code what channel to receive on
uint32_t 
fhop_receive_freqency(void)
{
	return frequencies[receive_channel];
}

// called to move to next frequency
void 
fhop_next(void)
{
	receive_channel++;
	if (receive_channel == 51) {
		receive_channel = 0;
	}
}


// called to move to previous frequency
void 
fhop_prev(void)
{
	if (receive_channel == 0 || receive_channel > 50) {
		receive_channel = 50;
	} else {
		receive_channel--;
	}
}
