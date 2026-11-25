/* Standalone SL25 Rev B diagnostic: red, green, blue, off, repeat.
 * Does not enumerate as a USB application or require a host agent.
 * USB ISP remains available by using JP1 during power-on.
 */
__sfr __at (0x92) P1_MOD_OC;
__sfr __at (0x93) P1_DIR_PU;
__sfr __at (0xA1) SAFE_MOD;
__sfr __at (0xB9) CLOCK_CFG;
__sfr __at (0xD1) UDEV_CTRL;
__sfr __at (0xE1) USB_INT_EN;
__sfr __at (0xE2) USB_CTRL;
__sfr __at (0x89) TMOD;
__sfr __at (0x8A) TL0;
__sfr __at (0x8C) TH0;
__sfr __at (0xC9) T2MOD;
__sbit __at (0x8C) TR0;
__sbit __at (0x8D) TF0;
__sbit __at (0x95) LED_DATA;
__sbit __at (0xAF) EA;

#ifndef SL25_GPIO_PROBE
#include "../common/ws2812_ch552.h"
#endif

static void delay_ms(unsigned short ms)
{
    while (ms--) {
        TR0 = 0;
        TF0 = 0;
        TH0 = 0xF8;
        TL0 = 0x30; /* 65536 - 2000; timer clock = 24 MHz / 12 */
        TR0 = 1;
        while (!TF0) {}
        TR0 = 0;
    }
}

void main(void)
{
    EA = 0;
    USB_INT_EN = 0;
    USB_CTRL = 0;
    UDEV_CTRL = 0;
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
    CLOCK_CFG = 0x86;
    SAFE_MOD = 0;
    LED_DATA = 0;
    P1_MOD_OC &= ~0x20;
    P1_DIR_PU |= 0x20;
    T2MOD &= ~0x10; /* Timer 0 uses Fsys / 12. */
    TMOD = (TMOD & 0xF0) | 0x01;
    delay_ms(20);
    while (1) {
#ifdef SL25_GPIO_PROBE
        /* For a bare LED footprint: read each plateau with a DC voltmeter. */
        LED_DATA = 0;
        delay_ms(3000);
        LED_DATA = 1;
        delay_ms(3000);
#else
        ws_color(32, 0, 0);
        delay_ms(1000);
        ws_color(0, 32, 0);
        delay_ms(1000);
        ws_color(0, 0, 32);
        delay_ms(1000);
        ws_color(0, 0, 0);
        delay_ms(1000);
#endif
    }
}
