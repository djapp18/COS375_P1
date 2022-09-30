#include <bitset>
#include <climits>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include "MemoryStore.h"
#include "RegisterInfo.h"

// OP_IDS: how to find opcode of blez etc.
// ZeroExtend and BranchExtend: how to pad zeros to the left/right?
// overflow flag?

using namespace std;

union REGS 
{
    RegisterInfo reg;
    uint32_t registers[32] {0};
};

union REGS regData;

// fill in the missing hex values of OP_IDs
enum OP_IDS
{
    //R-type opcodes...
    OP_ZERO = 0, // zero
    //I-type opcodes...
    OP_ADDI = 0x8, // addi
    OP_ADDIU = 0x9, // addiu
    OP_ANDI = 0xc, // andi
    OP_BEQ = 0x4, // beq
    OP_BNE = 0x5, // bne
    OP_LBU = 0x24, // lbu
    OP_LHU = 0x25, // lhu
    OP_LUI = 0xf, // lui
    OP_LW = 0x23, // lw
    OP_ORI = 0xd, // ori
    OP_SLTI = 0xa, // slti
    OP_SLTIU = 0xb, // sltiu
    OP_SB = 0x28, // sb
    OP_SC = 0x38, // sc
    OP_SH = 0x29, // sh
    OP_SW = 0x2b, // sw
    OP_BLEZ = 0x, // blez
    OP_BGTZ = 0x, // bgtz
    //J-type opcodes...
    OP_J = 0x2, // j
    OP_JAL = 0x3 // jal
};

// fill in the missing hex values of FUNCT_IDS
enum FUNCT_IDS
{
    FUN_ADD = 0x20, // add
    FUN_ADDU = 0x21, // add unsigned (addu)
    FUN_AND = 0x24, // and
    FUN_JR = 0x8, // jump register (jr)
    FUN_NOR = 0x27, // nor
    FUN_OR = 0x25, // or
    FUN_SLT = 0x2a, // set less than (slt)
    FUN_SLTU = 0x2b, // set less than unsigned (sltu)
    FUN_SLL = 0x0, // shift left logical (sll)
    FUN_SRL = 0x2, // shift right logical (srl)
    FUN_SUB = 0x22, // substract (sub)
    FUN_SUBU = 0x23 // substract unsigned (subu)
};

// extract specific bits [start, end] from an instruction
uint instructBits(uint32_t instruct, int start, int end)
{
    int run = start - end + 1;
    uint32_t mask = (1 << run) - 1;
    uint32_t clipped = instruct >> end;
    return clipped & mask;
}

// sign extend while keeping values as uint's
uint32_t signExt(uint16_t smol) 
{
    uint32_t x = smol;
    uint32_t extension = 0xffff0000;
    return (smol & 0x8000) ? x ^ extension : x;
}

// dump registers and memory
// change so registers is a union with reg. No copying of reg to registers - should not be hard
void dump(MemoryStore *myMem) {
    dumpRegisterState(regData.reg);
    dumpMemoryState(myMem);
}

int main(int argc, char **argv)
{
    ifstream infile;
    infile.open(argv[1], ios::binary | ios::in);

    // get length of file
    infile.seekg(0, ios::end);
    int length = infile.tellg();
    infile.seekg (0, ios::beg);

    char *buf = new char[length];
    infile.read(buf, length);
    infile.close();

    // initialize memory store
    MemoryStore *myMem = createMemoryStore();
    int memLength = length / sizeof(buf[0]);
    int i;
    for (i = 0; i < memLength; i++) {
        myMem->setMemValue(i * BYTE_SIZE, buf[i], BYTE_SIZE);
    }

    regData.reg = {};

    // initialize registers to 0
    uint32_t pc = 0; // initialize pc
    bool err = false;

    // branch delay - why diff branch and jal delay
    bool branchDelay, jalDelay = false;
    // branchTarget - Tagregt of branch taken
    // jal_valueToStore -  return address to be stores in reg 31 for JAL instruction
    uint32_t branchTarget, jal_valueToStore = 0;

    while (!err) {
        // fetch current instruction
        uint32_t instruct = 0;
        myMem->getMemValue(pc, instruct, WORD_SIZE);

        pc += 4;
        regData.registers[0] = 0; // reset value of $zero register

        // dump register and memory before exiting
        if (instruct == 0xfeedfeed) {
            dump(myMem);
            return 0;
        }

        // parse instruction 
        // complete instructBits function for correct bits for each operand
        uint32_t opcode = instructBits(instruct, 31, 26);
        uint32_t rs = instructBits(instruct, 25, 21);
        uint32_t rt = instructBits(instruct, 20, 16);
        uint32_t rd = instructBits(instruct, 15, 11);
        uint32_t shamt = instructBits(instruct, 10, 6);
        uint32_t funct = instructBits(instruct, 5, 0);
        uint16_t immediate = instructBits(instruct, 15, 0);
        uint32_t address = instructBits(instruct, 25, 0);

        int32_t signExtImm = signExt(immediate);
        uint32_t zeroExtImm = 
        uint32_t branchAddr = 
        uint32_t fallthruAddr = // assumes PC += 4 just happened

        // fill in operations for all functions and operands. Account for overflow where necessary.
        switch(opcode) {
            case OP_ZERO: // R-type instruction 
                switch(funct) {
                    case FUN_ADD:                         

                    case FUN_ADDU: 

                    case FUN_AND: 

                    case FUN_JR: 

                    case FUN_NOR: 

                    case FUN_OR: 

                    case FUN_SLT: 

                    case FUN_SLTU: 

                    case FUN_SLL: 

                    case FUN_SRL: 

                    case FUN_SUB:  
                    
                    case FUN_SUBU: 

                    default:
                        fprintf(stderr, "\tIllegal operation...\n");
                        err = true;
                }
                break;

            case OP_ADDI: 
                regData.registers[rt] = regData.registers[rs] + signExtImm;


                break;                
            case OP_ADDIU: 
                regData.registers[rt] = regData.registers[rs] + signExtImm;
                break;
            case OP_ANDI: 

            case OP_BEQ: 
                
            case OP_BNE: 
                
            case OP_J: 
                
            case OP_JAL: 
                
            case OP_LBU: 

            case OP_LHU: 
               
            case OP_LUI: 
                
            case OP_LW: 
                
            case OP_ORI: 
                
            case OP_SLTI: 
                
            case OP_SLTIU: 
                
            case OP_SB: 
                
            case OP_SH: 
               
            case OP_SW: 
                
            case OP_BLEZ: 
                
            case OP_BGTZ: 
                
            default:
                fprintf(stderr, "\tIllegal operation...\n");
                err = true;
                break;
        }
    }

    // dump and exit with error
    dump(myMem);
    exit(127);
    return -1;
}
