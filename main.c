#include "vm.h"
#include "ui.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv){
    if(argc<2){
        fprintf(stderr,"Usage: %s <bin> [--headless]\n", argv[0]);
        fprintf(stderr,"Ex: %s /tmp/fib.bin\n", argv[0]);
        fprintf(stderr,"    %s /tmp/fib.bin --headless\n", argv[0]);
        return 1;
    }
    const char *bin = argv[1];
    bool headless = (argc>2 && strcmp(argv[2],"--headless")==0);

    CPU cpu;
    if(!cpu_load_bin(&cpu, bin)){
        perror("load bin");
        return 1;
    }

    if(headless){
        // headless mode - like easy6502 Run
        while(!cpu.halted){
            if(!cpu_step(&cpu)) break;
        }
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
        printf("\nFramebuffer 0x200 (words):");
        for(int i=0;i<4;i++){
            uint32_t v = cpu.mem[0x200+i*4] | (cpu.mem[0x200+i*4+1]<<8) | (cpu.mem[0x200+i*4+2]<<16) | (cpu.mem[0x200+i*4+3]<<24);
            printf(" %u", v & 0xF);
        }
        printf(" raw bytes: %02x %02x %02x %02x\n", cpu.mem[0x200], cpu.mem[0x204], cpu.mem[0x208], cpu.mem[0x20c]);
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
        if(cmd==3) break;
        if(cmd==2){ cpu_reset(&cpu); cpu_load_bin(&cpu,bin); continue; }
        if(cmd==1){ // run até halt
            nodelay(stdscr, TRUE);
            while(!cpu.halted){
                if(!cpu_step(&cpu)) break;
                // throttling
                if((cpu.pc & 0x3F)==0){
                    ui_draw(&ui,&cpu);
                    int ch=getch();
                    if(ch=='q' || ch==' ') break;
                    if(ch=='R'){ cpu_reset(&cpu); cpu_load_bin(&cpu,bin); break; }
                    usleep(10000);
                }
            }
            nodelay(stdscr, FALSE);
            continue;
        }
        if(cmd==4){ // run slow (spin)
            nodelay(stdscr, TRUE);
            for(int i=0;i<200 && !cpu.halted;i++){
                cpu_step(&cpu);
                ui_draw(&ui,&cpu);
                usleep(30000);
                int ch=getch();
                if(ch!=ERR) { nodelay(stdscr,FALSE); break; }
            }
            nodelay(stdscr,FALSE);
            continue;
        }
        // step
        if(!cpu.halted) cpu_step(&cpu);
    }
    ui_destroy(&ui);
    printf("final a0=%u t0=%u t1=%u mem[0x40]=%u\n", cpu.regs[10], cpu.regs[5], cpu.regs[6],
        (uint32_t)(cpu.mem[0x40]|cpu.mem[0x41]<<8|cpu.mem[0x42]<<16|cpu.mem[0x43]<<24));
    return 0;
}
