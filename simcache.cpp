/*
CS-UY 2214
Adapted from Jeff Epstein
Fall 2024
Author: Jacqueline Guimac
Starter code for E20 simulator
sim.cpp
*/

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <limits>
#include <iomanip>
#include <list>
#include <regex>
#include <cstdint>

using namespace std;

// Some helpful constant values that we'll be using.
size_t const static NUM_REGS = 8;
size_t const static MEM_SIZE = 1<<13;
size_t const static REG_SIZE = 1<<16;

/*
    Prints out the correctly-formatted configuration of a cache.

    @param cache_name The name of the cache. "L1" or "L2"

    @param size The total size of the cache, measured in memory cells.
        Excludes metadata

    @param assoc The associativity of the cache. One of [1,2,4,8,16]

    @param blocksize The blocksize of the cache. One of [1,2,4,8,16,32,64])

    @param num_rows The number of rows in the given cache.

*/

void print_cache_config(const string& cache_name, int size, int assoc, int blocksize, int num_rows) {
    cout << "Cache " << cache_name << " has size " << size <<
        ", associativity " << assoc << ", blocksize " << blocksize <<
        ", rows " << num_rows << endl;
} 



void print_log_entry(const string& cache_name, const string& status, int pc, int addr, int row) {
    cout << left << setw(8) << cache_name + " " + status << right <<
        " pc:" << setw(5) << pc <<
        "\taddr:" << setw(5) << addr <<
        "\trow:" << setw(4) << row << endl;
}

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

bool hitStatus = false;
auto hit_check(vector<list<unsigned>>& cache, int row, int tag) {
    hitStatus = false;
    for (auto iter = cache[row].begin(); iter != cache[row].end();) {
        if (*iter == tag) {
            iter = cache[row].erase(iter);
            hitStatus = true;
        }
        else
            ++iter;
    }
    return hitStatus;
}

void add_or_evict(vector<list<unsigned>>& cache, int row, int assoc, int tag, string const& cacheLevel, bool writeEnable, uint16_t pc, 
    uint16_t memoryAddress) { 
    if (hitStatus) 
    {
        if (writeEnable) 
        {
        print_log_entry(cacheLevel, "SW", pc, memoryAddress, row);
        }
        else
            print_log_entry(cacheLevel, "HIT", pc, memoryAddress, row);
    } 
    else 
    {
        if (writeEnable) 
        {
            print_log_entry(cacheLevel, "SW", pc, memoryAddress, row);
        }
        else
            print_log_entry(cacheLevel, "MISS", pc, memoryAddress, row);
        if (cache[row].size() >= assoc) 
        {
            cache[row].pop_back(); 
        }
    }
    cache[row].push_front(tag); 
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
    string cache_config;
    for (int i=1; i<argc; i++) {
        string arg(argv[i]);
        if (arg.rfind("-",0)==0) {
            if (arg== "-h" || arg == "--help")
                do_help = true;
            else if (arg=="--cache") {
                i++;
                if (i>=argc)
                    arg_error = true;
                else
                    cache_config = argv[i];
            }
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
        cerr << "usage " << argv[0] << " [-h] [--cache CACHE] filename" << endl << endl;
        cerr << "Simulate E20 cache" << endl << endl;
        cerr << "positional arguments:" << endl;
        cerr << "  filename    The file containing machine code, typically with .bin suffix" << endl<<endl;
        cerr << "optional arguments:"<<endl;
        cerr << "  -h, --help  show this help message and exit"<<endl;
        cerr << "  --cache CACHE  Cache configuration: size,assoc,blocksize (for one"<<endl;
        cerr << "                 cache) or"<<endl;
        cerr << "                 size,assoc,blocksize,size,assoc,blocksize"<<endl;
        cerr << "                 (for two caches)"<<endl;
        return 1;
    }

    ifstream f(filename);
    if (!f.is_open()) {
        cerr << "Can't open file "<<filename<<endl;
        return 1;
    }
    uint16_t memory[MEM_SIZE] = {0};
    uint16_t regs[NUM_REGS] = {0};
    uint16_t pc = 0;
    load_machine_code(f, memory);

    int L1size, L1assoc, L1blocksize, L1rows, L2size, L2assoc, L2blocksize, L2rows;
    bool L2Enable = false;
    /* parse cache config */
    if (cache_config.size() > 0) {
        vector<int> parts;
        size_t pos;
        size_t lastpos = 0;
        while ((pos = cache_config.find(",", lastpos)) != string::npos) {
            parts.push_back(stoi(cache_config.substr(lastpos,pos)));
            lastpos = pos + 1;
        }
        parts.push_back(stoi(cache_config.substr(lastpos)));
        if (parts.size() == 3) {
            L1size = parts[0];
            L1assoc = parts[1];
            L1blocksize = parts[2];
            L1rows = L1size/L1assoc/L1blocksize;      
            print_cache_config("L1", L1size, L1assoc, L1blocksize, L1rows);
        } else if (parts.size() == 6) {
            L1size = parts[0];
            L1assoc = parts[1];
            L1blocksize = parts[2];
            L2size = parts[3];
            L2assoc = parts[4];
            L2blocksize = parts[5];  
            L1rows = L1size/L1assoc/L1blocksize; 
            L2rows = L2size/L2assoc/L2blocksize;
            print_cache_config("L1", L1size, L1assoc, L1blocksize, L1rows);
            print_cache_config("L2", L2size, L2assoc, L2blocksize, L2rows);
            L2Enable = true;
        } else {
            cerr << "Invalid cache config"  << endl;
            return 1;
        }
    }

    vector<list<unsigned>> L1;
    for (size_t i = 0; i < L1rows; i++) {
        L1.push_back(list<unsigned>()); 
    }
    vector<list<unsigned>> L2;
    if(L2Enable)
    {
        for (size_t i = 0; i < L2rows; i++) {
            L2.push_back(list<unsigned>());
        }
    }

    int L1BlockID, L2BlockID, L1Row, L2Row, L1Tag, L2Tag;

    bool running = true;

    while (running) {
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
        }

        if ((opcode == 0) && (final_four == 0)){ // (add aritmetic operation)
            regs[regC] = regs[regA] + regs[regB];
            pc++;
        }

        if ((opcode == 0) && (final_four == 1)){ // (subtraction)
            regs[regC] = regs[regA] - regs[regB];
            pc++;
        }

        if ((opcode == 0) && (final_four == 2)){ // (bitwise OR)
            regs[regC] = regs[regA]|regs[regB];
            pc++;
        }

        if ((opcode == 0) && (final_four == 3)){ // (bitwise AND)
            regs[regC] = regs[regA] & regs[regB];
            pc++;
        }

        if ((opcode == 0) && (final_four == 4)){ // ( set on less than )
            if (regs[regA] < regs[regB]){
                regs[regC] = 1;
            }
            else{
                regs[regC] = 0;
            }
            pc++;
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
            pc++;
            //cout << regs[regB] << " " << pc << " " << endl;
        }
        
        if (opcode == 4){ // (load word)
            uint16_t memory_address = (regs[regA] + imm) % 8192;
            regs[regB] = memory[memory_address];

            L1BlockID = memory_address / L1blocksize;
            L1Row = L1BlockID % L1rows;
            L1Tag = L1BlockID / L1rows;

            hit_check(L1, L1Row, L1Tag);
            add_or_evict(L1, L1Row, L1assoc, L1Tag, "L1", false, pc, memory_address);
            if((hitStatus == false && L2Enable)) {
                L2BlockID = memory_address / L2blocksize;
                L2Row = L2BlockID % L2rows;
                L2Tag = L2BlockID / L2rows;
                hit_check(L2, L2Row, L2Tag);
                add_or_evict(L2, L2Row, L2assoc, L2Tag, "L2", false, pc, memory_address);
            } 
            pc++;

        }
        if (opcode == 5){ // (store word)
            //added %8192
            uint16_t memory_address = (regs[regA] + imm) % 8192;
            memory[memory_address] = regs[regB];
            //uncommented pc++
            L1BlockID = memory_address / L1blocksize;
            L1Row = L1BlockID % L1rows;
            L1Tag = L1BlockID / L1rows;
            hit_check(L1, L1Row, L1Tag);
            add_or_evict(L1, L1Row, L1assoc, L1Tag, "L1", true, pc, memory_address);
            if((L2Enable)) {
                L2BlockID = memory_address / L2blocksize;
                L2Row = L2BlockID % L2rows;
                L2Tag = L2BlockID / L2rows;
                hit_check(L2, L2Row, L2Tag);
                add_or_evict(L2, L2Row, L2assoc, L2Tag, "L2", true, pc, memory_address);
            } 
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
        }

    //added code to set reg[0] to 0, to make it immutable
    regs[0] = 0;
    }

    return 0;
}