# Jumping

RISC-V has two jump forms: `jal` and `jalr`.

Run:

```bash
just debug 07-jumping
```

Source (`labs/07-jumping/prog.s`):

```asm
    jal ra, inc_one   // call
    j after           // unconditional jump (jal x0)
after:
    jalr zero, 0(ra)  // return
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

## Try It

`07-jumping` goes `0→1→5→15` with nested `outer/inner`. Watch `ra` and `sp` in `w_regs` as you step into `jal`. The framebuffer at `0x200` shows `1 5 15`.

## Exercises

1. Remove the `sw ra`/`lw ra` in `outer`. What does the last framebuffer word become? ([solution](../solutions/08-jumping/ex01-no-save-ra.s))
2. Replace `j after` with `beq zero, zero, after` — does it still jump? ([solution](../solutions/08-jumping/ex02-beq-jump.s))
3. Write a leaf function that doesn't save `ra` and a non-leaf that must. ([solution](../solutions/08-jumping/ex03-leaf-nonleaf.s))

This completes the bare-metal arc. The next capstone will be a Snake game at `0x200` using the framebuffer.
