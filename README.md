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

The device currently uses a minimal OS1.3 SANA-II skeleton plus plipbox-05 hardware protocol code. The default build uses bounded C SEND/RX servicing (`PLIPBOX13_C_SEND=1`) to avoid hangs in the original timer/assembly send path during bring-up.

Confirmed on real hardware with TheWire13 v1.5:

- SANA-II OpenDevice succeeds
- S2_DEVICEQUERY succeeds
- S2_GETSTATIONADDRESS returns `1a:11:a1:a0:47:11`
- S2_CONFIGINTERFACE succeeds
- DHCP DISCOVER/OFFER/REQUEST/ACK completes
- Lease observed: `192.168.7.11`, gateway/DNS `192.168.7.2`

Next validation targets are ARP, DNS, UDP, TCP, and sustained transfers.

## Important rule

Do not use plipbox-0.6 or newer protocol code as a reference for this project unless explicitly requested. The attached hardware is running plipbox-05 firmware.
