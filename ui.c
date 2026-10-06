#include "ui.h"
#include <string.h>
#include <stdlib.h>

void ui_init(UI *ui){
    initscr();
    cbreak(); noecho(); keypad(stdscr, TRUE);
    // Input with 500ms timeout (not blocking forever): the main loop wakes
    // twice a second, so ui_check_resize() heals a resize even with no key.
    // Timeout expiry reads as ERR, which maps to nop (code 5).
    timeout(500);
    // Consume clicks as KEY_MOUSE (1 event) instead of N ANSI bytes:
    // without this each click byte (\x1b [ M ...) became a separate getch()
    // — and \x1b even fell into the quit case. Clicks do nothing.
    mousemask(ALL_MOUSE_EVENTS, nullptr);
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
    ui->w_help = nullptr; ui->w_disasm = nullptr;
    ui->w_regs = nullptr; ui->w_mem = nullptr; ui->w_fb = nullptr;
    ui->last_h = 0; ui->last_w = 0;
    ui_layout(ui);
    // Sync stdscr/curscr before the first ui_draw: without this the first
    // frame sometimes only reaches the terminal after the next input.
    refresh();
}

// Responsive geometry: fixed 42-col disasm on the left; right column
// splits the rest. Vertical priority: help(1) > regs(7) > mem > fb.
// Everything clamped to the REAL size (no fake floor): a tiny terminal
// squeezes, never creates a window outside the screen.
// Do not install your own SIGWINCH handler: ncurses already queues
// KEY_RESIZE on getch — here we only react to it (resizeterm(3x)).
void ui_layout(UI *ui){
    int h, w; getmaxyx(stdscr, h, w);
    if(ui->w_help) delwin(ui->w_help);
    if(ui->w_disasm) delwin(ui->w_disasm);
    if(ui->w_regs) delwin(ui->w_regs);
    if(ui->w_mem) delwin(ui->w_mem);
    if(ui->w_fb) delwin(ui->w_fb);
    int dis_w = w >= 42 ? 42 : w; // disasm shrinks if even 42 do not fit
    int right = w - dis_w;
    ui->w_help   = newwin(1, w, 0, 0);
    ui->w_disasm = newwin(h-1, dis_w, 1, 0);
    // right column: split (h-1) rows across regs/mem/fb
    int avail = h-1;
    int regs_h = avail >= 7 ? 7 : avail;         // regs: all or whatever fits
    int rest = avail - regs_h;
    int mem_h = rest >= 9 ? 8 : (rest*2)/3;      // mem: up to 8, else 2/3
    if(mem_h > rest-2) mem_h = rest-2;           // keep >=2 for fb
    if(mem_h < 0) mem_h = 0;
    int fb_y = 1 + regs_h + mem_h;
    int fb_h = h - fb_y;
    if(fb_h < 0) fb_h = 0;
    ui->w_regs = newwin(regs_h, right, 1, dis_w);
    ui->w_mem  = newwin(mem_h, right, 1+regs_h, dis_w);
    ui->w_fb   = newwin(fb_h, right, fb_y, dis_w);
    ui->last_h = h; ui->last_w = w;
}

// Safety net for lost/coalesced KEY_RESIZE (drag sends several SIGWINCH,
// only one gets queued) and terminals that send no SIGWINCH at all:
// if the real size drifted from what ui_layout() built, rebuild now.
bool ui_check_resize(UI *ui){
    int h, w; getmaxyx(stdscr, h, w);
    if(h == ui->last_h && w == ui->last_w) return false;
    erase();
    ui_layout(ui);
    return true;
}

void ui_destroy(UI *ui){
    delwin(ui->w_disasm); delwin(ui->w_regs); delwin(ui->w_mem); delwin(ui->w_fb); delwin(ui->w_help);
    endwin();
}

static void draw_help(UI *ui){
    werase(ui->w_help);
    wbkgd(ui->w_help, A_REVERSE);
    mvwprintw(ui->w_help,0,1,"EasyRISC-V Terminal | [SPACE]/n:step  r:run  g:run-slow  R:reset  q:quit  | PC in yellow | a0 result");
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
    int mh = getmaxy(ui->w_mem);
    if(mh <= 0) return;
    werase(ui->w_mem);
    box(ui->w_mem,0,0);
    if(mh < 3) { wrefresh(ui->w_mem); return; } // frame only
    mvwprintw(ui->w_mem,0,2," Memory (hexdump 0x00) ");
    int rows = mh-2;
    int show = rows-1; // keep 1 for the 0x00 line (if it fits)
    if(show < 1) show = 1;
    if(show > 4) show = 4;
    for(int i=0;i<show && 1+i<rows;i++){
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
    if(show+1 < rows){
        mvwprintw(ui->w_mem,show+1,1,"0x00: ");
        for(int i=0;i<8;i++){
            uint32_t v = cpu->mem[0x00+i*4] | (cpu->mem[0x00+i*4+1]<<8) | (cpu->mem[0x00+i*4+2]<<16) | (cpu->mem[0x00+i*4+3]<<24);
            wprintw(ui->w_mem,"%d ", v);
        }
    }
    wrefresh(ui->w_mem);
}

static void draw_fb(UI *ui, CPU *cpu){
    int fh = getmaxy(ui->w_fb);
    if(fh <= 0) return;
    werase(ui->w_fb);
    box(ui->w_fb,0,0);
    if(fh < 3) { wrefresh(ui->w_fb); return; } // frame only
    mvwprintw(ui->w_fb,0,2," Framebuffer 32x32 @0x200 ");
    // Each '▀' shows 1 pixel row (the sibling row is skipped —
    // half the vertical resolution is lost; the hex footer keeps every byte visible).
    // Full: 16 rows; compact: sample (y*32)/rows.
    int max_h = fh-2;
    int max_w = getmaxx(ui->w_fb)-2;
    int rows = max_h-1; // keep the footer
    if(rows > 16) rows = 16;
    if(max_w >= 8 && rows >= 1){
        for(int y=0;y<rows;y++){
            int py = rows >= 16 ? y*2 : (y*32)/rows; // sampled row
            wmove(ui->w_fb, 1+y, 1);
            int cols = max_w < 32 ? max_w : 32;
            for(int x=0;x<cols;x++){
                uint8_t c1 = cpu->mem[FB_BASE + py*32 + x] & 0x0F;
                int pair1 = (c1%8)+1;
                wattron(ui->w_fb, COLOR_PAIR(pair1));
                waddstr(ui->w_fb, "▀");
                wattroff(ui->w_fb, COLOR_PAIR(pair1));
            }
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
    if(ch==KEY_RESIZE) return 6;
    if(ch==' ' || ch=='n' || ch=='s' || ch==KEY_RIGHT || ch=='\n') return 0;
    if(ch=='r') return 1;
    if(ch=='g') return 4;
    if(ch=='R') return 2;
    if(ch=='q' || ch==27) return 3;
    if(ch==KEY_MOUSE){ MEVENT ev; (void)getmouse(&ev); return 5; }
    // Unknown key (arrows, F-keys, WASD outside snake...): NOP.
    return 5;
}
