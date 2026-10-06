# Big addresses (STUB — new chapter, to be written in migration Phase 2)

> `lui` EMBARGO until this chapter: no earlier chapter's prose, lab, or `disasm`
> quote may show or require `lui`/`auipc`/`la`/`call`/`tail`.
> This is where `li t0, 0x12345` → `lui`+`addi` finally appears, with the central table.

Planned (per Directive 1): `lui`, `li` unmasked (`1×addi` vs `lui`+`addi`),
`slli`/`srli`/`srai`, `la` (near `auipc`+`addi` only). Lab: `labs/10-big-addresses/prog.s`
(≤ 15 lines). Pseudo budget: `li` itself + central pseudo table HERE.
