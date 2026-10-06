#pragma once
#include "vm.h"
#include <ncurses.h>

typedef struct {
    WINDOW *w_disasm;
    WINDOW *w_regs;
    WINDOW *w_mem;
    WINDOW *w_fb;
    WINDOW *w_help;
    int last_h, last_w; // geometry ui_layout() built for (0 = none yet)
} UI;

void ui_init(UI *ui);
void ui_destroy(UI *ui);
void ui_draw(UI *ui, CPU *cpu);
void ui_layout(UI *ui); // rebuild geometry (called on KEY_RESIZE or size drift)
bool ui_check_resize(UI *ui); // true if stdscr size drifted: erase + relayout
int  ui_handle_input(void); // returns 0=step, 1=run, 2=reset, 3=quit, 4=run slow, 5=nop, 6=resize
