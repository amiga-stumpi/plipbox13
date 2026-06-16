# plipbox13

Standalone AmigaOS 1.3 SANA-II `plipbox.device` experiment for plipbox-05 firmware.

This repository is intentionally separated from TheWire13. The TCP/IP stack and bsdsocket.library must stay generic SANA-II clients; all plipbox-specific code lives here.

## Target

- AmigaOS/Kickstart 1.3
- 68000-compatible m68k
- bebbo amiga-gcc with `-mcrt=nix13`
- plipbox-05 firmware as the protocol reference

## Build

```sh
make clean && make
```

The device is written to:

```text
build/plipbox.device
```

## Current status

Stable snapshot: `plipbox13-stable-2026-06-16` (`db76f55`).

The device is a standalone Kickstart/AmigaOS 1.3 SANA-II `plipbox.device` for plipbox-05 firmware. It is intentionally not integrated into TheWire13; TheWire13 and bsdsocket.library stay generic SANA-II clients.

Current device version: `plipbox.device v1.0 by Marcel Jaehne (c)2026`.

Current default build:

- plipbox-05 vendor code is used as the protocol reference.
- The OS1.3-safe SANA-II device skeleton lives in `src/plipbox13`.
- The C SEND/RX protocol path is enabled by default (`PLIPBOX13_C_SEND=1`).
- The plipbox-05 online/magic frame is sent during `S2_CONFIGINTERFACE`.
- A dedicated `plipbox.rx` process handles RX wakeups and signals; default priority is 25.
- RX only starts a receive transaction when the firmware reports a pending packet.
- Debug and handshake console diagnostics are removed from the default build.
- The final `build/plipbox.device` binary is stripped.

Confirmed on real hardware with TheWire13 v1.5:

- `OpenDevice`, `S2_DEVICEQUERY`, `S2_GETSTATIONADDRESS`, and `S2_CONFIGINTERFACE` succeed.
- `S2_GETSTATIONADDRESS` returns `1a:11:a1:a0:47:11`.
- DHCP DISCOVER/OFFER/REQUEST/ACK completes after the Raspberry Pi gateway/dnsmasq configuration was corrected.
- `mini_nslookup` works repeatedly.
- `mini_wget` and MajaRadio downloads work in current tests.
- `mini_ntp` works, though retries may still be needed.
- Workbench responsiveness remained acceptable in tests with RX task priority 25.

Known limitations:

- This is still experimental and not yet a drop-in replacement for the original OS2+ plipbox.device.
- Longer transfer soak tests and upload throughput tests are still needed.
- Upload speed is not yet optimized.
- TX is still synchronous per SANA write; original-style queued TX/RX scheduling is a likely next optimization.
- plipbox-06 and newer firmware protocols are out of scope for this branch.

## Important rule

Do not use plipbox-0.6 or newer protocol code as a reference for this project unless explicitly requested. The attached hardware is running plipbox-05 firmware.
