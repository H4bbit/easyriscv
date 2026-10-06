/* riscv.ld — easyriscv linker script: flat 4KB model, code at 0x600.
 *
 * Replaces -Ttext=0x600,--image-base=0x600 (which warns: address isn't
 * multiple of page size). -T is common ld syntax (ld.lld and ld.bfd),
 * so the future riscv64-unknown-elf-gcc path accepts it unchanged.
 *
 * Layout mirrors vm.h: .text at PROG_BASE=0x600, .rodata/.data/.bss
 * adjacent (not page-scattered), ASSERT bars >4KB programs at link.
 * Only .text reaches the VM (objcopy --only-section=.text); the ASSERT
 * is the link-time equivalent of vm.c's MEM_SIZE - PROG_BASE clamp.
 */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
  .text 0x600 : { *(.text*) }
  .rodata : { *(.rodata*) }
  .data : { *(.data*) }
  .bss : { *(.bss*) *(COMMON) }
  /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.note*) *(.eh_frame*) }
}
ASSERT(. <= 0x1000, "program exceeds 4KB flat memory");
