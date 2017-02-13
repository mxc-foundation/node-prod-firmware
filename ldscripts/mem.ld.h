/* Linker script to configure memory regions. 
 * Need modifying for a specific board. 
 *   ROM.ORIGIN: starting address of read-only RAM area
 *   ROM.LENGTH: length of read-only RAM area
 *   RetRAMx.ORIGIN: starting address of retained RAMx area
 *   RetRAMx.LENGTH: length of retained RAMx area
 *   RAM.ORIGIN: starting address of read-write RAM area
 *   RAM.LENGTH: length of read-write RAM area
 *
 **************************************************************************************************
 *
 * The positioning of the ROM changes depending on the type of the non-volatile memory used and the
 * execution mode. More specifically,
 *
 *   |------------------|------------------|------------------|
 *   |   execution mode |     mirrored     |      cached      |
 *   |code is stored in |                  |                  |
 *   |------------------|------------------|------------------|
 *   |    JTAG Download |  RAM (0x7FC0000) |  RAM (0x7FC0000) |
 *   |------------------|------------------|------------------|
 *   |              OTP |  RAM (0x7FC0000) |  OTP (0x7F80000) |
 *   |------------------|------------------|------------------|
 *   |            Flash |  RAM (0x7FC0000) | QSPI (0x8000000) |
 *   |------------------|------------------|------------------|
 *
 * The positioning of the RAM and the RetRAM areas depends on the positioning of the ROM area. More
 * specifically,
 *
 *   |------------------|------------------|------------------|
 *   |   execution mode |        RAM       |      RetRAM      |
 *   |code is stored in |                  |                  |
 *   |------------------|------------------|------------------|
 *   |  RAM (0x7FC0000) |  after the code  |  after the code  |
 *   |(JTAG or mirrored)|   or the RetRAM  |    or the RAM    |
 *   |------------------|------------------|------------------|
 *   |   OTP (cached)   |  at 0x7FC0000 or |  at 0x7FC0000 or |
 *   |                  | after the RetRAM |   after the RAM  |
 *   |------------------|------------------|------------------|
 *   |   QSPI (cached)  |  at 0x7FC0100 or |  at 0x7FC0100 or |
 *   |                  | after the RetRAM |   after the RAM  |
 *   |------------------|------------------|------------------|
 *
 * The above table are valid when the code is preconfigured to run from a specific location. If the
 * configuration is taken from the OTP header then the code is built to run from 0x0 and the RAM
 * base address is set by the user at RAM_BASE.
 * 
 * Up to 3 non-continuous Retention Memory areas may be defined. Up to 3 non-continuous RAM Memory
 * areas may be defined, also.
 *
 * Parameters that control the final memory layout:
 * - dg_configCONFIG_VIA_OTP_HEADER
 * - dg_configCODE_LOCATION
 * - dg_configEXEC_MODE
 * - CODE_SIZE     The size of the code in the format the linker understands (e.g. 64K).
 * - RETRAM_0_SIZE The size of the Retention RAM. 
 * - RAM_SIZE      The size of the RAM.
 * - RAM_START     The base address of the RAM in case of configuration via the OTP header. Since 
 *                 there is no way to know the proper value, it is initialized for mirrored mode.
 *
 * Optional parameterization (one option must be used):
 * (auto mode)
 * - RETRAM_FIRST  A switch that controls whether the RetRAM will be placed before the RAM or not.
 *
 * (manual mode)
 * - RETRAM_0_OFFSET The offset of the beginning of the RetRAM0 from the base address 
 *                 (RAM_BASE_ADDRESS).
 * - RAM_OFFSET    The offset of the beginning of the RAM from the base address (RAM_BASE_ADDRESS).
 * or
 * - RETRAM_0_BASE The absolute address where the RetRAM starts.
 * - RAM_BASE      The absolute address where the RAM starts.
 *
 * Limitations
 * - This version supports only 1 Retention RAM and 1 RAM area.
 *
 */

/*** Parameterization starts here ***/

#if dg_configCODE_LOCATION == NON_VOLATILE_IS_NONE
#define CODE_SIZE     ( 84 * 1024)
#define RETRAM_0_SIZE ( 44 * 1024)
#define RETRAM_1_SIZE (  0 * 1024)              // Retention RAM area for Heap(s)
#define RAM_SIZE      ( 16 * 1024)
/* Auto mode. Set to 1 to place RetRAM before RAM else set to 0. */
#define RETRAM_FIRST                    1
#else
#define CODE_SIZE     ( 56 * 1024)
#define RETRAM_0_SIZE ( 24 * 1024)
#define RETRAM_1_SIZE (  0 * 1024)              // Retention RAM area for Heap(s)
#define RAM_SIZE      ( 38 * 1024)
#define RAM_START     0x7FC0000 + CODE_SIZE // Used only when dg_configCONFIG_VIA_OTP_HEADER is 1
#endif

/* Auto mode. Set to 1 to place RetRAM before RAM else set to 0. */
#define RETRAM_FIRST                    1

/* Manual mode. RETRAM_FIRST is ignored. Comment-out for auto mode. */
//#define RETRAM_0_OFFSET                 0
//#define RAM_OFFSET                      (32 * 1024)
//#define RETRAM_0_BASE                   0x7FC8000
//#define RAM_BASE                        0x7FD0000

/*** Parameterization ends here  - Do not change anything below this line! ***/

/* --------------------------------------------------------------------------------------------- */

#define CACHED_OVERHEAD                 0x100

#if (RETRAM_1_SIZE > CACHED_OVERHEAD)
#define RETRAM_1_SZ                     (RETRAM_1_SIZE - CACHED_OVERHEAD)
#else
#define RETRAM_1_SZ                     0
#endif

#if (dg_configCONFIG_VIA_OTP_HEADER == 0)
# if (dg_configCODE_LOCATION == NON_VOLATILE_IS_NONE)
        #define CODE_BASE_ADDRESS       0x7FC0000
        #define RAM_BASE_ADDRESS        CODE_BASE_ADDRESS + CODE_SIZE
        
# elif (dg_configCODE_LOCATION == NON_VOLATILE_IS_OTP)
#  if (dg_configEXEC_MODE == MODE_IS_MIRRORED)
        #define CODE_BASE_ADDRESS       0x7FC0000
        #define RAM_BASE_ADDRESS        CODE_BASE_ADDRESS + CODE_SIZE
#  else
        #define CODE_BASE_ADDRESS       0x7F80000
        #define RAM_BASE_ADDRESS        0x7FC0000
#  endif

# elif (dg_configCODE_LOCATION == NON_VOLATILE_IS_FLASH)
#  if (dg_configEXEC_MODE == MODE_IS_MIRRORED)
        #warning "QSPI mirrored execution mode is not supported!"
        #undef CODE_SIZE
        #define CODE_SIZE               0
#  else
        #define CODE_BASE_ADDRESS       0x8000000 + dg_configIMAGE_FLASH_OFFSET
        #define RAM_BASE_ADDRESS        0x7FC0000
#  endif

# else
        #warning "Unknown code location type..."
        #undef CODE_SIZE
        #define CODE_SIZE               0
# endif

#else // dg_configCONFIG_VIA_OTP_HEADER
        #define CODE_BASE_ADDRESS       0x0
        #define RAM_BASE_ADDRESS        RAM_START
#endif

#if (dg_configCODE_LOCATION == NON_VOLATILE_IS_FLASH) && (dg_configEXEC_MODE == MODE_IS_CACHED)
        #define __DIFF__                CACHED_OVERHEAD
#else
        #define __DIFF__                0
#endif

#ifdef RETRAM_0_BASE
        #undef RETRAM_0_OFFSET
        #define RETRAM_0_OFFSET         (RETRAM_0_BASE - (RAM_BASE_ADDRESS))
#endif

#ifdef RAM_BASE
        #undef RAM_OFFSET
        #define RAM_OFFSET              (RAM_BASE + RETRAM_1_SZ - (RAM_BASE_ADDRESS))
#endif

#if (RETRAM_FIRST == 1)
        #ifndef RETRAM_0_OFFSET
        #define RETRAM_0_OFFSET         __DIFF__
        #endif
        
        #if (RETRAM_0_SIZE > CACHED_OVERHEAD)
        #define RETRAM_0_SZ             (RETRAM_0_SIZE - __DIFF__)
        #else
        #define RETRAM_0_SZ             0
        #endif
        
        #ifndef RAM_OFFSET
        #define RAM_OFFSET              (RETRAM_0_SZ + __DIFF__)
        #endif
        #define RAM_SZ                  RAM_SIZE
#else
        #ifndef RAM_OFFSET
        #define RAM_OFFSET              __DIFF__
        #endif
        
        #if (RAM_SIZE > __DIFF__)
                #if (RETRAM_1_SIZE > CACHED_OVERHEAD)
                #define RAM_SZ          (RAM_SIZE)
                #else
                #define RAM_SZ          (RAM_SIZE - __DIFF__)
                #endif
        #else
        #define RAM_SZ                  0
        #endif
        
        #ifndef RETRAM_0_OFFSET
        #define RETRAM_0_OFFSET         (RAM_SZ + __DIFF__)
        #endif
        #define RETRAM_0_SZ             RETRAM_0_SIZE
#endif

#if ((RAM_BASE_ADDRESS + RETRAM_0_OFFSET) < 0x7FC0000) || \
    ((RAM_BASE_ADDRESS + RETRAM_0_OFFSET) > 0x7FE0000) || \
    ((RAM_BASE_ADDRESS + RETRAM_0_OFFSET + RETRAM_0_SZ) > 0x7FE0000)
        #warning "RetRAM0 area is out of bounds!"
        #undef CODE_SIZE
        #define CODE_SIZE               0
#endif

#if (dg_configCODE_LOCATION == NON_VOLATILE_IS_NONE)
#if ((RAM_BASE_ADDRESS + RAM_OFFSET) < 0x7FC0000) || \
    ((RAM_BASE_ADDRESS + RAM_OFFSET) > 0x7FE4000) || \
    ((RAM_BASE_ADDRESS + RAM_OFFSET + RAM_SZ) > 0x7FE4000)
        #warning "RAM area is out of bounds!"
        #undef CODE_SIZE
        #define CODE_SIZE               0
#endif

#else

#if ((RAM_BASE_ADDRESS + RAM_OFFSET) < 0x7FC0000) || \
    ((RAM_BASE_ADDRESS + RAM_OFFSET) > 0x7FE0000) || \
    ((RAM_BASE_ADDRESS + RAM_OFFSET + RAM_SZ) > 0x7FE0000)
        #warning "RAM area is out of bounds!"
        #undef CODE_SIZE
        #define CODE_SIZE               0
#endif
#endif

/* --------------------------------------------------------------------------------------------- */

#define _RETRAM_0_BASE_ADDR             ((RAM_BASE_ADDRESS + RETRAM_0_OFFSET) & 0x7FFF000)

#if (dg_configMEM_RETENTION_MODE & 1)
        #define _MEM_1_SZ               0x2000
#else
        #define _MEM_1_SZ               0x0
#endif

#if (dg_configSHUFFLING_MODE == 0)
        #define _MEM_1_BASE             0x7FC0000
        #define RMEM_1_START            (_MEM_1_BASE)
        #define RMEM_1_END              (_MEM_1_BASE + _MEM_1_SZ)
#elif (dg_configSHUFFLING_MODE == 1)
        #define _MEM_1_BASE             0x7FC6000
        #define RMEM_2_START            (_MEM_1_BASE)
        #define RMEM_2_END              (_MEM_1_BASE + _MEM_1_SZ)
#elif (dg_configSHUFFLING_MODE == 2)
        #define _MEM_1_BASE             0x7FC8000
        #define RMEM_2_START            (_MEM_1_BASE)
        #define RMEM_2_END              (_MEM_1_BASE + _MEM_1_SZ)
#else
        #define _MEM_1_BASE             0x7FCE000
        #define RMEM_3_START            (_MEM_1_BASE)
        #define RMEM_3_END              (_MEM_1_BASE + _MEM_1_SZ)
#endif


#if (dg_configMEM_RETENTION_MODE & 2)
        #define _MEM_2_SZ               0x6000
#else
        #define _MEM_2_SZ               0x0
#endif

#if (dg_configSHUFFLING_MODE == 0)
        #define _MEM_2_BASE             0x7FC2000
        #define RMEM_2_START            (_MEM_2_BASE)
        #define RMEM_2_END              (_MEM_2_BASE + _MEM_2_SZ)
#elif (dg_configSHUFFLING_MODE == 1)
        #define _MEM_2_BASE             0x7FC0000
        #define RMEM_1_START            (_MEM_2_BASE)
        #define RMEM_1_END              (_MEM_2_BASE + _MEM_2_SZ)
#elif (dg_configSHUFFLING_MODE == 2)
        #define _MEM_2_BASE             0x7FCA000
        #define RMEM_3_START            (_MEM_2_BASE)
        #define RMEM_3_END              (_MEM_2_BASE + _MEM_2_SZ)
#else
        #define _MEM_2_BASE             0x7FC8000
        #define RMEM_2_START            (_MEM_2_BASE)
        #define RMEM_2_END              (_MEM_2_BASE + _MEM_2_SZ)
#endif


#if (dg_configMEM_RETENTION_MODE & 4)
        #define _MEM_3_SZ               0x8000
#else
        #define _MEM_3_SZ               0x0
#endif

#if (dg_configSHUFFLING_MODE == 0)
        #define _MEM_3_BASE             0x7FC8000
        #define RMEM_3_START            (_MEM_3_BASE)
        #define RMEM_3_END              (_MEM_3_BASE + _MEM_3_SZ)
#elif (dg_configSHUFFLING_MODE == 1)
        #define _MEM_3_BASE             0x7FC8000
        #define RMEM_3_START            (_MEM_3_BASE)
        #define RMEM_3_END              (_MEM_3_BASE + _MEM_3_SZ)
#elif (dg_configSHUFFLING_MODE == 2)
        #define _MEM_3_BASE             0x7FC0000
        #define RMEM_1_START            (_MEM_3_BASE)
        #define RMEM_1_END              (_MEM_3_BASE + _MEM_3_SZ)
#else
        #define _MEM_3_BASE             0x7FC0000
        #define RMEM_1_START            (_MEM_3_BASE)
        #define RMEM_1_END              (_MEM_3_BASE + _MEM_3_SZ)
#endif

#if (dg_configMEM_RETENTION_MODE & 8)
        #define _MEM_4_SZ               0x8000
#else
        #define _MEM_4_SZ               0x0
#endif
#define _MEM_4_BASE                     0x7FD0000
#define RMEM_4_START                    (_MEM_4_BASE)
#define RMEM_4_END                      (_MEM_4_BASE + _MEM_4_SZ)

#if (dg_configMEM_RETENTION_MODE & 16)
        #define _MEM_5_SZ               0x8000
#else
        #define _MEM_5_SZ               0x0
#endif
#define _MEM_5_BASE                     0x7FD8000
#define RMEM_5_START                    (_MEM_5_BASE)
#define RMEM_5_END                      (_MEM_5_BASE + _MEM_5_SZ)

// Retention RAM can be up to 3 blocks.
#if (RMEM_1_START != RMEM_1_END)
        #define RETBLOCK_1_START        RMEM_1_START
# if (RMEM_2_START != RMEM_2_END)
#  if (RMEM_3_START != RMEM_3_END)
#   if (RMEM_4_START != RMEM_4_END)
#    if (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_1_END          RMEM_5_END
#    else
        #define RETBLOCK_1_END          RMEM_4_END
#    endif // (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_2_START        0
        #define RETBLOCK_2_END          0
        #define RETBLOCK_3_START        0
        #define RETBLOCK_3_END          0
#   else // (RMEM_4_START != RMEM_4_END)
        #define RETBLOCK_1_END          RMEM_3_END
#    if (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_2_START        RMEM_5_START
        #define RETBLOCK_2_END          RMEM_5_END
#    else
        #define RETBLOCK_2_START        0
        #define RETBLOCK_2_END          0
#    endif // (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_3_START        0
        #define RETBLOCK_3_END          0
#   endif // (RMEM_4_START != RMEM_4_END)
#  else // (RMEM_3_START != RMEM_3_END)
        #define RETBLOCK_1_END          RMEM_2_END
#   if (RMEM_4_START != RMEM_4_END)
        #define RETBLOCK_2_START        RMEM_4_START
#    if (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_2_END          RMEM_5_END
#    else
        #define RETBLOCK_2_END          RMEM_4_END
#    endif // (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_3_START        0
        #define RETBLOCK_3_END          0
#   else // (RMEM_4_START != RMEM_4_END)
#    if (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_2_START        RMEM_5_START
        #define RETBLOCK_2_END          RMEM_5_END
#    else
        #define RETBLOCK_2_START        0
        #define RETBLOCK_2_END          0
#    endif // (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_3_START        0
        #define RETBLOCK_3_END          0
#   endif // (RMEM_4_START != RMEM_4_END)
#  endif // (RMEM_3_START != RMEM_3_END)
# else // (RMEM_2_START != RMEM_2_END)
        #define RETBLOCK_1_END          RMEM_1_END
#  if (RMEM_3_START != RMEM_3_END)
        #define RETBLOCK_2_START        RMEM_3_START
#   if (RMEM_4_START != RMEM_4_END)
#    if (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_2_END          RMEM_5_END
#    else
        #define RETBLOCK_2_END          RMEM_4_END
#    endif // (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_3_START        0
        #define RETBLOCK_3_END          0
#   else // (RMEM_4_START != RMEM_4_END)
        #define RETBLOCK_2_END          RMEM_3_END
#    if (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_3_START        RMEM_5_START
        #define RETBLOCK_3_END          RMEM_5_END
#    else
        #define RETBLOCK_3_START        0
        #define RETBLOCK_3_END          0
#    endif  // (RMEM_5_START != RMEM_5_END)
#   endif  // (RMEM_4_START != RMEM_4_END)
#  else // (RMEM_3_START != RMEM_3_END)
#   if (RMEM_4_START != RMEM_4_END)
        #define RETBLOCK_2_START        RMEM_4_START
#    if (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_2_END          RMEM_5_END
#    else
        #define RETBLOCK_2_END          RMEM_4_END
#    endif // (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_3_START        0
        #define RETBLOCK_3_END          0
#   else // (RMEM_4_START != RMEM_4_END)
#    if (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_2_START        RMEM_5_START
        #define RETBLOCK_2_END          RMEM_5_END
#    else
        #define RETBLOCK_2_START        0
        #define RETBLOCK_2_END          0
#    endif // (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_3_START        0
        #define RETBLOCK_3_END          0
#   endif // (RMEM_4_START != RMEM_4_END)
#  endif // (RMEM_3_START != RMEM_3_END)
# endif // (RMEM_2_START != RMEM_2_END)

#elif (RMEM_2_START != RMEM_2_END)
        #define RETBLOCK_1_START        RMEM_2_START
# if (RMEM_3_START != RMEM_3_END)
#  if (RMEM_4_START != RMEM_4_END)
#   if (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_1_END          RMEM_5_END
#   else
        #define RETBLOCK_1_END          RMEM_4_END
#   endif // (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_2_START        0
        #define RETBLOCK_2_END          0
        #define RETBLOCK_3_START        0
        #define RETBLOCK_3_END          0
#  else // (RMEM_4_START != RMEM_4_END)
        #define RETBLOCK_1_END          RMEM_3_END
#   if (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_2_START        RMEM_5_START
        #define RETBLOCK_2_END          RMEM_5_END
#   else
        #define RETBLOCK_2_START        0
        #define RETBLOCK_2_END          0
#   endif // (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_3_START        0
        #define RETBLOCK_3_END          0
#  endif // (RMEM_4_START != RMEM_4_END)
# else // (RMEM_3_START != RMEM_3_END)
        #define RETBLOCK_1_END          RMEM_2_END
#  if (RMEM_4_START != RMEM_4_END)
        #define RETBLOCK_2_START        RMEM_4_START
#   if (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_2_END          RMEM_5_END
#   else
        #define RETBLOCK_2_END          RMEM_4_END
#   endif // (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_3_START        0
        #define RETBLOCK_3_END          0
#  else // (RMEM_4_START != RMEM_4_END)
#   if (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_2_START        RMEM_5_START
        #define RETBLOCK_2_END          RMEM_5_END
#   else
        #define RETBLOCK_2_START        0
        #define RETBLOCK_2_END          0
#   endif // (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_3_START        0
        #define RETBLOCK_3_END          0
#  endif // (RMEM_4_START != RMEM_4_END)
# endif // (RMEM_3_START != RMEM_3_END)

#elif (RMEM_3_START != RMEM_3_END)
        #define RETBLOCK_1_START        RMEM_3_START
# if (RMEM_4_START != RMEM_4_END)
#  if (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_1_END          RMEM_5_END
#  else
        #define RETBLOCK_1_END          RMEM_4_END
#  endif // (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_2_START        0
        #define RETBLOCK_2_END          0
        #define RETBLOCK_3_START        0
        #define RETBLOCK_3_END          0
# else // (RMEM_4_START != RMEM_4_END)
        #define RETBLOCK_1_END          RMEM_3_END
#  if (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_2_START        RMEM_5_START
        #define RETBLOCK_2_END          RMEM_5_END
#  else
        #define RETBLOCK_2_START        0
        #define RETBLOCK_2_END          0
#  endif // (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_3_START        0
        #define RETBLOCK_3_END          0
# endif // (RMEM_4_START != RMEM_4_END)

#elif (RMEM_4_START != RMEM_4_END)
        #define RETBLOCK_1_START        RMEM_4_START
# if (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_1_END          RMEM_5_END
# else
        #define RETBLOCK_1_END          RMEM_4_END
# endif // (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_2_START        0
        #define RETBLOCK_2_END          0
        #define RETBLOCK_3_START        0
        #define RETBLOCK_3_END          0

#elif (RMEM_5_START != RMEM_5_END)
        #define RETBLOCK_1_START        RMEM_4_START
        #define RETBLOCK_1_END          RMEM_5_END
        #define RETBLOCK_2_START        0
        #define RETBLOCK_2_END          0
        #define RETBLOCK_3_START        0
        #define RETBLOCK_3_END          0
#endif

#if (RETRAM_0_SIZE != 0)
# if (dg_configMEM_RETENTION_MODE == 0)
# warning "RetRAM is used but dg_configMEM_RETENTION_MODE is 0!"
# undef CODE_SIZE
# define CODE_SIZE                      0
# endif

# if (_RETRAM_0_BASE_ADDR >= RETBLOCK_1_START) && ((_RETRAM_0_BASE_ADDR + RETRAM_0_SIZE) <= RETBLOCK_1_END)
        // RetRAM0 belongs to a retained block
# elif (_RETRAM_0_BASE_ADDR >= RETBLOCK_2_START) && ((_RETRAM_0_BASE_ADDR + RETRAM_0_SIZE) <= RETBLOCK_2_END)
        // RetRAM0 belongs to a retained block
# elif (_RETRAM_0_BASE_ADDR >= RETBLOCK_3_START) && ((_RETRAM_0_BASE_ADDR + RETRAM_0_SIZE) <= RETBLOCK_3_END)
        // RetRAM0 belongs to a retained block
# else
# warning "RetRAM0 is used but dg_configMEM_RETENTION_MODE (or dg_configSHUFFLING_MODE) is not correct!"
# undef CODE_SIZE
# define CODE_SIZE                      0
# endif
#endif

#if (RETRAM_1_SIZE > CACHED_OVERHEAD)
# if (RETRAM_FIRST == 0)
# define _RETRAM_1_BASE_ADDR            (RETBLOCK_1_START + CACHED_OVERHEAD)
# else
# define _RETRAM_1_BASE_ADDR            (_RETRAM_0_BASE_ADDR + RETRAM_0_SIZE)
# endif
#else
#define _RETRAM_1_BASE_ADDR             (0)
#endif

#if ((_RETRAM_1_BASE_ADDR + RETRAM_1_SZ) <= RETBLOCK_1_END)
        // RetRAM1 belongs to a retained block
#else
# warning "RetRAM for Heaps is used but dg_configMEM_RETENTION_MODE (or dg_configSHUFFLING_MODE) is not correct!"
#endif

#ifdef FOR_MEMLD
MEMORY
{
        ROM (rx)     : ORIGIN = CODE_BASE_ADDRESS,                  LENGTH = CODE_SIZE
        RetRAM0 (rwx): ORIGIN = RAM_BASE_ADDRESS + RETRAM_0_OFFSET, LENGTH = RETRAM_0_SZ
        RetRAM1 (rwx): ORIGIN = _RETRAM_1_BASE_ADDR,                LENGTH = RETRAM_1_SZ
        RAM (rw)     : ORIGIN = RAM_BASE_ADDRESS + RAM_OFFSET,      LENGTH = RAM_SZ
}
#endif
