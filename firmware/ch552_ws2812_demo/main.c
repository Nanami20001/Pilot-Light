/* SL25 Rev B: CH552G + WS2812B-V5, P1.5, full-white test. */
typedef unsigned int u16;
__sfr __at (0x92) P1_MOD_OC;
__sfr __at (0x93) P1_DIR_PU;
__sfr __at (0xA1) SAFE_MOD;
__sfr __at (0xB9) CLOCK_CFG;
__sbit __at (0x95) LED_DATA;
__sbit __at (0xAF) EA;
static void delay_ms(u16 ms)
{
    volatile u16 i;
    while (ms--) {
        for (i = 0; i < 1000; ++i) {
            __asm nop __endasm;
        }
    }
}
/* GRB=FF FF FF: 24 one bits, unrolled to avoid compiler overhead.
 * At 24MHz: NOP=1 cycle, SETB/CLR bit=2 cycles.
 * Nominal T1H=875ns, T1L=375ns. Verify with a scope on hardware.
 */
static void ws_white(void)
{
    __asm
        setb _LED_DATA
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
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
        nop
        setb _LED_DATA
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
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
        nop
        setb _LED_DATA
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
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
        nop
        setb _LED_DATA
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
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
        nop
        setb _LED_DATA
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
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
        nop
        setb _LED_DATA
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
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
        nop
        setb _LED_DATA
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
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
        nop
        setb _LED_DATA
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
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
        nop
        setb _LED_DATA
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
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
        nop
        setb _LED_DATA
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
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
        nop
        setb _LED_DATA
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
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
        nop
        setb _LED_DATA
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
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
        nop
        setb _LED_DATA
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
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
        nop
        setb _LED_DATA
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
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
        nop
        setb _LED_DATA
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
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
        nop
        setb _LED_DATA
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
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
        nop
        setb _LED_DATA
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
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
        nop
        setb _LED_DATA
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
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
        nop
        setb _LED_DATA
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
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
        nop
        setb _LED_DATA
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
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
        nop
        setb _LED_DATA
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
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
        nop
        setb _LED_DATA
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
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
        nop
        setb _LED_DATA
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
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
        nop
        setb _LED_DATA
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
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
        nop
    __endasm;
}
void main(void)
{
    EA = 0;
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
    CLOCK_CFG = 0x86; /* internal oscillator, 24MHz */
    SAFE_MOD = 0x00;
    LED_DATA = 0;
    P1_MOD_OC &= ~0x20; /* push-pull */
    P1_DIR_PU |= 0x20; /* output */
    delay_ms(10); /* LED power-up settling */
    while (1) {
        ws_white();
        LED_DATA = 0;
        delay_ms(10); /* reset >280us; refresh same white frame */
    }
}
