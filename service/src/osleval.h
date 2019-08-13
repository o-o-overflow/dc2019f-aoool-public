#ifndef OSLEVAL_H
#define OSLEVAL_H

#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <iomanip>
#include <vector>
#include <map>
#include <sys/mman.h>

#include <boost/filesystem.hpp>

#include "config.h"
#include "handler.h"
#include "osl.h"
#include "utils.h"

#define DATA_SECTION_SIZE 0x200
#define STACK_SECTION_SIZE 0x500
#define TEXT_SECTION_SIZE 0x2000
#define TOTAL_MEMORY_SIZE ((DATA_SECTION_SIZE) + (STACK_SECTION_SIZE) + (TEXT_SECTION_SIZE) + 8)

#define SAVED_STACK_PTR (m_mem)
#define DATA_PTR (m_mem + 16)
#define STACK_PTR (DATA_PTR + DATA_SECTION_SIZE)
#define TEXT_PTR (STACK_PTR + STACK_SECTION_SIZE)

#define MEMORY_ADDR_THRESHOLD ((uint64_t) 0x100000)


namespace aoool {

using namespace std;

class OslExecutor {
public:

    OslExecutor(RequestHandler rh) {
        m_mem = NULL;
        m_jitted_code = new uint8_t[TEXT_SECTION_SIZE];
        m_jitted_code_idx = 0;
        m_req_h = rh;
        m_output_fp = "";
        m_output_fd = -1;
    }

    ~OslExecutor() {
        delete m_jitted_code;
    }

    void alloc_memory() {
        m_mem = (uint8_t*) mmap(0, TOTAL_MEMORY_SIZE,
                   PROT_READ | PROT_WRITE | PROT_EXEC,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        DLOG("allocated memory: %p\n", (void*) m_mem);
    }

    void init_memory() {
        memset(m_mem, 0, TOTAL_MEMORY_SIZE);

        // .data is inited in a way that all cells will be treated as "free".
        for (size_t i=0; i<(DATA_SECTION_SIZE/16); i++) {
            uint8_t* m_data = DATA_PTR;
            // init with huge non-negative number (which is enough to be considered as free
            *(long long int *)(m_data + i*16) = ~( ((uint64_t) 1) << 63);
            *(long long int *)(m_data + i*16 + 8) = 0;
        }
        *((uint8_t **) SAVED_STACK_PTR) = (STACK_PTR+STACK_SECTION_SIZE);
    }

    void unalloc_memory() {
        munmap(m_mem, TOTAL_MEMORY_SIZE);
    }

    void open_output_file() {
        auto tmp_fn = boost::filesystem::unique_path("%%%%%%%%%%%%%%%%");
        m_output_fp = string((boost::filesystem::path("/tmp/") / tmp_fn).string());
        FILE* f_stream = fopen(m_output_fp.c_str(), "wb");
        if (f_stream != NULL) {
            m_output_fd = fileno(f_stream);
        }
        DLOG("open output file: %s fd: %d\n", m_output_fp.c_str(), m_output_fd);
    }

    void close_output_file() {
        close(m_output_fd);
    }

    string get_output() {
        DLOG("reading output from %s\n", m_output_fp.c_str());
        ifstream output_stream(m_output_fp, ios::in | ios::binary);
        string output = read_all_from_stream(output_stream);
        DLOG("output (len:%lu): %s\n", output.length(), output.c_str());
        return output;
    }

    void delete_output_file() {
        DLOG("about to delete %s\n", m_output_fp.c_str());
        unlink(m_output_fp.c_str());
    }

    void dump_p32(uint32_t x, uint8_t* buf) {
        uint8_t* x_ptr = (uint8_t *) &x;
        for (size_t i=0; i<4; i++) {
            buf[i] = x_ptr[i];
        }
    }

    void dump_p64(uint64_t x, uint8_t* buf) {
        uint8_t* x_ptr = (uint8_t *) &x;
        for (size_t i=0; i<8; i++) {
            buf[i] = x_ptr[i];
        }
    }

    inline void emit_mov_r13_x(uint64_t x) {
        memcpy(&m_jitted_code[m_jitted_code_idx], "\x49\xbd", 2);
        m_jitted_code_idx += 2;
        dump_p64(x, &m_jitted_code[m_jitted_code_idx]);
        m_jitted_code_idx += 8;
    }

    inline void emit_xchg_rsp_ptr_r13() {
        memcpy(&m_jitted_code[m_jitted_code_idx], "\x49\x87\x65\x00", 4);
        m_jitted_code_idx += 4;
    }

    inline void emit_push_r13() {
        memcpy(&m_jitted_code[m_jitted_code_idx], "\x41\x55", 2);
        m_jitted_code_idx += 2;
    }

    inline void emit_push_all_registers() {
        /*
            push rax
            push rbx
            push rcx
            push rdx
            push rdi
            push rsi
            push rbp
            push r8
            push r9
            push r10
            push r11
            push r12
            push r13
            push r14
            push r15
         */
            memcpy(&m_jitted_code[m_jitted_code_idx], "\x50\x53\x51\x52\x57\x56\x55\x41\x50\x41\x51\x41\x52\x41\x53\x41\x54\x41\x55\x41\x56\x41\x57", 23);
            m_jitted_code_idx += 23;
    }

    inline void emit_pop_all_registers() {
        /*
            pop r15
            pop r14
            pop r13
            pop r12
            pop r11
            pop r10
            pop r9
            pop r8
            pop rbp
            pop rsi
            pop rdi
            pop rdx
            pop rcx
            pop rbx
            pop rax
         */
            memcpy(&m_jitted_code[m_jitted_code_idx], "\x41\x5f\x41\x5e\x41\x5d\x41\x5c\x41\x5b\x41\x5a\x41\x59\x41\x58\x5d\x5e\x5f\x5a\x59\x5b\x58", 23);
            m_jitted_code_idx += 23;
    }

    inline void emit_mov_rbp_rsp() {
        memcpy(&m_jitted_code[m_jitted_code_idx], "\x48\x89\xe5", 3);
        m_jitted_code_idx += 3;
    }

    inline void emit_mov_rsp_rbp() {
        memcpy(&m_jitted_code[m_jitted_code_idx], "\x48\x89\xec", 3);
        m_jitted_code_idx += 3;
    }

    inline void emit_push_rbx() {
        memcpy(&m_jitted_code[m_jitted_code_idx], "\x53", 1);
        m_jitted_code_idx += 1;
    }

    inline void emit_pop_r13() {
        memcpy(&m_jitted_code[m_jitted_code_idx], "\x41\x5d", 2);
        m_jitted_code_idx += 2;
    }

    inline void emit_pop_rbx() {
        memcpy(&m_jitted_code[m_jitted_code_idx], "\x5b", 1);
        m_jitted_code_idx += 2;
    }

    inline void emit_mov_rdi_x_64(uint64_t x) {
        memcpy(&m_jitted_code[m_jitted_code_idx], "\x48\xbf", 2);
        m_jitted_code_idx += 2;
        dump_p64(x, &m_jitted_code[m_jitted_code_idx]);
        m_jitted_code_idx += 8;
    }

    inline void emit_mov_rsi_x_32(uint32_t x) {
        memcpy(&m_jitted_code[m_jitted_code_idx], "\x48\xc7\xc6", 3);
        m_jitted_code_idx += 3;
        dump_p32(x, &m_jitted_code[m_jitted_code_idx]);
        m_jitted_code_idx += 4;
    }

    inline void emit_lea_rdi_ptr_rdi_8_times_rsi() {
        memcpy(&m_jitted_code[m_jitted_code_idx], "\x48\x8d\x3c\xf7", 4);
        m_jitted_code_idx += 4;
    }

    inline void emit_push_rax() {
        m_jitted_code[m_jitted_code_idx] = 0x50;
        m_jitted_code_idx++;
    }

    inline void emit_push_rdi() {
        m_jitted_code[m_jitted_code_idx] = 0x57;
        m_jitted_code_idx++;
    }

    inline void emit_push_x_64(uint64_t x) {
        // this modifies rdi
        emit_mov_rdi_x_64(x);
        emit_push_rdi();
    }

    inline void emit_push_0() {
        memcpy(&m_jitted_code[m_jitted_code_idx], "\x6a\x00", 2);
        m_jitted_code_idx += 2;
    }

    inline void emit_mov_rax_ptr_rsp() {
        memcpy(&m_jitted_code[m_jitted_code_idx], "\x48\x8b\x04\x24", 4);
        m_jitted_code_idx += 4;
    }

    inline void emit_mov_rdi_ptr_rsp() {
        memcpy(&m_jitted_code[m_jitted_code_idx], "\x48\x8b\x3c\x24", 4);
        m_jitted_code_idx += 4;
    }

    inline void emit_mov_rdi_ptr_rsp_offset_x(uint8_t x) {
        memcpy(&m_jitted_code[m_jitted_code_idx], "\x48\x8b\x7c\x24", 4);
        m_jitted_code_idx += 4;
        m_jitted_code[m_jitted_code_idx] = x;
        m_jitted_code_idx++;
    }

    inline void emit_mov_rsi_ptr_rsp_offset_x(uint8_t x) {
        memcpy(&m_jitted_code[m_jitted_code_idx], "\x48\x8b\x74\x24", 4);
        m_jitted_code_idx += 4;
        m_jitted_code[m_jitted_code_idx] = x;
        m_jitted_code_idx++;
    }

    inline void emit_add_rsp_x(uint8_t x) {
        memcpy(&m_jitted_code[m_jitted_code_idx], "\x48\x83\xc4", 3);
        m_jitted_code_idx += 3;
        m_jitted_code[m_jitted_code_idx] = x;
        m_jitted_code_idx++;
    }

    inline void emit_add_rdi_rsi() {
        memcpy(&m_jitted_code[m_jitted_code_idx], "\x48\x01\xf7", 3);
        m_jitted_code_idx += 3;
    }

    inline void emit_sub_rdi_rsi() {
        memcpy(&m_jitted_code[m_jitted_code_idx], "\x48\x29\xf7", 3);
        m_jitted_code_idx += 3;
    }

    inline void emit_mul_rsi() {
        memcpy(&m_jitted_code[m_jitted_code_idx], "\x48\xf7\xe6", 3);
        m_jitted_code_idx += 3;
    }

    inline void emit_ret() {
        m_jitted_code[m_jitted_code_idx] = 0xc3;
        m_jitted_code_idx++;
    }

    inline void emit_cc() {
        m_jitted_code[m_jitted_code_idx] = 0xcc;
        m_jitted_code_idx++;
    }

    void emit_store_object() {
        /*
            This method takes the object on top of the stack (int/str) and copy
            it to the memory pointed to by rdi. This code checks the type of the
            object and copies/pops 16/32 bytes accordingly. Note: there is an
            intended bug in this implementation, see below.

            This is the code:
            // rdi = ptr to curr obj on the store
            // rsp = ptr to new obj on the stack

            entrypoint:
                mov rsi, [rsp+8]
                test rsi, rsi
                jne type_str

            type_int:
                // Intended bug: only copy the first 8 bytes. That is, don't
                // copy 16 bytes / the len field as it should.
                mov rsi, qword ptr [rsp]
                mov qword ptr [rdi], rsi
                add rsp, 0x10
                jmp exit

            type_str:
                // This code does not simply copy the 32 bytes of the string
                // object. What it does is to check the len of the string on
                // the store, and if this len is greater than the len of the
                // new string, then the code reuses the existing buffer to
                // store the new string. This is not a bug per-se, but combined
                // with other use-after-free tricks, an attacker can modify the
                // pointer arbitrarily, and she can thus use this
                // "optimization" feature to achieve arbitrary memory write.
                // This situation can be triggered by assigning an existing
                // variable (which maps already to the target memory in the
                // store).
                //
                // Mini exploit (this prints abbba):
                // x = 1 + "aaaaa";
                // x = "bbb";
                // x = -1 + x;
                // print x;
                //
                // If the existing string is not big enough, then there are two
                // cases: 1) if the string is "big" (longer than 15 chars),
                // then copy all the 32 bytes from the stack in the store. 2)
                // Chunk 1 is a pointer to 3. Chunk 2 is the len. Chunk 3 and 4
                // contains the actual string.
                //
                // This last strategy causes the jitter to place addresses of
                // the jitter area in the store -- and that's leakable. From
                // there, the attacker can leak jitter (and overwrite with
                // their code), leak rsp, leak libc, etc.

                // r8 = curr len
                // r9 = new len
                mov r8, qword ptr [rdi+8]
                mov r9, qword ptr [rsp+8]
                cmp r8, r9
                jg reuse_buffer

            no_buffer_reuse:
                // check the len
                mov qword ptr [rdi+8], rsi
                cmp rsi, 0x15
                jge full_overwrite

            cpy_in_place:
                // pop and discard
                pop rsi
                lea rsi, qword ptr [rdi+16]
                mov qword ptr [rdi], rsi
                // pop copy len
                pop rsi
                mov qword ptr [rdi+8], rsi
                // pop the two chunks, they contain the actual string
                pop rsi
                mov qword ptr [rdi+16], rsi
                pop rsi
                mov qword ptr [rdi+24], rsi
                jmp exit

            full_overwrite:
                pop rsi
                mov qword ptr [rdi], rsi
                pop rsi
                mov qword ptr [rdi+8], rsi
                pop rsi
                mov qword ptr [rdi+16], rsi
                pop rsi
                mov qword ptr [rdi+24], rsi
                jmp exit

            reuse_buffer:
                // get actual pointers to the strings
                mov rdi, qword ptr [rdi]
                mov rsi, qword ptr [rsp]
                add rsp, 0x20

                // copy r9 bytes
                xor rax, rax
            cpy_str_loop:
                mov al, byte ptr [rsi]
                mov byte ptr [rdi], al
                inc rdi
                inc rsi
                dec r9
                jne cpy_str_loop

            exit:
        */

        memcpy(&m_jitted_code[m_jitted_code_idx], "\x48\x8b\x74\x24\x08\x48\x85\xf6\x75\x0d\x48\x8b\x34\x24\x48\x89\x37\x48\x83\xc4\x10\xeb\x63\x4c\x8b\x47\x08\x4c\x8b\x4c\x24\x08\x4d\x39\xc8\x7f\x38\x48\x89\x77\x08\x48\x83\xfe\x15\x7d\x19\x5e\x48\x8d\x77\x10\x48\x89\x37\x5e\x48\x89\x77\x08\x5e\x48\x89\x77\x10\x5e\x48\x89\x77\x18\xeb\x32\x5e\x48\x89\x37\x5e\x48\x89\x77\x08\x5e\x48\x89\x77\x10\x5e\x48\x89\x77\x18\xeb\x1d\x48\x8b\x3f\x48\x8b\x34\x24\x48\x83\xc4\x20\x48\x31\xc0\x8a\x06\x88\x07\x48\xff\xc7\x48\xff\xc6\x49\xff\xc9\x75\xf1", 122);
        m_jitted_code_idx += 122;
    }

    void emit_load_object() {
        /* 
           This method takes the object on the store (pointed by rdi) and
           pushes it on the stack.  This code checks the type of the object and
           copies/pops 16/32 bytes accordingly. 
          
           These are the instructions:

        entrypoint:
           mov rsi, qword ptr [rdi+8]
           test rsi, rsi
           jne type_str
        type_int:
           push rsi
           push [rdi]
           jmp exit
        type_str:
           push [rdi+24]
           push [rdi+16]
           push [rdi+8]
           push [rdi]
        exit:
        */

        memcpy(&m_jitted_code[m_jitted_code_idx], "\x48\x8b\x77\x08\x48\x85\xf6\x75\x05\x56\xff\x37\xeb\x0b\xff\x77\x18\xff\x77\x10\xff\x77\x08\xff\x37", 25);
        m_jitted_code_idx += 25;
    }


    string eval(OslProgram* program) {

        DLOG("parsed program: %s\n", program->toString().c_str());

        alloc_memory();
        init_memory();
        open_output_file();

        for (auto &stmt : program->statements) {
            memset(TEXT_PTR, 0, TEXT_SECTION_SIZE);
            memset(m_jitted_code, 0, TEXT_SECTION_SIZE);
            m_jitted_code_idx = 0;

            // emit_cc();

            // save all registers
            emit_push_all_registers();
            // mov r13, SAVED_STACK_PTR
            emit_mov_r13_x((uint64_t) SAVED_STACK_PTR);
            // xchg rsp, qword ptr [r13]
            emit_xchg_rsp_ptr_r13();

            // this will add code m_jitted_code and update m_jitted_code_idx
            eval(stmt);

            // copy epilog to TEXT SECTION
            // xchg rsp, qword ptr [r13]
            emit_xchg_rsp_ptr_r13();

            // restore all registers
            emit_pop_all_registers();
            // ret
            emit_ret();

            memcpy(TEXT_PTR, m_jitted_code, m_jitted_code_idx);

            int (*jitted_code)(void) = (int(*)(void)) TEXT_PTR;

            jitted_code();
        }

        unalloc_memory();

        close_output_file();
        string output = get_output();
        delete_output_file();

        return output;
    }

    int8_t get_var_idx(OslVar* var) {
        // Returns a pointer to the memory that backs the content of this variable.
        //
        // A variable must have a name that is a single char long.
        //
        // This method follows the following algorithm:
        // - If the variable is already known to the executor, returns its
        // associated memory address.
        // - if the executor does not know about this variable, then scans the
        // .data section from the beginning, finds the first empty spot, and
        // return its address.
        //
        // This method uses the following algorithm to find the first empty spot:
        // - Start from the beginning of DATA_PTR
        // - If the current cell is in use, go to the next one. If int, +=16, if str, +=32
        // - A cell is considered in use in these two conditions:
        // - A cell is considered as empty if and only if one of the following holds:
        //    - if p[0] < threshold && p[1] == 0: in-use int, skip 16
        //    - if p[0] > threshold && p[1] != 0: in-use str, skip 32
        // - A cell is empty iff is not in use.
        //
        // Intended bugs:
        // - this will return a memory area even if the variable was not initialized
        // - this function will return a memory area that contains
        // uninitialized (or previously used data), allowing for use-after-free
        // bugs.
        // - returns -1 when running into problems

        string var_name = var->name;

        DLOG("get_var_idx (%s)\n", var->toShortString().c_str());

        if (var_name.length() != 1) {
            // check for -1?
            DLOG("error: bad var name\n");
            return -1;
        }

        if (m_vars.find(var_name) != m_vars.end()) {
            int8_t var_idx = m_vars[var_name];
            DLOG("get_var_idx (hit) %s: %d\n", var->toShortString().c_str(), var_idx);
            return var_idx;
        } else {
            DLOG("get_var_idx (miss) %s\n", var->toShortString().c_str());
        }

        // find a free spot in the store
        uint64_t* ptr = (uint64_t*) DATA_PTR;
        int8_t var_idx = -1;
        while (ptr< (uint64_t*) STACK_PTR) {
            if ( (ptr[1] == 0) &&  // this could be an in-use integer
                 ( (ptr[0] < MEMORY_ADDR_THRESHOLD) ||  // non-huge number
                     ( (ptr[0] & ( ((uint64_t) 1) << 63)) != 0 ) // negative number
                 )
               ) { 
                    // this is an in-use integer. Skip 2 qwords.
                    ptr += 2;
            } else if ( (ptr[0] > MEMORY_ADDR_THRESHOLD) && (ptr[1] != 0) ) {
                // this is an in-use str. Skip 4 qwords.
                ptr += 4;
            } else {
                // this is actually a free slot. Return this one.
                var_idx = ( ((uint64_t)ptr) - ((int64_t) DATA_PTR) ) / 8;
                break;
            }
        }

        if (var_idx == -1) {
            // could not find a free spot
            DLOG("error: could not find free spot\n");
            return -1;
        }

        // found something! store the var => addr association and return it
        m_vars[var_name] = var_idx;
        DLOG("get_var_idx (update) %s: %d\n", var->toShortString().c_str(), var_idx);
        
        return var_idx;
    }

    void eval(OslStatement* stmt) {
        switch(stmt->type) {
            case OslAssignStmtType: {
                auto assignStmt = static_cast<OslAssignStmt*>(stmt);
                eval(assignStmt);
                break;
            }
            case OslPrintStmtType: {
                auto printStmt = static_cast<OslPrintStmt*>(stmt);
                eval(printStmt);
                break;
            }
            case OslPrintLogStmtType: {
                evalPrintLogStmt();
                break;
            }
            case OslDelStmtType: {
                auto delStmt = static_cast<OslDelStmt*>(stmt);
                eval(delStmt);
                break;
            }
            default:
                throw "not supported";
        }
    }

    void eval(OslAssignStmt* stmt) {
        // eval the expression and place it on the stack
        eval(stmt->expr);

        // Copy the object on top of the stack to the store.
        //
        // First: find which spot in the store is reserved for this var.
        // Second: check the type of the top entry on the stack. Copy in the
        // store the top 16/32 bytes depending whether it's an int or a str.

        auto var = stmt->var;
        uint8_t var_idx = get_var_idx(var);

        emit_mov_rdi_x_64((uint64_t) DATA_PTR);
        emit_mov_rsi_x_32(var_idx);
        // mov rdi, qword ptr [rdi + 8*rsi]
        emit_lea_rdi_ptr_rdi_8_times_rsi();

        emit_store_object();
    }

    void eval(OslPrintStmt* stmt) {
        // This function emits assembly that actually prints a variable to stdout.
        // It does everything "in the store", without touching the stack. In
        // fact, the print statement only supports printing variables, not
        // arbitrary expressions.
        // 
        // This function is type-aware.
        //
        // If var x is a string, it actually starts printing, char by char, the
        // actual string. It prints "len" bytes (as specificed in the store).
        // That is, this will NOT stop when there are null bytes (allowing
        // attackers to leak addresses, etc.)
        //
        // If var x is an integer, it's more complicated. Most of this
        // complication is to have a chance to add another intended bug.  So,
        // it works in this way: the integer number is converted -- in place --
        // to decimal ascii. The ascii number is then printed (via syscall).
        // Then, the original integer is restored in the store. It only
        // supports positive numbers; for negative numbers, their unsigned
        // representation will be printed instead. The bug is that, for big
        // numbers, the in-place conversion will overflow to the len field,
        // making it different than zero. Which means that, next time this
        // variable will be used, it will be interpreted as a string.  This
        // gives the attacker an arbitrary read primitive:
        // - x = <address> (which is a big number)
        // - print x (treated as int => triggers conversion, and len overwrite)
        // - print x (treated as str => it starts reading len bytes starting from <address>)

        OslVar* var = stmt->var;
        int8_t var_idx = get_var_idx(var);

        emit_mov_rdi_x_64((uint64_t) DATA_PTR);
        emit_mov_rsi_x_32(var_idx);
        // mov rdi, qword ptr [rdi + 8*rsi]
        emit_lea_rdi_ptr_rdi_8_times_rsi();

        /* This is the code:
            // this code assumes rdi points to the right location in the store
            entrypoint:
                mov rsi, [rdi+8]
                test rsi, rsi
                je type_int

                // treat it as a str
                // r8 <= curr_ptr to the string, r9 <= rem_size
                mov r8, qword ptr [rdi]
                mov r9, rsi
                call start_print_str
                jmp exit

            type_int:
                // prepare the string in place

                // r10 = original ptr to buffer
                mov r10, rdi
                // r11 = original number
                mov r11, qword ptr [rdi]
                // rcx = how many chars
                xor rcx, rcx
                // rdi is the "current ptr to buffer"

                // init setup for division
                mov rax, r11
                mov rbx, 10

            do_div:
                // this divides rdx:rax by rbx
                // result: rax=quotient, rdx=remainder
                xor rdx, rdx
                div rbx

                add dl, 0x30
                mov byte ptr [rdi], dl
                inc rdi
                inc rcx

                test rax, rax
                jne do_div

                // invert the string in place
                // assumption: rcx = length
                mov rdi, r10
                mov rsi, rdi
                add rsi, rcx
                dec rsi
            do_inv:
                mov al, byte ptr [rdi]
                xchg al, byte ptr [rsi]
                mov byte ptr [rdi], al
                inc rdi
                dec rsi
                cmp rdi, rsi
                // if rdi < rsi, keep looping
                jl do_inv

                // setup to start_print_str
                mov r8, r10
                mov r9, rcx
                call start_print_str

                // restore the number
                mov qword ptr [r10], r11

                // alright, we are done
                jmp exit

            start_print_str:
                // This is a generic function to print a string + "\n"
                // It expects r8 = &str and r9 = len(str)
                mov rsi, r8
                mov rdi, 1
                mov rdx, r9
                mov rax, 1
                syscall

            print_newline:
                xor rax, rax
                mov al, 10
                push rax
                mov rsi, rsp
                mov rdi, 1
                mov rdx, 1
                mov rax, 1
                syscall
                pop rax
                ret

            exit:
        */

        memcpy(&m_jitted_code[m_jitted_code_idx], "\x48\x8b\x77\x08\x48\x85\xf6\x74\x10\x4c\x8b\x07\x49\x89\xf1\xe8\x5b\x00\x00\x00\xe9\x8e\x00\x00\x00\x49\x89\xfa\x4c\x8b\x1f\x48\x31\xc9\x4c\x89\xd8\x48\xc7\xc3\x0a\x00\x00\x00\x48\x31\xd2\x48\xf7\xf3\x80\xc2\x30\x88\x17\x48\xff\xc7\x48\xff\xc1\x48\x85\xc0\x75\xea\x4c\x89\xd7\x48\x89\xfe\x48\x01\xce\x48\xff\xce\x8a\x07\x86\x06\x88\x07\x48\xff\xc7\x48\xff\xce\x48\x39\xf7\x7c\xef\x4d\x89\xd0\x49\x89\xc9\xe8\x05\x00\x00\x00\x4d\x89\x1a\xeb\x38\x4c\x89\xc6\x48\xc7\xc7\x01\x00\x00\x00\x4c\x89\xca\x48\xc7\xc0\x01\x00\x00\x00\x0f\x05\x48\x31\xc0\xb0\x0a\x50\x48\x89\xe6\x48\xc7\xc7\x01\x00\x00\x00\x48\xc7\xc2\x01\x00\x00\x00\x48\xc7\xc0\x01\x00\x00\x00\x0f\x05\x58\xc3", 167);

        // patch the fd for the sys_write with m_output_fd
        m_jitted_code[m_jitted_code_idx+117] = m_output_fd;
        m_jitted_code[m_jitted_code_idx+145] = m_output_fd;

        m_jitted_code_idx += 167;
    }

    void evalPrintLogStmt() {
        // Dump the log.
        //
        // Intended bug: if this is a relative path, the absolute path is
        // computed starting from /, while the previous code (that does all the
        // security checks) use /aoool/var/log/ as base.  So the player's
        // "malicious" log_fp can arrive here and somehow still point to the
        // flag.

        // Note: it's important to copy log_fp by reference, because otherwise
        // we reference a variable local to this method.
        auto& log_fp = m_req_h.log_fp;
        DLOG("dumping %s to stdout\n", log_fp.c_str());

        emit_mov_rdi_x_64((uint64_t) &log_fp);

        /*
           This is the code:

        entrypoint:
           // r12 <= stores ptr to std::string
           mov r12, rdi
           // save rbp on the stack
           push rbp
           // rbp <= where the stack should be at the end
           mov rbp, rsp

           // rdi <= stores ptr to the actual str
           mov rdi, qword ptr [r12]
           // check if log_fp starts with /
           xor rax, rax
           mov al, byte ptr [rdi]
           cmp al, 0x2f
           je dump_log

           // create_abs_path, relative to /
           // rcx <= size of original log_fp + 1 (for \0)
           mov rcx, qword ptr [r12+8]
           inc rcx
           mov rdx, rcx
           // add space for an extra / and \0
           add rdx, 2
           // round rcx to the next mul of 8
           or rdx, 0x7
           inc rdx

           // allocate space on the stack for the new string
           sub rsp, rdx

           // rsi <= ptr to new location on stac
           mov rsi, rsp

           // init with /
           mov byte ptr [rsi], 0x2f
           inc rsi
        copy_str:
           xor rax, rax
           mov al, byte ptr [rdi]
           mov byte ptr [rsi], al
           inc rdi
           inc rsi
           dec rcx
           jne copy_str

           // make rdi point to the new string
           mov rdi, rsp

        dump_log:
            // open('rsp', 'O_RDONLY', 0)
            // O_RDONLY
            xor rsi, rsi
            // rdx = 0
            cdq
            // SYS_open
            mov rax, 2
            syscall

            // r12 <= fd
            mov r12, rax
            // rsp <= buf
            // note: this is the JIT stack, not the real one. We don't have much space.
            sub rsp, 0x20

        read_write_loop:
            // read('r12', 'rsp', 0x20)

            // rdx <= n
            mov rdx, 0x20

            // rsi <= buf on stack
            mov rsi, rsp

            // rdi <= fd
            mov rdi, r12

            // SYS_read
            xor rax, rax
            syscall

            // an error occurred
            cmp rax, -1
            je epilog

            // write(output_fd, 'rsp', 'rax')
            // rdx <= read_n
            mov rdx, rax

            // rdi <= stdout
            mov rdi, 66

            // SYS_write
            xor rax, rax
            inc rax
            syscall

            cmp rdx, 0x20
            je read_write_loop

        close_fd:
            // rdi <= fd
            mov rdi, r12
            // SYS_close
            mov rax, 3
            syscall

        epilog:
            leave
        */

        memcpy(&m_jitted_code[m_jitted_code_idx], "\x49\x89\xfc\x55\x48\x89\xe5\x49\x8b\x3c\x24\x48\x31\xc0\x8a\x07\x3c\x2f\x74\x37\x49\x8b\x4c\x24\x08\x48\xff\xc1\x48\x89\xca\x48\x83\xc2\x02\x48\x83\xca\x07\x48\xff\xc2\x48\x29\xd4\x48\x89\xe6\xc6\x06\x2f\x48\xff\xc6\x48\x31\xc0\x8a\x07\x88\x06\x48\xff\xc7\x48\xff\xc6\x48\xff\xc9\x75\xee\x48\x89\xe7\x48\x31\xf6\x99\x48\xc7\xc0\x02\x00\x00\x00\x0f\x05\x49\x89\xc4\x48\x83\xec\x20\x48\xc7\xc2\x20\x00\x00\x00\x48\x89\xe6\x4c\x89\xe7\x48\x31\xc0\x0f\x05\x48\x83\xf8\xff\x74\x24\x48\x89\xc2\x48\xc7\xc7\x42\x00\x00\x00\x48\x31\xc0\x48\xff\xc0\x0f\x05\x48\x83\xfa\x20\x74\xd0\x4c\x89\xe7\x48\xc7\xc0\x03\x00\x00\x00\x0f\x05\xc9", 156);

        // patch the fd for the sys_write with m_output_fd
        m_jitted_code[m_jitted_code_idx+125] = m_output_fd;
        m_jitted_code_idx += 156;
    }

    void eval(OslDelStmt* stmt) {
        // This function emits assembly that "deletes" a variable.

        OslVar* var = stmt->var;
        int8_t var_idx = get_var_idx(var);

        m_vars.erase(var->name);

        emit_mov_rdi_x_64((uint64_t) DATA_PTR);
        emit_mov_rsi_x_32(var_idx);
        // mov rdi, qword ptr [rdi + 8*rsi]
        emit_lea_rdi_ptr_rdi_8_times_rsi();

        /* The del checks the type and mark the store area appropriately as freed
            - if type == int: p[0] = ~(1 << 63)
            - if type == str: p[1] = 0

           Intended bug:
              - if the var was a str, the eval of a variable that points there will be treated as an int (containing the address of the previous str)
              - possible exploit: x = "abc"; del x; echo x; => this will leak the address of "abc"
              - also: the second (leftover) chunk from the str will be considered as "allocable"

            Note: this "del" policy is compatible with the "get a free cell"
            algorithm, in the sense that a memory area that has been del-ed,
            would be actually considered as "free".

            This is the code:

            entrypoint:
                mov rsi, qword ptr [rdi+8]
                test rsi, rsi
                je type_int

                // treat it as a str
                // p[1] = 0
                mov qword ptr [rdi+8], 0
                jmp exit

            type_int:
                // p[0] = ~(1 << 63)
                mov qword ptr [rdi], 1
                shl qword ptr [rdi], 63
                dec qword ptr [rdi]

            exit:

        */

        memcpy(&m_jitted_code[m_jitted_code_idx], "\x48\x8b\x77\x08\x48\x85\xf6\x74\x0a\x48\xc7\x47\x08\x00\x00\x00\x00\xeb\x0e\x48\xc7\x07\x01\x00\x00\x00\x48\xc1\x27\x3f\x48\xff\x0f", 33);
        m_jitted_code_idx += 33;
    }

    void eval(OslExpr* expr) {
        switch(expr->type) {
            case OslIntExprType:
            case OslStrExprType:
                eval(expr->value);
                break;
            case OslVarExprType:
                eval(expr->var);
                break;
            case OslAddExprType: {
                // Intended bug (for this and other operations): the code
                // assumes that the object on top of the stack is/are integers.
                // It does not do type checking.

                eval(expr->right);
                eval(expr->left);

                emit_mov_rdi_ptr_rsp();
                emit_mov_rsi_ptr_rsp_offset_x(16);
                emit_add_rdi_rsi();

                // push result
                // intended bug: keep the type/len of the second argument:
                // 2 + "abcd" will result in another string object, pointing to "cd"
                emit_add_rsp_x(24);
                emit_push_rdi();
                break;
            }
            case OslSubExprType: {
                eval(expr->right);
                eval(expr->left);

                emit_mov_rdi_ptr_rsp();
                emit_mov_rsi_ptr_rsp_offset_x(16);
                emit_sub_rdi_rsi();

                // push result
                // same intended bug as above
                emit_add_rsp_x(24);
                emit_push_rdi();
                break;
            }
            case OslMulExprType: {
                eval(expr->right);
                eval(expr->left);

                emit_mov_rax_ptr_rsp();
                emit_mov_rsi_ptr_rsp_offset_x(16);
                emit_mul_rsi();

                // push result
                // same intended bug as above
                emit_add_rsp_x(24);
                emit_push_rax();
                break;
            }
            case OslMinusExprType: {
                eval(expr->left);
                
                // push int 0
                emit_push_0();
                emit_push_0();

                emit_mov_rdi_ptr_rsp();
                emit_mov_rsi_ptr_rsp_offset_x(16);
                emit_sub_rdi_rsi();

                // push result
                // same intended bug as above
                emit_add_rsp_x(24);
                emit_push_rdi();
                break;
            }
            case OslUndefExprType:
                throw "undef";
            default: {
                throw "not supported";
            }
        }
    }

    void eval(OslVar* var) {
        // This takes the object from the store and puts it on top of the stack.
        //
        // First: find which spot in the store is reserved for this var.
        // Second: check the type of the object in the store.
        // Third: push 16/32 bytes on the stack, depending whether it's an int or a str.

        uint8_t var_idx = get_var_idx(var);

        emit_mov_rdi_x_64((uint64_t) DATA_PTR);
        emit_mov_rsi_x_32(var_idx);
        // mov rdi, qword ptr [rdi + 8*rsi]
        emit_lea_rdi_ptr_rdi_8_times_rsi();

        emit_load_object();
    }

    void eval(OslValue* value) {
        switch(value->type) {
            case OslIntValueType: {
                uint64_t int_val = static_cast<OslIntValue*>(value)->value;
                
                // we push the (int value, 0) on the stack
                emit_push_0();
                emit_push_x_64(int_val);
                break;
            }
            case OslStrValueType: {
                string* str_val_ptr = &(static_cast<OslStrValue*>(value)->value);

                // we push the 32 bytes of the string objects, as is
                uint64_t* chunks = (uint64_t*) str_val_ptr;
                emit_push_x_64(chunks[3]);
                emit_push_x_64(chunks[2]);
                emit_push_x_64(chunks[1]);
                emit_push_x_64(chunks[0]);
                break;
            }
            default:
                throw "not supported";
        }
    }

    map<string,int8_t> m_vars;
    uint8_t* m_mem;
    uint8_t* m_jitted_code;
    size_t m_jitted_code_idx;
    RequestHandler m_req_h;
    string m_output_fp;
    int m_output_fd;
};

}

#endif // OSLEVAL_H
