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

The device currently uses a minimal OS1.3 SANA-II skeleton plus plipbox-05 hardware protocol code. The default build uses a bounded C SEND path (`PLIPBOX13_C_SEND=1`) to avoid hangs in the original timer/assembly send path during bring-up.

## Important rule

Do not use plipbox-0.6 or newer protocol code as a reference for this project unless explicitly requested. The attached hardware is running plipbox-05 firmware.
