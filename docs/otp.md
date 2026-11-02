# OTP programming

How to make the zero position and the settings permanent: what the chip requires, what `ProgramOTP()` does, and
what you still check yourself.

The settings registers (DISABLE, ZPOSM, ZPOSL, SETTINGS1 to 3, ECC at 0x0015 to 0x001B) are volatile copies of the
one-time-programmable memory. Every setter in the driver writes the volatile copy: it works at once and is lost
at the next power cycle. Burning the OTP keeps it forever, and it can be done **once per chip**.

!!! danger "One attempt per chip"
    A burn cannot be repeated or undone, and the datasheet requires a part that fails verification to be
    scrapped. Program on the bench first and verify every setting in volatile form before you burn.

## Conditions

| Parameter | Value |
| --- | --- |
| Supply during the burn, 5 V mode | 4.5 V to 5.5 V on VDD, 1 µF on VDD3V3 |
| Supply during the burn, 3.3 V mode | 3.3 V to 3.5 V (narrower than the 3.0 V to 3.6 V operating range) |
| Current | Up to 100 mA during the burn: the supply must deliver it |
| Temperature | 5 °C to 45 °C |
| Frames | 24- or 32-bit, with CRC |

## The datasheet sequence and the driver

The datasheet's step-by-step procedure (page 41-42, flowchart Figure 60) and what `ProgramOTP()` does at each
step. The driver checks the result of every step and stops at the first that fails; `OtpResult::failed_step`
names it.

| Step | Datasheet | `ProgramOTP()` |
| --- | --- | --- |
| 1 | Power-on cycle | Your job, before the call |
| 2 | Write the settings registers | Your job: call the setters before `ProgramOTP()` |
| 3 | Place the magnet at the zero position | Your job |
| 4, 5 | Read ANGLECOM, write it into ZPOSL[5:0] and ZPOSM[7:0] | Done, each write verified |
| 6 | Read the settings | Done, 0x0015..0x001B |
| 7 to 9 | ECC = `0x80` (`ECC_en`), read `ECC_s` from 0x3FD0, ECC = `0x80` \| `ECC_s` | Done, each write verified |
| 10, 11 | Read the settings back and compare with what was written | Done, 0x0015..0x001B |
| 12 | PROG = `0x01` (`PROGEN`) | Done, verified |
| 13 | PROG = `0x08` (`PROGOTP`): the burn starts | Sent once, never retried; `burn_started` is set |
| 14 | Poll PROG until it reads `0x0001` | Done; gives up after 15000 polls |
| 15 | Clear the volatile registers: `0x0000` into 0x0015..0x001B | Done, each write verified |
| 16, 17 | PROG = `0x40` (`PROGVER`, guard band), PROG = `0x04` (`OTPREF`, reload from OTP) | Done; the chip must accept each frame (no CRC, framing or command error) |
| 18, 19 | Read 0x0015..0x001B, compare with step 10 (guard-band test) | Done; returns `false` on a difference |
| 20 to 22 | Power-on cycle, read back, compare | Your job, after the call: `VerifyOTP(result.written)` |

The step text reads 0x0016..0x001A or 0x001B; Figure 60 reads and clears 0x0015 (DISABLE) too, which is also a
non-volatile register, so the driver reads and compares all seven, 0x0015..0x001B. The PROG values are those of
Figure 60. Steps 12 to 17 always use frames with CRC: `ProgramOTP()` switches to 24-bit frames for the duration
when the driver is in `SPI_16` mode and restores the format afterwards.

Step 4 reads ANGLECOM, which already includes the zero position and the direction: start from a power-on cycle
(ZPOSM = ZPOSL = 0) as step 1 asks, or the new zero is offset by the old one.

!!! warning "A failure after the burn started"
    `failed_step` 4 to 12 means nothing was burned (`burn_started` is false): fix the cause and try again. From step
    13 on the fuses may be burned. A part that fails step 19, or the power-on check in steps 20 to 22, must not be
    used: the datasheet does not allow reprogramming.

## Use it

```cpp
--8<-- "tests/docs/api_tasks.cpp:otp"
```

`VerifyOTP()` is the power-on check of steps 21 and 22: after cycling the supply it reads 0x0015..0x001B and compares
them with `OtpResult::written`, the image that was burned.

Next: [Configuration](configuration.md) lists every setting the burn makes permanent.
