/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions ROM, RetRAM0 and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __exidx_start
 *   __exidx_end
 *   __copy_table_start__
 *   __copy_table_end__
 *   __zero_table_start__
 *   __zero_table_end__
 *   __etext
 *   __image_size
 *   __mirrored_image_size
 *   __data_start__
 *   __preinit_array_start
 *   __preinit_array_end
 *   __init_array_start
 *   __init_array_end
 *   __fini_array_start
 *   __fini_array_end
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 *   __RetRAM0_code_start__
 *   __RetRAM0_code_end__
 *   __RetRAM0_data_end__
 *   __RetRAM0_start
 *   __RetRAM0_size
 *   __RetRAM1_start
 *   __RetRAM1_end
 */
ENTRY(Reset_Handler)

SECTIONS
{
        .text :
        {
                KEEP(*(.isr_vector))
                /* make sure that IVT doesn't cross 0xC0 */
                . = 0xC0;
                
                KEEP(*(.patch_table))
                . = 0x100; /* make sure that patch_table entries are exactly 16 - else fill */
                KEEP(*(.patch_table_flash))
                . = 0x130; /* make sure that patch_table_flash entries are exactly 12 - else fill */
                
                KEEP(*(.default_patch_code_handler_section))

                *(EXCLUDE_FILE(*libgcc.a:_aeabi_uldivmod.o *libgcc.a:_muldi3.o *libgcc.a:_dvmd_tls.o *libgcc.a:bpabi.o *libgcc.a:_udivdi3.o *libgcc.a:_clzdi2.o *libgcc.a:_clzsi2.o) .text*)

                KEEP(*(.init))
                KEEP(*(.fini))

                /* .ctors */
                *crtbegin.o(.ctors)
                *crtbegin?.o(.ctors)
                *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
                *(SORT(.ctors.*))
                *(.ctors)

                /* .dtors */
                *crtbegin.o(.dtors)
                *crtbegin?.o(.dtors)
                *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
                *(SORT(.dtors.*))
                *(.dtors)

                . = ALIGN(4);
                /* preinit data */
                PROVIDE_HIDDEN (__preinit_array_start = .);
                KEEP(*(.preinit_array))
                PROVIDE_HIDDEN (__preinit_array_end = .);

                . = ALIGN(4);
                /* init data */
                PROVIDE_HIDDEN (__init_array_start = .);
                KEEP(*(SORT(.init_array.*)))
                KEEP(*(.init_array))
                PROVIDE_HIDDEN (__init_array_end = .);


                . = ALIGN(4);
                /* finit data */
                PROVIDE_HIDDEN (__fini_array_start = .);
                KEEP(*(SORT(.fini_array.*)))
                KEEP(*(.fini_array))
                PROVIDE_HIDDEN (__fini_array_end = .);

                *(.rodata*)

                KEEP(*(.eh_frame*))
        } > ROM

        .ARM.extab :
        {
                *(.ARM.extab* .gnu.linkonce.armextab.*)
        } > ROM

        __exidx_start = .;
        .ARM.exidx :
        {
                *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        } > ROM
        __exidx_end = .;

        /* To copy multiple ROM to RAM sections,
         * uncomment .copy.table section and,
         * define __STARTUP_COPY_MULTIPLE in startup_ARMCMx.S */
        .copy.table :
        {
                . = ALIGN(4);
                __copy_table_start__ = .;
                LONG (__etext + (__data_end__ - __data_start__))
                LONG (__RetRAM0_code_start__)
                LONG (__RetRAM0_code_end__ - __RetRAM0_code_start__)
                LONG (__etext)
                LONG (__data_start__)
                LONG (__data_end__ - __data_start__)
                __copy_table_end__ = .;
        } > ROM

        /* To clear multiple BSS sections,
         * uncomment .zero.table section and,
         * define __STARTUP_CLEAR_BSS_MULTIPLE in startup_ARMCMx.S */
        .zero.table :
        {
                . = ALIGN(4);
                __zero_table_start__ = .;
                LONG (__bss_start__)
                LONG (__bss_end__ - __bss_start__)
                LONG (__RetRAM0_data_start)
                LONG (__RetRAM0_size)
                LONG (__RetRAM1_start__)
                LONG (__RetRAM1_end__ - __RetRAM1_start__)
                __zero_table_end__ = .;
        } > ROM

        __etext = .;

        /* The initialised data section is stored immediately
        at the end of the text section */
        .data : AT (__etext)
        {
                __data_start__ = .;
                *(vtable)
                *(.data*)

                . = ALIGN(4);
                /* init_array/fini_array moved to flash, align preserved */

                KEEP(*(.jcr*))
                . = ALIGN(4);
                /* All data end */
                __data_end__ = .;

        } > RAM

        .bss :
        {
                . = ALIGN(4);
                __bss_start__ = .;
                *(.bss*)
                *(COMMON)
                . = ALIGN(4);
                __bss_end__ = .;
        } > RAM

        .heap (COPY):
        {
                __end__ = .;
                PROVIDE(end = .);
                *(.heap*)
                __HeapLimit = .;
        } > RAM

        /* .stack_dummy section doesn't contains any symbols. It is only
         * used for linker to calculate size of stack sections, and assign
         * values to stack symbols later */
        .stack_dummy (COPY):
        {
                *(.stack*)
        } > RAM

        /* Set stack top to end of RAM, and stack limit move down by
         * size of stack_dummy section */
        __StackTop = ORIGIN(RAM) + LENGTH(RAM);
        __StackLimit = __StackTop - SIZEOF(.stack_dummy);
        PROVIDE(__stack = __StackTop);

        /* Check if data + heap + stack exceeds RAM limit */
        ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")

        /* Retention RAM - Code and Initialized data section
         * Put any code that has to be retained during pm_mode_extended_sleep or pm_mode_deep_sleep in this
         * area. Any initialized variables are also placed in this area.
         * WARNING: THE IMAGE SIZE WILL BE INCREASED BY THE SIZE OF THIS AREA. Put ONLY the
         *          ABSOLUTELY NECESSARY IN HERE!
         */
        RETENTION_ROM0 : AT (__etext + (__data_end__ - __data_start__))
        {
                . = ALIGN(4);
                __RetRAM0_code_start__ = .;
                *(text_retained)
                *libgcc.a:_aeabi_uldivmod.o (.text*)
                *libgcc.a:_muldi3.o (.text*)
                *libgcc.a:_dvmd_tls.o (.text*)
                *libgcc.a:bpabi.o (.text*)
                *libgcc.a:_udivdi3.o (.text*)
                *libgcc.a:_clzdi2.o (.text*)
                *libgcc.a:_clzsi2.o (.text*)
                . = ALIGN(4);
                *(retention_mem_rw)
                *(privileged_data_rw)
                *(.retention)
                . = ALIGN(4);
                __RetRAM0_code_end__ = .;
                . = ALIGN(4);
        } > RetRAM0

        /* Retention RAM 0 - Non-init and Zero-init data section */
        RETENTION_RAM0 (NOLOAD) :
        {
                . = ALIGN(4);
                __RetRAM0_start = .;
                *(nmi_info)
                *(hard_fault_info)
                *(retention_mem_uninit)
                . = ALIGN(4);
                __RetRAM0_data_start = .;
                *(privileged_data_zi)
                *(retention_mem_zi)
                *(os_heap)
                *(ble_msg_heap)
                *(ble_env_heap)
                *(ble_db_heap)
                . = ALIGN(4);
                __RetRAM0_data_end__ = .;
        } > RetRAM0

        /* Retention RAM 1 - Zero-init heaps section */
        RETENTION_RAM1 (NOLOAD) :
        {
                . = ALIGN(4);
                __RetRAM1_start__ = .;
                . = ALIGN(4);
                __RetRAM1_end__ = .;
        } > RetRAM1

#if ((dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A) \
                && (dg_configBLACK_ORCA_IC_STEP <= BLACK_ORCA_IC_STEP_D))
        RETENTION_BLE 0x7FDEC00 (NOLOAD) :
        {
                KEEP(*(ble_variables))
        } > RetRAM0
#elif ((dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A) \
                && (dg_configBLACK_ORCA_IC_STEP == BLACK_ORCA_IC_STEP_E))
                        /* Check if RAM does not overlap ROM variables */
        RETENTION_BLE 0x07FDC000 (NOLOAD) :
        {
                KEEP(*(ble_variables))
        } > RetRAM0
#else
	ASSERT(0, "Unsupported chip version")
#endif

        /* Set start and size of RetRAMs */
        __RetRAM0_size = LENGTH(RetRAM0) - (__RetRAM0_data_start - __RetRAM0_code_start__);

        /* Set the image size */
        __image_size = __etext + (__data_end__ - __data_start__)
                                                   + (__RetRAM0_code_end__ - __RetRAM0_code_start__)
                                                   - ORIGIN(ROM);

        /* Set the mirrored image size.
         * The RETENTION_ROM0 area does not have to be copied again from the image.
         */
        __mirrored_image_size = __etext + (__data_end__ - __data_start__) - ORIGIN(ROM);

#if ((dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A) \
                && (dg_configBLACK_ORCA_IC_STEP <= BLACK_ORCA_IC_STEP_D))

        /* Check if RAM does not overlap ROM variables */
        ASSERT((ORIGIN(RAM) == 0x7FE0000) || (ORIGIN(RAM) + LENGTH(RAM) < 0x7FDEC00), "ROM variables region (starting at 0x7FDEC00) overflowed by RAM")

        /* Check if RetRAM0 does not overlap ROM variables */
        ASSERT(__RetRAM0_data_end__ < 0x7FDEC00, "ROM variables region (starting at 0x7FDEC00) overflowed by RetRAM0")

        INCLUDE da14681_00_rom.symbols
#elif ((dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A) \
                && (dg_configBLACK_ORCA_IC_STEP == BLACK_ORCA_IC_STEP_E))
                        /* Check if RAM does not overlap ROM variables */
        ASSERT((ORIGIN(RAM) == 0x7FE0000) || (ORIGIN(RAM) + LENGTH(RAM) < 0x07FDC000), "ROM variables region (starting at 0x07FDC000) overflowed by RAM")

        /* Check if RetRAM0 does not overlap ROM variables */
        ASSERT(__RetRAM0_data_end__ < 0x07FDC000, "ROM variables region (starting at 0x07FDC000) overflowed by RetRAM0")

        INCLUDE da14681_01_rom.symbols
#endif
}
