#! /usr/bin/python2.7

import sys
import argparse

import ila
from uc8051ast import uc8051

def readhex(filename):
    data_dict = {}
    data = []
    with open(filename, 'rt') as fileobject:
        for line in fileobject:
            line = line.strip()
            if len(line) == 0:
                continue
            if line[0] != ':':
                print (filename, line)
            assert line[0] == ':'
            line = line[1:]
            length_str = line[:2]
            addr_str = line[2:6]
            type_str = line[6:8]
            data_str = line[8:-2]

            data_length = int(length_str, 16)
            start_addr = int(addr_str, 16)
            record_type = int(type_str, 16)

            #print data_str
            #print 2*data_length, len(data_str)
            assert 2*data_length == len(data_str)
            data_bytes = [int(data_str[i:i+2], 16) for i in xrange(0,2*data_length,2)]
            #print data_bytes
            assert len(data_bytes) == data_length

            if record_type == 0:
                for i, addr in enumerate(xrange(start_addr, start_addr+data_length)):
                    data_dict[addr] = data_bytes[i]

        data = [-1] * (max(data_dict.keys())+1)
        for addr in data_dict:
            data_value = data_dict[addr]
            assert data[addr] == -1
            data[addr] = data_value
        for i in xrange(len(data)):
            if data[i] == -1:
                data[i] = 0

    return data

stage = 0
def stage_print(description):
    global stage
    stage += 1
    msg = ('[Stage #%d] %s') % (stage, description)
    print (msg)

def import_8051_ila(enable_ps):
    model = ila.Abstraction("oc8051")
    # fetch and decode.
    uc = uc8051()
    model.fetch_expr = uc.op0 # s/hand for uc.rom[uc.pc]
    model.decode_exprs = [uc.op0 == i for i in xrange(0x0, 0x100)]
   
    # program counter
    pc = model.reg('PC', 16)
    # code memory
    rom = model.mem('ROM', 16, 8)
    # IRAM
    iram = model.mem('IRAM', 8, 8)
    # main SFRs
    acc = model.reg('ACC', 8)
    b = model.reg('B', 8)
    psw = model.reg('PSW', 8)
    sp = model.reg('SP', 8)
    dpl = model.reg('DPL', 8)
    dph = model.reg('DPH', 8)
    # ports
    p0 = model.reg('P0', 8)
    p1 = model.reg('P1', 8)
    p2 = model.reg('P2', 8)
    p3 = model.reg('P3', 8)
    # misc SFRs
    pcon = model.reg('PCON', 8)
    tcon = model.reg('TCON', 8)
    tmod = model.reg('TMOD', 8)
    tl0 = model.reg('TL0', 8)
    tl1 = model.reg('TH0', 8)
    tl1 = model.reg('TL1', 8)
    th1 = model.reg('TH1', 8)
    scon = model.reg('SCON', 8)
    sbuf = model.reg('SBUF', 8)
    ie = model.reg('IE', 8)
    ip = model.reg('IP', 8)

    # XRAM
    #xram_data_in = model.reg('XRAM_DATA_IN', 8)  FIXME
    #xram_data_in = model.inp('XRAM_DATA_IN', 8)
    xram_data_out = model.reg('XRAM_DATA_OUT', 8)
    xram_addr = model.reg('XRAM_ADDR', 16)

    # get synthesized states
    regs = [
        'PC', 'ACC', 'B', 'PSW', 'SP', 
        'DPL', 'DPH', 
        'P0', 'P1', 'P2', 'P3', 
        'PCON', 'TCON', 'TMOD', 'TL0', 
        'TH0', 'TL1', 'TH1', 'SCON', 
        'SBUF', 'IE', 'IP', 'XRAM_DATA_OUT', 
        'XRAM_ADDR'
    ]
    states = regs + ['IRAM']


    for s in states:
        ast = model.importOne('asts/%s_%s' % (s, 'en' if enable_ps else 'dis'))
        model.set_next(s, ast)

    for r in regs:
        reg = model.getreg(r)
        zero = model.const(0, reg.type.bitwidth)
        model.set_init(r, zero)
    stage_print ('Finished importing 8051 ASTs.')
    return (model, rom)

def gen_uclid5(hexfile, enable_ps):
    (model, rom) = import_8051_ila(enable_ps)

    # set ROM initial value.
    data = readhex(hexfile)
    romvalue = ila.MemValues(16, 8, 0xff)
    for a, d in enumerate(data):
        # print '%04X -> %02X' % (a, d)
        romvalue[a] = d
    romconst = model.const(romvalue)
    model.set_init('ROM', romconst)
    model.set_next('ROM', rom)
    stage_print ('Set ROM initial value.')

    # setup uclid converter.
    uclid5 = model.toUclid5("test")
    uclid5.initVar('ROM')
    uclid5.initVar('PC')
    rom = model.getmem('ROM')
    pc = model.getreg('PC')
    pc_next = model.get_next('PC')
    inst_next = rom[pc]

    # setup DFS
    stack = [ (p, tuple([])) for p in uclid5.getExprValues(pc) ]
    visited = set()
    statemap = {}

    # utlity function to create names for states.
    def state_name(pc, call_stack):
        stack_repr = '_'.join('%X' % pc for pc in call_stack)
        return 'pc_%X_stack_%s' % (pc, stack_repr)

    while len(stack):
        top_pc, call_stack = stack.pop()
        # keep track of visited states.
        state = (top_pc, call_stack)
        if state in visited: continue
        else: visited.add(state)

        name = state_name(top_pc, call_stack)
        assert name not in statemap
        statemap[name] = (top_pc, call_stack)

        # set the current PC value.
        uclid5.setVar('PC', top_pc)
        # get current opcode.
        thisInst = uclid5.getExprValues(inst_next)
        assert len(thisInst) == 1
        opcode = thisInst[0]
        # if it is a call, we have to add it to the stack.
        if iscall(opcode):
            call_stack_new = tuple(list(call_stack) + [nextpc(top_pc, opcode)])
            nextPCs = uclid5.getExprValues(pc_next)
        # if it is a return, don't symbolically execute, instead pop off the stack.
        elif isret(opcode):
            call_stack_new_list = list(call_stack)
            ret_pc = call_stack_new_list.pop()
            call_stack_new = tuple(call_stack_new_list)
            nextPCs = [ret_pc]
        # else symbolic execution rules.
        else:
            call_stack_new = call_stack
            nextPCs = uclid5.getExprValues(pc_next)
        # add new states to be visited to the stack.
        stack += [(n, call_stack_new) for n in nextPCs]
        # need to add edges here.

        # now print out where we are.
        assert len(nextPCs) <= 16
        next_string = ' '.join('%04X' % nextPC_i for nextPC_i in nextPCs)
        call_stack_string = ' '.join('%04X' % pc for pc in call_stack)
        print 'PC: %04X [%20s]; OP: %02X -> NEXT: %s' % (top_pc, call_stack_string, opcode, next_string)


def iscall(opcode):
    return (((opcode & 0xF) == 1) and ((opcode & 0x10) == 0x10)) or (opcode == 0x12)

def nextpc(pc, opcode):
    if opcode == 0x12:
        return pc + 0x3
    elif (((opcode & 0xF) == 1) and ((opcode & 0x10) == 0x10)):
        return pc + 0x2
    else:
        assert False

def isret(opcode):
    return opcode == 0x22

def main():
    # ila.setloglevel(2, "")
    parser = argparse.ArgumentParser()
    parser.add_argument("hexfile", type=str, help='hex file.')
    parser.add_argument("--en", type=int, default=1, 
                        help="enable parameterized synthesis.")

    args = parser.parse_args()
    gen_uclid5(args.hexfile, args.en) 


if __name__ == '__main__':
    main()
