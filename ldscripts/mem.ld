MEMORY
{
        ROM (rx) : ORIGIN = 0x8000000 + (0x20000), LENGTH = ( 128 * 1024)
        RetRAM0 (rwx): ORIGIN = 0x7FC0000 + (0x7FE0000-( 64 * 1024) - (0x7FC0000)), LENGTH = (( 64 * 1024) - 0x100)
        RetRAM1 (rwx): ORIGIN = (0), LENGTH = 0
        RAM (rw) : ORIGIN = 0x7FC0000 + (0x7FC0100 + 0 - (0x7FC0000)), LENGTH = (( 32 * 1024) - 0x100)
}
