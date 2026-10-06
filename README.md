# easyriscv (web)

[![Open the book](https://img.shields.io/badge/ebook-open-brightgreen)](https://h4bbit.github.io/easyriscv/)
[![Open the simulator](https://img.shields.io/badge/simulator-open-blue)](https://h4bbit.github.io/easyriscv/simulator.html)

Bare-metal RV32I ebook in the browser, inspired by [easy6502](https://github.com/skilldrick/easy6502) (CC BY 4.0).

Scope: RV32I base integer instruction set only — no M/A/F/D/C extensions,
no privileged ISA, no CSRs.

This branch (`gh-pages`) holds the web frontend. The terminal VM lives in
[`main`](https://github.com/H4bbit/easyriscv/tree/main): same lessons, same
memory map, C + ncurses instead of JavaScript + canvas.

## Use

Open `index.html` (or the GitHub Pages URL once deployed). No toolchain, no
server, no build step: each chapter's widget is an RV32I assembler and CPU
in `riscv.js`. Click **Assemble**, then **Run**.

For free play without the prose, open `simulator.html` (same widget with
a lab picker instead of fixed chapter sources).

## Layout

- `index.html` — the book: prose with one `.widget` per chapter, each
  prefilled with that lab's source (mirrors `main:labs/*/prog.s`)
- `simulator.html` — standalone playground: same widget with a lab picker
  (`labs.js`, generated from `main:labs/*/prog.s`), no prose
- `riscv.js` — assembler + simulator (JS port of `main:vm.c`):
  4KB flat mem, code at `0x600`, zero-page RAM `0x00`, stack top `0x1FC`,
  framebuffer `0x200` (32x32, 16 colors), MMIO `0xFE` random /
  `0xFF` last-key on load, `0x1000` print-char on store.
  One `RiscvWidget` per `.widget` node; debugger shows the full
  32-register file with change highlight
- `style.css` — widget layout (600px, responsive)

Source of truth for prose, labs, and solutions is the `main` branch;
this branch holds only the built site.

## Chapters

Intro, Numbers, First pixel, Registers, Branching, Memory, Stack, Jumping,
IO, ALU, Fib in RAM, Snake capstone — plus memory map and assembler notes.
No toolchain chapter on purpose: the web is zero-toolchain (the terminal
workflow is `main:book/02-toolchain.md`).

## Solutions

Exercises link to `main:solutions/` (per-lab answers, layout adapted from
[cpantel/Easy6502](https://github.com/cpantel/Easy6502)).
