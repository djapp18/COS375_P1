#include <bitset>
#include <climits>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include "MemoryStore.h"
#include "RegisterInfo.h"
#include "EndianHelpers.h"

// fallthroughaddress
// endianness - 
// failing daras extra test on line 78
// 

using namespace std;

union REGS 
{
    RegisterInfo reg;
    uint32_t registers[32] {0};
};

union REGS regData;

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
    OP_BLEZ = 0x6, // blez
    OP_BGTZ = 0x7, // bgtz
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
uint32_t instructBits(uint32_t instruct, int start, int end)
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

// zero extend input
uint32_t zeroSignExt(uint16_t smol) 
{
    uint32_t x = smol;
    return x;
}

// determine branch address while keeping values as uint's
uint32_t branchAddress(uint16_t smol) 
{
    uint32_t x = smol;
    uint32_t extension = 0xffff0000;
    uint32_t extended_x = (smol & 0x8000) ? x ^ extension : x;
    return extended_x << 2;
}

// check overflow
bool checkOverflow(uint32_t sum, uint32_t operandRS, uint32_t operandRT) {

    // check parities for overflow conditions
    bool rsNegative = operandRS & 0x80000000;
    bool rtNegative = operandRT & 0x80000000;
    bool sumNegative = sum & 0x80000000;

    // find overflow and print an error
    if ((rsNegative & rtNegative & !sumNegative) || (!rsNegative & !rtNegative & sumNegative)) {
        fprintf(stderr, "\tOverflow occured...\n");
        return true;
    }

    return false;
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
        uint32_t zeroExtImm = zeroSignExt(immediate);
        uint32_t branchAddr = branchAddress(immediate);
        uint32_t fallthruAddr = pc; // assumes PC += 4 just happened

        // fill in operations for all functions and operands. Account for overflow where necessary.
        switch(opcode) {
            case OP_ZERO: // R-type instruction 
                switch(funct) {
                    case FUN_ADD:{    
                        regData.registers[rd] = regData.registers[rs] + regData.registers[rt];
                        err = checkOverflow(regData.registers[rd], regData.registers[rs], regData.registers[rt]);
                        break;
                    }
                    case FUN_ADDU:{ 
                        regData.registers[rd] = regData.registers[rs] + regData.registers[rt];
                        break;
                    }
                    case FUN_AND:{ 
                        regData.registers[rd] = regData.registers[rs] & regData.registers[rt];
                        break;
                    }
                    case FUN_JR:{ 
                        branchDelay = true;
                        branchTarget = regData.registers[rs];
                        break;
                    }
                    case FUN_NOR:{ 
                        regData.registers[rd] = !(regData.registers[rs] ^ regData.registers[rt]);
                        break;
                    }
                    case FUN_OR:{ 
                        regData.registers[rd] = regData.registers[rs] ^ regData.registers[rt];
                        break;
                    }
                    case FUN_SLT:{ 
                        regData.registers[rd] = ((int32_t) regData.registers[rs] < (int32_t) regData.registers[rt]) ? 1 : 0;
                        break;
                    }
                    case FUN_SLTU:{ 
                        regData.registers[rd] = (regData.registers[rs] < regData.registers[rt]) ? 1 : 0;
                        break;
                    }
                    case FUN_SLL:{ 
                        regData.registers[rd] = regData.registers[rt] << shamt;
                        break;
                    }
                    case FUN_SRL: 
                        regData.registers[rd] = regData.registers[rt] >> shamt;
                        break;
                    case FUN_SUB:{  
                        regData.registers[rd] = regData.registers[rs] + (~regData.registers[rt] + 1);
                        err = checkOverflow(regData.registers[rd], regData.registers[rs], regData.registers[rt]);
                        break;
                    }
                    case FUN_SUBU:{ 
                        regData.registers[rd] = regData.registers[rs] + (~regData.registers[rt] + 1);
                        break;
                    }
                    default:{
                        fprintf(stderr, "\tIllegal operation...\n");
                        err = true;
                    }
                }
                break;

            case OP_ADDI:{ 
                regData.registers[rt] = regData.registers[rs] + signExtImm;
                err = checkOverflow(regData.registers[rd], regData.registers[rs], signExtImm);
                break;                
            }
            case OP_ADDIU:{ 
                regData.registers[rt] = regData.registers[rs] + signExtImm;
                break;
            }
            case OP_ANDI:{ 
                regData.registers[rt] = regData.registers[rs] & zeroExtImm;
                break;
            }
            case OP_BEQ:{ // make sure to implement logic about executign instruction immediately afterwards
                if(regData.registers[rs] == regData.registers[rt]) {
                    branchDelay = true;
                    branchTarget = pc + branchAddr;
                } else {
                    pc = pc + 4;
                }
                break;
            }
            case OP_BNE:{ 
                if(regData.registers[rs] != regData.registers[rt]) {
                    branchDelay = true;
                    branchTarget = pc + branchAddr;
                }
                break;      
            }
            case OP_J:{
                branchDelay = true;
                uint32_t extend_address = address << 2;  
                uint32_t region = instructBits(pc, 31, 28) << 28;
                branchTarget = extend_address ^ region;
                break;
	    }
            case OP_JAL:{
                branchDelay = true; 
                regData.registers[31] = pc + 4;
                uint32_t extend_address = address << 2;  
                uint32_t region = instructBits(pc, 31, 28) << 28;
                branchTarget = extend_address ^ region;
                break;
	    }
	    case OP_LBU:{ 
                uint32_t addr = regData.registers[rs] + signExtImm;
                uint32_t value = 0;
                myMem->getMemValue(addr, value, BYTE_SIZE);
                regData.registers[rt] = value;
                break;
	    }
            case OP_LHU:{ 
                uint32_t addr = regData.registers[rs] + signExtImm;
                uint32_t value = 0;
                myMem->getMemValue(addr, value, HALF_SIZE);
                regData.registers[rt] = value;
                break;
	    }
            case OP_LUI:{
                regData.registers[rt] = immediate << 16;    
                break;  
	    }	
            case OP_LW:{ 
                uint32_t addr = regData.registers[rs] + signExtImm;
                uint32_t value = 0;
                myMem->getMemValue(addr, value, WORD_SIZE);
                regData.registers[rt] = value;  
                break;  
	    }	
            case OP_ORI: 
                regData.registers[rt] = regData.registers[rs] | zeroExtImm;
                break;
            case OP_SLTI: 
                regData.registers[rt] = ((int32_t) regData.registers[rs] < (int32_t) signExtImm) ? 1 : 0;
                break;
            case OP_SLTIU: 
                regData.registers[rt] = (regData.registers[rs] < signExtImm) ? 1 : 0;
                break;
            case OP_SB:{ 
                uint32_t addr = regData.registers[rs] + signExtImm;
                myMem->setMemValue(addr, regData.registers[rt], BYTE_SIZE);
                break;
	    }
            case OP_SH:{ 
                uint32_t addr = regData.registers[rs] + signExtImm;
                myMem->setMemValue(addr, regData.registers[rt], HALF_SIZE); 
                break;
	    }
            case OP_SW:{ 
                uint32_t addr = regData.registers[rs] + signExtImm;
                myMem->setMemValue(addr, regData.registers[rt], WORD_SIZE) ;  
                break;
	    }
            case OP_BLEZ: 
                if(regData.registers[rs] <= 0) {
                    branchDelay = true;
                    branchTarget = pc + branchAddr;
                }
                break;   
            case OP_BGTZ: 
                if(regData.registers[rs] > 0) {
                    branchDelay = true;
                    branchTarget = pc + branchAddr;
                }
                break;
            default:
                fprintf(stderr, "\tIllegal operation...\n");
                err = true;
                break;
        }

        if(branchDelay) {
            pc = branchTarget;
            branchDelay = false;
        }

    }

    // dump and exit with error
    dump(myMem);
    exit(127);
    return -1;
}
