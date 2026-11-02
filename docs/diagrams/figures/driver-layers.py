"""The three layers of the driver: hardware-agnostic core, SPI interface, platform port.

Reads: From top to bottom. Your application talks to one AS5047U<SpiType> object. Its methods read and write the
chip's typed registers (listed from inc/as5047u_registers.hpp), build 16-, 24- or 32-bit frames with CRC, check
writes by reading them back and collect the ERRFL error flags. The SPI interface (SpiInterface in
inc/as5047u_spi_interface.hpp) is a CRTP base with one method. The platform port is the only code you write: a class
whose transfer() moves one frame over SPI with chip select held low. Below it is the hardware.
"""

import re
from pathlib import Path

from hardfoc_mkdocs_theme.figkit import Fig

ROOT = Path(__file__).resolve().parents[3]
REGS = "inc/as5047u_registers.hpp"
SPI = "inc/as5047u_spi_interface.hpp"


def data():
    regs = re.findall(r"^struct (\w+) \{\n\s+static constexpr uint16_t ADDRESS", (ROOT / REGS).read_text(), re.M)
    spi = (ROOT / SPI).read_text()
    hooks = re.findall(r"^  void (\w+)\(const uint8_t", spi, re.M)
    return {
        "registers": regs,
        "port_hooks": hooks,
        "sources": [f"{REGS}:register structs", f"{SPI}:SpiInterface"],
    }


def build(d):
    f = Fig(
        "driver-layers",
        "Three layers, one of them yours",
        alt="Layer diagram: the application calls the hardware-agnostic AS5047U driver, which reads and writes the "
        "chip's typed registers in CRC-protected SPI frames through the SpiInterface base class, which calls the "
        "transfer() method of the platform port you write, which drives the SPI bus of the AS5047U.",
        width=1200,
        kicker="Architecture",
        subtitle="The driver and the interface are the same on every MCU. The port is a class with one method.",
        profile="overview",
        legend=[("brand", "this library"), ("teal", "you write"), ("slate", "hardware")],
    )
    W = f.body_w
    app = f.card(0, 0, W, "Your application", ["motor control loop, position servo, CLI ..."], tone="slate")
    top = app.b + 40
    chips_h = f.chips(16, f.region_inner_top(top), W - 32, d["registers"], tone="brand")
    reg = f.region(0, top, W, f.region_inner_top(top) - top + chips_h + 18, "Hardware-agnostic core", tone="brand",
                   sub="AS5047U<SpiType>, inc/as5047u.hpp: angle, velocity, diagnostics, outputs, OTP")
    comm = f.card(0, reg.b + 40, W, "SPI interface",
                  ["SpiInterface<Derived>: CRTP base, no virtual calls. The driver builds 16-, 24- or 32-bit frames,",
                   "the CRC-8, the one-frame-late reads and the read-back check of every write"],
                  tone="brand", accent=True)
    port = f.card(0, comm.b + 40, W, "Platform port (your class)",
                  ["  ".join(h + "(tx, rx, len)" for h in d["port_hooks"])],
                  tone="teal", accent=True, mono=True)
    hw = f.card(0, port.b + 40, W, "AS5047U", ["SPI mode 1 up to 10 MHz  /  CSn, CLK, MOSI, MISO"], tone="slate")
    for a, b in ((app, reg), (reg, comm), (comm, port), (port, hw)):
        f.connect(a, b, "bottom", "top", tone="rail")
    return f
