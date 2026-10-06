# IO

> Renamed: old `10-io` → new `08-dice` (randomness-early; print/keys move to new `13-keys-print`).


So far every program was closed: same input, same output. Now let's meet
the VM's outside world — three memory-mapped ports. Everything is a load
or a store: no syscalls, bare metal like the framebuffer.

Run:

```bash
just debug 08-dice
```

Source (`labs/08-dice/prog.s`) — canonical loads and stores only:

```asm
    li t0, 0xFE
    lbu t1, 0(t0)      # random 0-255
    andi t1, t1, 0x0F
    sb t1, 0(t2)       # FB[0] = random color
    li t0, 0x1000
    sw t1, 0(t0)       # prints char to stdout
```

The mechanism is the same `lbu`/`sw` you met in [06-bytes-ram](06-bytes-ram.md) —
only the address has a side effect. `0xFE`/`0xFF` are intercepted on load,
`0x1000` on store; the instruction encoding never changes.

## The three doors

| Address | Name | Direction | Meaning |
|---------|------|-----------|---------|
| `0xFE` | random | read | new random byte on every instruction |
| `0xFF` | last key | read | ASCII of last key pressed (`0` if none) |
| `0x1000` | print-char | write | low byte of stored word goes to stdout |

`0xFE` is how games roll dice: mask it down (`andi t1, t1, 0x0F` for `0-15`,
`andi` + `addi` for a range like `2-5`). Headless runs print real randomness —
run `just run 08-dice` twice and `FB[0]` differs.

`0xFF` is how games read input: compare against `0x77` (`w`), `0x64` (`d`),
`0x73` (`s`), `0x61` (`a`). In `just debug`, pressing a key stores its ASCII
there; headless it stays `0`.

`0x1000` prints: a store to this address (outside the 4KB RAM) sends its low
byte to stdout instead of memory. The mechanism is the same as the
framebuffer — an address with a side effect — only the device differs
(pixels vs terminal output).

`just disasm 08-dice` is the proof — and the first real `lui`: `li t0, 0xFE`
at `0x600` fits 12 bits (one `addi`, `0fe00293`), but `li t0, 0x1000` at
`0x614` does not, so it becomes `lui t0, 0x1` (`000012b7`, upper 20 bits
`1`, lower 12 zero). The `andi` mask at `0x608` is one `andi` (`00f37313`);
`sw`/`sb` keep their funct3 (`0062a023` vs `00638023`). One spelling per
encoding, as always.

## Try It

Step through `08-dice` headless and watch stdout: it prints `OK`. In debug
mode, press keys, then step a `lbu t1, 0(t0)` with `t0=0xFF` and watch
`t1` take the key's ASCII in `w_regs` (`0xFF` is intercepted on load, not
RAM, so it never appears in the `w_mem` hexdump); the lab's exercise 3
turns a keypress into a pixel.

## Exercises

1. Print `"HI"` instead: change the two characters (`72='H'`, `73='I'`). ([solution](../solutions/08-dice/ex01-hi.s))
2. Print digit `'5'` (`53`) via a `0x1000` store. ([solution](../solutions/08-dice/ex02-digit.s))
3. Echo the last key: load `0xFF`, mask the low nibble, store to `FB[1]`. ([solution](../solutions/08-dice/ex03-echo-key.s))

## Translation exercise

Translate both ways and confirm `disasm` does not change: `li t0, 0xFE` ↔
`addi t0, zero, 0xFE` (12 bits, one instruction); `li t0, 0x1000` ↔
`lui t0, 0x1` (20 upper bits, lower 12 zero — the `addi` vanishes because
the low part is `0`). Then try `li t0, 0x1234`: 12 bits no longer hold it,
so the full `lui + addi` pair appears.

Next: [09-logic.md](09-logic.md)
