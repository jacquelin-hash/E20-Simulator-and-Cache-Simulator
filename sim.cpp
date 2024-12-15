/*
CS-UY 2214
Adapted from Jeff Epstein
Starter code for E20 simulator
sim.cpp
*/

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include <regex>
#include <cstdlib>
#include <cstdint>

using namespace std;

// Some helpful constant values that we'll be using.
size_t const static NUM_REGS = 8;
size_t const static MEM_SIZE = 1<<13;
size_t const static REG_SIZE = 1<<16;

/*
    Loads an E20 machine code file into the list
    provided by mem. We assume that mem is
    large enough to hold the values in the machine
    code file.

    @param f Open file to read from
    @param mem Array represetnting memory into which to read program
*/
void load_machine_code(ifstream &f, uint16_t mem[]) {
    regex machine_code_re("^ram\\[(\\d+)\\] = 16'b(\\d+);.*$");
    size_t expectedaddr = 0;
    string line;
    while (getline(f, line)) {
        smatch sm;
        if (!regex_match(line, sm, machine_code_re)) {
            cerr << "Can't parse line: " << line << endl;
            exit(1);
        }
        size_t addr = stoi(sm[1], nullptr, 10);
        unsigned instr = stoi(sm[2], nullptr, 2);
        if (addr != expectedaddr) {
            cerr << "Memory addresses encountered out of sequence: " << addr << endl;
            exit(1);
        }
        if (addr >= MEM_SIZE) {
            cerr << "Program too big for memory" << endl;
            exit(1);
        }
        expectedaddr ++;
        mem[addr] = instr;
    }
}

/*
    Prints the current state of the simulator, including
    the current program counter, the current register values,
    and the first memquantity elements of memory.

    @param pc The final value of the program counter
    @param regs Final value of all registers
    @param memory Final value of memory
    @param memquantity How many words of memory to dump
*/
void print_state(uint16_t pc, uint16_t regs[], uint16_t memory[], size_t memquantity) {
    cout << setfill(' ');
    cout << "Final state:" << endl;
    cout << "\tpc=" <<setw(5)<< pc << endl;

    for (size_t reg=0; reg<NUM_REGS; reg++)
        cout << "\t$" << reg << "="<<setw(5)<<regs[reg]<<endl;

    cout << setfill('0');
    bool cr = false;
    for (size_t count=0; count<memquantity; count++) {
        cout << hex << setw(4) << memory[count] << " ";
        cr = true;
        if (count % 8 == 7) {
            cout << endl;
            cr = false;
        }
    }
    if (cr)
        cout << endl;
}

/**
    Main function
    Takes command-line args as documented below
*/
int main(int argc, char *argv[]) {
    /*
        Parse the command-line arguments
    */
    char *filename = nullptr;
    bool do_help = false;
    bool arg_error = false;
    for (int i=1; i<argc; i++) {
        string arg(argv[i]);
        if (arg.rfind("-",0)==0) {
            if (arg== "-h" || arg == "--help")
                do_help = true;
            else
                arg_error = true;
        } else {
            if (filename == nullptr)
                filename = argv[i];
            else
                arg_error = true;
        }
    }
    /* Display error message if appropriate */
    if (arg_error || do_help || filename == nullptr) {
        cerr << "usage " << argv[0] << " [-h] filename" << endl << endl;
        cerr << "Simulate E20 machine" << endl << endl;
        cerr << "positional arguments:" << endl;
        cerr << "  filename    The file containing machine code, typically with .bin suffix" << endl<<endl;
        cerr << "optional arguments:"<<endl;
        cerr << "  -h, --help  show this help message and exit"<<endl;
        return 1;
    }

    ifstream f(filename);
    if (!f.is_open()) {
        cerr << "Can't open file "<<filename<<endl;
        return 1;
    }
    // TODO: your code here. Load f and parse using load_machine_code
    uint16_t memory[MEM_SIZE] = {0};
    uint16_t regs[NUM_REGS] = {0};
    uint16_t pc = 0;
    // Load the machine code into memory
    load_machine_code(f, memory);

    // TODO: your code here. Do simulation.

    bool running = true;

    while (running) {
        //cout << "running" << endl;
        uint16_t instr = memory[pc & 8191];
        uint16_t opcode = (instr >> 13) & 7;
        uint16_t regA = (instr >> 10) & 7;
        uint16_t regB = (instr >> 7) & 7;
        uint16_t regC = (instr >> 4) & 7;

        uint16_t imm = instr & 127;
        uint16_t imm13 = instr & 8191;
        uint16_t final_four = instr & 15;  

        // sign extend imm 
        imm = (imm & 64) ? (imm | 65408) : imm; 
        imm13 = (imm13 & 4096) ? (imm13 | 57344) : imm13;
        //remove pc here
        //pc++;

        // int imm = instr & 127;
        // if (imm & 64) imm |= -128;

        //int imm13 = instr & 8191;
        // if (imm13 & 4096) imm13 |= -8191;


        // Handle each opcode with if-else conditions.
        if (opcode == 2) {  // j (jump)
            if (pc%8192 == imm13)
                running = false;
            pc = imm13;
        } 
        
        if (opcode == 3) { // jal (jump - link)
            if(pc == imm13)
                running = false;
            regs[7] = pc + 1;
            pc = imm13;
            //cout << pc << " " << imm13 << " " << running << " " << " " << regs[7] << " " << endl;
        }

        if ((opcode == 0) && (final_four == 0)){ // (add aritmetic operation)
            regs[regC] = regs[regA] + regs[regB];
            pc++;
            //cout << regs[regC] << " " << pc << " " << endl;
        }

        if ((opcode == 0) && (final_four == 1)){ // (subtraction)
            regs[regC] = regs[regA] - regs[regB];
            pc++;
            //cout << regs[regC] << " " << pc << " " << endl;
        }

        if ((opcode == 0) && (final_four == 2)){ // (bitwise OR)
            regs[regC] = regs[regA]|regs[regB];
            pc++;
            // cout << regs[regC] << " " << pc << " " << endl;
        }

        if ((opcode == 0) && (final_four == 3)){ // (bitwise AND)
            regs[regC] = regs[regA] & regs[regB];
            pc++;
            //cout << regs[regC] << " " << pc << " " << endl;
        }

        if ((opcode == 0) && (final_four == 4)){ // ( set on less than )
            if (regs[regA] < regs[regB]){
                regs[regC] = 1;
            }
            else{
                regs[regC] = 0;
            }
            //moved outside if/else, either result should increment pc
            pc++;
            //cout << regs[regC] << " " << pc << " " << endl;
        }

        if ((opcode == 0) && (final_four == 8)){ // ( jump register )
            if (pc == regs[regA])
                running = false;
            pc = regs[regA];
            //cout << running << " " << running << " " << pc << " " << endl; 
        }

        if (opcode == 7){ // (slti)
            if(regs[regA] < imm){
                regs[regB] = 1;
            }
            else{
                regs[regB] = 0;
            }
            //moved outside if/else, either result should increment pc
            pc++;
            //cout << regs[regB] << " " << pc << " " << endl;
        }
        
        if (opcode == 4){ // (load word)
            uint16_t memory_address = (regs[regA] + imm) % 8192;
            //edited above to be %8192
            regs[regB] = memory[memory_address];
            //added pc++
            pc++;
            //cout << memory_address << " " << regs[regB] << " " << endl;
        }
        if (opcode == 5){ // (store word)
            uint16_t memory_address = regs[regA] + imm;
            //added %8192
            memory[memory_address % 8192] = regs[regB];
            //uncommented pc++
            pc++;
        }

        if (opcode == 6){ // (branch or equal)
            int temp_imm = 0;
            if (regs[regA] == regs[regB]){
                temp_imm = imm + pc + 1;
                pc = temp_imm;
            }
            else {
                pc++;
            }
        }

        if (opcode == 1){ // (addi)
            regs[regB] = regs[regA] + imm;
            pc++;
            //cout << imm <<endl;
        }

    //added code to set reg[0] to 0, to make it immutable
    regs[0] = 0;
    }

    print_state(pc, regs, memory, 128);
    return 0;
}
