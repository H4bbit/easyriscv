#pragma once
#include "vm.h"
#include <ncurses.h>

typedef struct {
    WINDOW *w_disasm;
    WINDOW *w_regs;
    WINDOW *w_mem;
    WINDOW *w_fb;
    WINDOW *w_help;
} UI;

void ui_init(UI *ui);
void ui_destroy(UI *ui);
void ui_draw(UI *ui, CPU *cpu);
int  ui_handle_input(void); // retorna 0=step, 1=run, 2=reset, 3=quit, 4=run slow, 5=nop (tecla irreconhecida: nao faz nada)
