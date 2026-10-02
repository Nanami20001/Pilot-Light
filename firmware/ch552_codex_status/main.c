/*
 * SL25 Rev B final application firmware
 *
 * CH552G USB HID OUT endpoint + one WS2812B-V5 on P1.5.
 * Build with SDCC. Register addresses follow the WCH CH552 EVT header.
 *
 * HID report (8 bytes, no report ID):
 *   [state, effect, brightness, R, G, B, sequence, reserved]
 * The host agent sends an HID report with a leading report-ID byte 0. The
 * CH552 USB OUT endpoint receives the eight payload bytes after that ID.
 */
/* Keep the application buildable with current SDCC as well as the older WCH
 * EVT compiler. The EVT header uses the older `sfr P = 0xNN` spelling. */
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned char __xdata *PUINT8;
typedef const unsigned char __code *PCUINT8;
typedef struct {
    UINT8 bRequestType;
    UINT8 bRequest;
    UINT8 wValueL;
    UINT8 wValueH;
    UINT8 wIndexL;
    UINT8 wIndexH;
    UINT8 wLengthL;
    UINT8 wLengthH;
} USB_SETUP_REQ;
typedef USB_SETUP_REQ __xdata *PUSB_SETUP_REQ;

__sfr __at (0x90) P1;
__sfr __at (0x92) P1_MOD_OC;
__sfr __at (0x93) P1_DIR_PU;
__sfr __at (0xD1) UDEV_CTRL;
__sfr __at (0xD2) UEP1_CTRL;
__sfr __at (0xD3) UEP1_T_LEN;
__sfr __at (0xD8) USB_INT_FG;
__sfr __at (0xD9) USB_INT_ST;
__sfr __at (0xDA) USB_MIS_ST;
__sfr __at (0xDB) USB_RX_LEN;
__sfr __at (0xDC) UEP0_CTRL;
__sfr __at (0xDD) UEP0_T_LEN;
__sfr __at (0xE1) USB_INT_EN;
__sfr __at (0xE2) USB_CTRL;
__sfr __at (0xE3) USB_DEV_AD;
__sfr __at (0xEA) UEP4_1_MOD;
__sfr __at (0xEC) UEP0_DMA_L;
__sfr __at (0xED) UEP0_DMA_H;
__sfr __at (0xEE) UEP1_DMA_L;
__sfr __at (0xEF) UEP1_DMA_H;
__sfr __at (0xA8) IE;
__sbit __at (0xAF) EA;
__sbit __at (0xEA) IE_USB;
__sbit __at (0x95) LED_DATA;       /* P1.5 */

#define INT_NO_USB             8
#define bUD_PD_DIS             0x80
#define bUD_PORT_EN            0x01
#define bUC_DEV_PU_EN          0x20
#define bUC_INT_BUSY           0x08
#define bUC_DMA_EN             0x01
#define bUIE_DEV_SOF           0x80
#define bUIE_SUSPEND           0x04
#define bUIE_TRANSFER          0x02
#define bUIE_BUS_RST           0x01
#define bUEP1_RX_EN            0x80
#define bUEP_AUTO_TOG          0x10
#define MASK_UEP_R_RES         0x0C
#define UEP_R_RES_ACK          0x00
#define UEP_T_RES_ACK          0x00
#define UEP_T_RES_NAK          0x02
#define bUEP_R_TOG             0x80
#define bUEP_T_TOG             0x40
#define MASK_UIS_TOKEN         0x30
#define MASK_UIS_ENDP          0x0F
#define UIS_TOKEN_OUT          0x00
#define UIS_TOKEN_SOF          0x10
#define UIS_TOKEN_IN           0x20
#define UIS_TOKEN_SETUP        0x30
#define bUMS_SUSPEND           0x04
#define UIF_SUSPEND            0x04
#define UIF_TRANSFER           0x02
#define UIF_BUS_RST            0x01
#define USB_GET_STATUS         0x00
#define USB_GET_DESCRIPTOR     0x06
#define USB_SET_ADDRESS        0x05
#define USB_GET_CONFIGURATION  0x08
#define USB_SET_CONFIGURATION  0x09
#define USB_GET_INTERFACE      0x0A
#define USB_SET_INTERFACE      0x0B
#define USB_CLEAR_FEATURE      0x01
#define USB_SET_FEATURE        0x03
#define USB_REQ_TYP_IN         0x80
#define USB_REQ_TYP_OUT        0x00
#define USB_REQ_TYP_STANDARD   0x00
#define USB_REQ_TYP_CLASS      0x20
#define USB_REQ_TYP_MASK       0x60
#define USB_REQ_RECIP_INTERF   0x01
#define USB_REQ_RECIP_MASK     0x1F
#define HID_GET_REPORT         0x01
#define HID_GET_IDLE           0x02
#define HID_GET_PROTOCOL       0x03
#define HID_SET_REPORT         0x09
#define HID_SET_IDLE           0x0A
#define HID_SET_PROTOCOL       0x0B
#define USB_DESCR_TYP_DEVICE   0x01
#define USB_DESCR_TYP_CONFIG   0x02
#define USB_DESCR_TYP_STRING   0x03
#define USB_DESCR_TYP_HID      0x22
#define USB_REQ_TYP_CLASS      0x20
#define bUDA_GP_BIT            0x80
#define MASK_UEP_T_RES         0x03

#define SL25_USB_VID_L       0x55
#define SL25_USB_VID_H       0xF0  /* prototype/private VID; replace before distribution */
#define SL25_USB_PID_L       0x25
#define SL25_USB_PID_H       0x25

#define SL25_STATE_OFFLINE   0
#define SL25_STATE_WORKING   1
#define SL25_STATE_WAITING   2
#define SL25_STATE_READY     3
#define SL25_STATE_ERROR     4

#define SL25_EFFECT_SOLID    0
#define SL25_EFFECT_BREATHE  1
#define SL25_EFFECT_BLINK    2

#define SL25_REPORT_SIZE     8
#define SL25_TIMEOUT_MS      3000

/* USB RAM buffers. EP0 starts at 0x0000; EP1 OUT follows at an even address. */
__xdata __at (0x0000) UINT8 Ep0Buffer[8];
__xdata __at (0x000A) UINT8 Ep1Buffer[8];

static volatile UINT8 UsbConfig;
static volatile UINT8 SetupReq;
static volatile UINT8 SetupLen;
static volatile UINT16 SystemMs;
static volatile UINT16 LastReportMs;
static volatile UINT8 Report[SL25_REPORT_SIZE];
static PCUINT8 pDescr;

static const UINT8 __code DevDesc[] = {
    0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x08,
    SL25_USB_VID_L, SL25_USB_VID_H, SL25_USB_PID_L, SL25_USB_PID_H,
    0x00, 0x01, 0x01, 0x02, 0x00, 0x01
};

/* Configuration + interface + HID + one interrupt OUT endpoint. */
static const UINT8 __code CfgDesc[] = {
    0x09, 0x02, 0x22, 0x00, 0x01, 0x01, 0x00, 0xA0, 0x32,
    0x09, 0x04, 0x00, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00,
    0x09, 0x21, 0x11, 0x01, 0x00, 0x01, 0x22, 0x15, 0x00,
    0x07, 0x05, 0x01, 0x03, 0x08, 0x00, 0x01
};

static const UINT8 __code HidReportDesc[] = {
    0x06, 0x00, 0xFF, 0x09, 0x01, 0xA1, 0x01,
    0x15, 0x00, 0x26, 0xFF, 0x00, 0x75, 0x08,
    0x95, 0x08, 0x09, 0x02, 0x91, 0x02, 0xC0
};

static const UINT8 __code LangDesc[] = { 0x04, 0x03, 0x09, 0x04 };
static const UINT8 __code ManuDesc[] = {
    0x10, 0x03, 'S', 0, 'L', 0, '2', 0, '5', 0, ' ', 0, 'P', 0, 'r', 0
};
static const UINT8 __code ProdDesc[] = {
    0x26, 0x03,
    'C',0,'o',0,'d',0,'e',0,'x',0,' ',0,'S',0,'t',0,'a',0,'t',0,'u',0,'s',0,
    ' ',0,'L',0,'i',0,'g',0,'h',0,'t',0
};

static void ws_reset_delay(void)
{
    UINT16 i;
    /* At 24 MHz this keeps DIN low for comfortably more than 280 us. */
    for (i = 0; i < 900; ++i) {
        __asm nop; nop; nop; nop; __endasm;
    }
}

static void ws_bit(UINT8 one)
{
    LED_DATA = 1;
    if (one) {
        __asm nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; __endasm;
        LED_DATA = 0;
        __asm nop; nop; nop; nop; __endasm;
    } else {
        __asm nop; nop; nop; __endasm;
        LED_DATA = 0;
        __asm nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; __endasm;
    }
}

static void ws_byte(UINT8 value)
{
    UINT8 mask;
    for (mask = 0x80; mask != 0; mask >>= 1) ws_bit((value & mask) != 0);
}

static void ws_color(UINT8 red, UINT8 green, UINT8 blue)
{
    EA = 0;
    ws_byte(green);  /* WS2812 order is GRB, MSB first. */
    ws_byte(red);
    ws_byte(blue);
    LED_DATA = 0;
    EA = 1;
    ws_reset_delay();
}

static UINT8 scale8(UINT8 value, UINT8 brightness)
{
    return (UINT8)(((UINT16)value * brightness) >> 8);
}

static void render_status(void)
{
    UINT8 state = Report[0];
    UINT8 effect = Report[1];
    UINT8 brightness = Report[2];
    UINT8 red = Report[3];
    UINT8 green = Report[4];
    UINT8 blue = Report[5];
    UINT16 now;
    UINT8 phase;

    EA = 0;
    now = SystemMs;
    EA = 1;
    if (!UsbConfig || (UINT16)(now - LastReportMs) > SL25_TIMEOUT_MS || state == SL25_STATE_OFFLINE) {
        ws_color(0, 0, 0);
        return;
    }

    /* The host supplies RGB, while the state still selects a safe default. */
    if (state == SL25_STATE_WORKING) {
        red = 0; green = 180; blue = 0;
    } else if (state == SL25_STATE_WAITING) {
        red = 220; green = 150; blue = 0;
    } else if (state == SL25_STATE_READY) {
        red = 0; green = 70; blue = 0;
    } else if (state == SL25_STATE_ERROR) {
        red = 220; green = 0; blue = 0;
    }

    if (effect == SL25_EFFECT_BLINK) {
        phase = (UINT8)((now / 300) & 1);
        if (!phase) red = green = blue = 0;
    } else if (effect == SL25_EFFECT_BREATHE) {
        phase = (UINT8)((now / 16) & 0x1F);
        if (phase > 16) phase = 32 - phase;
        brightness = (UINT8)(((UINT16)brightness * phase) / 16);
    }
    ws_color(scale8(red, brightness), scale8(green, brightness), scale8(blue, brightness));
}

static UINT8 descriptor_for_request(PCUINT8 *descriptor)
{
    UINT8 dtype = ((PUSB_SETUP_REQ)Ep0Buffer)->wValueH;
    UINT8 index = ((PUSB_SETUP_REQ)Ep0Buffer)->wValueL;
    switch (dtype) {
        case USB_DESCR_TYP_DEVICE: *descriptor = DevDesc; return sizeof(DevDesc);
        case USB_DESCR_TYP_CONFIG: *descriptor = CfgDesc; return sizeof(CfgDesc);
        case USB_DESCR_TYP_HID: *descriptor = HidReportDesc; return sizeof(HidReportDesc);
        case USB_DESCR_TYP_STRING:
            if (index == 0) { *descriptor = LangDesc; return sizeof(LangDesc); }
            if (index == 1) { *descriptor = ManuDesc; return sizeof(ManuDesc); }
            if (index == 2) { *descriptor = ProdDesc; return sizeof(ProdDesc); }
            return 0xFF;
        default: return 0xFF;
    }
}

static void usb_init(void)
{
    IE_USB = 0;
    USB_CTRL = 0x00;
    UEP0_DMA_L = 0x00;
    UEP0_DMA_H = 0x00;
    UEP1_DMA_L = 0x0A;
    UEP1_DMA_H = 0x00;
    UEP4_1_MOD = bUEP1_RX_EN;
    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
    UEP1_CTRL = bUEP_AUTO_TOG | UEP_R_RES_ACK;
    USB_DEV_AD = 0;
    UDEV_CTRL = bUD_PD_DIS | bUD_PORT_EN;
    USB_CTRL = bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN;
    USB_INT_FG = 0xFF;
    USB_INT_EN = bUIE_DEV_SOF | bUIE_SUSPEND | bUIE_TRANSFER | bUIE_BUS_RST;
    IE_USB = 1;
}

void usb_interrupt(void) __interrupt(INT_NO_USB) __using(1)
{
    UINT8 len;
    UINT8 i;
    UINT8 token = USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP);

    if (UIF_TRANSFER) {
        if (token == UIS_TOKEN_SOF) {
            ++SystemMs;
        } else if (token == (UIS_TOKEN_OUT | 1)) {
            len = USB_RX_LEN;
            if (len >= SL25_REPORT_SIZE) {
                for (i = 0; i < SL25_REPORT_SIZE; ++i) Report[i] = Ep1Buffer[i];
                LastReportMs = SystemMs;
            }
            UEP1_CTRL = (UEP1_CTRL & ~MASK_UEP_R_RES) | UEP_R_RES_ACK;
        } else if (token == UIS_TOKEN_SETUP) {
            PUSB_SETUP_REQ setup = (PUSB_SETUP_REQ)Ep0Buffer;
            UINT8 reqType = setup->bRequestType;
            UINT8 lenOut = 0;
            SetupReq = setup->bRequest;
            SetupLen = setup->wLengthL;
            if (setup->wLengthH || SetupLen > 0x7F) SetupLen = 0x7F;
            if ((reqType & USB_REQ_TYP_MASK) == USB_REQ_TYP_STANDARD) {
                if (SetupReq == USB_GET_DESCRIPTOR) {
                    UINT8 total = descriptor_for_request(&pDescr);
                    if (total != 0xFF) {
                        if (SetupLen > total) SetupLen = total;
                        lenOut = SetupLen >= 8 ? 8 : SetupLen;
                        for (i = 0; i < lenOut; ++i) Ep0Buffer[i] = pDescr[i];
                        pDescr += lenOut; SetupLen -= lenOut;
                    } else SetupReq = 0xFF;
                } else if (SetupReq == USB_SET_ADDRESS) {
                    SetupLen = setup->wValueL;
                } else if (SetupReq == USB_GET_CONFIGURATION) {
                    Ep0Buffer[0] = UsbConfig; lenOut = 1;
                } else if (SetupReq == USB_SET_CONFIGURATION) {
                    UsbConfig = setup->wValueL;
                } else if (SetupReq == USB_GET_INTERFACE) {
                    Ep0Buffer[0] = 0; lenOut = 1;
                } else if (SetupReq == USB_GET_STATUS) {
                    Ep0Buffer[0] = 0; Ep0Buffer[1] = 0; lenOut = SetupLen >= 2 ? 2 : SetupLen;
                } else if (SetupReq != USB_SET_INTERFACE && SetupReq != USB_CLEAR_FEATURE && SetupReq != USB_SET_FEATURE) {
                    SetupReq = 0xFF;
                }
            } else if (reqType == (USB_REQ_TYP_IN | USB_REQ_TYP_CLASS | USB_REQ_RECIP_INTERF)) {
                if (SetupReq == HID_GET_REPORT || SetupReq == HID_GET_IDLE || SetupReq == HID_GET_PROTOCOL) {
                    Ep0Buffer[0] = 0; lenOut = 1;
                } else if (SetupReq != HID_SET_REPORT && SetupReq != HID_SET_IDLE && SetupReq != HID_SET_PROTOCOL) {
                    SetupReq = 0xFF;
                }
            } else if (reqType == (USB_REQ_TYP_OUT | USB_REQ_TYP_CLASS | USB_REQ_RECIP_INTERF)) {
                if (SetupReq != HID_SET_REPORT && SetupReq != HID_SET_IDLE && SetupReq != HID_SET_PROTOCOL) SetupReq = 0xFF;
            } else SetupReq = 0xFF;
            UEP0_T_LEN = (SetupReq == 0xFF) ? 0 : lenOut;
            UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
        } else if (token == UIS_TOKEN_IN) {
            if (SetupReq == USB_GET_DESCRIPTOR) {
                len = SetupLen >= 8 ? 8 : SetupLen;
                for (i = 0; i < len; ++i) Ep0Buffer[i] = pDescr[i];
                SetupLen -= len; pDescr += len; UEP0_T_LEN = len; UEP0_CTRL ^= bUEP_T_TOG;
            } else if (SetupReq == USB_SET_ADDRESS) {
                USB_DEV_AD = (USB_DEV_AD & bUDA_GP_BIT) | SetupLen;
                UEP0_T_LEN = 0; UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
            } else {
                UEP0_T_LEN = 0; UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
            }
        } else if (token == UIS_TOKEN_OUT) {
            UEP0_T_LEN = 0; UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_ACK;
        }
        USB_INT_FG = UIF_TRANSFER;
    }
    if (UIF_BUS_RST) {
        UsbConfig = 0; LastReportMs = SystemMs;
        UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        UEP1_CTRL = bUEP_AUTO_TOG | UEP_R_RES_ACK;
        USB_DEV_AD = 0;
        USB_INT_FG = UIF_BUS_RST;
    }
    if (UIF_SUSPEND) {
        USB_INT_FG = UIF_SUSPEND;
        if (USB_MIS_ST & bUMS_SUSPEND) {
            Report[0] = SL25_STATE_OFFLINE;
            UsbConfig = 0;
        }
    }
}

void main(void)
{
    P1_MOD_OC &= ~(1 << 5); /* push-pull */
    P1_DIR_PU |= (1 << 5);  /* output */
    LED_DATA = 0;
    Report[0] = SL25_STATE_OFFLINE;
    Report[1] = SL25_EFFECT_SOLID;
    Report[2] = 0;
    LastReportMs = 0;
    usb_init();
    EA = 1;
    while (1) {
        render_status();
    }
}
