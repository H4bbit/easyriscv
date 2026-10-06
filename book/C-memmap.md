# Memory map (STUB — new appendix, to be written in migration Phase 2)

Planned: the canonical map — `0x00` zero-page RAM, `0x100`–`0x1FF` stack
(top `0x1FC`, grows down), `0x200`–`0x5FF` framebuffer 32x32 1 byte/pixel
(low nibble = color), `0x600` code (`PROG_BASE`), MMIO `0xFE` random /
`0xFF` last-key (loads), `0x1000` print-char (store).
