MEMORY
{
        ROM (rx) : ORIGIN = 0x8000000 + (0), LENGTH = ( 56 * 1024)
        RetRAM0 (rwx): ORIGIN = 0x7FC0000 + 0x100, LENGTH = (( 24 * 1024) - 0x100)
        RetRAM1 (rwx): ORIGIN = (0), LENGTH = 0
        RAM (rw) : ORIGIN = 0x7FC0000 + ((( 24 * 1024) - 0x100) + 0x100), LENGTH = ( 38 * 1024)
}
