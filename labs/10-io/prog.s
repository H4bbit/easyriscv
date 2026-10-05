// IO - the VM's outside world: MMIO ports and ecall
// 0xFE: random byte (new value every instruction)
// 0xFF: ASCII of last key pressed (0 headless, set by UI in debug)
// 0x1000: print-char port (store a byte -> stdout)
// ecall with a7=1: print char in a0 (like a bare-metal syscall)

.section .text
.globl _start
_start:
    // 1. random byte -> low nibble -> framebuffer
    li t0, 0xFE
    lbu t1, 0(t0)      // random 0-255
    andi t1, t1, 0x0F  // 0-15
    li t2, 0x200
    sb t1, 0(t2)       // FB[0] = random color

    // 2. print 'O' 'K' via stores to 0x1000
    li t0, 0x1000
    li t1, 79          // 'O'
    sw t1, 0(t0)
    li t1, 75          // 'K'
    sw t1, 0(t0)

    // 3. print '!' via ecall (a7=1, a0=char)
    li a0, 33          // '!'
    li a7, 1
    ecall

    j .

// Exercises:
// 1. Print "HI" instead: change the two characters (72='H', 73='I')
// 2. Print a digit via ecall: set a0 to '5' (53) and keep a7=1
// 3. Echo the last key: load 0xFF, mask the low nibble, store to FB[1]
//    (press a key in 'just debug 10-io' and watch the pixel change)
