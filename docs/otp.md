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

The datasheet's step-by-step procedure (Figure 60) and what `ProgramOTP()` does at each step:

| Step | Datasheet | `ProgramOTP()` |
| --- | --- | --- |
| 1 | Power-on cycle | Your job, before the call |
| 2 | Write the settings registers | Your job: call the setters before `ProgramOTP()` |
| 3 to 5 | Place the magnet at zero, read ANGLECOM, write it into ZPOSM / ZPOSL | Done: `SetZeroPosition(GetAngle())` |
| 6 to 9 | Read 0x0016..0x001A, set `ECC_en`, read `ECC_s` from 0x3FD0, write it into ECC | Done |
| 10, 11 | Read the settings back and compare | Done for 0x0016..0x001A; returns `false` on a difference |
| 12 to 14 | `PROGEN` = 1, `PROGOTP` = 1, poll PROG until it reads `0x0001` | Done; gives up after 15000 polls and returns `false` |
| 15 | Clear the volatile registers (write 0) | Not done |
| 16 to 19 | `PROGVER` = 1 (guard band), `OTPREF` = 1 (reload from OTP), compare | Done, compares 0x0016..0x001A |
| 20 to 22 | Power-on cycle, read back, compare | Your job, after the call |

`ProgramOTP()` switches to 24-bit frames for the duration when the driver is in `SPI_16` mode and restores the
format afterwards.

!!! warning "Verify after a power cycle"
    Because step 15 is skipped, the guard-band comparison inside `ProgramOTP()` can pass on values that were still
    in the volatile registers. The power-on check in steps 20 to 22 is the one that proves the burn: cycle the
    supply, read DISABLE, ZPOSM, ZPOSL, SETTINGS1 to 3 and ECC, and compare them with what you wrote.

## Use it

```cpp
--8<-- "tests/docs/api_tasks.cpp:otp"
```

Next: [Configuration](configuration.md) lists every setting the burn makes permanent.
