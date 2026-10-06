# Jumping

RISC-V has two jump forms: `jal` and `jalr`. Neither touches the stack: `jal rd, label` only writes the return address into `rd`, and a nested `jal` overwrites it. Saving `ra` with `sw ra, 0(sp)` before calling another function is software convention — your code pushes, your code pops. Forgetting the save in a nested call loses the return address (exercise 1 shows exactly that).

Run:

```bash
just debug 07-jumping
```

Source (`labs/07-jumping/prog.s`) — explicit `x0`/`x1` first, aliases after:

```asm
    jal x1, inc_one   # canonical call: x1 is ra
    jal x0, after     # canonical jump: x0 discards the address
after:
    jalr x0, 0(x1)    # canonical return through x1
```

* `jal rd, label` — `rd = pc+4; pc = label`. `jal x1, func` calls (`jal ra, func` is the same instruction spelled with the ABI name), `jal x0, label` is an unconditional jump (`j`), `jal x0, 0` is an infinite loop halt.
* `jalr rd, offset(rs1)` — `rd = pc+4; pc = rs1+offset & ~1`. `jalr x0, 0(x1)` returns; the shorthand is `ret` (exactly `jalr x0, x1, 0`), and `jr rs` is `jalr x0, rs, 0`.

For nested calls, save `ra` on the stack — because the inner `jal` overwrites it:

```asm
outer:
    addi sp, sp, -4
    sw ra, 0(sp)
    jal ra, inner
    lw ra, 0(sp)
    addi sp, sp, 4
    ret
```

A leaf function (calls nobody) needs no save: its `jalr` returns through the `ra` it received. A non-leaf (calls another) must save first. Without saving, `inner` would overwrite `ra` and `outer` could never return.

## PC-relative: the address is the distance

Every jump so far encodes a *distance*, not a destination. `jal ra, inc_one`
at `0x600` reaching `0x630` stores offset `+0x30` in the instruction —
move the whole program to `0x800` and it still lands right, because
`pc + offset` moves along. Same for branches (`bne`, ±4KB) and `jal`
(±1MB). Contrast with `li t1, 0x200`: the absolute address sits inside
the instruction and only works because the framebuffer never moves.

`auipc` brings PC-relative addressing to data: `auipc rd, hi` does
`rd = pc + (hi << 12)`. Two pseudo-ops are built on it:

* `la rd, label` — `auipc + addi`, loads the address of a label.
  (`la rd, number` is just `li`: no PC math needed for a constant.)
* `call label` — `jal` when near (everything fits in ±1MB here, so in
  practice always `jal`); `tail label` — `j` when near. The `auipc+jalr`
  long-call form exists for bigger models than our 4KB.

`just disasm` shows the expansion: `la`/`call` vanish, only `auipc`,
`addi`, `jal` remain — the hardware never sees the pseudo-op.

## Try It

`07-jumping` goes `0→1→5→15` with nested `outer/inner`. Watch `ra` and `sp` in `w_regs` as you step into `jal`. The framebuffer at `0x200` shows `1 5 15`. `disasm` is the answer key: canonical `jal x0`/`jal x1`/`jalr x0, 0(x1)` print as the aliases `j` (at `0x610`, `0x62c`), `jal` (at `0x604`, `0x624`) and `ret` (at `0x634`, `0x650`, `0x658`).

## Translation exercise

Translate both ways and confirm `disasm` does not change: `jal x0, after` ↔ `j after`, `jal x1, inc_one` ↔ `jal ra, inc_one` (and the bare `jal inc_one` form, which also means `x1`), `jalr x0, 0(x1)` ↔ `ret`, `jalr x0, 0(ra)` ↔ `jr ra`.

## Exercises

1. Remove the `sw ra`/`lw ra` in `outer`. What does the last framebuffer word become? ([solution](../solutions/08-jumping/ex01-no-save-ra.s))
2. Replace `jal x0, after` with `beq zero, zero, after` — does it still jump? ([solution](../solutions/08-jumping/ex02-beq-jump.s))
3. Write a leaf function that doesn't save `ra` and a non-leaf that must. ([solution](../solutions/08-jumping/ex03-leaf-nonleaf.s))
4. Load a label address with `la` and call through it with `call` — check `just disasm` to see the `auipc+addi`/`jal` expansion. ([solution](../solutions/08-jumping/ex04-pc-relative.s))

Next: [10-io.md](10-io.md)
