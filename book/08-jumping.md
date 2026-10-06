# Jumping

RISC-V has two jump forms: `jal` and `jalr`.

Run:

```bash
just debug 07-jumping
```

Source (`labs/07-jumping/prog.s`):

```asm
    jal ra, inc_one   # call
    j after           # unconditional jump (jal x0)
after:
    jalr zero, 0(ra)  # return
```

* `jal rd, label` — `rd = pc+4; pc = label`. `jal ra, func` calls a function, `jal x0, label` is an unconditional jump (`j`), `jal x0, 0` is an infinite loop halt.
* `jalr rd, offset(rs1)` — `rd = pc+4; pc = rs1+offset & ~1`. `jalr zero, 0(ra)` returns (`ret` pseudo).

For nested calls, save `ra` on the stack:

```asm
outer:
    addi sp, sp, -4
    sw ra, 0(sp)
    jal ra, inner
    lw ra, 0(sp)
    addi sp, sp, 4
    ret
```

Without saving, `inner` would overwrite `ra` and `outer` could never return.

## PC-relative: the address is the distance

Every jump so far encodes a *distance*, not a destination. `jal ra, inc_one`
at `0x600` reaching `0x624` stores offset `+0x24` in the instruction —
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

`07-jumping` goes `0→1→5→15` with nested `outer/inner`. Watch `ra` and `sp` in `w_regs` as you step into `jal`. The framebuffer at `0x200` shows `1 5 15`.

## Exercises

1. Remove the `sw ra`/`lw ra` in `outer`. What does the last framebuffer word become? ([solution](../solutions/08-jumping/ex01-no-save-ra.s))
2. Replace `j after` with `beq zero, zero, after` — does it still jump? ([solution](../solutions/08-jumping/ex02-beq-jump.s))
3. Write a leaf function that doesn't save `ra` and a non-leaf that must. ([solution](../solutions/08-jumping/ex03-leaf-nonleaf.s))
4. Load a label address with `la` and call through it with `call` — check `just disasm` to see the `auipc+addi`/`jal` expansion. ([solution](../solutions/08-jumping/ex04-pc-relative.s))

Next: [10-io.md](10-io.md)
