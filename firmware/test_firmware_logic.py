"""Run the actual USB request/state C logic with emulated SFR variables.

Requires a host gcc. USB hardware, W1C register behavior and timing are not
simulated; WS2812 linked instruction timing is checked by verify_ws2812.py.
"""
import re
import shutil
import subprocess
import tempfile
from pathlib import Path

root = Path(__file__).resolve().parent
source = (root / "ch552_codex_status/main.c").read_text(encoding="utf-8")
source = re.sub(r"__(?:sfr|sbit)\s+__at\s*\([^)]*\)\s+(\w+);",
                r"static volatile unsigned char \1;", source)
source = re.sub(r"__at\s*\([^)]*\)", "", source)
source = re.sub(r"__(?:interrupt|using)\([^)]*\)", "", source)
source = re.sub(r"\b__(?:xdata|code)\b", "", source)
source = re.sub(r"__asm\b.*?__endasm;", "", source, flags=re.S)
source = source.replace('#include "../common/ws2812_ch552.h"', """
static UINT8 observed_rgb[3];
static void ws_color(UINT8 r, UINT8 g, UINT8 b) {
    observed_rgb[0] = r; observed_rgb[1] = g; observed_rgb[2] = b;
}
""")
source = source.replace("void main(void)", "void firmware_main(void)")
harness = r"""
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void setup_report(unsigned char type, unsigned char id,
                         unsigned char interface, unsigned char length) {
    const unsigned char setup[] = {0x21, 9, id, type, interface, 0, length, 0};
    memcpy(Ep0Buffer, setup, 8);
    USB_INT_FG = UIF_TRANSFER;
    USB_INT_ST = UIS_TOKEN_SETUP;
    usb_interrupt();
}
static void out(unsigned char endpoint, unsigned char length,
                const unsigned char *payload) {
    memcpy(endpoint == 0 ? Ep0Buffer : Ep2Buffer, payload, 8);
    USB_RX_LEN = length;
    USB_INT_ST = UIS_TOKEN_OUT | endpoint;
    USB_INT_FG = UIF_TRANSFER;
    usb_interrupt();
}
int main(void) {
    const unsigned char payload[] = {1, 0, 96, 0, 180, 0, 42, 0};
    const unsigned char next[] = {4, 0, 64, 220, 0, 0, 43, 0};
    assert(sizeof(CfgDesc) == CfgDesc[2]);
    assert(sizeof(HidReportDesc) == CfgDesc[25]);
    assert(sizeof(ProdDesc) == ProdDesc[0]);
    SystemMs = 1234;
    setup_report(2, 0, 0, 8);
    assert(SetReportPending && (UEP0_CTRL & 15) == 0);
    out(0, 8, payload);
    assert(!SetReportPending && memcmp((const void *)Report, payload, 8) == 0);
    assert(LastReportMs == 1234 && UEP0_T_LEN == 0);
    assert(UEP0_CTRL == bUEP_T_TOG);  /* DATA1 status IN */
    USB_INT_FG = UIF_TRANSFER;
    USB_INT_ST = UIS_TOKEN_IN;
    usb_interrupt();
    assert((UEP0_CTRL & MASK_UEP_T_RES) == UEP_T_RES_NAK);

    setup_report(3, 0, 0, 8); /* Feature report is not declared */
    assert(!SetReportPending && (UEP0_CTRL & 15) == 15);
    setup_report(2, 1, 0, 8); /* report ID */
    assert(!SetReportPending && (UEP0_CTRL & 15) == 15);
    setup_report(2, 0, 1, 8); /* interface */
    assert(!SetReportPending && (UEP0_CTRL & 15) == 15);
    setup_report(2, 0, 0, 7); /* length */
    assert(!SetReportPending && (UEP0_CTRL & 15) == 15);
    setup_report(2, 0, 0, 8);
    out(0, 7, next);
    assert((UEP0_CTRL & 15) == 15);
    assert(memcmp((const void *)Report, payload, 8) == 0);

    setup_report(2, 0, 0, 8);
    memset(Ep0Buffer, 0, 8);
    Ep0Buffer[0] = 0x80; Ep0Buffer[1] = USB_GET_CONFIGURATION; Ep0Buffer[6] = 1;
    USB_INT_FG = UIF_TRANSFER; USB_INT_ST = UIS_TOKEN_SETUP;
    usb_interrupt();
    assert(!SetReportPending); /* a new SETUP aborts the preceding transfer */
    out(0, 8, next);
    assert(memcmp((const void *)Report, payload, 8) == 0);

    SystemMs = 1500;
    out(2, 8, next); /* existing interrupt OUT path */
    assert(memcmp((const void *)Report, next, 8) == 0 && LastReportMs == 1500);
    render_status();
    assert(observed_rgb[0] > 0 && observed_rgb[1] == 0 && observed_rgb[2] == 0);
    SystemMs = 4501;
    render_status();
    assert(observed_rgb[0] == 0 && observed_rgb[1] == 0 && observed_rgb[2] == 0);

    setup_report(2, 0, 0, 8);
    USB_INT_FG = UIF_BUS_RST;
    usb_interrupt();
    assert(!SetReportPending && Report[0] == SL25_STATE_OFFLINE);
    puts("PASS: control OUT, interrupt OUT, status stage, invalid/aborted reports, timeout, bus reset, descriptors");
    return 0;
}
"""
gcc = shutil.which("gcc")
if not gcc:
    raise SystemExit("gcc not found on PATH")
with tempfile.TemporaryDirectory(prefix="sl25-firmware-test-") as directory:
    path = Path(directory)
    c_file, executable = path / "test.c", path / "test.exe"
    c_file.write_text(source + harness, encoding="utf-8")
    subprocess.run([gcc, "-std=c99", "-O2", str(c_file), "-o", str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
