/* One WS2812B-V5 on CH552 P1.5, SDCC, Fsys = 24 MHz.
 * The including file declares the LED_DATA and EA bit registers.
 * Called from main (register bank 0), never from an interrupt.
 */
#ifndef SL25_WS2812_CH552_H
#define SL25_WS2812_CH552_H

/* Expand GRB before disabling interrupts: no calls or C branches per bit. */
static __data unsigned char ws_symbols[24];

static void ws_send_symbols(void) __naked
{
    __asm
        mov r0,#_ws_symbols
        mov r7,#24
00001$:
        mov a,@r0
        rlc a
        setb _LED_DATA
        nop
        nop
        nop
        nop
        nop
        nop
        mov _LED_DATA,c
        nop
        nop
        nop
        nop
        nop
        clr _LED_DATA
        nop
        nop
        nop
        nop
        nop
        nop
        inc r0
        djnz r7,00001$
        ret
    __endasm;
}

/* WCH instruction timing at 24 MHz:
 * T0H = 8 clocks = 333 ns; T1H = 15 clocks = 625 ns.
 * Taken DJNZ costs 4/5 clocks depending on the linked target's parity:
 * T0L = 22/23 clocks = 917/958 ns; T1L = 15/16 = 625/667 ns.
 * Both cases meet the project's WS2812B-V5 data sheet. The final low
 * interval is the latch interval, not another data bit.
 * Each assembly instruction MUST occupy its own line: ';' starts a comment.
 */

/* More than 400 us at 24 MHz, even with the shorter branch timing.
 * Interrupts may extend this low interval; they cannot shorten it.
 */
static void ws_reset_delay(void) __naked
{
    __asm
        mov r6,#8
00002$:
        mov r7,#250
00003$:
        nop
        djnz r7,00003$
        djnz r6,00002$
        ret
    __endasm;
}

static void ws_pack_byte(unsigned char value, unsigned char offset)
{
    unsigned char bit;
    for (bit = 0; bit < 8; ++bit) {
        ws_symbols[offset + bit] = value & 0x80;
        value <<= 1;
    }
}

static void ws_color(unsigned char red, unsigned char green, unsigned char blue)
{
    __bit saved_ea;
    ws_pack_byte(green, 0);
    ws_pack_byte(red, 8);
    ws_pack_byte(blue, 16);
    saved_ea = EA;
    EA = 0;
    ws_send_symbols();
    EA = saved_ea;
    ws_reset_delay();
}

#endif
