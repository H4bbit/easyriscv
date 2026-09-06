#include "vm.h"
#include "ui.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char **argv){
    if(argc<2){
        fprintf(stderr,"Usage: %s <bin> [--headless] [--steps N]\n", argv[0]);
        fprintf(stderr,"Ex: %s /tmp/fib.bin\n", argv[0]);
        fprintf(stderr,"    %s /tmp/fib.bin --headless\n", argv[0]);
        fprintf(stderr,"    %s /tmp/fib.bin --headless --steps 1000\n", argv[0]);
        return 1;
    }
    const char *bin = argv[1];
    bool headless = false;
    long max_steps = -1; // -1 = until halt
    for(int i=2;i<argc;i++){
        if(strcmp(argv[i],"--headless")==0) headless = true;
        else if(strcmp(argv[i],"--steps")==0 && i+1<argc) max_steps = atol(argv[++i]);
    }

    CPU cpu;
    srand((unsigned)time(nullptr));
    if(!cpu_load_bin(&cpu, bin)){
        perror("load bin");
        return 1;
    }

    if(headless){
        // headless mode - like easy6502 Run (supports --steps for infinite loops like Snake)
        long steps = 0;
        while(!cpu.halted && (max_steps < 0 || steps < max_steps)){
            if(!cpu_step(&cpu)) break;
            steps++;
        }
        if(max_steps >= 0 && steps >= max_steps) printf("\n[steps limit %ld reached]\n", max_steps);
        printf("\n[halt] pc=0x%04x a0=%u (0x%x) t0=%u t1=%u t2=%u\n",
            cpu.pc, cpu.regs[10], cpu.regs[10], cpu.regs[5], cpu.regs[6], cpu.regs[7]);
        // dump RAM (0x40 and 0x300 for labs)
        printf("RAM 0x40 :");
        for(int i=0;i<8;i++){
            uint32_t v = cpu.mem[0x40+i*4] | (cpu.mem[0x40+i*4+1]<<8) | (cpu.mem[0x40+i*4+2]<<16) | (cpu.mem[0x40+i*4+3]<<24);
            printf(" %u", v);
        }
        printf("\nRAM 0x300:");
        for(int i=0;i<8;i++){
            uint32_t v = cpu.mem[0x300+i*4] | (cpu.mem[0x300+i*4+1]<<8) | (cpu.mem[0x300+i*4+2]<<16) | (cpu.mem[0x300+i*4+3]<<24);
            printf(" %u", v);
        }
        printf("\nFramebuffer 0x200 (bytes, 1 byte/pixel):");
        for(int i=0;i<8;i++) printf(" %u", cpu.mem[0x200+i] & 0xF);
        printf(" raw: %02x %02x %02x %02x\n", cpu.mem[0x200], cpu.mem[0x201], cpu.mem[0x202], cpu.mem[0x203]);
        return 0;
    }

    UI ui;
    ui_init(&ui);
    while(1){
        ui_draw(&ui,&cpu);
        if(cpu.halted){
            // pisca halt
            mvprintw(0,60," HALT - R:reset q:quit ");
            refresh();
        }
        int cmd = ui_handle_input();
        // feed last key to $FF like easy6502 (for Snake labs)
        if(cmd >= 32 && cmd < 127) cpu.mem[0xFF] = (uint8_t)cmd;
        if(cmd==3) break;
        if(cmd==2){ cpu_reset(&cpu); (void)cpu_load_bin(&cpu,bin); continue; }
        if(cmd==1){ // run até halt
            nodelay(stdscr, TRUE);
            while(!cpu.halted){
                if(!cpu_step(&cpu)) break;
                // throttling
                if((cpu.pc & 0x3F)==0){
                    ui_draw(&ui,&cpu);
                    int ch=getch();
                    if(ch=='q' || ch==' ') break;
                    if(ch=='R'){ cpu_reset(&cpu); (void)cpu_load_bin(&cpu,bin); break; }
                    usleep(10000);
                }
            }
            nodelay(stdscr, FALSE);
            continue;
        }
        if(cmd==4){ // run slow (spin)
            nodelay(stdscr, TRUE);
            for(int i=0;i<200 && !cpu.halted;i++){
                (void)cpu_step(&cpu);
                ui_draw(&ui,&cpu);
                usleep(30000);
                int ch=getch();
                if(ch!=ERR) { nodelay(stdscr,FALSE); break; }
            }
            nodelay(stdscr,FALSE);
            continue;
        }
        // step (store any pending key)
        if(!cpu.halted) (void)cpu_step(&cpu);
    }
    ui_destroy(&ui);
    printf("final a0=%u t0=%u t1=%u mem[0x40]=%u\n", cpu.regs[10], cpu.regs[5], cpu.regs[6],
        (uint32_t)(cpu.mem[0x40]|cpu.mem[0x41]<<8|cpu.mem[0x42]<<16|cpu.mem[0x43]<<24));
    return 0;
}
