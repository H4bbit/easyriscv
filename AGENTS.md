# AGENTS.md — easyriscv (LOCAL ONLY — DO NOT COMMIT)

> **DO NOT COMMIT THIS FILE. DO NOT `git add` IT. DO NOT PUSH IT.**
> Local working directions for the agent. It stays untracked in the
> working tree on purpose. If `git status` shows it, that is expected —
> leave it uncommitted.

## Project identity

Bare-metal RV32I terminal playground inspired by
[skilldrick/easy6502](https://github.com/skilldrick/easy6502) (CC BY 4.0)
and `solutions/` layout adapted from
[cpantel/Easy6502](https://github.com/cpantel/Easy6502).
`vm.c` (RV32I CPU, 4KB flat mem, FB at `0x200`, MMIO `0xFE`/`0xFF`/`0x1000`),
`ui.c` (ncurses debugger), `labs/*/prog.s` (bare-metal at `0x600`),
`book/` (lessons), `solutions/` (exercises).

## Directive 1 — canonical first, pseudo right after, in every chapter

This project DIVERGES from easy6502 pedagogy on purpose.

- Every chapter teaches BOTH: first the canonical RV32I instruction
  (what the hardware executes), then immediately its assembler
  pseudo-instruction (what the programmer writes).
- There is NO isolated pseudo-instruction chapter.
- A passing comment (`// shorthand for ...`, `// see mv below`) is NOT
  enough. The chapter must make the student READ and WRITE both forms.
- Never teach only one version and let the student use only that one
  from then on (negative example: `04-registers.md` saying
  "from here on the book writes `mv`").
- Repeatable skeleton per lab + book chapter:
  1. Concept on hardware — canonical form only.
  2. Proof in disasm — `just disasm <lab>` shows only canonical encodings.
  3. Assembler shortcut — introduce the pseudo as alias + expansion.
  4. Lab uses both — first half of `prog.s` canonical, second half
     pseudo, same observable effect on FB/RAM.
  5. Translation exercise — canonical→pseudo AND pseudo→canonical,
     with `disasm` output as answer key.
- Every `solutions/` exercise has a double expected answer
  (both forms + identical `disasm`).

## Directive 2 — bare metal, MMIO-only, no syscalls

Deliberate exclusion, confirmed against the reference via `gh`:
`skilldrick/easy6502` (`index.markdown`, 810 lines;
`simulator/assembler.js`, 2670 lines) contains ZERO `syscall`,
`system call`, `interrupt`, `trap`, `kernel`, `Linux`.
Its "OS" is three mechanisms: `STA $0200` (pixel), key poll (`$FF`),
random (`$FE`). Halt is `BRK`.

Our equivalents:

| Mechanism | Address | Direction | Meaning |
|---|---|---|---|
| framebuffer 32x32, 1 byte/pixel | `0x200` | write | pixels, low nibble = color |
| random byte (new every step) | `0xFE` | read | dice via `lbu` + `andi` mask |
| last key ASCII (`0` if none) | `0xFF` | read | input via compare (`0x77=w` etc.) |
| print-char port | `0x1000` | write | low byte of stored word → stdout |
| halt | — | — | `jal x0, 0` (jump-to-self, `pc` stops) |

Rules:

- `ecall` / `ebreak` / `fence` / CSR instructions exist in RV32I but are
  OUT OF SCOPE. At most cite as "exists, not used here".
  (Precedent: commit `e531291` dropped `ecall` from `10-io`, MMIO-only.)
- `JAL`/`JALR` never touch the stack: `ra` is just a register, saving it
  with `sw ra, 0(sp)` is software convention. Contrast explicitly with
  6502 `JSR/RTS` (hardware push/pop of PC). This is the biggest inherited
  divergence and must be taught, not assumed.
- Known residues contradicting this rule (fix when editing is allowed):
  `vm.c` `OP_SYSTEM` handles `sret (0x10200073)` as NOP (privileged,
  no place here) and `ecall` with `a7==1` print-char (dead path since
  `10-io` went MMIO-only).

## Directive 3 — canonical ↔ pseudo mapping (normative table)

RV32I = 40 unique instructions, fixed 32-bit R/I/S/B/U/J formats,
`IALIGN=32` ([RV32I v2.1](https://docs.riscv.org/reference/isa/v20250508/unpriv/rv32.html)).
Canonical `NOP` and `MV` encodings exist so disassembly is readable.
`ADDI rd, rs1, 0` implements `MV`; `JR` = `JALR rd=x0`;
`RET` = `JALR rd=x0, rs1=x1, imm=0`.
Full alias list: [riscv-asm-manual](https://github.com/riscv-non-isa/riscv-asm-manual/blob/main/src/asm-manual.adoc).

Minimal table the project must cover (pseudo → canonical):

- `nop` → `addi x0, x0, 0`
- `li rd, imm` → myriad sequences: `-2048..2047` = 1× `addi rd, x0, imm`;
  larger = `lui + addi` (± `slli`/`ori`). NOTE: `li t1, 0x200` fits 12 bits,
  still 1× `addi` — a lab needs e.g. `li t0, 0x12345` to really show `lui`.
- `mv rd, rs` → `addi rd, rs, 0`
- `not rd, rs` → `xori rd, rs, -1`
- `neg rd, rs` → `sub rd, x0, rs`
- `seqz/snez/sltz/sgtz` → `sltiu rd,rs,1` / `sltu rd,x0,rs` /
  `slt rd,rs,x0` / `slt rd,x0,rs`
- `beqz/bnez` → `beq/bne rs, x0, off`
- `blez/bgez/bltz/bgtz` → `bge x0,rs` / `bge rs,x0` / `blt rs,x0` / `blt x0,rs`
- `bgt/ble/bgtu/bleu` → `blt/bge/bltu/bgeu` with swapped operands
- `j off` → `jal x0, off`
- `jal off` → `jal x1, off` (distinguish from `jal ra,`)
- `jr rs` / `jr off(rs)` → `jalr x0, rs, 0/off`
- `jalr rs` → `jalr x1, rs, 0`
- `ret` → `jalr x0, x1, 0`
- `call/tail` (`auipc + jalr`) and `la` (`auipc + addi`): IMPLEMENTED
  2026-10-05 in both assemblers (`main:asm.c`, `gh-pages:riscv.js`,
  clang-identical split) + lesson in `08-jumping` (`## PC-relative`)
  + `solutions/08-jumping/ex04-pc-relative.s`. In our fixed `0x600`-linked
  4KB model the near form always wins (`la`→`auipc+addi`, `call`→`jal`,
  `tail`→`j`); the far `auipc+jalr` form exists only for bigger models.
- `lh/lhu`: implemented in `vm.c`, never used in labs — pair with `lb/lbu`
  lesson or drop from decoder claims.

Distribution (no new chapter):

- `03-first-pixel`: `li`↔`addi/lui`, `j`↔`jal x0`, `nop`
- `04-registers`: `mv`↔`addi rd,rs,0`
- `05-branching`: `beqz/bnez`, then `blez/bgez/bltz/bgtz`, `bgt/ble`
- `08-jumping`: `jal`↔`jal x1`, `jr`, `jalr`, `ret`↔`jalr x0,x1,0`
- `11-alu`: `not`↔`xori -1`, `neg`↔`sub x0`, `seqz/snez/sltz/sgtz`↔`slt(i)(u)`
- `01-numbers` or `02-toolchain`: central reference table + real-`lui` case
- `09-snake` (heaviest `ret/j/li` user, 181 instr, complete since
  `fc7671a`, still no book chapter): gets chapter or glossary before
  capstone.

## Decoder / UI policy

`vm.c:decode_to_str` is half-pseudo today: folds `addi→li/mv`,
`beq/bne→beqz/bnez`, but NOT `jal x0→j`, `jal x1→jal`, `jalr x0→jr`,
`jalr x0,x1→ret`. `llvm-objdump` (what clang emitted) and `w_disasm`
(what we print) diverge exactly on the new lesson. Decision needed:
all-canonical decoder, or explicit bilingual mode (e.g. `mv (addi …)`)
— but consistent across `j/jr/jalr/ret/nop/beqz`.
Halt `pc == cur_pc` only catches `jal x0, 0`: keep, but teach it as
`jal x0, 0` semantics, not "VM magic".

## Numbering fragility (known, do not fix in this step)

`labs/` (`01-pixel, 02-fib, 03-fib-ram, 04-branching…`),
`book/` (`03-first-pixel, 04-registers, 05-branching…08-jumping, 10-io…`,
no `09`, no dedicated `02-fib`/`03-fib-ram`), `solutions/` named after
chapters not labs. Mapping table lives only in `book/00-intro.md`.
Example of drift: `solutions/04-registers/ex01-addi-move.s` is already
pure-canonical while its exercise text assumes the opposite.

## Web frontend plan (branch `gh-pages`, mirrors reference)

Like `skilldrick/easy6502` (site on `gh-pages`, Jekyll + `6502js` +
canvas widget), this project gets an analogous `gh-pages` branch holding
the browser ebook. `main` stays terminal-only (`vm` + ncurses).

CORRECTION 2026-10-05 (user, confirmed via `gh` against reference):
the current `index.html` (single playground widget + lab picker) is NOT
what is wanted. The reference site IS the book: `index.markdown`
(~810 lines, 8 sections: intro, first-program, registers/instructions,
branching, addressing, stack, jumping, snake-capstone) with prose +
embedded widgets per section via `_includes/start.html` / code /
`_includes/end.html` (one `.widget` div each: buttons, textarea,
canvas, debugger, monitor), plus standalone `simulator.markdown` and
`snake.markdown` pages. Our `index.html` must become the same thing:
the full ebook with one widget per chapter, not a lone playground.

- Page structure (mirrors reference): `index.html` = the book — intro
  through snake, each chapter = prose adapted from `book/*.md` +
  one `.widget` div with textarea prefilled with that chapter's
  `labs/*/prog.s` source (HTML-escaped, like `start.html`...`end.html`).
  Terminal commands (`just debug`/`just run`) become
  "click Assemble, then Run" in web prose. Optional standalone
  `simulator.html` (mirrors `simulator.markdown`: bare widget +
  lab picker for free play) and snake section live inside the book
  like the reference's `{% include snake.html %}`.
- `riscv.js` is already multi-widget ready (`querySelectorAll('.widget')`
  + one `RiscvWidget` per node, all queries scoped to `node`) — verified
  2026-10-05, no JS arch change needed. Consequence: DROP the lab picker
  from book widgets (a shared `labs.js` + duplicated `id="labSelect"`
  across widgets would break; each chapter textarea is prefilled
  directly). Keep `labs.js` only for the optional standalone
  `simulator.html` page, if built.
- Content source: `book/` chapters adapted to web (same Directive-1
  canonical-first skeleton per chapter), plus `solutions/` linked as
  "try it" exercises like the reference's `### Exercises ###`.
- Widget parity with the reference per widget: Assemble / Run / Reset /
  Hexdump / Disassemble / Notes buttons, code textarea, 32x32 canvas
  screen, Step debugger, memory monitor, `Notes` tab documenting `0xFE`
  random, `0xFF` last-key, `0x200-0x5FF` screen, 16-color palette.
- VM strategy DECIDED 2026-10-05: JS port of `vm.c` (like `6502js`,
  zero toolchain). WASM is explicitly OUT for now — if ever revisited,
  it goes in a separate branch, not `gh-pages`.
- Deploy target: GitHub Pages from the `gh-pages` branch (LIVE since
  2026-10-05 at https://h4bbit.github.io/easyriscv/ — verified HTTP 200;
  remote `origin git@github.com:H4bbit/easyriscv.git` exists, badges in
  both READMEs link to it).
- Memory map must match `main`: `PROG_BASE=0x600`, zero-page `0x00`,
  stack `0x1FC`, FB `0x200`, MMIO `0xFE`/`0xFF`/`0x1000`.
- Book source of truth stays in `main:book/`; the web branch adapts (not
  verbatim copies) into `index.html` prose.
- Worktree hygiene (fixed 2026-10-05): a `gh-pages` checkout holds ONLY
  the site files (`index.html`, `riscv.js`, `style.css`, ...). `main`
  files (`book/`, `labs/`, `solutions/`, `vm.c`, ...) must never sit as
  untracked files here — work on `main` in a separate worktree
  (`git worktree add ../easyriscv-main main`).
- TESTING PROTOCOL: agent cannot open a browser. When browser testing
  is needed, the agent itself opens the server in a tmux session/pane
  in the branch worktree — never as a blocking `bash` command, never
  asking the user to run it:
  `tmux new -d -s pi-http "bash -c 'python3 -m http.server 8000'"`
  (extra panes via `tmux split-window -t pi-http` if needed). Verify with
  `curl -s -o /dev/null -w "%{http_code}" localhost:8000` + logs via
  `tmux capture-pane -p -t pi-http`, then give the user the URL and PAUSE
  for their report. Keep the session alive while the user tests; close with
  `tmux kill-session -t pi-http` when done.

## Non-goals

- No `ecall`/`Linux`-style syscall lessons. Ever.
- No pseudo-only chapter. No canonical-only chapter that ignores the alias.
- No commit of this file.

## Decided 2026-10-05 — memory map fixed (was architecture mistake)

No documented reason existed for linking code at `0x0` inside the
framebuffer area. Fixed to mirror easy6502: code at `PROG_BASE=0x600`
(`riscv.ld` linker script: `.text` at `0x600`, `ASSERT(. <= 0x1000)`,
`objcopy --only-section=.text` — without the
flag the ELF headers leak into the flat binary), zero-page RAM `0x00`,
stack top `STACK_TOP=0x1FC` (grows down), FB `0x200-0x5FF` pixels-only.
`vm.h` documents the map; `cpu_load_bin`/`cpu_reset` use `PROG_BASE`;
`ui.c` disasm centers on `PROG_BASE`. All labs/books/solutions migrated:
RAM `0x300→0x00`, stack `0x900→0x1FC`, snake `STATE 0x600→0x00` with full
32x32 arena. `06-stack` FB[3] changed `0→12` (`0x1FC & 0xF`), expected.
