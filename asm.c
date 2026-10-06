// asm.c - easyriscv assembler: prog.s -> flat .bin (loaded at 0x600).
//
// C port of the gh-pages riscv.js Assembler (two passes, RV32I subset +
// pseudo-ops). Byte-identical to clang output for all labs and solutions;
// clang stays as independent ground truth (just disasm/elf/hex,
// just check-asm).
//
// Usage: asm labs/01-pixel/prog.s /tmp/01-pixel.bin
//
// Supported, mirroring riscv.js exactly:
//   canonical RV32I: lui, jal/jalr (all forms), beq/bne/blt/bge/bltu/bgeu,
//     lb/lh/lw/lbu/lhu, sb/sh/sw, OP-IMM (addi/slli/slti/sltiu/xori/srli/
//     srai/ori/andi), OP (add/sub/sll/slt/sltu/xor/srl/sra/or/and)
//   pseudo-ops: li (addi / lui / lui+addi, clang-identical split),
//     mv, nop, not, neg, seqz/snez/sltz/sgtz,
//     beqz/bnez/blez/bgez/bltz/bgtz, bgt/ble/bgtu/bleu,
//     j, jal (1-arg), jr, jalr (1-arg), ret,
//     la (auipc+addi for labels, li for numbers — clang-identical),
//     call (jal near / j far, clang-identical), tail (j, clang-identical)
//   directives: .section/.globl/.text/.option (ignored), .equ/.define,
//     .byte/.word/.space.

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

constexpr uint32_t PROG_BASE = 0x600;
constexpr uint32_t MEM_MAX = 0x1000;
constexpr int MAX_LINES = 4096;
constexpr int MAX_LINE = 1024;
constexpr int MAX_SYMS = 1024;

typedef struct {
    char name[128];
    int32_t val;
} Sym;

static Sym equ_syms[MAX_SYMS];
static int nequ = 0;
static Sym labels[MAX_SYMS];
static int nlabels = 0;

static char lines[MAX_LINES][MAX_LINE];
static int nlines = 0;

static uint8_t out[MEM_MAX];
static uint32_t out_len = 0;

[[noreturn]] static void fail(int lineno, const char *fmt, ...) {
    va_list ap;
    fprintf(stderr, "asm:%d: error: ", lineno + 1);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(1);
}

static bool is_name_start(char c) {
    return isalpha((unsigned char)c) || c == '_' || c == '.';
}

static bool is_name_char(char c) {
    return isalnum((unsigned char)c) || c == '_' || c == '.';
}

// In-place trim; returns pointer to first non-space char.
static char *trim(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1])) s[--n] = '\0';
    return s;
}

// Strip //, # and ; comments (riscv.js parity). # is the GNU/clang-common
// comment char; // stays accepted for old sources.
static void sanitize(char *line) {
    for (char *p = line; *p; p++) {
        if ((p[0] == '/' && p[1] == '/') || p[0] == '#' || p[0] == ';') {
            *p = '\0';
            break;
        }
    }
}

static const struct {
    const char *name;
    int id;
} REG_ABI[] = {
    {"zero", 0}, {"ra", 1}, {"sp", 2}, {"gp", 3}, {"tp", 4},
    {"t0", 5}, {"t1", 6}, {"t2", 7}, {"s0", 8}, {"fp", 8},
    {"s1", 9}, {"a0", 10}, {"a1", 11}, {"a2", 12}, {"a3", 13},
    {"a4", 14}, {"a5", 15}, {"a6", 16}, {"a7", 17}, {"s2", 18},
    {"s3", 19}, {"s4", 20}, {"s5", 21}, {"s6", 22}, {"s7", 23},
    {"s8", 24}, {"s9", 25}, {"s10", 26}, {"s11", 27}, {"t3", 28},
    {"t4", 29}, {"t5", 30}, {"t6", 31},
};

static int reg_id(const char *s) {
    if (s[0] == 'x') {
        char *end = nullptr;
        long v = strtol(s + 1, &end, 10);
        if (end != s + 1 && *end == '\0' && v >= 0 && v <= 31) return (int)v;
        return -1;
    }
    for (size_t i = 0; i < sizeof(REG_ABI) / sizeof(REG_ABI[0]); i++)
        if (strcmp(s, REG_ABI[i].name) == 0) return REG_ABI[i].id;
    return -1;
}

// Strict integer: -? (decimal | 0x hex | $hex | %bin).
static bool parse_int_c(const char *s, int32_t *val) {
    bool neg = false;
    if (*s == '-') {
        neg = true;
        s++;
    }
    int base = 10;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        base = 16;
        s += 2;
    } else if (*s == '$') {
        base = 16;
        s++;
    } else if (*s == '%') {
        base = 2;
        s++;
    }
    if (*s == '\0') return false;
    uint32_t v = 0;
    for (; *s; s++) {
        int d;
        if (*s >= '0' && *s <= '9') d = *s - '0';
        else if (*s >= 'a' && *s <= 'f') d = *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'F') d = *s - 'A' + 10;
        else return false;
        if (d >= base) return false;
        v = v * (uint32_t)base + (uint32_t)d;
    }
    *val = neg ? -(int32_t)v : (int32_t)v;
    return true;
}

static bool equ_find(const char *name, int32_t *val) {
    for (int i = 0; i < nequ; i++)
        if (strcmp(equ_syms[i].name, name) == 0) {
            *val = equ_syms[i].val;
            return true;
        }
    return false;
}

static bool label_find(const char *name, int32_t *val) {
    for (int i = 0; i < nlabels; i++)
        if (strcmp(labels[i].name, name) == 0) {
            *val = labels[i].val;
            return true;
        }
    return false;
}

// Number: SYM+N / SYM-N (equ only), strict int, or bare equ symbol.
static bool num_c(const char *s, int32_t *val) {
    // SYM+N / SYM-N: equ only here (labels are PC-relative and resolve
    // via pctarget_c in la/call/tail/branch/jump paths).
    if (is_name_start(s[0])) {
        size_t i = 0;
        while (is_name_char(s[i])) i++;
        if ((s[i] == '+' || s[i] == '-') && s[i + 1] != '\0') {
            char name[128];
            if (i >= sizeof(name)) return false;
            memcpy(name, s, i);
            name[i] = '\0';
            int32_t base, delta;
            if (equ_find(name, &base) && parse_int_c(s + i + 1, &delta)) {
                *val = s[i] == '+' ? base + delta : base - delta;
                return true;
            }
            return false;
        }
    }
    if (parse_int_c(s, val)) return true;
    if (is_name_start(s[0])) {
        size_t i = 0;
        while (is_name_char(s[i])) i++;
        if (s[i] == '\0') {
            char name[128];
            if (i >= sizeof(name)) return false;
            memcpy(name, s, i);
            name[i] = '\0';
            return equ_find(name, val);
        }
    }
    return false;
}

// Jump/branch target: '.' (current addr), label, or number.
static bool target_c(const char *s, uint32_t addr, int32_t *val) {
    if (strcmp(s, ".") == 0) {
        *val = (int32_t)addr;
        return true;
    }
    if (is_name_start(s[0])) {
        size_t i = 0;
        while (is_name_char(s[i])) i++;
        if (s[i] == '\0') {
            char name[128];
            if (i >= sizeof(name)) return false;
            memcpy(name, s, i);
            name[i] = '\0';
            if (label_find(name, val)) return true;
            // Not a label: fall through to num (equ symbol).
        }
    }
    return num_c(s, val);
}

static uint32_t encR(uint32_t f7, uint32_t rs2, uint32_t rs1, uint32_t f3, uint32_t rd, uint32_t op) {
    return ((f7 << 25) | (rs2 << 20) | (rs1 << 15) | (f3 << 12) | (rd << 7) | op);
}

static uint32_t encI(int32_t imm, uint32_t rs1, uint32_t f3, uint32_t rd, uint32_t op) {
    return ((((uint32_t)imm & 0xFFF) << 20) | (rs1 << 15) | (f3 << 12) | (rd << 7) | op);
}

static uint32_t encS(int32_t imm, uint32_t rs1, uint32_t rs2, uint32_t f3) {
    uint32_t u = (uint32_t)imm & 0xFFF;
    return ((((u >> 5) & 0x7F) << 25) | (rs2 << 20) | (rs1 << 15) | (f3 << 12) | ((u & 0x1F) << 7) | 0x23);
}

static uint32_t encB(int32_t imm, uint32_t rs1, uint32_t rs2, uint32_t f3) {
    uint32_t u = (uint32_t)imm & 0x1FFF;
    return ((((u >> 12) & 1) << 31) | (((u >> 5) & 0x3F) << 25) | (rs2 << 20) | (rs1 << 15) |
             (f3 << 12) | (((u >> 1) & 0xF) << 8) | (((u >> 11) & 1) << 7) | 0x63);
}

static uint32_t encU(uint32_t hi20, uint32_t rd, uint32_t op) {
    return (((hi20 & 0xFFFFF) << 12) | (rd << 7) | op);
}

static uint32_t encJ(int32_t imm, uint32_t rd) {
    uint32_t u = (uint32_t)imm & 0x1FFFFF;
    return ((((u >> 20) & 1) << 31) | (((u >> 1) & 0x3FF) << 21) | (((u >> 11) & 1) << 20) |
             (((u >> 12) & 0xFF) << 12) | (rd << 7) | 0x6F);
}

static void emit(uint32_t w) {
    if (out_len + 4 > MEM_MAX - PROG_BASE) {
        fprintf(stderr, "asm: error: code too large for the 4KB model\n");
        exit(1);
    }
    out[out_len++] = (uint8_t)(w & 0xFF);
    out[out_len++] = (uint8_t)((w >> 8) & 0xFF);
    out[out_len++] = (uint8_t)((w >> 16) & 0xFF);
    out[out_len++] = (uint8_t)((w >> 24) & 0xFF);
}

static void emit_byte(uint8_t b) {
    if (out_len + 1 > MEM_MAX - PROG_BASE) {
        fprintf(stderr, "asm: error: code too large for the 4KB model\n");
        exit(1);
    }
    out[out_len++] = b;
}

static bool fit12(int32_t v) {
    return v >= -2048 && v <= 2047;
}

// Floor division by 4096 (C truncates toward zero; JS Math.floor floors).
static int32_t floor4096(int32_t v) {
    if (v >= 0) return v / 4096;
    return -((-v + 4095) / 4096);
}

// clang-identical li split: 1x addi (12-bit), 1x lui (lo==0), else lui+addi.
static void li_split(int32_t v, int *n, uint32_t *hi, int32_t *lo, bool *addi_only) {
    if (fit12(v)) {
        *n = 1;
        *addi_only = true;
        return;
    }
    int32_t full = floor4096(v + 0x800);
    *hi = (uint32_t)full & 0xFFFFF;
    *lo = v - full * 4096; // always in [-2048, 2047] by construction
    *addi_only = false;
    *n = (*lo == 0) ? 1 : 2;
}

// clang-identical auipc+addi split for la/call: hi20 = floor((tgt-pc+0x800)/4096),
// lo12 = (tgt-pc) - hi20*4096 (always in [-2048,2047] by construction).
static void pcrel_split(int32_t tgt, uint32_t pc, uint32_t *hi, int32_t *lo) {
    int32_t d = tgt - (int32_t)pc;
    int32_t full = floor4096(d + 0x800);
    *hi = (uint32_t)full & 0xFFFFF;
    *lo = d - full * 4096;
}

// Is s a bare symbol name (label or equ, no dots/offsets)?
static bool is_bare_sym(const char *s, char *name) {
    if (!is_name_start(s[0]) || s[0] == '.') return false;
    size_t i = 0;
    while (is_name_char(s[i])) i++;
    if (s[i] != '\0' || i == 0 || i >= 128) return false;
    memcpy(name, s, i);
    name[i] = '\0';
    return true;
}

// Resolve la/call/tail target: bare label > label SYM+N > target_c
// (equ, equ-expr, number, '.'). Labels win so a label shadowing an equ
// resolves like clang (to the label).
static bool pctarget_c(const char *s, uint32_t addr, int32_t *val) {
    char name[128];
    if (is_bare_sym(s, name)) {
        int32_t lv;
        if (label_find(name, &lv)) {
            *val = lv;
            return true;
        }
    } else if (is_name_start(s[0])) {
        // SYM+N / SYM-N with a label base (forward labels included:
        // label_find fails in pass 1, succeeds in pass 1b/2).
        size_t i = 0;
        while (is_name_char(s[i])) i++;
        if ((s[i] == '+' || s[i] == '-') && s[i + 1] != '\0' && i < sizeof(name)) {
            memcpy(name, s, i);
            name[i] = '\0';
            int32_t base, delta;
            if (label_find(name, &base) && parse_int_c(s + i + 1, &delta)) {
                *val = s[i] == '+' ? base + delta : base - delta;
                return true;
            }
        }
    }
    return target_c(s, addr, val);
}

static bool is_ignored_directive(const char *rest) {
    // .section / .globl / .text / .option (word boundary)
    if (rest[0] != '.') return false;
    size_t i = 1;
    while (isalnum((unsigned char)rest[i]) || rest[i] == '_') i++;
    char w[16];
    if (i - 1 >= sizeof(w)) return false;
    memcpy(w, rest + 1, i - 1);
    w[i - 1] = '\0';
    if (strcmp(w, "section") != 0 && strcmp(w, "globl") != 0 && strcmp(w, "text") != 0 &&
        strcmp(w, "option") != 0)
        return false;
    char c = rest[i];
    return c == '\0' || isspace((unsigned char)c);
}

// Size of one logical line in bytes. Fails on syntax error.
// addr is the pass-1 cursor (needed for call/tail range sizing).
static int line_size(char *rest, uint32_t addr, int lineno) {
    if (*rest == '\0') return 0;
    if (is_ignored_directive(rest)) return 0;
    if (rest[0] == '.' && (strncmp(rest, ".byte", 5) == 0 || strncmp(rest, ".word", 5) == 0) &&
        (rest[5] == '\0' || isspace((unsigned char)rest[5]))) {
        char *args = trim(rest + 5);
        if (*args == '\0') fail(lineno, "empty %s directive", rest[1] == 'b' ? ".byte" : ".word");
        int n = 1;
        for (char *p = args; *p; p++)
            if (*p == ',') n++;
        return rest[1] == 'b' ? n : n * 4;
    }
    if (rest[0] == '.') {
        // .space N[,fill]: N fill bytes (default 0). GAS pads .text with
        // zeros under alignment, but here it is an explicit directive.
        if (strncmp(rest, ".space", 6) == 0 && (rest[6] == '\0' || isspace((unsigned char)rest[6]))) {
            char *args = trim(rest + 6);
            if (*args == '\0') fail(lineno, "empty .space directive");
            char *comma = strchr(args, ',');
            if (comma) *comma = '\0';
            int32_t n, fill = 0;
            if (!num_c(trim(args), &n) || n < 0) fail(lineno, "bad .space size: %s", args);
            if (comma && !num_c(trim(comma + 1), &fill)) fail(lineno, "bad .space fill");
            (void)fill;
            return n;
        }
        fail(lineno, "unknown directive: %s (only .section/.globl/.text/.option/.equ/.byte/.word/.space supported)", rest);
    }
    // mnemonic
    size_t i = 0;
    while (isalnum((unsigned char)rest[i]) || rest[i] == '_') i++;
    if (i == 0) fail(lineno, "syntax error: %s", rest);
    char op[32];
    if (i >= sizeof(op)) fail(lineno, "syntax error: %s", rest);
    memcpy(op, rest, i);
    op[i] = '\0';
    char *args = trim(rest + i);
    if (strcmp(op, "li") == 0) {
        char *comma = strchr(args, ',');
        if (!comma || strchr(comma + 1, ',')) fail(lineno, "li needs 2 args: %s", rest);
        int32_t v;
        if (!num_c(trim(comma + 1), &v))
            fail(lineno, "li with a label address needs la (PC-relative), not li");
        int n;
        uint32_t hi;
        int32_t lo;
        bool addi_only;
        li_split(v, &n, &hi, &lo, &addi_only);
        return n * 4;
    }
    if (strcmp(op, "la") == 0) {
        // la rd, sym: auipc+addi if label (or label-expr), li split if
        // equ/number. Forward labels are not indexed yet in pass 1:
        // unknown bare symbol => 8 (pass 2 errors if truly undefined).
        char *comma = strchr(args, ',');
        if (!comma || strchr(comma + 1, ',')) fail(lineno, "la needs 2 args: %s", rest);
        char name[128];
        char *targ = trim(comma + 1);
        if (is_bare_sym(targ, name)) {
            int32_t lv, ev;
            if (label_find(name, &lv)) return 8; // auipc+addi
            if (equ_find(name, &ev)) {
                int n;
                uint32_t hi;
                int32_t lo;
                bool addi_only;
                li_split(ev, &n, &hi, &lo, &addi_only);
                return n * 4;
            }
            return 8; // forward label (pass 2 resolves or errors)
        }
        int32_t v;
        if (num_c(targ, &v)) {
            int n;
            uint32_t hi;
            int32_t lo;
            bool addi_only;
            li_split(v, &n, &hi, &lo, &addi_only);
            return n * 4;
        }
        return 8; // label expression (SYM+N): auipc+addi
    }
    if (strcmp(op, "call") == 0) {
        // call sym: jal if in range from this addr, else auipc+jalr.
        // Forward labels are not indexed yet in pass 1: size 8
        // (auipc+jalr); pass 2 re-checks with final addresses and
        // shrinks to jal when near — pass 1b below fixes the layout.
        // Here: resolve what we can; unknown label => 8.
        char *a = trim(args);
        if (*a == '\0' || strchr(a, ',')) fail(lineno, "call needs 1 arg: %s", rest);
        int32_t tgt;
        if (!pctarget_c(a, addr, &tgt)) return 8; // forward label: worst case
        int32_t d = tgt - (int32_t)addr;
        if (d >= -1048576 && d <= 1048574 && (d % 2 == 0)) return 4;
        return 8;
    }
    if (strcmp(op, "tail") == 0) {
        char *a = trim(args);
        if (*a == '\0' || strchr(a, ',')) fail(lineno, "tail needs 1 arg: %s", rest);
        int32_t tgt;
        if (!pctarget_c(a, addr, &tgt)) return 8; // forward label: worst case
        int32_t d = tgt - (int32_t)addr;
        if (d >= -1048576 && d <= 1048574 && (d % 2 == 0)) return 4;
        return 8;
    }
    return 4;
}

// Split "label: rest" (label must start the line). Returns rest (maybe empty).
static char *split_label(char *line, char *label, int lineno) {
    if (!is_name_start(line[0])) return line;
    size_t i = 0;
    while (is_name_char(line[i])) i++;
    if (line[i] != ':') return line;
    if (i == 0 || i >= 128) fail(lineno, "bad label");
    memcpy(label, line, i);
    label[i] = '\0';
    return trim(line + i + 1);
}

static int split_args(char *args, char *argv[4]) {
    if (*args == '\0') return 0;
    int argc = 0;
    argv[argc++] = trim(args);
    for (char *p = args; *p && argc < 4; p++) {
        if (*p == ',') {
            *p = '\0';
            argv[argc++] = trim(p + 1);
        }
    }
    return argc;
}

// Parse off(base); off may be empty (0), number, or equ symbol.
static void parse_mem(char *s, int lineno, int32_t *off, int *base) {
    char *lp = strchr(s, '(');
    char *rp = strrchr(s, ')');
    if (!lp || !rp || rp[1] != '\0' || rp < lp) fail(lineno, "bad memory operand: %s", s);
    *rp = '\0';
    *base = reg_id(trim(lp + 1));
    if (*base < 0) fail(lineno, "bad base register: %s", s);
    *lp = '\0';
    char *offstr = trim(s);
    if (*offstr == '\0') {
        *off = 0;
        return;
    }
    if (!num_c(offstr, off)) fail(lineno, "bad offset: %s", offstr);
}

static int br_funct3(const char *op) {
    if (strcmp(op, "beq") == 0) return 0;
    if (strcmp(op, "bne") == 0) return 1;
    if (strcmp(op, "blt") == 0) return 4;
    if (strcmp(op, "bge") == 0) return 5;
    if (strcmp(op, "bltu") == 0) return 6;
    if (strcmp(op, "bgeu") == 0) return 7;
    return -1;
}

static int load_funct3(const char *op) {
    if (strcmp(op, "lb") == 0) return 0;
    if (strcmp(op, "lh") == 0) return 1;
    if (strcmp(op, "lw") == 0) return 2;
    if (strcmp(op, "lbu") == 0) return 4;
    if (strcmp(op, "lhu") == 0) return 5;
    return -1;
}

static int store_funct3(const char *op) {
    if (strcmp(op, "sb") == 0) return 0;
    if (strcmp(op, "sh") == 0) return 1;
    if (strcmp(op, "sw") == 0) return 2;
    return -1;
}

static bool op_lookup(const char *op, int *f3, uint32_t *f7) {
    struct {
        const char *n;
        int f3;
        uint32_t f7;
    } static const T[] = {
        {"add", 0, 0}, {"sub", 0, 0x20}, {"sll", 1, 0}, {"slt", 2, 0},
        {"sltu", 3, 0}, {"xor", 4, 0}, {"srl", 5, 0}, {"sra", 5, 0x20},
        {"or", 6, 0}, {"and", 7, 0},
    };
    for (size_t i = 0; i < sizeof(T) / sizeof(T[0]); i++)
        if (strcmp(op, T[i].n) == 0) {
            *f3 = T[i].f3;
            *f7 = T[i].f7;
            return true;
        }
    return false;
}

static int opi_funct3(const char *op) {
    if (strcmp(op, "addi") == 0) return 0;
    if (strcmp(op, "slli") == 0) return 1;
    if (strcmp(op, "slti") == 0) return 2;
    if (strcmp(op, "sltiu") == 0) return 3;
    if (strcmp(op, "xori") == 0) return 4;
    if (strcmp(op, "srli") == 0) return 5;
    if (strcmp(op, "srai") == 0) return 5;
    if (strcmp(op, "ori") == 0) return 6;
    if (strcmp(op, "andi") == 0) return 7;
    return -1;
}

// Assemble one logical line at addr.
static void assemble_line(char *rest, uint32_t addr, int lineno) {
    if (*rest == '\0') return;
    if (is_ignored_directive(rest)) return;
    if ((strncmp(rest, ".byte", 5) == 0 || strncmp(rest, ".word", 5) == 0) &&
        (rest[5] == '\0' || isspace((unsigned char)rest[5]))) {
        bool is_byte = rest[1] == 'b';
        char *argv[4];
        // .byte/.word can have more than 4 items; handle manually
        char *p = trim(rest + 5);
        for (;;) {
            char *comma = strchr(p, ',');
            if (comma) *comma = '\0';
            int32_t v;
            if (!num_c(trim(p), &v)) fail(lineno, "bad value: %s", p);
            if (is_byte) emit_byte((uint8_t)(v & 0xFF));
            else {
                uint32_t w = (uint32_t)v;
                emit_byte((uint8_t)(w & 0xFF));
                emit_byte((uint8_t)((w >> 8) & 0xFF));
                emit_byte((uint8_t)((w >> 16) & 0xFF));
                emit_byte((uint8_t)((w >> 24) & 0xFF));
            }
            (void)argv;
            if (!comma) break;
            p = comma + 1;
        }
        return;
    }
    if (rest[0] == '.') {
        if (strncmp(rest, ".space", 6) == 0 && (rest[6] == '\0' || isspace((unsigned char)rest[6]))) {
            char *args = trim(rest + 6);
            char *comma = strchr(args, ',');
            if (comma) *comma = '\0';
            int32_t n, fill = 0;
            if (!num_c(trim(args), &n) || n < 0) fail(lineno, "bad .space size");
            if (comma && !num_c(trim(comma + 1), &fill)) fail(lineno, "bad .space fill");
            for (int32_t k = 0; k < n; k++) emit_byte((uint8_t)(fill & 0xFF));
            return;
        }
        fail(lineno, "unknown directive: %s (only .section/.globl/.text/.option/.equ/.byte/.word/.space supported)", rest);
    }

    size_t i = 0;
    while (isalnum((unsigned char)rest[i]) || rest[i] == '_') i++;
    if (i == 0) fail(lineno, "syntax error: %s", rest);
    char op[32];
    if (i >= sizeof(op)) fail(lineno, "syntax error: %s", rest);
    memcpy(op, rest, i);
    op[i] = '\0';
    char *argv[4];
    int argc = split_args(trim(rest + i), argv);
    if (argc > 0 && strlen(argv[argc - 1]) == 0) argc--; // trailing comma guard

    int rd, rs1, rs2;
    int32_t imm, tgt;

    // li / mv / nop / not / neg / seqz / snez / sltz / sgtz
    if (strcmp(op, "li") == 0) {
        if (argc != 2) fail(lineno, "li needs 2 args");
        rd = reg_id(argv[0]);
        if (rd < 0 || !num_c(argv[1], &imm)) fail(lineno, "bad li args");
        int n;
        uint32_t hi;
        int32_t lo;
        bool addi_only;
        li_split(imm, &n, &hi, &lo, &addi_only);
        if (addi_only) emit(encI(imm, 0, 0, (uint32_t)rd, 0x13));
        else if (n == 1) emit(encU(hi, (uint32_t)rd, 0x37));
        else {
            emit(encU(hi, (uint32_t)rd, 0x37));
            emit(encI(lo, (uint32_t)rd, 0, (uint32_t)rd, 0x13));
        }
        return;
    }
    if (strcmp(op, "mv") == 0) {
        if (argc != 2) fail(lineno, "mv needs 2 args");
        rd = reg_id(argv[0]);
        rs1 = reg_id(argv[1]);
        if (rd < 0 || rs1 < 0) fail(lineno, "bad mv args");
        emit(encI(0, (uint32_t)rs1, 0, (uint32_t)rd, 0x13));
        return;
    }
    if (strcmp(op, "nop") == 0) {
        if (argc != 0) fail(lineno, "nop takes no args");
        emit(encI(0, 0, 0, 0, 0x13));
        return;
    }
    if (strcmp(op, "not") == 0) {
        if (argc != 2) fail(lineno, "not needs 2 args");
        rd = reg_id(argv[0]);
        rs1 = reg_id(argv[1]);
        if (rd < 0 || rs1 < 0) fail(lineno, "bad not args");
        emit(encI(-1, (uint32_t)rs1, 4, (uint32_t)rd, 0x13));
        return;
    }
    if (strcmp(op, "neg") == 0) {
        if (argc != 2) fail(lineno, "neg needs 2 args");
        rd = reg_id(argv[0]);
        rs1 = reg_id(argv[1]);
        if (rd < 0 || rs1 < 0) fail(lineno, "bad neg args");
        emit(encR(0x20, (uint32_t)rs1, 0, 0, (uint32_t)rd, 0x33));
        return;
    }
    if (strcmp(op, "seqz") == 0 || strcmp(op, "snez") == 0 || strcmp(op, "sltz") == 0 ||
        strcmp(op, "sgtz") == 0) {
        if (argc != 2) fail(lineno, "%s needs 2 args", op);
        rd = reg_id(argv[0]);
        rs1 = reg_id(argv[1]);
        if (rd < 0 || rs1 < 0) fail(lineno, "bad %s args", op);
        if (strcmp(op, "seqz") == 0) emit(encI(1, (uint32_t)rs1, 3, (uint32_t)rd, 0x13));
        else if (strcmp(op, "snez") == 0)
            emit(encR(0, (uint32_t)rs1, 0, 3, (uint32_t)rd, 0x33));
        else if (strcmp(op, "sltz") == 0)
            emit(encR(0, 0, (uint32_t)rs1, 2, (uint32_t)rd, 0x33));
        else emit(encR(0, (uint32_t)rs1, 0, 2, (uint32_t)rd, 0x33));
        return;
    }
    // jumps
    if (strcmp(op, "j") == 0) {
        if (argc != 1) fail(lineno, "j needs 1 arg");
        if (!target_c(argv[0], addr, &tgt)) fail(lineno, "bad jump target: %s", argv[0]);
        if (tgt - (int32_t)addr < -1048576 || tgt - (int32_t)addr > 1048574)
            fail(lineno, "jump out of range");
        emit(encJ(tgt - (int32_t)addr, 0));
        return;
    }
    if (strcmp(op, "jal") == 0) {
        if (argc == 1) {
            if (!target_c(argv[0], addr, &tgt)) fail(lineno, "bad jump target: %s", argv[0]);
            if (tgt - (int32_t)addr < -1048576 || tgt - (int32_t)addr > 1048574)
                fail(lineno, "jump out of range");
            emit(encJ(tgt - (int32_t)addr, 1));
            return;
        }
        if (argc != 2) fail(lineno, "jal needs 1 or 2 args");
        rd = reg_id(argv[0]);
        if (rd < 0) fail(lineno, "bad jal register");
        if (!target_c(argv[1], addr, &tgt)) fail(lineno, "bad jump target: %s", argv[1]);
        if (tgt - (int32_t)addr < -1048576 || tgt - (int32_t)addr > 1048574)
            fail(lineno, "jump out of range");
        emit(encJ(tgt - (int32_t)addr, (uint32_t)rd));
        return;
    }
    if (strcmp(op, "jr") == 0) {
        if (argc != 1) fail(lineno, "jr needs 1 arg");
        if (strchr(argv[0], '(')) {
            parse_mem(argv[0], lineno, &imm, &rs1);
            if (!fit12(imm)) fail(lineno, "jr offset out of range");
            emit(encI(imm, (uint32_t)rs1, 0, 0, 0x67));
        } else {
            rs1 = reg_id(argv[0]);
            if (rs1 < 0) fail(lineno, "bad jr register");
            emit(encI(0, (uint32_t)rs1, 0, 0, 0x67));
        }
        return;
    }
    if (strcmp(op, "jalr") == 0) {
        if (argc == 1) {
            rs1 = reg_id(argv[0]);
            if (rs1 < 0) fail(lineno, "bad jalr register");
            emit(encI(0, (uint32_t)rs1, 0, 1, 0x67));
            return;
        }
        if (argc == 2) {
            rd = reg_id(argv[0]);
            if (rd < 0 || !strchr(argv[1], '(')) fail(lineno, "bad jalr args");
            parse_mem(argv[1], lineno, &imm, &rs1);
            if (!fit12(imm)) fail(lineno, "jalr offset out of range");
            emit(encI(imm, (uint32_t)rs1, 0, (uint32_t)rd, 0x67));
            return;
        }
        if (argc != 3) fail(lineno, "jalr needs 1, 2 or 3 args");
        rd = reg_id(argv[0]);
        rs1 = reg_id(argv[1]);
        if (rd < 0 || rs1 < 0 || !num_c(argv[2], &imm) || !fit12(imm))
            fail(lineno, "bad jalr args");
        emit(encI(imm, (uint32_t)rs1, 0, (uint32_t)rd, 0x67));
        return;
    }
    if (strcmp(op, "ret") == 0) {
        if (argc != 0) fail(lineno, "ret takes no args");
        emit(encI(0, 1, 0, 0, 0x67));
        return;
    }
    if (strcmp(op, "la") == 0) {
        // clang-identical: la rd, number/equ => li split;
        // la rd, label-expr => auipc+addi (PC-relative).
        if (argc != 2) fail(lineno, "la needs 2 args");
        rd = reg_id(argv[0]);
        if (rd < 0) fail(lineno, "bad la register");
        char name[128];
        int32_t v;
        if (is_bare_sym(argv[1], name)) {
            int32_t lv;
            if (label_find(name, &lv)) {
                uint32_t hi;
                int32_t lo;
                pcrel_split(lv, addr, &hi, &lo);
                emit(encU(hi, (uint32_t)rd, 0x17));
                emit(encI(lo, (uint32_t)rd, 0, (uint32_t)rd, 0x13));
                return;
            }
            if (equ_find(name, &v)) {
                // fall through to li split below
            } else {
                fail(lineno, "unknown symbol: %s", name);
            }
        } else if (num_c(argv[1], &v)) {
            // pure number or equ-expr: li split below
        } else {
            // label expression (SYM+N): auipc+addi
            if (!pctarget_c(argv[1], addr, &v)) fail(lineno, "bad la target: %s", argv[1]);
            uint32_t hi;
            int32_t lo;
            pcrel_split(v, addr, &hi, &lo);
            emit(encU(hi, (uint32_t)rd, 0x17));
            emit(encI(lo, (uint32_t)rd, 0, (uint32_t)rd, 0x13));
            return;
        }
        int n;
        uint32_t hi;
        int32_t lo;
        bool addi_only;
        li_split(v, &n, &hi, &lo, &addi_only);
        if (addi_only) emit(encI(v, 0, 0, (uint32_t)rd, 0x13));
        else if (n == 1) emit(encU(hi, (uint32_t)rd, 0x37));
        else {
            emit(encU(hi, (uint32_t)rd, 0x37));
            emit(encI(lo, (uint32_t)rd, 0, (uint32_t)rd, 0x13));
        }
        return;
    }
    if (strcmp(op, "call") == 0) {
        // clang-identical: jal if target in ±1MB from here, else auipc+jalr.
        if (argc != 1) fail(lineno, "call needs 1 arg");
        if (!pctarget_c(argv[0], addr, &tgt)) fail(lineno, "bad call target: %s", argv[0]);
        int32_t d = tgt - (int32_t)addr;
        if (d >= -1048576 && d <= 1048574 && (d % 2 == 0)) {
            emit(encJ(d, 1));
        } else {
            uint32_t hi;
            int32_t lo;
            pcrel_split(tgt, addr, &hi, &lo);
            emit(encU(hi, 1, 0x17)); // auipc ra, hi
            emit(encI(lo, 1, 0, 1, 0x67)); // jalr ra, lo(ra)
        }
        return;
    }
    if (strcmp(op, "tail") == 0) {
        // clang-identical: j if in range, else auipc+jalr with x0 (no link).
        if (argc != 1) fail(lineno, "tail needs 1 arg");
        if (!pctarget_c(argv[0], addr, &tgt)) fail(lineno, "bad tail target: %s", argv[0]);
        int32_t d = tgt - (int32_t)addr;
        if (d >= -1048576 && d <= 1048574 && (d % 2 == 0)) {
            emit(encJ(d, 0));
        } else {
            uint32_t hi;
            int32_t lo;
            pcrel_split(tgt, addr, &hi, &lo);
            emit(encU(hi, 6, 0x17)); // auipc t1, hi (clang scratch reg)
            emit(encI(lo, 6, 0, 0, 0x67)); // jalr x0, lo(t1)
        }
        return;
    }
    // branches: beqz/bnez/blez/bgez/bltz/bgtz, bgt/ble/bgtu/bleu, canonical
    {
        struct {
            const char *n;
            const char *base;
            int order;
        } static const BMAP[] = {
            {"beqz", "beq", 1}, {"bnez", "bne", 1}, {"blez", "bge", 0},
            {"bgez", "bge", 1}, {"bltz", "blt", 1}, {"bgtz", "blt", 0},
        };
        for (size_t k = 0; k < sizeof(BMAP) / sizeof(BMAP[0]); k++) {
            if (strcmp(op, BMAP[k].n) != 0) continue;
            if (argc != 2) fail(lineno, "%s needs 2 args", op);
            rs1 = reg_id(argv[0]);
            if (rs1 < 0 || !target_c(argv[1], addr, &tgt)) fail(lineno, "bad %s args", op);
            int32_t d = tgt - (int32_t)addr;
            if (d < -4096 || d > 4094 || d % 2 != 0) fail(lineno, "%s out of range", op);
            uint32_t s1 = BMAP[k].order ? (uint32_t)rs1 : 0;
            uint32_t s2 = BMAP[k].order ? 0 : (uint32_t)rs1;
            emit(encB(d, s1, s2, (uint32_t)br_funct3(BMAP[k].base)));
            return;
        }
    }
    if (strcmp(op, "bgt") == 0 || strcmp(op, "ble") == 0 || strcmp(op, "bgtu") == 0 ||
        strcmp(op, "bleu") == 0) {
        const char *canon =
            strcmp(op, "bgt") == 0 ? "blt" : strcmp(op, "ble") == 0 ? "bge" : strcmp(op, "bgtu") == 0 ? "bltu" : "bgeu";
        if (argc != 3) fail(lineno, "%s needs 3 args", op);
        rs1 = reg_id(argv[0]);
        rs2 = reg_id(argv[1]);
        if (rs1 < 0 || rs2 < 0 || !target_c(argv[2], addr, &tgt)) fail(lineno, "bad %s args", op);
        int32_t d = tgt - (int32_t)addr;
        if (d < -4096 || d > 4094 || d % 2 != 0) fail(lineno, "%s out of range", op);
        emit(encB(d, (uint32_t)rs2, (uint32_t)rs1, (uint32_t)br_funct3(canon)));
        return;
    }
    {
        int f3 = br_funct3(op);
        if (f3 >= 0) {
            if (argc != 3) fail(lineno, "%s needs 3 args", op);
            rs1 = reg_id(argv[0]);
            rs2 = reg_id(argv[1]);
            if (rs1 < 0 || rs2 < 0 || !target_c(argv[2], addr, &tgt))
                fail(lineno, "bad %s args", op);
            int32_t d = tgt - (int32_t)addr;
            if (d < -4096 || d > 4094 || d % 2 != 0) fail(lineno, "%s out of range", op);
            emit(encB(d, (uint32_t)rs1, (uint32_t)rs2, (uint32_t)f3));
            return;
        }
    }
    // loads / stores
    {
        int f3 = load_funct3(op);
        if (f3 >= 0) {
            if (argc != 2) fail(lineno, "%s needs 2 args", op);
            rd = reg_id(argv[0]);
            if (rd < 0 || !strchr(argv[1], '(')) fail(lineno, "bad %s args", op);
            parse_mem(argv[1], lineno, &imm, &rs1);
            if (!fit12(imm)) fail(lineno, "%s offset out of range", op);
            emit(encI(imm, (uint32_t)rs1, (uint32_t)f3, (uint32_t)rd, 0x03));
            return;
        }
        f3 = store_funct3(op);
        if (f3 >= 0) {
            if (argc != 2) fail(lineno, "%s needs 2 args", op);
            rs2 = reg_id(argv[0]);
            if (rs2 < 0 || !strchr(argv[1], '(')) fail(lineno, "bad %s args", op);
            parse_mem(argv[1], lineno, &imm, &rs1);
            if (!fit12(imm)) fail(lineno, "%s offset out of range", op);
            emit(encS(imm, (uint32_t)rs1, (uint32_t)rs2, (uint32_t)f3));
            return;
        }
    }
    // OP-IMM
    {
        int f3 = opi_funct3(op);
        if (f3 >= 0) {
            if (argc != 3) fail(lineno, "%s needs 3 args", op);
            rd = reg_id(argv[0]);
            rs1 = reg_id(argv[1]);
            if (rd < 0 || rs1 < 0 || !num_c(argv[2], &imm)) fail(lineno, "bad %s args", op);
            if (strcmp(op, "slli") == 0 || strcmp(op, "srli") == 0 || strcmp(op, "srai") == 0) {
                if (imm < 0 || imm > 31) fail(lineno, "%s shift out of range", op);
                uint32_t f7 = strcmp(op, "srai") == 0 ? 0x20 : 0;
                emit(encI((int32_t)((f7 << 5) | (uint32_t)imm), rs1, (uint32_t)f3, rd, 0x13));
            } else {
                if (!fit12(imm)) fail(lineno, "%s immediate out of range", op);
                emit(encI(imm, (uint32_t)rs1, (uint32_t)f3, (uint32_t)rd, 0x13));
            }
            return;
        }
    }
    // OP
    {
        int f3;
        uint32_t f7;
        if (op_lookup(op, &f3, &f7)) {
            if (argc != 3) fail(lineno, "%s needs 3 args", op);
            rd = reg_id(argv[0]);
            rs1 = reg_id(argv[1]);
            rs2 = reg_id(argv[2]);
            if (rd < 0 || rs1 < 0 || rs2 < 0) fail(lineno, "bad %s args", op);
            emit(encR(f7, (uint32_t)rs2, (uint32_t)rs1, (uint32_t)f3, (uint32_t)rd, 0x33));
            return;
        }
    }
    if (strcmp(op, "lui") == 0 || strcmp(op, "auipc") == 0) {
        if (argc != 2) fail(lineno, "%s needs 2 args", op);
        rd = reg_id(argv[0]);
        if (rd < 0 || !num_c(argv[1], &imm) || imm < 0 || imm > 0xFFFFF)
            fail(lineno, "bad %s args", op);
        emit(encU((uint32_t)imm, (uint32_t)rd, strcmp(op, "lui") == 0 ? 0x37 : 0x17));
        return;
    }
    fail(lineno, "unknown instruction: %s", op);
}

// .equ NAME, val  |  .equ NAME val  |  define NAME val
static void preprocess(int lineno, char *line) {
    char *p = line;
    bool is_equ = strncmp(p, ".equ", 4) == 0 && (p[4] == '\0' || isspace((unsigned char)p[4]));
    bool is_def = strncmp(p, "define", 6) == 0 && (p[6] == '\0' || isspace((unsigned char)p[6]));
    if (!is_equ && !is_def) return;
    p = trim(p + (is_equ ? 4 : 6));
    size_t i = 0;
    if (!(isalpha((unsigned char)p[0]) || p[0] == '_')) fail(lineno, "bad .equ name");
    while (isalnum((unsigned char)p[i]) || p[i] == '_') i++;
    char name[128];
    if (i == 0 || i >= sizeof(name)) fail(lineno, "bad .equ name");
    memcpy(name, p, i);
    name[i] = '\0';
    p = trim(p + i);
    if (*p == ',') p = trim(p + 1);
    int32_t v;
    if (*p == '\0' || !num_c(p, &v)) fail(lineno, "bad .equ value: %s", line);
    if (nequ >= MAX_SYMS) fail(lineno, "too many symbols");
    int32_t old;
    if (equ_find(name, &old)) {
        for (int k = 0; k < nequ; k++)
            if (strcmp(equ_syms[k].name, name) == 0) equ_syms[k].val = v;
    } else {
        snprintf(equ_syms[nequ].name, sizeof(equ_syms[nequ].name), "%s", name);
        equ_syms[nequ].val = v;
        nequ++;
    }
    line[0] = '\0';
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <prog.s> <out.bin>\n", argv[0]);
        return 1;
    }
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        fprintf(stderr, "asm: cannot open %s\n", argv[1]);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0 || sz > 1 << 20) {
        fprintf(stderr, "asm: bad input size\n");
        fclose(f);
        return 1;
    }
    char *src = malloc((size_t)sz + 1);
    if (!src) {
        fclose(f);
        return 1;
    }
    size_t n = fread(src, 1, (size_t)sz, f);
    fclose(f);
    src[n] = '\0';

    // Split into lines.
    char *p = src;
    char *start = src;
    while (*p) {
        if (*p == '\n') {
            *p = '\0';
            if (nlines >= MAX_LINES) {
                fprintf(stderr, "asm: too many lines\n");
                return 1;
            }
            size_t len = strlen(start);
            if (len > 0 && start[len - 1] == '\r') start[len - 1] = '\0';
            snprintf(lines[nlines++], MAX_LINE, "%s", start);
            start = p + 1;
        }
        p++;
    }
    if (*start) {
        if (nlines >= MAX_LINES) {
            fprintf(stderr, "asm: too many lines\n");
            return 1;
        }
        snprintf(lines[nlines++], MAX_LINE, "%s", start);
    }
    free(src);

    for (int i = 0; i < nlines; i++) {
        sanitize(lines[i]);
        char *t = trim(lines[i]);
        if (t != lines[i]) memmove(lines[i], t, strlen(t) + 1);
        preprocess(i, lines[i]);
    }

    // Pass 1: index labels, size lines. call/tail to a forward label
    // sizes 8 (worst case); pass 1b shrinks near-calls to 4 once all
    // label addresses are final, then re-flows the layout to fixpoint.
    // labline[i] = label table index defined on line i (or -1).
    static int labline[MAX_LINES];
    for (int i = 0; i < nlines; i++) labline[i] = -1;
    uint32_t addr = PROG_BASE;
    static int sizes[MAX_LINES];
    for (int i = 0; i < nlines; i++) {
        char label[128] = {0};
        char *rest = split_label(lines[i], label, i);
        if (label[0]) {
            int32_t old;
            if (label_find(label, &old)) fail(i, "label already defined: %s", label);
            if (nlabels >= MAX_SYMS) fail(i, "too many labels");
            snprintf(labels[nlabels].name, sizeof(labels[nlabels].name), "%s", label);
            labels[nlabels].val = (int32_t)addr;
            labline[i] = nlabels;
            nlabels++;
            // Compact the consumed label out of the line.
            memmove(lines[i], rest, strlen(rest) + 1);
            rest = lines[i];
        }
        int ssz = line_size(rest, addr, i);
        sizes[i] = ssz;
        addr += (uint32_t)ssz;
        if (addr > MEM_MAX) fail(i, "code too large for the 4KB model");
    }

    // Pass 1b: with all labels known, shrink call/tail auipc+jalr to jal/j
    // when in range, and re-flow addresses to fixpoint. Shrinking only
    // moves later labels down, which keeps near-calls near (monotone:
    // terminates). la/jal/branches never change size, so only call/tail
    // lines are re-sized here.
    for (;;) {
        bool changed = false;
        addr = PROG_BASE;
        // line addresses under current sizes
        static uint32_t lineaddr[MAX_LINES];
        for (int i = 0; i < nlines; i++) {
            lineaddr[i] = addr;
            addr += (uint32_t)sizes[i];
        }
        // refresh label addresses
        for (int i = 0; i < nlines; i++)
            if (labline[i] >= 0) labels[labline[i]].val = (int32_t)lineaddr[i];
        // re-size call/tail lines
        for (int i = 0; i < nlines; i++) {
            if (sizes[i] != 8) continue;
            char *rest = lines[i];
            size_t k = 0;
            while (isalnum((unsigned char)rest[k]) || rest[k] == '_') k++;
            char op[32];
            if (k == 0 || k >= sizeof(op)) continue;
            memcpy(op, rest, k);
            op[k] = '\0';
            if (strcmp(op, "call") != 0 && strcmp(op, "tail") != 0) continue;
            char *a = trim(rest + k);
            if (*a == '\0' || strchr(a, ',')) fail(i, "%s needs 1 arg", op);
            int32_t tgt;
            if (!pctarget_c(a, lineaddr[i], &tgt)) fail(i, "bad %s target: %s", op, a);
            int32_t d = tgt - (int32_t)lineaddr[i];
            if (d >= -1048576 && d <= 1048574 && (d % 2 == 0)) {
                sizes[i] = 4;
                changed = true;
            }
        }
        if (!changed) break;
    }

    // Pass 2: emit.
    addr = PROG_BASE;
    out_len = 0;
    for (int i = 0; i < nlines; i++) {
        if (sizes[i] > 0 && (addr & 3)) fail(i, "misaligned instruction: %s", lines[i]);
        uint32_t base = out_len;
        (void)base;
        assemble_line(lines[i], addr, i);
        addr += (uint32_t)sizes[i];
    }
    if (out_len == 0) {
        fprintf(stderr, "asm: no code to emit\n");
        return 1;
    }

    FILE *o = fopen(argv[2], "wb");
    if (!o) {
        fprintf(stderr, "asm: cannot write %s\n", argv[2]);
        return 1;
    }
    fwrite(out, 1, out_len, o);
    fclose(o);
    return 0;
}
