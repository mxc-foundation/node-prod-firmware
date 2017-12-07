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

#ifndef _lorabase_h_
#define _lorabase_h_

// ================================================================================
// BEG: Keep in sync with lorabase.hpp
//

#define REGION_EU       0
#define REGION_US       1
#define REGION_AU       2
#define REGION_AS1      3
#define REGION_KR       4
#define REGION_MASK     0x07
#define REGION_FULL     8
#define REGION_FLAGS    0x08

enum _cr_t { CR_4_5=0, CR_4_6, CR_4_7, CR_4_8 };
enum _sf_t { FSK=0, SF7, SF8, SF9, SF10, SF11, SF12, SFrfu };
enum _bw_t { BW125=0, BW250, BW500, BWrfu };
typedef u1_t cr_t;
typedef u1_t sf_t;
typedef u1_t bw_t;
typedef u1_t dr_t;
// Radio parameter set (encodes SF/BW/CR/IH/NOCRC)
typedef u2_t rps_t;
TYPEDEF_xref2rps_t;

enum { ILLEGAL_RPS = 0xFF };
enum { DR_PAGE_EU868 = 0x00 };
enum { DR_PAGE_US915 = 0x10 };

// Global maximum frame length
enum { STD_PREAMBLE_LEN  =  8 };
enum { MAX_LEN_FRAME     = 64 };
enum { LEN_DEVNONCE      =  2 };
enum { LEN_ARTNONCE      =  3 };
enum { LEN_NETID         =  3 };
enum { DELAY_JACC1       =  5 }; // in secs
enum { DELAY_DNW1        =  1 }; // in secs down window #1
enum { DELAY_EXTDNW2     =  1 }; // in secs
enum { DELAY_JACC2       =  DELAY_JACC1+(int)DELAY_EXTDNW2 }; // in secs
enum { DELAY_DNW2        =  DELAY_DNW1 +(int)DELAY_EXTDNW2 }; // in secs down window #1
enum { BCN_INTV_exp      = 7 };
enum { BCN_INTV_sec      = 1<<BCN_INTV_exp };
enum { BCN_INTV_ms       = BCN_INTV_sec*1000L };
enum { BCN_INTV_us       = BCN_INTV_ms*1000L };
enum { BCN_RESERVE_ms    = 2120 };   // space reserved for beacon and NWK management
enum { BCN_GUARD_ms      = 3000 };   // end of beacon period to prevent interference with beacon
enum { BCN_SLOT_SPAN_ms  =   30 };   // 2^12 reception slots a this span
enum { BCN_WINDOW_ms     = BCN_INTV_ms-(int)BCN_GUARD_ms-(int)BCN_RESERVE_ms };
enum { BCN_RESERVE_us    = 2120000 };
enum { BCN_GUARD_us      = 3000000 };
enum { BCN_SLOT_SPAN_us  =   30000 };

enum _dr_eu868_t {
    DR_SF12_EU=0,
    DR_SF11_EU,
    DR_SF10_EU,
    DR_SF9_EU,
    DR_SF8_EU,
    DR_SF7_EU,
    DR_SF7B_EU,
    DR_FSK_EU,
    DR_NONE_EU,
};

enum _dr_us915_t {
    DR_SF10_US=0,
    DR_SF9_US,
    DR_SF8_US,
    DR_SF7_US,
    DR_SF8C_US,
    DR_NONE_US,
    // Devices behind a router:
    DR_SF12CR_US=8,
    DR_SF11CR_US,
    DR_SF10CR_US,
    DR_SF9CR_US,
    DR_SF8CR_US,
    DR_SF7CR_US,
};

enum _dr_au915_t {
    DR_SF12_AU=0,
    DR_SF11_AU,
    DR_SF10_AU,
    DR_SF9_AU,
    DR_SF8_AU,
    DR_SF7_AU,
    DR_SF8C_AU,
    DR_NONE_AU,
    // Devices behind a router:
    DR_SF12CR_AU=8,
    DR_SF11CR_AU,
    DR_SF10CR_AU,
    DR_SF9CR_AU,
    DR_SF8CR_AU,
    DR_SF7CR_AU,
};

enum { DR_PAGE_EU = DR_PAGE_EU868 };

// MatchX frequency plan for EU 868MHz ISM band
// Bands:
//  g1 :   1%  14dBm
//  g2 : 0.1%  14dBm
//  g3 :  10%  27dBm
//  g4 : 0.1%  14dBm
//  g5 :   1% 6.5dBm
//                 freq             band     datarates
enum { EU868_F1 = 868100000,      // g1   SF7-12 
       EU868_F2 = 868300000,      // g1   SF7-12 FSK SF7/250         
       EU868_F3 = 868500000,      // g1   SF7-12         
       EU868_F4 = 868800000,      // g2   SF7-12
       EU868_F5 = 864700000,      // g4   SF7-12
       EU868_F6 = 864900000,      // g4   SF7-12
       EU868_F7 = 865100000,      // g5   SF7-12
       EU868_F8 = 865300000,      // g5   SF7-12
       EU868_J4 = 864100000,      // g2   SF7-12  used during join
       EU868_J5 = 864300000,      // g2   SF7-12   ditto
       EU868_J6 = 864500000,      // g2   SF7-12   ditto
};
enum { EU868_FREQ_MIN = 863000000,
       EU868_FREQ_MAX = 870000000 };

enum { CHNL_PING_EU      = 5 };
enum { FREQ_PING_EU      = EU868_F6 };  // default ping freq
enum { DR_PING_EU        = SF9 };       // default ping DR
enum { CHNL_DNW2_EU      = 5 };
enum { FREQ_DNW2_EU      = EU868_F6 };
enum { DR_DNW2_EU        = DR_SF12_EU };
enum { CHNL_BCN_EU       = 5 };
enum { FREQ_BCN_EU       = EU868_F6 };
enum { DR_BCN_EU         = DR_SF9_EU };
enum { AIRTIME_BCN_EU    = 144384 };  // micros

enum { DR_PAGE_US = DR_PAGE_US915 };

// Default frequency plan for US 915MHz
enum { US915_125kHz_UPFBASE = 902300000,
       US915_125kHz_UPFSTEP =    200000,
       US915_125kHz_1STCHAN =         8,
       US915_125kHz_CHANS   =         8,
       US915_500kHz_UPFBASE = 903000000,
       US915_500kHz_UPFSTEP =   1600000,
       US915_500kHz_DNFBASE = 923300000,
       US915_500kHz_DNFSTEP =    600000,
       US915_500kHz_CHAN    =        65,
};
enum { US915_FREQ_MIN = 902000000,
       US915_FREQ_MAX = 928000000 };

enum { CHNL_PING_US      = 0 }; // used only for default init of state (follows beacon - rotating)
enum { FREQ_PING_US      = US915_500kHz_DNFBASE + CHNL_PING_US*US915_500kHz_DNFSTEP };  // default ping freq
enum { DR_PING_US        = DR_SF10CR_US };       // default ping DR
enum { CHNL_DNW2_US      = 0 };
enum { FREQ_DNW2_US      = US915_500kHz_DNFBASE + CHNL_DNW2_US*US915_500kHz_DNFSTEP };
enum { DR_DNW2_US        = DR_SF12CR_US };
enum { CHNL_BCN_US       = 0 }; // used only for default init of state (rotating beacon scheme)
enum { DR_BCN_US         = DR_SF10CR_US };
enum { AIRTIME_BCN_US    = 72192 };  // micros

enum {
    // Beacon frame formats EU SF9 and US SF10
    OFF_BCN_NETID    = 0,
    OFF_BCN_TIME     = 3,
    OFF_BCN_CRC1     = 7,
    OFF_BCN_INFO_EU  = 8,
    OFF_BCN_INFO_US  = 9,
    OFF_BCN_LAT_EU   = 9,
    OFF_BCN_LAT_US   = 10,
    OFF_BCN_LON_EU   = 12,
    OFF_BCN_LON_US   = 13,
    OFF_BCN_RFU1     = 16,
    OFF_BCN_CRC2_EU  = 15,
    OFF_BCN_CRC2_US  = 17,
    LEN_BCN_EU       = 17,
    LEN_BCN_US       = 19,
};

enum {
    // Join Request frame format
    OFF_JR_HDR      = 0,
    OFF_JR_ARTEUI   = 1,
    OFF_JR_DEVEUI   = 9,
    OFF_JR_DEVNONCE = 17,
    OFF_JR_MIC      = 19,
    LEN_JR          = 23
};
enum {
    // Join Accept frame format
    OFF_JA_HDR      = 0,
    OFF_JA_ARTNONCE = 1,
    OFF_JA_NETID    = 4,
    OFF_JA_DEVADDR  = 7,
    OFF_JA_RFU      = 11,
    OFF_JA_DLSET    = 11,
    OFF_JA_RXDLY    = 12,
    OFF_CFLIST      = 13,
    LEN_JA          = 17,
    LEN_JAEXT       = 17+16
};
enum {
    // Data frame format
    OFF_DAT_HDR      = 0,
    OFF_DAT_ADDR     = 1,
    OFF_DAT_FCT      = 5,
    OFF_DAT_SEQNO    = 6,
    OFF_DAT_OPTS     = 8,
};
enum { MAX_LEN_PAYLOAD = MAX_LEN_FRAME-(int)OFF_DAT_OPTS-4 };
enum {
    // Bitfields in frame format octet
    HDR_FTYPE   = 0xE0,
    HDR_RFU     = 0x1C,
    HDR_MAJOR   = 0x03
};
enum { HDR_FTYPE_DNFLAG = 0x20 };  // flags DN frame except for HDR_FTYPE_PROP
enum {
    // Values of frame type bit field
    HDR_FTYPE_JREQ   = 0x00,
    HDR_FTYPE_JACC   = 0x20,
    HDR_FTYPE_DAUP   = 0x40,  // data (unconfirmed) up
    HDR_FTYPE_DADN   = 0x60,  // data (unconfirmed) dn
    HDR_FTYPE_DCUP   = 0x80,  // data confirmed up
    HDR_FTYPE_DCDN   = 0xA0,  // data confirmed dn
    HDR_FTYPE_REJOIN = 0xC0,  // rejoin for roaming
    HDR_FTYPE_PROP   = 0xE0
};
enum {
    HDR_MAJOR_V1 = 0x00,
};
enum {
    // Bitfields in frame control octet
    FCT_ADREN  = 0x80,
    FCT_ADRARQ = 0x40,
    FCT_ACK    = 0x20,
    FCT_MORE   = 0x10,   // also in DN direction: Class B indicator
    FCT_OPTLEN = 0x0F,
};
enum {
    // In UP direction: signals class B enabled
    FCT_CLASSB = FCT_MORE
};
enum {
    NWKID_MASK = (int)0xFE000000,
    NWKID_BITS = 7
};

// MAC uplink commands   downwlink too
enum {
    // Class A
    MCMD_LCHK_REQ = 0x02, // -  link check request : -
    MCMD_LADR_ANS = 0x03, // -  link ADR answer    : u1:7-3:RFU, 3/2/1: pow/DR/Ch ACK
    MCMD_DCAP_ANS = 0x04, // -  duty cycle answer  : -
    MCMD_DN2P_ANS = 0x05, // -  2nd DN slot status : u1:7-2:RFU  1/0:datarate/channel ack
    MCMD_DEVS_ANS = 0x06, // -  device status ans  : u1:battery 0,1-254,255=?, u1:7-6:RFU,5-0:margin(-32..31)
    MCMD_SNCH_ANS = 0x07, // -  set new channel    : u1: 7-2=RFU, 1/0:DR/freq ACK
    // Class B
    MCMD_PING_IND = 0x10, // -  pingability indic  : u1: 7=RFU, 6-4:interval, 3-0:datarate
    MCMD_PING_ANS = 0x11, // -  ack ping freq      : u1: 7-1:RFU, 0:freq ok
    MCMD_BCNI_REQ = 0x12, // -  next beacon start  : -
};

// MAC downlink commands
enum {
    // Class A
    MCMD_LCHK_ANS = 0x02, // link check answer  : u1:margin 0-254,255=unknown margin / u1:gwcnt
    MCMD_LADR_REQ = 0x03, // link ADR request   : u1:DR/TXPow, u2:chmask, u1:chpage/repeat
    MCMD_DCAP_REQ = 0x04, // duty cycle cap     : u1:255 dead [7-4]:RFU, [3-0]:cap 2^-k
    MCMD_DN2P_SET = 0x05, // 2nd DN window param: u1:7-4:RFU/3-0:datarate, u3:freq
    MCMD_DEVS_REQ = 0x06, // device status req  : -
    MCMD_SNCH_REQ = 0x07, // set new channel    : u1:chidx, u3:freq, u1:DRrange
    // Class B
    MCMD_PING_SET = 0x11, // set ping freq      : u3: freq
    MCMD_BCNI_ANS = 0x12, // next beacon start  : u2: delay(in TUNIT millis), u1:channel
};

enum {
    MCMD_BCNI_TUNIT = 30  // time unit of delay value in millis
};
enum {
    MCMD_LADR_ANS_RFU    = 0xF8, // RFU bits
    MCMD_LADR_ANS_POWACK = 0x04, // 0=not supported power level
    MCMD_LADR_ANS_DRACK  = 0x02, // 0=unknown data rate
    MCMD_LADR_ANS_CHACK  = 0x01, // 0=unknown channel enabled
};
enum {
    MCMD_DN2P_ANS_RFU    = 0xFC, // RFU bits
    MCMD_DN2P_ANS_DRACK  = 0x02, // 0=unknown data rate
    MCMD_DN2P_ANS_CHACK  = 0x01, // 0=unknown channel enabled
};
enum {
    MCMD_SNCH_ANS_RFU    = 0xFC, // RFU bits
    MCMD_SNCH_ANS_DRACK  = 0x02, // 0=unknown data rate
    MCMD_SNCH_ANS_FQACK  = 0x01, // 0=rejected channel frequency
};
enum {
    MCMD_PING_ANS_RFU   = 0xFE,
    MCMD_PING_ANS_FQACK = 0x01
};

enum {
    MCMD_DEVS_EXT_POWER   = 0x00, // external power supply
    MCMD_DEVS_BATT_MIN    = 0x01, // min battery value
    MCMD_DEVS_BATT_MAX    = 0xFE, // max battery value
    MCMD_DEVS_BATT_NOINFO = 0xFF, // unknown battery level
};

// Bit fields byte#3 of MCMD_LADR_REQ payload
enum {
    MCMD_LADR_CHP_125ON   = 0x60,  // special channel page enable, bits applied to 64..71
    MCMD_LADR_CHP_125OFF  = 0x70,  //  ditto
    MCMD_LADR_N3RFU_MASK  = 0x80,
    MCMD_LADR_CHPAGE_MASK = 0xF0,
    MCMD_LADR_REPEAT_MASK = 0x0F,
    MCMD_LADR_REPEAT_1    = 0x01,
    MCMD_LADR_CHPAGE_1    = 0x10
};
// Bit fields byte#0 of MCMD_LADR_REQ payload
enum {
    MCMD_LADR_DR_MASK    = 0xF0,
    MCMD_LADR_POW_MASK   = 0x0F,
    MCMD_LADR_DR_SHIFT   = 4,
    MCMD_LADR_POW_SHIFT  = 0,
#if 0
#if defined(CFG_eu868)
    MCMD_LADR_SF12      = DR_SF12<<4,
    MCMD_LADR_SF11      = DR_SF11<<4,
    MCMD_LADR_SF10      = DR_SF10<<4,
    MCMD_LADR_SF9       = DR_SF9 <<4,
    MCMD_LADR_SF8       = DR_SF8 <<4,
    MCMD_LADR_SF7       = DR_SF7 <<4,
    MCMD_LADR_SF7B      = DR_SF7B<<4,
    MCMD_LADR_FSK       = DR_FSK <<4,

    MCMD_LADR_20dBm     = 0,
    MCMD_LADR_14dBm     = 1,
    MCMD_LADR_11dBm     = 2,
    MCMD_LADR_8dBm      = 3,
    MCMD_LADR_5dBm      = 4,
    MCMD_LADR_2dBm      = 5,
#elif defined(CFG_us915)
    MCMD_LADR_SF10      = DR_SF10<<4,
    MCMD_LADR_SF9       = DR_SF9 <<4,
    MCMD_LADR_SF8       = DR_SF8 <<4,
    MCMD_LADR_SF7       = DR_SF7 <<4,
    MCMD_LADR_SF8C      = DR_SF8C<<4,
    MCMD_LADR_SF12CR    = DR_SF12CR<<4,
    MCMD_LADR_SF11CR    = DR_SF11CR<<4,
    MCMD_LADR_SF10CR    = DR_SF10CR<<4,
    MCMD_LADR_SF9CR     = DR_SF9CR<<4,
    MCMD_LADR_SF8CR     = DR_SF8CR<<4,
    MCMD_LADR_SF7CR     = DR_SF7CR<<4,

    MCMD_LADR_30dBm     = 0,
    MCMD_LADR_28dBm     = 1,
    MCMD_LADR_26dBm     = 2,
    MCMD_LADR_24dBm     = 3,
    MCMD_LADR_22dBm     = 4,
    MCMD_LADR_20dBm     = 5,
    MCMD_LADR_18dBm     = 6,
    MCMD_LADR_16dBm     = 7,
    MCMD_LADR_14dBm     = 8,
    MCMD_LADR_12dBm     = 9,
    MCMD_LADR_10dBm     = 10
#endif
#endif
};

// Device address
typedef u4_t devaddr_t;

// RX quality (device)
enum { RSSI_OFF=64, SNR_SCALEUP=4 };

inline sf_t  getSf   (rps_t params)            { return   (sf_t)(params &  0x7); }
inline rps_t setSf   (rps_t params, sf_t sf)   { return (rps_t)((params & ~0x7) | sf); }
inline bw_t  getBw   (rps_t params)            { return  (bw_t)((params >> 3) & 0x3); }
inline rps_t setBw   (rps_t params, bw_t cr)   { return (rps_t)((params & ~0x18) | (cr<<3)); }
inline cr_t  getCr   (rps_t params)            { return  (cr_t)((params >> 5) & 0x3); }
inline rps_t setCr   (rps_t params, cr_t cr)   { return (rps_t)((params & ~0x60) | (cr<<5)); }
inline int   getNocrc(rps_t params)            { return        ((params >> 7) & 0x1); }
inline rps_t setNocrc(rps_t params, int nocrc) { return (rps_t)((params & ~0x80) | (nocrc<<7)); }
inline int   getIh   (rps_t params)            { return        ((params >> 8) & 0xFF); }
inline rps_t setIh   (rps_t params, int ih)    { return (rps_t)((params & ~0xFF00) | (ih<<8)); }
inline rps_t makeRps (sf_t sf, bw_t bw, cr_t cr, int ih, int nocrc) {
    return sf | (bw<<3) | (cr<<5) | (nocrc?(1<<7):0) | ((ih&0xFF)<<8);
}
#define MAKERPS(sf,bw,cr,ih,nocrc) ((rps_t)((sf) | ((bw)<<3) | ((cr)<<5) | ((nocrc)?(1<<7):0) | ((ih&0xFF)<<8)))
// Two frames with params r1/r2 would interfere on air: same SFx + BWx 
inline int sameSfBw(rps_t r1, rps_t r2) { return ((r1^r2)&0x1F) == 0; }

//
// BEG: Keep in sync with lorabase.hpp
// ================================================================================


// Convert between dBm values and power codes (MCMD_LADR_XdBm)
s1_t pow2dBm (u1_t mcmd_ladr_p1);
// Calculate airtime
ostime_t calcAirTime (rps_t rps, u1_t plen);
// Sensitivity at given SF/BW
int getSensitivity (rps_t rps);


#endif // _lorabase_h_
