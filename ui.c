#include "ui.h"
#include <string.h>
#include <stdlib.h>

void ui_init(UI *ui){
    initscr();
    cbreak(); noecho(); keypad(stdscr, TRUE);
    nodelay(stdscr, FALSE);
    curs_set(0);
    if(has_colors()){
        start_color();
        use_default_colors();
        // 16 pairs for framebuffer (colored background)
        for(int i=0;i<16;i++){
            // map palette_256[i] to approximate ncurses color (0-15) + 16..
            // simplify: black foreground, repeated 0-7 background
            init_pair(i+1, COLOR_BLACK, i%8);
        }
        // pairs for highlight
        init_pair(20, COLOR_YELLOW, -1);
        init_pair(21, COLOR_CYAN, -1);
        init_pair(22, COLOR_GREEN, -1);
    }
    int h,w; getmaxyx(stdscr,h,w);
    // Layout: top help (1), left disasm (40), right regs+mem+fb
    ui->w_help   = newwin(1,w,0,0);
    ui->w_disasm = newwin(h-1, 42, 1, 0);
    ui->w_regs   = newwin(10, w-42, 1, 42);
    ui->w_mem    = newwin(8, w-42, 11, 42);
    // fb 32x32 takes 16 rows when doubled, fits below
    ui->w_fb     = newwin(16, w-42, 19, 42);
}

void ui_destroy(UI *ui){
    delwin(ui->w_disasm); delwin(ui->w_regs); delwin(ui->w_mem); delwin(ui->w_fb); delwin(ui->w_help);
    endwin();
}

static void draw_help(UI *ui){
    werase(ui->w_help);
    wbkgd(ui->w_help, A_REVERSE);
    mvwprintw(ui->w_help,0,1,"EasyRISC-V (RV32I) | [SPACE]/n:step  r:run  g:run-slow  R:reset  q:quit  | PC in yellow | a0 result");
    wrefresh(ui->w_help);
}

static void draw_disasm(UI *ui, CPU *cpu){
    werase(ui->w_disasm);
    box(ui->w_disasm,0,0);
    mvwprintw(ui->w_disasm,0,2," Disassembly ");
    int max_lines = getmaxy(ui->w_disasm)-2;
    // center on PC (code lives at PROG_BASE, not 0x0)
    int pc_idx = ((int)cpu->pc - (int)PROG_BASE)/4;
    int start = pc_idx - max_lines/2;
    if(start<0) start=0;
    int prog_words = (cpu->prog_size+3)/4;
    char buf[64];
    for(int i=0;i<max_lines;i++){
        int idx = start+i;
        int addr = idx*4 + (int)PROG_BASE;
        if(addr >= (int)MEM_SIZE) break;
        uint32_t instr=0;
        if(addr+3 < (int)MEM_SIZE) instr = cpu->mem[addr] | (cpu->mem[addr+1]<<8) | (cpu->mem[addr+2]<<16) | (cpu->mem[addr+3]<<24);
        bool is_pc = (addr==(int)cpu->pc);
        if(idx >= prog_words && instr==0) continue;
        decode_to_str(instr, addr, buf, sizeof(buf));
        if(is_pc) wattron(ui->w_disasm, A_REVERSE | COLOR_PAIR(20));
        mvwprintw(ui->w_disasm,1+i,1,"%s %04x: %08x  %-22s", is_pc?"->":"  ", addr, instr, buf);
        if(is_pc) wattroff(ui->w_disasm, A_REVERSE | COLOR_PAIR(20));
    }
    wrefresh(ui->w_disasm);
}

static void draw_regs(UI *ui, CPU *cpu){
    werase(ui->w_regs);
    box(ui->w_regs,0,0);
    mvwprintw(ui->w_regs,0,2," Registers (a0=t1 final) ");
    // show t0,t1,t2,a0,t3,s0,s1 etc - key result registers
    int row=1;
    // row 1: zero ra sp gp
    mvwprintw(ui->w_regs,row++,1,"zero:%08x ra:%08x sp:%08x gp:%08x", cpu->regs[0],cpu->regs[1],cpu->regs[2],cpu->regs[3]);
    mvwprintw(ui->w_regs,row++,1," t0:%08x  t1:%08x  t2:%08x  t3:%08x", cpu->regs[5],cpu->regs[6],cpu->regs[7],cpu->regs[28]);
    mvwprintw(ui->w_regs,row++,1," s0:%08x  s1:%08x  a0:%08x  a1:%08x", cpu->regs[8],cpu->regs[9],cpu->regs[10],cpu->regs[11]);
    mvwprintw(ui->w_regs,row++,1," a2:%08x  a3:%08x  a4:%08x  a5:%08x", cpu->regs[12],cpu->regs[13],cpu->regs[14],cpu->regs[15]);
    mvwprintw(ui->w_regs,row++,1," a6:%08x  a7:%08x  pc:%08x  %s", cpu->regs[16],cpu->regs[17],cpu->pc, cpu->halted?"HALT":"RUN");
    // highlight a0
    mvwchgat(ui->w_regs,4, 18, 8, A_BOLD, 22, NULL);
    wrefresh(ui->w_regs);
}

static void draw_mem(UI *ui, CPU *cpu){
    werase(ui->w_mem);
    box(ui->w_mem,0,0);
    mvwprintw(ui->w_mem,0,2," Memory (hexdump 0x00) ");
    for(int i=0;i<4;i++){
        int base=i*16;
        char hex[64]="", ascii[17]="";
        for(int j=0;j<16;j++){
            uint8_t b = cpu->mem[base+j];
            char tmp[4]; snprintf(tmp,sizeof(tmp),"%02x ",b);
            strcat(hex,tmp);
            ascii[j] = (b>=32 && b<127)? b:'.';
        }
        ascii[16]=0;
        mvwprintw(ui->w_mem,1+i,1,"%04x: %s |%s|", base, hex, ascii);
    }
    // also show zero-page RAM 0x00 (fib vector / snake state)
    mvwprintw(ui->w_mem,6,1,"0x00: ");
    for(int i=0;i<8;i++){
        uint32_t v = cpu->mem[0x00+i*4] | (cpu->mem[0x00+i*4+1]<<8) | (cpu->mem[0x00+i*4+2]<<16) | (cpu->mem[0x00+i*4+3]<<24);
        wprintw(ui->w_mem,"%d ", v);
    }
    wrefresh(ui->w_mem);
}

static void draw_fb(UI *ui, CPU *cpu){
    werase(ui->w_fb);
    box(ui->w_fb,0,0);
    mvwprintw(ui->w_fb,0,2," Framebuffer 32x32 @0x200 ");
    // Render 32x32 -> 16 rows x 32 cols with 2 pixels per row using half-block
    // Simplified: 16 rows, each row shows 2 pixel rows with ' ' char and bg color
    int start_y=1, start_x=1;
    int max_h = getmaxy(ui->w_fb)-2;
    int max_w = getmaxx(ui->w_fb)-2;
    if(max_w < 32) return;
    if(max_h < 16) return;
    for(int y=0;y<16;y++){
        wmove(ui->w_fb, start_y+y, start_x);
        for(int x=0;x<32;x++){
            uint8_t c1 = cpu->mem[FB_BASE + (y*2)*32 + x] & 0x0F;
            uint8_t c2 = cpu->mem[FB_BASE + (y*2+1)*32 + x] & 0x0F;
            // use upper half block to show 2 pixels
            // ncurses: no truecolor, uses 8 colors; map 0-15 -> pairs 1-16
            int pair1 = (c1%8)+1;
            int pair2 = (c2%8)+1;
            // draw with '▀' fg=c1 bg=c2
            // trick: write space with bg=c1, then adjust
            // simple: use ' ' with bg = c1, show second pixel on next row if needed
            // here 1 row = 2 pixels via unicode
            wattron(ui->w_fb, COLOR_PAIR(pair1));
            // try block char
            waddstr(ui->w_fb, "▀");
            wattroff(ui->w_fb, COLOR_PAIR(pair1));
            // for accuracy, ignore c2 on this row (show only c1)
            (void)pair2; (void)c2;
        }
    }
    // fallback: show info
    mvwprintw(ui->w_fb, getmaxy(ui->w_fb)-1, 1, " mem[0x200]=%02x ... mem[0x00]=zero-page ", cpu->mem[0x200]);
    wrefresh(ui->w_fb);
}

void ui_draw(UI *ui, CPU *cpu){
    draw_help(ui);
    draw_disasm(ui,cpu);
    draw_regs(ui,cpu);
    draw_mem(ui,cpu);
    draw_fb(ui,cpu);
    // general refresh
    doupdate();
}

int ui_handle_input(void){
    int ch = getch();
    if(ch==' ' || ch=='n' || ch=='s' || ch==KEY_RIGHT || ch=='\n') return 0;
    if(ch=='r') return 1;
    if(ch=='g') return 4;
    if(ch=='R') return 2;
    if(ch=='q' || ch==27) return 3;
    return 0;
}
