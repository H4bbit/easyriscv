# Jumping

6502 has `JMP` (unconditional) and `JSR/RTS` (call/return via stack). RISC-V uses `jal`/`jalr`.

Run:

```bash
just debug 07-jumping
```

Source (`labs/07-jumping/prog.s`):

```asm
    jal ra, inc_one   // JSR
    j after           // JMP (jal x0)
after:
    jalr zero, 0(ra)  // RTS (ret pseudo)
```

* `jal rd, label` — `rd = pc+4; pc = label`. `jal ra, func` is `JSR`, `jal x0, label` is `JMP`, `jal ra, 0` is `j .` infinite loop halt.
* `jalr rd, offset(rs1)` — `rd = pc+4; pc = rs1+offset & ~1`. `jalr zero, 0(ra)` is `ret`.

For nested calls, save `ra` on stack (see `outer`):

```asm
outer:
    addi sp, sp, -4
    sw ra, 0(sp)
    jal ra, inner
    lw ra, 0(sp)
    addi sp, sp, 4
    ret
```

Without saving, `inner` would overwrite `ra` — same bug as 6502's `JSR` nesting without stack discipline.

## Try It

`07-jumping` goes `0→1→5→15` with nested `outer/inner`. Watch `ra` and `sp` in `w_regs` as you step into `jal`. `FB 0x200` shows `1 5 15`.

## Exercises

1. Remove the `sw ra`/`lw ra` in `outer`. What does `FB[2]` become?
2. Replace `j after` with `beq zero, zero, after` — does it still jump?
3. Write a leaf function that doesn't save `ra` and a non-leaf that must.

This completes the bare-metal arc. Next capstone will be Snake at `0x200` (like easy6502) using `0xFE/0xFF` input.
