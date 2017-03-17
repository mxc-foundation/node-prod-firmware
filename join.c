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

#include <stdio.h>
#include "lmic.h"
//#include "debug.h"
void debug_event (int ev);

//////////////////////////////////////////////////
// CONFIGURATION (FOR APPLICATION CALLBACKS BELOW)
//////////////////////////////////////////////////

//#define BAHN
#ifdef BAHN
// application router ID (LSBF)
static const u1_t APPEUI[8]  = { 0x22, 0x25, 0x00, 0xF0, 0x7E, 0xD5, 0xB3, 0x70, };

// unique device ID (LSBF)
static const u1_t DEVEUI[8]  = { 0x46, 0x34, 0x52, 0x34, 0x55, 0x34, 0x34, 0x23, };

// device-specific AES key (derived from device EUI)
static const u1_t DEVKEY[16] = { 0x49, 0x8F, 0x37, 0x46, 0x81, 0x0C, 0x04, 0x86, 0xDC, 0x20, 0x70, 0x32, 0x0B, 0x97, 0xE1, 0x77 };
#else
// application router ID (LSBF)
static const u1_t APPEUI[8]  = { 0x02, 0x00, 0x00, 0x00, 0x00, 0xEE, 0xFF, 0xC0 };

// unique device ID (LSBF)
static const u1_t DEVEUI[8]  = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF };

// device-specific AES key (derived from device EUI)
static const u1_t DEVKEY[16] = { 0xAB, 0x89, 0xEF, 0xCD, 0x23, 0x01, 0x67, 0x45, 0x54, 0x76, 0x10, 0x32, 0xDC, 0xFE, 0x98, 0xBA };
#endif


//////////////////////////////////////////////////
// APPLICATION CALLBACKS
//////////////////////////////////////////////////

// provide application router ID (8 bytes, LSBF)
void os_getArtEui (u1_t* buf) {
    memcpy(buf, APPEUI, 8);
}

// provide device ID (8 bytes, LSBF)
void os_getDevEui (u1_t* buf) {
    memcpy(buf, DEVEUI, 8);
}

// provide device key (16 bytes)
void os_getDevKey (u1_t* buf) {
    memcpy(buf, DEVKEY, 16);
}


//////////////////////////////////////////////////
// MAIN - INITIALIZATION AND STARTUP
//////////////////////////////////////////////////

// initial job
static void initfunc (osjob_t* j) {
    // reset MAC state
    LMIC_reset();
    // start joining
    LMIC_startJoining();
    // init done - onEvent() callback will be invoked...
}


// application entry point
int join_main () {
    osjob_t initjob;

    // initialize runtime env
    //os_init();
    // initialize debug library
    //debug_init();
    // setup initial job
    os_setCallback(&initjob, initfunc);
    // execute scheduled jobs and events
    os_runloop();
    // (not reached)
    return 0;
}


//////////////////////////////////////////////////
// UTILITY JOB
//////////////////////////////////////////////////

#if 0
static osjob_t blinkjob;
static u1_t ledstate = 0;

static void blinkfunc (osjob_t* j) {
    // toggle LED
    ledstate = !ledstate;
    debug_led(ledstate);
    // reschedule blink job
    os_setTimedCallback(j, os_getTime()+ms2osticks(100), blinkfunc);
}
#endif


//////////////////////////////////////////////////
// LMIC EVENT CALLBACK
//////////////////////////////////////////////////

#if 0
void onEvent (ev_t ev) {
    debug_event(ev);

    switch(ev) {

      // starting to join network
      case EV_JOINING:
          // start blinking
          //blinkfunc(&blinkjob);
          break;

      // network joined, session established
      case EV_JOINED:
          // cancel blink job
          //os_clearCallback(&blinkjob);
          // switch on LED
          //debug_led(1);
          // (don't schedule any new actions)
	  printf("netid = %lu\r\n", LMIC.netid);
	  goto tx;

      case EV_TXCOMPLETE:
	  if (LMIC.dataLen) {
	      printf("rx:");
	      for (int i = 0; i < LMIC.dataLen; i++)
		  printf(" %02x", LMIC.frame[LMIC.dataBeg + i]);
	      printf("\r\n");
	  }
tx:
	  // immediately prepare next transmission
	  LMIC.frame[0] = LMIC.snr;
	  // schedule transmission (port 1, datalen 1, no ack requested)
	  LMIC_setTxData2(1, LMIC.frame, 1, 0);
	  //LMIC_setTxData2(1, LMIC.frame, 1, 1);
	  // (will be sent as soon as duty cycle permits)
	  break;

      default:
	  break;
    }
}
#endif
