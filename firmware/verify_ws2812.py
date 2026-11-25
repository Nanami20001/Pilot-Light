"""Check the linked CH552 sender, not SDCC's classic-8051 cycle annotations.

Usage: python firmware/verify_ws2812.py path/to/program.hex [...]
Each HEX must have its SDCC .rst listing beside it.
Instruction cycles: https://www.wch.cn/uploads/file/20220728/1658941041195430.pdf
This checks a digital timing model; it does not replace a scope measurement.
"""
import json
import re
import sys
from pathlib import Path


def read_hex(path):
    memory = {}
    base = 0
    eof = False
    for line in path.read_text(encoding="ascii").splitlines():
        record = bytes.fromhex(line.removeprefix(":"))
        assert sum(record) & 255 == 0, "HEX checksum mismatch"
        count, hi, lo, kind = record[:4]
        assert len(record) == count + 5
        address = base + (hi << 8) + lo
        if kind == 0:
            for offset, byte in enumerate(record[4:-1]):
                assert address + offset not in memory, "overlapping HEX records"
                memory[address + offset] = byte
        elif kind == 4:
            base = int.from_bytes(record[4:-1], "big") << 16
        elif kind == 1:
            eof = True
        else:
            raise AssertionError(f"Unsupported HEX record {kind}")
    assert eof and memory
    assert max(memory) < 0x3800, "HEX reaches reserved ISP space"
    return memory


def symbols(path):
    return {
        name: int(address, 16)
        for address, name in re.findall(
            r"^\s*([0-9A-F]{6})\s+\d+\s+(_\w+):\s*$",
            path.read_text(encoding="utf-8"), re.M
        )
    }


def trace(memory, start, data=None):
    """Interpret the small branchless sender/latch instruction subset."""
    ram = [0] * 256
    if data:
        address, values = data
        ram[address:address + len(values)] = values
    registers = [0] * 8
    accumulator = carry = level = cycles = 0
    pc = start
    edges = []
    for _ in range(30000):
        op = memory[pc]
        if op == 0x22:  # RET: end of the measured routine
            return edges, cycles, level
        size = cost = 1
        new_level = None
        if 0x78 <= op <= 0x7F:  # MOV Rn,#imm
            registers[op - 0x78] = memory[pc + 1]
            size = cost = 2
        elif op == 0xE6:  # MOV A,@R0
            accumulator = ram[registers[0]]
        elif op == 0x33:  # RLC A
            accumulator, carry = ((accumulator << 1) | carry) & 255, accumulator >> 7
        elif op in (0xD2, 0xC2, 0x92):  # SETB / CLR / MOV bit,C
            assert memory[pc + 1] == 0x95, "unexpected output pin"
            size = cost = 2
            new_level = 1 if op == 0xD2 else 0 if op == 0xC2 else carry
        elif op == 0x08:  # INC R0
            registers[0] = (registers[0] + 1) & 255
        elif 0xD8 <= op <= 0xDF:  # DJNZ Rn,rel
            index = op - 0xD8
            registers[index] = (registers[index] - 1) & 255
            displacement = memory[pc + 1]
            if displacement >= 128:
                displacement -= 256
            target = pc + 2 + displacement
            if registers[index]:
                cycles += 4 + (target & 1)
                pc = target
                continue
            size = cost = 2
        elif op != 0x00:  # NOP
            raise AssertionError(f"Unhandled opcode {op:02x} at {pc:04x}")
        cycles += cost
        if new_level is not None and new_level != level:
            level = new_level
            edges.append((cycles, level))
        pc += size
    raise AssertionError("routine did not return")


def verify(path):
    memory = read_hex(path)
    labels = symbols(path.with_suffix(".rst"))
    assert labels["_ws_symbols"] + 24 <= 128
    cases = [(v, v ^ 0xAA, v ^ 0x55) for v in range(256)]
    cases += [(0, 0, 0), (255, 255, 255), (32, 0, 0), (0, 32, 0), (0, 0, 32)]
    highs = {0: set(), 1: set()}
    lows = {0: set(), 1: set()}
    for grb in cases:
        bits = [(byte >> bit) & 1 for byte in grb for bit in range(7, -1, -1)]
        edges, _, level = trace(memory, labels["_ws_send_symbols"],
                                (labels["_ws_symbols"], [bit << 7 for bit in bits]))
        assert len(edges) == 48 and level == 0, "expected 24 pulses ending low"
        for index, bit in enumerate(bits):
            rise, fall = edges[index * 2:index * 2 + 2]
            assert rise[1] == 1 and fall[1] == 0
            high = fall[0] - rise[0]
            highs[bit].add(high)
            # Check the full specified oscillator range (23.64..24.36 MHz).
            minimum, maximum = (220, 380) if bit == 0 else (580, 1000)
            assert minimum <= high * 1000 / 24.36
            assert high * 1000 / 23.64 <= maximum
            if index != 23:
                low = edges[index * 2 + 2][0] - fall[0]
                lows[bit].add(low)
                assert 580 <= low * 1000 / 24.36
                assert low * 1000 / 23.64 <= 1000
    _, latch_cycles, _ = trace(memory, labels["_ws_reset_delay"])
    assert latch_cycles / 24.36 > 280
    return {
        "file": str(path), "frames_checked": len(cases),
        "high_ns_at_24MHz": {str(k): [round(v * 1000 / 24, 2) for v in sorted(s)] for k, s in highs.items()},
        "low_ns_at_24MHz": {str(k): [round(v * 1000 / 24, 2) for v in sorted(s)] for k, s in lows.items()},
        "latch_us_min_at_24_36MHz": round(latch_cycles / 24.36, 2),
        "flash_bytes": len(memory), "highest_address": hex(max(memory)),
        "result": "PASS (instruction model, hardware waveform not measured)"
    }


if __name__ == "__main__":
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    print(json.dumps([verify(Path(arg)) for arg in sys.argv[1:]], indent=2))
