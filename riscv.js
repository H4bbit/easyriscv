/* EasyRISC-V - RV32I assembler + simulator (no toolchain, no server).
 * Mirrors skilldrick/easy6502 simulator/assembler.js structure (CC BY 4.0):
 * one RiscvWidget per .widget node: UI + Display + Memory + CPU + Assembler.
 * CPU is a faithful JS port of main-branch vm.c: 4KB flat mem, code at
 * PROG_BASE=0x600, zero-page RAM 0x00, stack top 0x1FC, FB at 0x200,
 * MMIO 0xFE/0xFF intercepted on LOAD, 0x1000 print-char on STORE,
 * halt on jal x0,0 (pc stops). Vanilla JS, no dependencies.
 */
'use strict';

function RiscvWidget(node) {
  var ui = UI();
  var display = Display();
  var memory = Memory();
  var cpu = CPU();
  var assembler = Assembler();

  function initialize() {
    stripText();
    ui.initialize();
    display.initialize();
    cpu.reset();

    node.querySelector('.assembleButton').addEventListener('click', function () {
      assembler.assembleCode();
    });
    node.querySelector('.runButton').addEventListener('click', function () {
      cpu.runBinary();
      cpu.stopDebugger();
    });
    node.querySelector('.resetButton').addEventListener('click', function () {
      cpu.reset();
    });
    node.querySelector('.hexdumpButton').addEventListener('click', assembler.hexdump);
    node.querySelector('.disassembleButton').addEventListener('click', assembler.disassemble);
    node.querySelector('.debug').addEventListener('change', function () {
      if (this.checked) {
        ui.debugOn();
        cpu.enableDebugger();
      } else {
        ui.debugOff();
        cpu.stopDebugger();
      }
    });
    node.querySelector('.monitoring').addEventListener('change', function () {
      ui.toggleMonitor(this.checked);
      cpu.toggleMonitor(this.checked);
    });
    var starts = node.querySelectorAll('.start, .length');
    for (var i = 0; i < starts.length; i++) {
      starts[i].addEventListener('blur', cpu.handleMonitorRangeChange);
    }
    node.querySelector('.stepButton').addEventListener('click', cpu.debugExec);
    node.querySelector('.gotoButton').addEventListener('click', cpu.gotoAddr);
    node.querySelector('.notesButton').addEventListener('click', ui.showNotes);

    var editor = node.querySelector('.code');
    editor.addEventListener('keypress', onEdit);
    editor.addEventListener('input', onEdit);
    editor.addEventListener('keydown', ui.captureTabInEditor);

    document.addEventListener('keypress', memory.storeKeypress);

    initLabPicker();

    cpu.handleMonitorRangeChange();
  }

  // Lab picker: fills the textarea with a lab source (labs.js, generated
  // from main-branch labs/*/prog.s). Changing it resets the widget state.
  function initLabPicker() {
    var sel = node.querySelector('.labSelect');
    if (!sel || typeof LAB_SOURCES === 'undefined') return;
    var ids = Object.keys(LAB_SOURCES).sort();
    for (var i = 0; i < ids.length; i++) {
      var opt = document.createElement('option');
      opt.value = ids[i];
      opt.textContent = ids[i];
      sel.appendChild(opt);
    }
    sel.addEventListener('change', function () {
      node.querySelector('.code').value = LAB_SOURCES[sel.value];
      cpu.stop();
      cpu.reset();
      ui.initialize();
      setMessages('Loaded ' + sel.value + ' - click Assemble, then Run.');
    });
  }

  function onEdit() {
    cpu.stop();
    ui.initialize();
  }

  function stripText() {
    var box = node.querySelector('.code');
    box.value = box.value.replace(/^\n+/, '').replace(/\s+$/, '');
  }

  /* ---------------- UI: button state machine (ports easy6502 UI states) --- */
  function UI() {
    var states = {
      start:         { assemble: true,  run: [false, 'Run'],  reset: false, hexdump: false, disassemble: false, debug: [false, false] },
      assembled:     { assemble: false, run: [true, 'Run'],   reset: true,  hexdump: true,  disassemble: true,  debug: [true, false] },
      running:       { assemble: false, run: [true, 'Stop'],  reset: true,  hexdump: false, disassemble: false, debug: [true, false] },
      debugging:     { assemble: false,                       reset: true,  hexdump: true,  disassemble: true,  debug: [true, true] },
      postDebugging: { assemble: false,                       reset: true,  hexdump: true,  disassemble: true,  debug: [true, false] }
    };

    function setState(s) {
      setBtn('.assembleButton', s.assemble);
      if (s.run) {
        var b = node.querySelector('.runButton');
        b.disabled = !s.run[0];
        b.value = s.run[1];
      }
      setBtn('.resetButton', s.reset);
      setBtn('.hexdumpButton', s.hexdump);
      setBtn('.disassembleButton', s.disassemble);
      var dbg = node.querySelector('.debug');
      dbg.disabled = !s.debug[0];
      dbg.checked = s.debug[1];
      setBtn('.stepButton', s.debug[1]);
      setBtn('.gotoButton', s.debug[1]);
    }

    function setBtn(sel, on) {
      node.querySelector(sel).disabled = !on;
    }

    return {
      initialize: function () { setState(states.start); },
      play: function () { setState(states.running); },
      stop: function () { setState(states.assembled); },
      assembleSuccess: function () { setState(states.assembled); },
      debugOn: function () { setState(states.debugging); },
      debugOff: function () { setState(states.postDebugging); },
      toggleMonitor: function (on) {
        node.querySelector('.monitor').style.display = on ? 'block' : 'none';
      },
      showNotes: function () {
        setMessages(node.querySelector('.notes').textContent);
      },
      captureTabInEditor: function (e) {
        if (e.keyCode === 9) {
          e.preventDefault();
          var s = this.selectionStart, en = this.selectionEnd;
          this.value = this.value.substring(0, s) + '\t' + this.value.substring(en);
          this.selectionStart = this.selectionEnd = s + 1;
        }
      }
    };
  }

  /* ---------------- Display: 32x32 canvas, easy6502 16-colour palette ----- */
  function Display() {
    var palette = [
      '#000000', '#ffffff', '#880000', '#aaffee',
      '#cc44cc', '#00cc55', '#0000aa', '#eeee77',
      '#dd8855', '#664400', '#ff7777', '#333333',
      '#777777', '#aaff66', '#0088ff', '#bbbbbb'
    ];
    var ctx, pixelSize;
    var numX = 32, numY = 32;

    function initialize() {
      var canvas = node.querySelector('.screen');
      pixelSize = canvas.width / numX;
      ctx = canvas.getContext('2d');
      reset();
    }

    function reset() {
      ctx.fillStyle = 'black';
      ctx.fillRect(0, 0, node.querySelector('.screen').width,
        node.querySelector('.screen').height);
    }

    function updatePixel(addr) {
      ctx.fillStyle = palette[memory.get(addr) & 0x0f];
      var y = Math.floor((addr - 0x200) / 32);
      var x = (addr - 0x200) % 32;
      ctx.fillRect(x * pixelSize, y * pixelSize, pixelSize, pixelSize);
    }

    function redrawAll() {
      reset();
      for (var a = 0x200; a <= 0x5ff; a++) {
        if (memory.get(a)) updatePixel(a);
      }
    }

    return { initialize: initialize, reset: reset, updatePixel: updatePixel, redrawAll: redrawAll };
  }

  /* ---------------- Memory: 4KB flat, pixel + key hooks ------------------ */
  function Memory() {
    var mem = new Uint8Array(0x1000);

    function set(addr, v) { mem[addr & 0xfff] = v & 0xff; }
    function get(addr) { return mem[addr & 0xfff]; }
    function getWord(addr) {
      return get(addr) | (get(addr + 1) << 8) | (get(addr + 2) << 16) | (get(addr + 3) << 24);
    }
    function setWord(addr, v) {
      v = v >>> 0;
      set(addr, v & 0xff);
      set(addr + 1, (v >> 8) & 0xff);
      set(addr + 2, (v >> 16) & 0xff);
      set(addr + 3, (v >> 24) & 0xff);
    }

    // Poke a byte; painting the screen like easy6502 storeByte.
    function storeByte(addr, v) {
      set(addr, v);
      if (addr >= 0x200 && addr <= 0x5ff) display.updatePixel(addr);
    }

    function storeKeypress(e) {
      if (e.key && e.key.length === 1) {
        cpu.setLastKey(e.key.charCodeAt(0) & 0xff);
      } else if (e.which) {
        cpu.setLastKey(e.which & 0xff);
      }
    }

    function format(start, length) {
      var html = '';
      for (var x = 0; x < length; x++) {
        if ((x & 15) === 0) {
          if (x > 0) html += '\n';
          html += num2hex((start + x >> 8) & 0xff) + num2hex((start + x) & 0xff) + ': ';
        }
        html += num2hex(get(start + x)) + ' ';
      }
      return html;
    }

    return {
      set: set, get: get, getWord: getWord, setWord: setWord,
      storeByte: storeByte, storeKeypress: storeKeypress, format: format
    };
  }

  /* ---------------- CPU: JS port of main-branch vm.c --------------------- */
  function CPU() {
    var MEM_SIZE = 0x1000, PROG_BASE = 0x600;
    var regNames = [
      'zero', 'ra', 'sp', 'gp', 'tp', 't0', 't1', 't2',
      's0', 's1', 'a0', 'a1', 'a2', 'a3', 'a4', 'a5', 'a6', 'a7',
      's2', 's3', 's4', 's5', 's6', 's7', 's8', 's9', 's10', 's11',
      't3', 't4', 't5', 't6'
    ];
    var regs = new Uint32Array(32);
    var prevRegs = null, prevPc = null; // last-step snapshot for change highlight
    var pc = PROG_BASE, progSize = 0, halted = false, lastKey = 0;
    var codeRunning = false, debug = false, monitoring = false;
    var executeId = null;

    function signExtend(val, bits) {
      var m = 1 << (bits - 1);
      return ((val ^ m) - m) | 0;
    }
    function u32(v) { return v >>> 0; }

    function fetch() {
      if (pc + 3 >= MEM_SIZE) return 0;
      return u32(memory.get(pc) | (memory.get(pc + 1) << 8) |
        (memory.get(pc + 2) << 16) | (memory.get(pc + 3) << 24));
    }

    // One step; returns false when the CPU halts. Mirrors cpu_step().
    function step() {
      if (halted) return false;
      regs[0] = 0;
      if (pc + 3 >= MEM_SIZE) { halted = true; return false; }
      var instr = fetch();
      var curPc = pc;
      pc = u32(pc + 4);
      var opcode = instr & 0x7f;
      var rd = (instr >> 7) & 0x1f;
      var funct3 = (instr >> 12) & 0x7;
      var rs1 = (instr >> 15) & 0x1f;
      var rs2 = (instr >> 20) & 0x1f;
      var funct7 = (instr >> 25) & 0x7f;

      switch (opcode) {
        case 0x37: { // LUI
          if (rd !== 0) regs[rd] = u32(instr & 0xfffff000);
          break;
        }
        case 0x17: { // AUIPC
          if (rd !== 0) regs[rd] = u32(curPc + u32(instr & 0xfffff000));
          break;
        }
        case 0x6f: { // JAL
          var imm = (((instr >> 31) & 1) << 20) | (((instr >> 12) & 0xff) << 12) |
            (((instr >> 20) & 1) << 11) | (((instr >> 21) & 0x3ff) << 1);
          imm = signExtend(imm, 21);
          if (rd !== 0) regs[rd] = u32(curPc + 4);
          pc = u32(curPc + imm);
          break;
        }
        case 0x67: { // JALR
          var off = signExtend(instr >>> 20, 12);
          var ret = u32(curPc + 4);
          pc = u32((regs[rs1] + off) & ~1);
          if (rd !== 0) regs[rd] = ret;
          break;
        }
        case 0x63: { // BRANCH
          var bimm = (((instr >> 31) & 1) << 12) | (((instr >> 7) & 1) << 11) |
            (((instr >> 25) & 0x3f) << 5) | (((instr >> 8) & 0xf) << 1);
          bimm = signExtend(bimm, 13);
          var v1 = regs[rs1] | 0, v2 = regs[rs2] | 0;
          var u1 = regs[rs1] >>> 0, u2 = regs[rs2] >>> 0;
          var take = false;
          if (funct3 === 0) take = (u1 === u2);
          else if (funct3 === 1) take = (u1 !== u2);
          else if (funct3 === 4) take = (v1 < v2);
          else if (funct3 === 5) take = (v1 >= v2);
          else if (funct3 === 6) take = (u1 < u2);
          else if (funct3 === 7) take = (u1 >= u2);
          if (take) pc = u32(curPc + bimm);
          break;
        }
        case 0x03: { // LOAD (MMIO 0xFE/0xFF intercepted, never RAM)
          var limm = signExtend(instr >>> 20, 12);
          var addr = u32(regs[rs1] + limm) & 0xfff;
          if (addr === 0xfe || addr === 0x1fe) {
            var r = (Math.random() * 256) & 0xff;
            if (funct3 === 2) regs[rd] = u32(r);
            else if (funct3 === 0) regs[rd] = u32(r << 24 >> 24); // lb sign-extends
            else regs[rd] = u32(r);
            break;
          }
          if (addr === 0xff || addr === 0x1ff) {
            if (funct3 === 2) regs[rd] = u32(lastKey);
            else if (funct3 === 0) regs[rd] = u32(lastKey << 24 >> 24);
            else regs[rd] = u32(lastKey);
            break;
          }
          if (addr >= MEM_SIZE) break;
          if (funct3 === 2) regs[rd] = u32(memory.getWord(addr));
          else if (funct3 === 0) regs[rd] = u32(memory.get(addr) << 24 >> 24);
          else if (funct3 === 1) regs[rd] = u32((memory.get(addr) | (memory.get(addr + 1) << 8)) << 16 >> 16);
          else if (funct3 === 4) regs[rd] = u32(memory.get(addr));
          else if (funct3 === 5) regs[rd] = u32(memory.get(addr) | (memory.get(addr + 1) << 8));
          break;
        }
        case 0x23: { // STORE (0x1000 prints the low byte)
          var simm = signExtend((((instr >> 25) << 5) | ((instr >> 7) & 0x1f)) >>> 0, 12);
          var saddr = u32(regs[rs1] + simm);
          if (saddr === 0x1000) {
            putc(String.fromCharCode(regs[rs2] & 0xff));
            break;
          }
          if (saddr >= MEM_SIZE) break;
          if (funct3 === 2) memory.setWord(saddr, regs[rs2]);
          else if (funct3 === 0) memory.storeByte(saddr, regs[rs2] & 0xff);
          else if (funct3 === 1) {
            memory.storeByte(saddr, regs[rs2] & 0xff);
            memory.set(saddr + 1, (regs[rs2] >> 8) & 0xff);
          }
          break;
        }
        case 0x13: { // OP-IMM
          var iimm = signExtend(instr >>> 20, 12);
          var shamt = (instr >> 20) & 0x1f;
          if (funct3 === 0) { if (rd !== 0) regs[rd] = u32(regs[rs1] + iimm); }
          else if (funct3 === 1) { if (rd !== 0) regs[rd] = u32(regs[rs1] << shamt); }
          else if (funct3 === 5) {
            if (funct7 === 0) { if (rd !== 0) regs[rd] = u32(regs[rs1] >>> shamt); }
            else { if (rd !== 0) regs[rd] = u32((regs[rs1] | 0) >> shamt); }
          }
          else if (funct3 === 2) { if (rd !== 0) regs[rd] = u32(((regs[rs1] | 0) < iimm) ? 1 : 0); }
          else if (funct3 === 3) { if (rd !== 0) regs[rd] = u32((regs[rs1] >>> 0 < iimm >>> 0) ? 1 : 0); }
          else if (funct3 === 4) { if (rd !== 0) regs[rd] = u32(regs[rs1] ^ iimm); }
          else if (funct3 === 6) { if (rd !== 0) regs[rd] = u32(regs[rs1] | iimm); }
          else if (funct3 === 7) { if (rd !== 0) regs[rd] = u32(regs[rs1] & iimm); }
          break;
        }
        case 0x33: { // OP
          if (funct3 === 0) {
            if (funct7 === 0) { if (rd !== 0) regs[rd] = u32(regs[rs1] + regs[rs2]); }
            else { if (rd !== 0) regs[rd] = u32(regs[rs1] - regs[rs2]); }
          }
          else if (funct3 === 1) { if (rd !== 0) regs[rd] = u32(regs[rs1] << (regs[rs2] & 0x1f)); }
          else if (funct3 === 2) { if (rd !== 0) regs[rd] = u32(((regs[rs1] | 0) < (regs[rs2] | 0)) ? 1 : 0); }
          else if (funct3 === 3) { if (rd !== 0) regs[rd] = u32(((regs[rs1] >>> 0) < (regs[rs2] >>> 0)) ? 1 : 0); }
          else if (funct3 === 4) { if (rd !== 0) regs[rd] = u32(regs[rs1] ^ regs[rs2]); }
          else if (funct3 === 5) {
            if (funct7 === 0) { if (rd !== 0) regs[rd] = u32(regs[rs1] >>> (regs[rs2] & 0x1f)); }
            else { if (rd !== 0) regs[rd] = u32((regs[rs1] | 0) >> (regs[rs2] & 0x1f)); }
          }
          else if (funct3 === 6) { if (rd !== 0) regs[rd] = u32(regs[rs1] | regs[rs2]); }
          else if (funct3 === 7) { if (rd !== 0) regs[rd] = u32(regs[rs1] & regs[rs2]); }
          break;
        }
        case 0x73: { // SYSTEM: ecall print-char is a dead path (MMIO-only); sret NOP
          if (instr === 0x00000073 && regs[17] === 1) {
            putc(String.fromCharCode(regs[10] & 0xff));
          }
          break;
        }
        default: break;
      }
      regs[0] = 0;
      if (pc === curPc) { // halt on infinite loop j . (jal x0,0)
        halted = true;
        return false;
      }
      return true;
    }

    // Disassembler with the same pseudo folds as vm.c decode_to_str.
    function decode(instr, addr) {
      var opcode = instr & 0x7f;
      var rd = (instr >> 7) & 0x1f;
      var funct3 = (instr >> 12) & 0x7;
      var rs1 = (instr >> 15) & 0x1f;
      var rs2 = (instr >> 20) & 0x1f;
      var funct7 = (instr >> 25) & 0x7f;
      function hx(n) { return '0x' + (n >>> 0).toString(16); }
      if (instr === 0) return 'unimp';
      if (opcode === 0x37) return 'lui     ' + regNames[rd] + ',' + hx((instr >>> 12) & 0xfffff);
      if (opcode === 0x17) return 'auipc   ' + regNames[rd] + ',' + hx((instr >>> 12) & 0xfffff);
      if (opcode === 0x6f) {
        var jimm = (((instr >> 31) & 1) << 20) | (((instr >> 12) & 0xff) << 12) |
          (((instr >> 20) & 1) << 11) | (((instr >> 21) & 0x3ff) << 1);
        var jtgt = hx(addr + signExtend(jimm, 21));
        if (rd === 0) return 'j       ' + jtgt;
        if (rd === 1) return 'jal     ' + jtgt;
        return 'jal     ' + regNames[rd] + ',' + jtgt;
      }
      if (opcode === 0x67) {
        var rimm = signExtend(instr >>> 20, 12);
        if (rd === 0 && rs1 === 1 && rimm === 0) return 'ret';
        if (rd === 0) {
          if (rimm === 0) return 'jr      ' + regNames[rs1];
          return 'jr      ' + rimm + '(' + regNames[rs1] + ')';
        }
        if (rd === 1) {
          if (rimm === 0) return 'jalr    ' + regNames[rs1];
          return 'jalr    ' + rimm + '(' + regNames[rs1] + ')';
        }
        return 'jalr    ' + regNames[rd] + ',' + rimm + '(' + regNames[rs1] + ')';
      }
      if (opcode === 0x63) {
        var bimm = (((instr >> 31) & 1) << 12) | (((instr >> 7) & 1) << 11) |
          (((instr >> 25) & 0x3f) << 5) | (((instr >> 8) & 0xf) << 1);
        var tgt = hx(addr + signExtend(bimm, 13));
        var mn = funct3 === 0 ? 'beq' : funct3 === 1 ? 'bne' : funct3 === 4 ? 'blt' :
          funct3 === 5 ? 'bge' : funct3 === 6 ? 'bltu' : funct3 === 7 ? 'bgeu' : 'b??';
        if (funct3 === 0 && rs2 === 0) return 'beqz    ' + regNames[rs1] + ',' + tgt;
        if (funct3 === 1 && rs2 === 0) return 'bnez    ' + regNames[rs1] + ',' + tgt;
        if (funct3 === 5 && rs1 === 0) return 'blez    ' + regNames[rs2] + ',' + tgt;
        if (funct3 === 5 && rs2 === 0) return 'bgez    ' + regNames[rs1] + ',' + tgt;
        if (funct3 === 4 && rs2 === 0) return 'bltz    ' + regNames[rs1] + ',' + tgt;
        if (funct3 === 4 && rs1 === 0) return 'bgtz    ' + regNames[rs2] + ',' + tgt;
        return (mn + '     ').slice(0, 8) + regNames[rs1] + ',' + regNames[rs2] + ',' + tgt;
      }
      if (opcode === 0x03) {
        var lmn = funct3 === 0 ? 'lb' : funct3 === 1 ? 'lh' : funct3 === 2 ? 'lw' :
          funct3 === 4 ? 'lbu' : funct3 === 5 ? 'lhu' : 'ld?';
        return (lmn + '       ').slice(0, 8) + regNames[rd] + ',' + signExtend(instr >>> 20, 12) + '(' + regNames[rs1] + ')';
      }
      if (opcode === 0x23) {
        var smn = funct3 === 0 ? 'sb' : funct3 === 1 ? 'sh' : 'sw';
        return (smn + '       ').slice(0, 8) +
          regNames[rs2] + ',' + signExtend((((instr >> 25) << 5) | ((instr >> 7) & 0x1f)) >>> 0, 12) +
          '(' + regNames[rs1] + ')';
      }
      if (opcode === 0x13) {
        var oimm = signExtend(instr >>> 20, 12);
        var sh = (instr >> 20) & 0x1f;
        if (funct3 === 0) {
          if (rd === 0 && rs1 === 0 && oimm === 0) return 'nop';
          if (rs1 === 0) return 'li      ' + regNames[rd] + ',' + oimm;
          if (oimm === 0) return 'mv      ' + regNames[rd] + ',' + regNames[rs1];
          return 'addi    ' + regNames[rd] + ',' + regNames[rs1] + ',' + oimm;
        }
        if (funct3 === 1 && funct7 === 0) return 'slli    ' + regNames[rd] + ',' + regNames[rs1] + ',' + sh;
        if (funct3 === 5) {
          if (funct7 === 0) return 'srli    ' + regNames[rd] + ',' + regNames[rs1] + ',' + sh;
          if (funct7 === 0x20) return 'srai    ' + regNames[rd] + ',' + regNames[rs1] + ',' + sh;
          return 'op-imm  ' + hx(instr);
        }
        var imn = funct3 === 2 ? 'slti' : funct3 === 3 ? 'sltiu' : funct3 === 4 ? 'xori' :
          funct3 === 6 ? 'ori' : funct3 === 7 ? 'andi' : 'op-imm';
        if (funct3 === 4 && oimm === -1) return 'not     ' + regNames[rd] + ',' + regNames[rs1];
        if (funct3 === 3 && oimm === 1) return 'seqz    ' + regNames[rd] + ',' + regNames[rs1];
        return (imn + '       ').slice(0, 8) + regNames[rd] + ',' + regNames[rs1] + ',' + oimm;
      }
      if (opcode === 0x33) {
        var rmn = funct3 === 0 && funct7 === 0 ? 'add' : funct3 === 0 ? 'sub' :
          funct3 === 1 ? 'sll' : funct3 === 2 ? 'slt' : funct3 === 3 ? 'sltu' :
          funct3 === 4 ? 'xor' : funct3 === 5 && funct7 === 0 ? 'srl' :
          funct3 === 5 ? 'sra' : funct3 === 6 ? 'or' : funct3 === 7 ? 'and' : 'op';
        if (funct3 === 0 && funct7 === 0x20 && rs1 === 0) return 'neg     ' + regNames[rd] + ',' + regNames[rs2];
        if (funct3 === 3 && funct7 === 0 && rs1 === 0) return 'snez    ' + regNames[rd] + ',' + regNames[rs2];
        if (funct3 === 2 && funct7 === 0 && rs2 === 0) return 'sltz    ' + regNames[rd] + ',' + regNames[rs1];
        if (funct3 === 2 && funct7 === 0 && rs1 === 0) return 'sgtz    ' + regNames[rd] + ',' + regNames[rs2];
        return (rmn + '       ').slice(0, 8) + regNames[rd] + ',' + regNames[rs1] + ',' + regNames[rs2];
      }
      if (opcode === 0x73) {
        if (instr === 0x00000073) return 'ecall';
        if (instr === 0x10200073) return 'sret';
        return 'system  ' + hx(instr);
      }
      return 'unknown ' + hx(instr);
    }

    /* -- run/debug controls (ports easy6502 Simulator controls) ---------- */
    function execute(debugging) {
      if (!codeRunning && !debugging) return;
      step();
      if (halted || (!codeRunning && !debugging)) {
        stop();
        message('\nHalted at PC=0x' + pc.toString(16));
        ui.stop();
      }
    }

    function multiExecute() {
      if (!debug) {
        for (var w = 0; w < 97; w++) { // prime count, like the reference
          if (halted) break;
          step();
        }
        if (halted) {
          stop();
          message('\nHalted at PC=0x' + pc.toString(16));
          ui.stop();
        }
      }
      updateDebugInfo();
    }

    function runBinary() {
      if (codeRunning) {
        stop();
        ui.stop();
      } else {
        ui.play();
        codeRunning = true;
        halted = false;
        executeId = setInterval(multiExecute, 15);
      }
    }

    function stop() {
      codeRunning = false;
      if (executeId !== null) { clearInterval(executeId); executeId = null; }
    }

    function reset() {
      stop();
      display.reset();
      for (var i = 0; i < 0x600; i++) memory.set(i, 0); // clear RAM, stack, screen
      for (var r = 0; r < 32; r++) regs[r] = 0;
      pc = PROG_BASE;
      halted = false;
      lastKey = 0;
      display.redrawAll();
      updateDebugInfo();
    }

    function debugExec() {
      execute(true);
      updateDebugInfo();
    }

    function gotoAddr() {
      var inp = prompt('Enter address or label', '');
      if (inp === null) return;
      inp = inp.trim();
      var addr = -1;
      if (assembler.findLabel(inp)) addr = assembler.labelAddr(inp);
      else if (/^0x[0-9a-f]{1,4}$/i.test(inp)) addr = parseInt(inp.slice(2), 16);
      else if (/^\$[0-9a-f]{1,4}$/i.test(inp)) addr = parseInt(inp.slice(1), 16);
      else if (/^[0-9]{1,5}$/.test(inp)) addr = parseInt(inp, 10);
      if (addr < 0 || addr >= MEM_SIZE) message('Unable to find/parse given address/label');
      else { pc = addr; halted = false; }
      updateDebugInfo();
    }

    function updateDebugInfo() {
      function h(n) {
        n = n >>> 0;
        return ('00000000' + n.toString(16)).slice(-8);
      }
      // Full register file, not the reference's 4-register box:
      // RV32I has 32 registers and the book watches all of them
      // (02-fib traces t0/t1/t2/a0, 07-jumping watches ra/sp).
      // Changed-since-last-step values render bold so the eye catches
      // what the last instruction wrote.
      var html = '<table class="regfile">';
      var shown = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31];
      for (var i = 0; i < shown.length; i += 2) {
        html += '<tr>';
        for (var c = 0; c < 2; c++) {
          var r = shown[i + c];
          var changed = prevRegs && ((regs[r] ^ prevRegs[r]) !== 0);
          html += '<td class="rn">' + regNames[r] + '</td>' +
            '<td class="rv' + (changed ? ' changed' : '') + '">' + h(regs[r]) + '</td>';
        }
        html += '</tr>';
      }
      html += '</table>';
      var pcChanged = (typeof prevPc === 'number') && (pc !== prevPc);
      html += '<div class="rpc' + (pcChanged ? ' changed' : '') + '">pc=0x' + h(pc) +
        ' ' + (halted ? 'HALT' : 'RUN') + '</div>';
      node.querySelector('.minidebugger').innerHTML = html;
      prevRegs = Array.prototype.slice.call(regs);
      prevPc = pc;
      if (monitoring) {
        var start = parseInt(node.querySelector('.start').value, 16);
        var len = parseInt(node.querySelector('.length').value, 16);
        if (!isNaN(start) && !isNaN(len) && start >= 0 && start + len <= 0x1000) {
          setMonitor(memory.format(start, len));
        } else {
          setMonitor('Cannot monitor this range. Valid ranges are between $0000 and $0fff, inclusive.');
        }
      }
    }

    function handleMonitorRangeChange() {
      var s = node.querySelector('.start'), l = node.querySelector('.length');
      var start = parseInt(s.value, 16), len = parseInt(l.value, 16);
      s.classList.remove('monitor-invalid');
      l.classList.remove('monitor-invalid');
      if (isNaN(start) || start < 0 || start > 0xfff) s.classList.add('monitor-invalid');
      else if (isNaN(len) || start + len > 0x1000) l.classList.add('monitor-invalid');
    }

    return {
      step: step, decode: decode,
      runBinary: runBinary, stop: stop, reset: reset,
      debugExec: debugExec, gotoAddr: gotoAddr,
      enableDebugger: function () { debug = true; if (codeRunning) updateDebugInfo(); },
      stopDebugger: function () { debug = false; },
      toggleMonitor: function (on) { monitoring = on; },
      handleMonitorRangeChange: handleMonitorRangeChange,
      setLastKey: function (v) { lastKey = v & 0xff; },
      getPC: function () { return pc; },
      setPC: function (a) { pc = a >>> 0; halted = false; updateDebugInfo(); },
      isHalted: function () { return halted; },
      setProgSize: function (n) { progSize = n; },
      getProgSize: function () { return progSize; }
    };
  }

  /* ---------------- Assembler: two passes, RV32I subset ------------------ */
  function Assembler() {
    var PROG_BASE = 0x600, MEM_MAX = 0x1000;
    var defaultPC = PROG_BASE, codeLen = 0, ok = true;
    var explained = false; // set when assembleLine already reported the error
    var labelMap = {};
    var labelOrder = [];

    var regIds = {
      zero: 0, ra: 1, sp: 2, gp: 3, tp: 4, t0: 5, t1: 6, t2: 7,
      s0: 8, fp: 8, s1: 9, a0: 10, a1: 11, a2: 12, a3: 13, a4: 14,
      a5: 15, a6: 16, a7: 17, s2: 18, s3: 19, s4: 20, s5: 21,
      s6: 22, s7: 23, s8: 24, s9: 25, s10: 26, s11: 27,
      t3: 28, t4: 29, t5: 30, t6: 31
    };

    function reg(s) {
      s = s.trim();
      if (/^x([0-9]|[12][0-9]|3[01])$/.test(s)) return parseInt(s.slice(1), 10);
      if (regIds.hasOwnProperty(s)) return regIds[s];
      return -1;
    }

    function num(s, symbols) {
      // decimal, 0x hex, $hex, %bin, symbol, SYM+N, SYM-N
      s = s.trim();
      var m = s.match(/^([A-Za-z_.][A-Za-z0-9_.]*)([+-])(\d+|0x[0-9a-fA-F]+|\$[0-9a-fA-F]+|%[01]+)$/);
      if (m && symbols.hasOwnProperty('K' + m[1])) {
        var dl = num(m[3], symbols);
        if (dl === null) return null;
        return m[2] === '+' ? symbols['K' + m[1]] + dl : symbols['K' + m[1]] - dl;
      }
      if (/^-?\d+$/.test(s)) return parseInt(s, 10);
      if (/^-?0x[0-9a-fA-F]+$/.test(s)) return parseInt(s, 16);
      if (/^-?\$[0-9a-fA-F]+$/.test(s)) {
        var neg = s[0] === '-';
        var v = parseInt(s.replace(/^-?\$/, ''), 16);
        return neg ? -v : v;
      }
      if (/^-?%[01]+$/.test(s)) {
        var neg2 = s[0] === '-';
        var v2 = parseInt(s.replace(/^-?%/, ''), 2);
        return neg2 ? -v2 : v2;
      }
      if (/^[A-Za-z_.][A-Za-z0-9_.]*$/.test(s) && symbols.hasOwnProperty('K' + s)) {
        return symbols['K' + s];
      }
      return null;
    }

    function sanitize(line) {
      var cut = line.replace(/\/\/.*$/, ''); // // comments (legacy)
      cut = cut.replace(/#.*$/, '');           // # comments (GNU/clang-common)
      cut = cut.replace(/;.*$/, '');         // ; comments
      return cut.replace(/^\s+/, '').replace(/\s+$/, '');
    }

    function preprocess(lines) {
      var symbols = {};
      for (var i = 0; i < lines.length; i++) {
        lines[i] = sanitize(lines[i]);
        var m = lines[i].match(/^define\s+([A-Za-z_]\w*)\s+(.+)$/) ||
          lines[i].match(/^\.equ\s+([A-Za-z_]\w*)\s*,?\s*(.+)$/);
        if (m) {
          var v = num(m[2].trim(), symbols);
          if (v === null) {
            message('**Cannot parse value in line ' + (i + 1) + ': ' + lines[i] + '**');
            ok = false;
            return null;
          }
          symbols['K' + m[1]] = v;
          lines[i] = '';
        }
      }
      return symbols;
    }

    function splitLabel(line) {
      var m = line.match(/^([A-Za-z_.][A-Za-z0-9_.]*):(.*)$/);
      if (m) return { label: m[1], rest: m[2].replace(/^\s+/, '').replace(/\s+$/, '') };
      return { label: null, rest: line };
    }

    // Size of one line in bytes (li may take 8, la/call/tail may take 8).
    // -1 = error. addr is the pass-1 cursor (call/tail range sizing).
    function lineSize(rest, addr, symbols, lineno) {
      if (rest === '') return 0;
      if (/^\.(section|globl|text|option)\b/.test(rest)) return 0;
      var dm = rest.match(/^\.(byte|word)\s+(.+)$/);
      if (dm) {
        var parts = dm[2].split(',');
        if (!parts.length) return -1;
        return dm[1] === 'byte' ? parts.length : parts.length * 4;
      }
      var dsp = rest.match(/^\.space\s+(.+)$/);
      if (dsp) {
        var sp2 = dsp[1].split(',');
        var sn = num(sp2[0], symbols);
        if (sn === null || sn < 0) return -1;
        if (sp2.length > 1 && num(sp2[1], symbols) === null) return -1;
        return sn;
      }
      if (/^\./.test(rest)) return -1;
      var sp = rest.match(/^(\w+)\s*(.*)$/);
      if (!sp) return -1;
      var op = sp[1];
      if (op === 'li') {
        var args = sp[2].split(',');
        if (args.length !== 2) return -1;
        var v = num(args[1], symbols);
        if (v === null) {
          message('**li with a label address needs la (PC-relative), not li at line ' +
            (lineno + 1) + '**');
          explained = true;
          return -1;
        }
        return liSplit(v).n * 4;
      }
      if (op === 'la') {
        // auipc+addi (8) for labels, li split for equ/numbers.
        // Forward labels are not indexed yet: unknown => 8 (worst case).
        var largs = sp[2].split(',');
        if (largs.length !== 2) return -1;
        var lt = largs[1].trim();
        if (/^[A-Za-z_.][A-Za-z0-9_.]*$/.test(lt) && lt !== '.') {
          if (labelMap.hasOwnProperty('K' + lt)) return 8;
          var lev = num(lt, symbols);
          if (lev === null) return 8; // forward label (pass 2 resolves)
          return liSplit(lev).n * 4;
        }
        if (num(lt, symbols) !== null) return liSplit(num(lt, symbols)).n * 4;
        return 8; // label expression (SYM+N): auipc+addi
      }
      if (op === 'call' || op === 'tail') {
        // jal/j (4) when near, auipc+jalr (8) when far.
        // Forward labels: 8 (pass 1b shrinks to 4 at fixpoint).
        var cargs = sp[2].split(',');
        if (cargs.length !== 1 || cargs[0].trim() === '') return -1;
        var ct = pctarget(cargs[0], addr, symbols);
        if (ct === null) return 8;
        var cd = ct - addr;
        if (cd >= -1048576 && cd <= 1048574 && (cd & 1) === 0) return 4;
        return 8;
      }
      return 4;
    }

    function encR(f7, rs2, rs1, f3, rd, op) {
      return (((f7 << 25) | (rs2 << 20) | (rs1 << 15) | (f3 << 12) | (rd << 7) | op) >>> 0);
    }
    function encI(imm, rs1, f3, rd, op) {
      return ((((imm & 0xfff) << 20) | (rs1 << 15) | (f3 << 12) | (rd << 7) | op) >>> 0);
    }
    function encS(imm, rs1, rs2, f3) {
      var u = imm & 0xfff;
      return (((((u >> 5) & 0x7f) << 25) | (rs2 << 20) | (rs1 << 15) | (f3 << 12) | ((u & 0x1f) << 7) | 0x23) >>> 0);
    }
    function encB(imm, rs1, rs2, f3) {
      var u = imm & 0x1fff;
      return (((((u >> 12) & 1) << 31) | (((u >> 5) & 0x3f) << 25) | (rs2 << 20) |
        (rs1 << 15) | (f3 << 12) | (((u >> 1) & 0xf) << 8) | (((u >> 11) & 1) << 7) | 0x63) >>> 0);
    }
    function encU(hi20, rd, op) {
      return ((((hi20 & 0xfffff) << 12) | (rd << 7) | op) >>> 0);
    }
    function encJ(imm, rd) {
      var u = imm & 0x1fffff;
      return (((((u >> 20) & 1) << 31) | (((u >> 1) & 0x3ff) << 21) |
        (((u >> 11) & 1) << 20) | (((u >> 12) & 0xff) << 12) | (rd << 7) | 0x6f) >>> 0);
    }

    var OP = {
      add: [0, 0x00], sub: [0, 0x20], sll: [1, 0], slt: [2, 0], sltu: [3, 0],
      xor: [4, 0], srl: [5, 0x00], sra: [5, 0x20], or: [6, 0], and: [7, 0]
    };
    var OPI = { addi: 0, slli: 1, slti: 2, sltiu: 3, xori: 4, srli: 5, srai: 5, ori: 6, andi: 7 };
    var LOADF = { lb: 0, lh: 1, lw: 2, lbu: 4, lhu: 5 };
    var STOREF = { sb: 0, sh: 1, sw: 2 };
    var BRF = { beq: 0, bne: 1, blt: 4, bge: 5, bltu: 6, bgeu: 7 };

    function emit(w) {
      memory.setWord(defaultPC, w);
      defaultPC += 4;
      codeLen += 4;
    }

    function fit12(v) { return v >= -2048 && v <= 2047; }

    // Split for li: clang emits 1x addi (12-bit), 1x lui (lo==0), else lui+addi.
    function liSplit(v) {
      if (fit12(v)) return { n: 1, addiOnly: true };
      var vs = v | 0;
      var full = Math.floor((vs + 0x800) / 4096);
      var hi = full & 0xfffff;
      var lo = vs - full * 4096; // always in [-2048, 2047] by construction
      if (lo === 0) return { n: 1, addiOnly: false, hi: hi };
      return { n: 2, addiOnly: false, hi: hi, lo: lo };
    }

    // Split for auipc+addi (la/call tail-far): hi20 = floor((d+0x800)/4096),
    // lo12 = d - hi20*4096 (always in [-2048,2047] by construction).
    function pcrelSplit(tgt, pc) {
      var d = (tgt - pc) | 0;
      var full = Math.floor((d + 0x800) / 4096);
      return { hi: full & 0xfffff, lo: d - full * 4096 };
    }

    function target(s, addr, symbols) {
      s = s.trim();
      if (s === '.') return addr;
      if (/^[A-Za-z_.][A-Za-z0-9_.]*$/.test(s)) {
        if (labelMap.hasOwnProperty('K' + s)) return labelMap['K' + s];
        return null;
      }
      return num(s, symbols);
    }

    // Resolve la/call/tail target: bare label first (labels shadow equ,
    // like clang), then label SYM+N, then target() (equ/number/'.').
    function pctarget(s, addr, symbols) {
      s = s.trim();
      if (/^[A-Za-z_.][A-Za-z0-9_.]*$/.test(s) && s !== '.') {
        if (labelMap.hasOwnProperty('K' + s)) return labelMap['K' + s];
      } else {
        var lm = s.match(/^([A-Za-z_.][A-Za-z0-9_.]*)([+-])(.+)$/);
        if (lm && labelMap.hasOwnProperty('K' + lm[1])) {
          var dl = num(lm[3], symbols);
          if (dl !== null) return lm[2] === '+' ? labelMap['K' + lm[1]] + dl : labelMap['K' + lm[1]] - dl;
        }
      }
      return target(s, addr, symbols);
    }

    // Assemble one logical line at addr. Returns true on success.
    function assembleLine(rest, addr, symbols, lineno) {
      if (rest === '') return true;
      if (/^\.(section|globl|text|option)\b/.test(rest)) return true;
      var dsp = rest.match(/^\.space\s+(.+)$/);
      if (dsp) {
        var sargs = dsp[1].split(',');
        var sn = num(sargs[0], symbols);
        var sfill = sargs.length > 1 ? num(sargs[1], symbols) : 0;
        if (sn === null || sn < 0 || sfill === null) return false;
        for (var sk = 0; sk < sn; sk++) {
          memory.set(defaultPC, sfill);
          defaultPC += 1;
          codeLen += 1;
        }
        return true;
      }
      var dm = rest.match(/^\.(byte|word)\s+(.+)$/);
      if (dm) {
        var parts = dm[2].split(',');
        for (var i = 0; i < parts.length; i++) {
          var v = num(parts[i], symbols);
          if (v === null) return false;
          if (dm[1] === 'byte') {
            memory.set(defaultPC, v);
            defaultPC += 1;
            codeLen += 1;
          } else {
            memory.setWord(defaultPC, v);
            defaultPC += 4;
            codeLen += 4;
          }
        }
        return true;
      }
      if (/^\./.test(rest)) return false;
      var sp = rest.match(/^(\w+)\s*(.*)$/);
      if (!sp) return false;
      var op = sp[1];
      var argv = sp[2] === '' ? [] : sp[2].split(',').map(function (a) { return a.trim(); });

      function need(n) { return argv.length === n; }
      function R2() { return reg(argv[0]); }
      function R1() { return reg(argv[1]); }
      function R3() { return reg(argv[2]); }

      // li / mv / nop
      if (op === 'li') {
        if (!need(2)) return false;
        var d = R2();
        var v = num(argv[1], symbols);
        if (d < 0 || v === null) return false;
        var s = liSplit(v);
        if (s.addiOnly) {
          emit(encI(v, 0, 0, d, 0x13));
        } else if (s.n === 1) {
          emit(encU(s.hi, d, 0x37)); // lo == 0: lui alone, like clang
        } else {
          emit(encU(s.hi, d, 0x37));
          emit(encI(s.lo, d, 0, d, 0x13));
        }
        return true;
      }
      if (op === 'mv') {
        if (!need(2) || R2() < 0 || R1() < 0) return false;
        emit(encI(0, R1(), 0, R2(), 0x13));
        return true;
      }
      if (op === 'nop') {
        if (argv.length > 0) return false;
        emit(encI(0, 0, 0, 0, 0x13));
        return true;
      }
      if (op === 'not') {
        if (!need(2) || R2() < 0 || R1() < 0) return false;
        emit(encI(-1, R1(), 4, R2(), 0x13));
        return true;
      }
      if (op === 'neg') {
        if (!need(2) || R2() < 0 || R1() < 0) return false;
        emit(encR(0x20, R1(), 0, 0, R2(), 0x33));
        return true;
      }
      if (op === 'seqz' || op === 'snez' || op === 'sltz' || op === 'sgtz') {
        if (!need(2) || R2() < 0 || R1() < 0) return false;
        if (op === 'seqz') emit(encI(1, R1(), 3, R2(), 0x13));
        else if (op === 'snez') emit(encR(0, 0, R1(), 3, R2(), 0x33));
        else if (op === 'sltz') emit(encR(0, 0, R1(), 2, R2(), 0x33));
        else emit(encR(0, R1(), 0, 2, R2(), 0x33));
        return true;
      }
      // jumps
      if (op === 'j') {
        if (!need(1)) return false;
        var jt = target(argv[0], addr, symbols);
        if (jt === null || jt - addr < -1048576 || jt - addr > 1048574) return false;
        emit(encJ(jt - addr, 0));
        return true;
      }
      if (op === 'jal') {
        if (argv.length === 1) {
          var t1 = target(argv[0], addr, symbols);
          if (t1 === null || t1 - addr < -1048576 || t1 - addr > 1048574) return false;
          emit(encJ(t1 - addr, 1));
          return true;
        }
        if (!need(2) || R2() < 0) return false;
        var t2 = target(argv[1], addr, symbols);
        if (t2 === null || t2 - addr < -1048576 || t2 - addr > 1048574) return false;
        emit(encJ(t2 - addr, R2()));
        return true;
      }
      if (op === 'jr') {
        if (!need(1)) return false;
        var m = argv[0].match(/^(-?[0-9a-zA-Z_.$%+-]+)\(([A-Za-z][A-Za-z0-9]*)\)$/);
        if (m) {
          var off = num(m[1], symbols);
          var b = reg(m[2]);
          if (off === null || b < 0 || !fit12(off)) return false;
          emit(encI(off, b, 0, 0, 0x67));
        } else {
          var b2 = reg(argv[0]);
          if (b2 < 0) return false;
          emit(encI(0, b2, 0, 0, 0x67));
        }
        return true;
      }
      if (op === 'jalr') {
        if (argv.length === 1) {
          var b3 = reg(argv[0]);
          if (b3 < 0) return false;
          emit(encI(0, b3, 0, 1, 0x67));
          return true;
        }
        if (argv.length === 2) {
          // jalr rd, off(base)
          var m2 = argv[1].match(/^(-?[0-9a-zA-Z_.$%+-]+)\(([A-Za-z][A-Za-z0-9]*)\)$/);
          var d2 = reg(argv[0]);
          if (!m2 || d2 < 0) return false;
          var o2 = num(m2[1], symbols);
          var b4 = reg(m2[2]);
          if (o2 === null || b4 < 0 || !fit12(o2)) return false;
          emit(encI(o2, b4, 0, d2, 0x67));
          return true;
        }
        if (!need(3)) return false;
        if (R2() < 0 || R1() < 0) return false;
        var o3 = num(argv[2], symbols);
        if (o3 === null || !fit12(o3)) return false;
        emit(encI(o3, R1(), 0, R2(), 0x67));
        return true;
      }
      if (op === 'ret') {
        if (argv.length > 0) return false;
        emit(encI(0, 1, 0, 0, 0x67));
        return true;
      }
      if (op === 'la') {
        // clang-identical: la rd, number/equ => li split;
        // la rd, label-expr => auipc+addi (PC-relative).
        if (!need(2)) return false;
        var lad = R2();
        if (lad < 0) return false;
        var lat = argv[1].trim();
        if (/^[A-Za-z_.][A-Za-z0-9_.]*$/.test(lat) && lat !== '.') {
          if (labelMap.hasOwnProperty('K' + lat)) {
            var lps = pcrelSplit(labelMap['K' + lat], addr);
            emit(encU(lps.hi, lad, 0x17));
            emit(encI(lps.lo, lad, 0, lad, 0x13));
            return true;
          }
          var laev = num(lat, symbols);
          if (laev === null) return false;
          var laes = liSplit(laev);
          if (laes.addiOnly) emit(encI(laev, 0, 0, lad, 0x13));
          else if (laes.n === 1) emit(encU(laes.hi, lad, 0x37));
          else { emit(encU(laes.hi, lad, 0x37)); emit(encI(laes.lo, lad, 0, lad, 0x13)); }
          return true;
        }
        var lan = num(lat, symbols);
        if (lan !== null) {
          var lans = liSplit(lan);
          if (lans.addiOnly) emit(encI(lan, 0, 0, lad, 0x13));
          else if (lans.n === 1) emit(encU(lans.hi, lad, 0x37));
          else { emit(encU(lans.hi, lad, 0x37)); emit(encI(lans.lo, lad, 0, lad, 0x13)); }
          return true;
        }
        var lal = pctarget(lat, addr, symbols);
        if (lal === null) return false;
        var lals = pcrelSplit(lal, addr);
        emit(encU(lals.hi, lad, 0x17));
        emit(encI(lals.lo, lad, 0, lad, 0x13));
        return true;
      }
      if (op === 'call') {
        // clang-identical: jal if in range, else auipc+jalr (ra).
        if (!need(1)) return false;
        var calt = pctarget(argv[0], addr, symbols);
        if (calt === null) return false;
        var cald = calt - addr;
        if (cald >= -1048576 && cald <= 1048574 && (cald & 1) === 0) {
          emit(encJ(cald, 1));
        } else {
          var cas = pcrelSplit(calt, addr);
          emit(encU(cas.hi, 1, 0x17));
          emit(encI(cas.lo, 1, 0, 1, 0x67));
        }
        return true;
      }
      if (op === 'tail') {
        // clang-identical: j if in range, else auipc+jalr via t1, no link.
        if (!need(1)) return false;
        var talt = pctarget(argv[0], addr, symbols);
        if (talt === null) return false;
        var tald = talt - addr;
        if (tald >= -1048576 && tald <= 1048574 && (tald & 1) === 0) {
          emit(encJ(tald, 0));
        } else {
          var tas = pcrelSplit(talt, addr);
          emit(encU(tas.hi, 6, 0x17));
          emit(encI(tas.lo, 6, 0, 0, 0x67));
        }
        return true;
      }
      // Branches (range-checked): canonical + beqz/bnez/blez/bgez/bltz/bgtz + bgt/ble/bgtu/bleu.
      // B-type: signed 13-bit, LSB must be zero (even offsets, -4096..+4094).
      function branchRange(addr, tgt) {
        if (tgt === null) return false;
        var d = tgt - addr;
        return d >= -4096 && d <= 4094 && (d & 1) === 0;
      }
      var bmap = { beqz: ['beq', 1], bnez: ['bne', 1], blez: ['bge', 0], bgez: ['bge', 1],
        bltz: ['blt', 1], bgtz: ['blt', 0] };
      if (bmap.hasOwnProperty(op)) {
        if (!need(2)) return false;
        var brs = reg(argv[0]);
        var bt = target(argv[1], addr, symbols);
        if (brs < 0 || !branchRange(addr, bt)) return false;
        var base = bmap[op];
        var s1 = base[1] ? brs : 0, s2 = base[1] ? 0 : brs;
        emit(encB(bt - addr, s1, s2, BRF[base[0]]));
        return true;
      }
      if (op === 'bgt' || op === 'ble' || op === 'bgtu' || op === 'bleu') {
        if (!need(3)) return false;
        var a1 = reg(argv[0]), a2 = reg(argv[1]);
        var at = target(argv[2], addr, symbols);
        if (a1 < 0 || a2 < 0 || !branchRange(addr, at)) return false;
        var canon = op === 'bgt' ? 'blt' : op === 'ble' ? 'bge' : op === 'bgtu' ? 'bltu' : 'bgeu';
        emit(encB(at - addr, a2, a1, BRF[canon]));
        return true;
      }
      if (BRF.hasOwnProperty(op)) {
        if (!need(3)) return false;
        var c1 = reg(argv[0]), c2 = reg(argv[1]);
        var ct = target(argv[2], addr, symbols);
        if (c1 < 0 || c2 < 0 || !branchRange(addr, ct)) return false;
        emit(encB(ct - addr, c1, c2, BRF[op]));
        return true;
      }
      // loads / stores
      if (LOADF.hasOwnProperty(op) || STOREF.hasOwnProperty(op)) {
        if (!need(2)) return false;
        var dd = reg(argv[0]);
        if (dd < 0) return false;
        var mo = argv[1].match(/^(-?[0-9a-zA-Z_.$%+-]*)\(([A-Za-z][A-Za-z0-9]*)\)$/);
        if (!mo) return false;
        var moff = mo[1] === '' ? 0 : num(mo[1], symbols);
        var mbase = reg(mo[2]);
        if (moff === null || mbase < 0 || !fit12(moff)) return false;
        if (LOADF.hasOwnProperty(op)) emit(encI(moff, mbase, LOADF[op], dd, 0x03));
        else emit(encS(moff, mbase, dd, STOREF[op]));
        return true;
      }
      // OP-IMM
      if (OPI.hasOwnProperty(op)) {
        if (!need(3)) return false;
        var id = reg(argv[0]), is1 = reg(argv[1]);
        var iv = num(argv[2], symbols);
        if (id < 0 || is1 < 0 || iv === null) return false;
        if (op === 'slli' || op === 'srli' || op === 'srai') {
          if (iv < 0 || iv > 31) return false;
          var f7 = op === 'srai' ? 0x20 : 0;
          emit(encI((f7 << 5) | iv, is1, OPI[op], id, 0x13));
        } else {
          if (!fit12(iv)) return false;
          emit(encI(iv, is1, OPI[op], id, 0x13));
        }
        return true;
      }
      // OP
      if (OP.hasOwnProperty(op)) {
        if (!need(3)) return false;
        var od = reg(argv[0]), o1 = reg(argv[1]), o2 = reg(argv[2]);
        if (od < 0 || o1 < 0 || o2 < 0) return false;
        emit(encR(OP[op][1], o2, o1, OP[op][0], od, 0x33));
        return true;
      }
      if (op === 'lui' || op === 'auipc') {
        if (!need(2)) return false;
        var ud = reg(argv[0]);
        var uv = num(argv[1], symbols);
        if (ud < 0 || uv === null || uv < 0 || uv > 0xfffff) return false;
        emit(encU(uv, ud, op === 'lui' ? 0x37 : 0x17));
        return true;
      }
      if (op === 'ecall' || op === 'ebreak') {
        if (argv.length > 0) return false;
        emit(op === 'ecall' ? 0x00000073 : 0x00100073);
        return true;
      }
      return false;
    }

    function assembleCode() {
      cpu.reset();
      defaultPC = PROG_BASE;
      labelMap = {};
      labelOrder = [];
      codeLen = 0;
      ok = true;
      setMessages('');
      var lines = (node.querySelector('.code').value + '\n\n').split('\n');

      message('Preprocessing ...');
      var symbols = preprocess(lines);
      if (!ok || symbols === null) { ui.initialize(); return false; }

      message('Indexing labels ...');
      var addr = PROG_BASE;
      var labelLine = {}; // label -> line index (for the shrink re-flow)
      for (var i = 0; i < lines.length; i++) {
        var parts = splitLabel(lines[i]);
        if (parts.label) {
          if (labelMap.hasOwnProperty('K' + parts.label)) {
            message('**Label already defined at line ' + (i + 1) + ':** ' + parts.label);
            ui.initialize();
            return false;
          }
          labelMap['K' + parts.label] = addr;
          labelOrder.push(parts.label);
          labelLine[parts.label] = i;
          lines[i] = parts.rest;
        }
        var sz = lineSize(lines[i], addr, symbols, i);
        if (sz < 0) {
          message('**Syntax error line ' + (i + 1) + ': ' + lines[i] + '**');
          ui.initialize();
          return false;
        }
        addr += sz;
        if (addr > MEM_MAX) {
          message('**Code too large for the 4KB model at line ' + (i + 1) + '**');
          ui.initialize();
          return false;
        }
      }
      message('Found ' + labelOrder.length + ' label' + (labelOrder.length === 1 ? '' : 's') + '.');

      // Shrink pass: call/tail to a forward label sized 8 above; now that
      // all labels are known, shrink near ones to 4 and re-flow to fixpoint.
      // Shrinking only moves later lines down, which keeps near-calls near
      // (monotone: terminates). la/jal/branches never change size.
      var sizes = [];
      for (var si = 0; si < lines.length; si++) sizes.push(0);
      for (;;) {
        var lineaddr = [];
        var a2 = PROG_BASE;
        var bad = false;
        for (var k = 0; k < lines.length; k++) {
          var sk = lineSize(lines[k], a2, symbols, k);
          if (sk < 0) { bad = true; break; }
          sizes[k] = sk;
          lineaddr.push(a2);
          a2 += sk;
          if (a2 > MEM_MAX) {
            message('**Code too large for the 4KB model at line ' + (k + 1) + '**');
            ui.initialize();
            return false;
          }
        }
        if (bad) {
          // lineSize already messaged (li/la errors set explained)
          message('**Syntax error during shrink pass**');
          ui.initialize();
          return false;
        }
        // refresh label addresses under current sizes
        for (var li2 = 0; li2 < labelOrder.length; li2++) {
          labelMap['K' + labelOrder[li2]] = lineaddr[labelLine[labelOrder[li2]]];
        }
        // shrink call/tail lines that are now near
        var changed = false;
        for (var n = 0; n < lines.length; n++) {
          if (sizes[n] !== 8) continue;
          var opm = lines[n].match(/^(\w+)/);
          if (!opm || (opm[1] !== 'call' && opm[1] !== 'tail')) continue;
          var carg = lines[n].match(/^\w+\s*(.*)$/);
          var ct2 = carg ? pctarget(carg[1], lineaddr[n], symbols) : null;
          if (ct2 === null) continue; // still unknown: keep 8
          var dd = ct2 - lineaddr[n];
          if (dd >= -1048576 && dd <= 1048574 && (dd & 1) === 0) {
            sizes[n] = 4;
            changed = true;
          }
        }
        if (!changed) break;
      }

      message('Assembling code ...');
      defaultPC = PROG_BASE;
      codeLen = 0;
      addr = PROG_BASE;
      // recompute line addresses from final sizes (do NOT re-run lineSize:
      // a forward call sizes 8 in isolation but 4 after the shrink pass).
      var finaddr = [];
      var fa = PROG_BASE;
      for (var q = 0; q < lines.length; q++) { finaddr.push(fa); fa += sizes[q]; }
      for (var lq = 0; lq < labelOrder.length; lq++) {
        labelMap['K' + labelOrder[lq]] = finaddr[labelLine[labelOrder[lq]]];
      }
      for (var j = 0; j < lines.length; j++) {
        var sz2 = sizes[j];
        addr = finaddr[j];
        if (sz2 > 0 && (addr & 3)) {
          message('**Misaligned instruction at line ' + (j + 1) + ': ' + lines[j] + '**');
          ui.initialize();
          return false;
        }
        defaultPC = addr;
        if (!assembleLine(lines[j], addr, symbols, j)) {
          if (!explained) message('**Syntax error line ' + (j + 1) + ': ' + lines[j] + '**');
          explained = false;
          ui.initialize();
          return false;
        }
        addr += sz2;
      }
      if (codeLen === 0) {
        message('No code to run.');
        ui.initialize();
        return false;
      }
      cpu.setProgSize(codeLen);
      cpu.setPC(PROG_BASE);
      ui.assembleSuccess();
      message('Code assembled successfully, ' + codeLen + ' bytes.');
      return true;
    }

    function hexdump() {
      setMessages(memory.format(PROG_BASE, cpu.getProgSize()));
    }

    function disassemble() {
      var out = 'Address  Hexdump   Disassembly\n';
      out += '-------------------------------\n';
      for (var a = PROG_BASE; a < PROG_BASE + cpu.getProgSize(); a += 4) {
        var w = memory.getWord(a);
        var hex = ('00000000' + (w >>> 0).toString(16)).slice(-8);
        var ah = ('0000' + a.toString(16)).slice(-4);
        out += '0x' + ah + '    ' + hex + '  ' + cpu.decode(w, a) + '\n';
      }
      setMessages(out);
    }

    return {
      assembleCode: assembleCode, hexdump: hexdump, disassemble: disassemble,
      findLabel: function (n) { return labelMap.hasOwnProperty('K' + n); },
      labelAddr: function (n) { return labelMap['K' + n]; }
    };
  }

  /* ---------------- helpers: messages, hex ------------------------------- */
  function message(text) {
    var box = node.querySelector('.messages code');
    box.textContent += (box.textContent ? '\n' : '') + text;
    box.parentNode.scrollTop = 100000;
  }

  function setMessages(text) {
    var box = node.querySelector('.messages code');
    box.textContent = text;
  }

  function setMonitor(text) {
    var box = node.querySelector('.monitor code');
    box.textContent = text;
  }

  function putc(ch) {
    var box = node.querySelector('.messages code');
    box.textContent += ch;
    box.parentNode.scrollTop = 100000;
  }

  function num2hex(n) {
    var s = '0123456789abcdef';
    return s[(n >> 4) & 15] + s[n & 15];
  }

  initialize();
}

document.addEventListener('DOMContentLoaded', function () {
  var widgets = document.querySelectorAll('.widget');
  for (var i = 0; i < widgets.length; i++) RiscvWidget(widgets[i]);
});
