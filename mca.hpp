// Heavily modified from https://github.com/DispatchCode/Machine-Code-Analyzer
// Apache License 2.0, https://github.com/DispatchCode/Machine-Code-Analyzer/blob/master/LICENSE

// Usage:
// #define MCA_IMPLEMENTATION
// #include "mca.hpp"

// Available configurations:
// #define MCA_ENABLE_VEX_INFO
// #define MCA_ENABLE_RAW_BYTES

#pragma once
#include <cstdint>
#include <cstring>

namespace mca {

    enum supported_architecture : uint8_t {
        X86 = 1,
        X64 = 2,
    };

    enum prefixes : uint16_t {
        ES    = 1,    // 0x26
        CS    = 2,    // 0x2E
        SS    = 4,    // 0x36
        DS    = 8,    // 0x3E
        FS    = 16,   // 0x64
        GS    = 32,   // 0x65
        OS    = 64,   // 0x66
        AS    = 128,  // 0x67
        REPNE = 256,
        REPE  = 512,
        OP64  = 1024,
        VEX   = 2048,
    };

    enum instruction_feature : uint16_t {
        PREFIX = 1,
        ESCAPE = 2, // 0x0F
        OP     = 4,
        OP3B   = 8,
        MODRM  = 16,
        SIB    = 32,
        REX    = 64,
        DISP   = 128,
        IMM    = 512,
        FPU    = 1024,
    };

    enum jmp_type : uint8_t {
        JCC_SHORT = 1,  // 1-byte JCC
        JCC_FAR   = 2,  // 2-byte JCC, 4bytes imm
        JMP_SHORT = 4,  // 1-byte JMP
        JMP_FAR   = 8,  // 4-byte JMP
    };

#ifdef MCA_ENABLE_VEX_INFO
    struct vex_info {
        struct {
            uint8_t type;
            union {
                struct {
                    uint8_t vex_pp : 2;
                    uint8_t vex_l : 1;
                    uint8_t vex_v : 4;
                    uint8_t vex_r : 1;
                } vexc5b;
                uint8_t val5;
            };
            union {
                struct {
                    uint8_t vex_m : 5;
                    uint8_t vex_b : 1;
                    uint8_t vex_x : 1;
                    uint8_t vex_r : 1;

                    uint8_t vex_pp : 2;
                    uint8_t vex_l : 1;
                    uint8_t vex_v : 4;
                    uint8_t vex_w : 1;
                } vexc4b;
                uint16_t val4;
            };
        };
    };
#endif

    struct instruction {
        uint64_t disp;
        uint64_t imm;
        uint64_t label;

#ifdef MCA_ENABLE_VEX_INFO
        vex_info _vex;
#endif

#ifdef MCA_ENABLE_RAW_BYTES
        uint8_t instr[15];
#endif

        uint8_t prefixes[4];
        uint8_t op;

        union {
            struct {
                uint8_t rm : 3;
                uint8_t reg : 3;
                uint8_t mod : 2;
            } bits;
            uint8_t value;
        } modrm;

        union {
            struct {
                uint8_t rex_b : 1;
                uint8_t rex_x : 1;
                uint8_t rex_r : 1;
                uint8_t rex_w : 1;
            } bits;
            uint8_t value;
        } rex;

        union {
            struct {
                uint8_t  base : 3;
                uint8_t  index : 3;
                uint8_t  scaled : 2;
            } bits;
            uint8_t value;
        } sib;

        uint8_t vex[3];

        int length;
        int disp_len;
        int imm_len;

        uint16_t set_prefix; // bit mask
        uint16_t set_field;
        uint8_t  jcc_type;

        uint8_t vex_cnt;
        uint8_t prefix_cnt;
    };

    int mca_decode(instruction* instr, supported_architecture arch, const uint8_t* data, int offset);

#ifdef MCA_IMPLEMENTATION
    namespace impl {

#if 1 // Table declarations

        enum supported_architecture_ex : uint8_t {
            // using supported_architecture
            ALL = X86 | X64,
        };

        enum imm_byte : uint8_t {
            b    = 1, // byte
            v    = 2, // word, dword or qword (64bit mode), depending on OS attribute
            z    = 3, // word for 16bit OS or dword for 32/64-bit OS
            p    = 4, // 32-bit, 48-bit, or 80-bit pointer, depending on operand-size attribute
            z1   = 6, // word for 16bit OS or dword for 32/64-bit OS
            w    = 7, // word
            wb   = 8, // word, byte
            gr3b = 9, // byte (imm exists only if mod.reg == 0)
            gr3z = 10, // word, dword depending on OS (imm exists only if mod.reg == 0)
        };

        enum op_label : uint8_t {
            j1  = 0x12,
            j2  = 0x22,
            jc1 = 0x11,
            jc2 = 0x21,
        };

        enum modrm : uint8_t {
            OE  = 0x01, // 0x0f
            O66 = 0x02, // 0x66 0x0f
            OF2 = 0x04, // 0xf2 0x0f
            OF3 = 0x08, // 0xf3 0x0f

            P1  = OE,
            P2  = OE  | O66,
            P4  = OE  | OF3,
            P5  = O66 | OF2,
            P6  = OE  | O66 | OF3,
            P7  = OE  | O66 | OF2 | OF3,
            P8  = O66 | OF2 | OF3,

            OP2 = OE  | OF2 | OF3,
            OP3 = OE  | OF2 | O66,
            OP4 = O66 | OF2 | OF3,
            OP5 = OE  | O66,
        };

        // supported_architecture_ex
        static const uint8_t x86_64_prefix[256] = {
            //       00  01  02  03  04  05  06  07  08  09  0A  0B  0C  0D  0E  0F
            /* 00 */ 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  ALL,
            /* 10 */ 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            /* 20 */ 0,  0,  0,  0,  0,  0,  ALL,0,  0,  0,  0,  0,  0,  0,  ALL,0,
            /* 30 */ 0,  0,  0,  0,  0,  0,  ALL,0,  0,  0,  0,  0,  0,  0,  ALL,0,
            /* 40 */ X64,X64,X64,X64,X64,X64,X64,X64,X64,X64,X64,X64,X64,X64,X64,X64, // REX prefixes
            /* 50 */ 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            /* 60 */ 0,  0,  0,  0,  ALL,ALL,ALL,ALL,0,  0,  0,  0,  0,  0,  0,  0,
            /* 70 */ 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            /* 80 */ 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            /* 90 */ 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            /* A0 */ 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            /* B0 */ 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            /* C0 */ 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            /* D0 */ 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            /* E0 */ 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            /* F0 */ ALL,0,ALL,  ALL,0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
        };

        static const uint8_t modrm_1b[256] = {
            //      00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
            /* 00 */ 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
            /* 10 */ 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
            /* 20 */ 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
            /* 30 */ 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
            /* 40 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 50 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 60 */ 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0,
            /* 70 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 80 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            /* 90 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* A0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* B0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* C0 */ 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
            /* D0 */ 1, 1, 1, 1, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, // 2 = Coprocessor Escape
            /* E0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* F0 */ 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1
        };

        // imm_byte
        static const uint8_t imm_byte_1b[256] = {
            //      00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
            /* 00 */ 0, 0, 0, 0, b, z, 0, 0, 0, 0, 0, 0, b, z, 0, 0,
            /* 10 */ 0, 0, 0, 0, b, z, 0, 0, 0, 0, 0, 0, b, z, 0, 0,
            /* 20 */ 0, 0, 0, 0, b, z, 0, 0, 0, 0, 0, 0, b, z, 0, 0,
            /* 30 */ 0, 0, 0, 0, b, z, 0, 0, 0, 0, 0, 0, b, z, 0, 0,
            /* 40 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 50 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 60 */ 0, 0, 0, 0, 0, 0, 0, 0, z, z, b, b, 0, 0, 0, 0,
            /* 70 */ b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,
            /* 80 */ b, z, b, b, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 90 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, p, 0, 0, 0, 0, 0,
            /* A0 */ z1,z1,z1,z1, 0, 0, 0, 0, b, z, 0, 0, 0, 0, 0, 0,
            /* B0 */ b, b, b, b, b, b, b, b, v, v, v, v, v, v, v, v,
            /* C0 */ b, b, w, 0, 0, 0, b, z, wb, 0, w, 0, 0, b, 0, 0,
            /* D0 */ 0, 0, 0, 0, b, b, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* E0 */ b, b, b, b, b, b, b, b, z, z, p, b, 0, 0, 0, 0,
            /* F0 */ 0, 0, 0, 0, 0, 0, gr3b, gr3z, 0, 0, 0, 0, 0, 0, 0, 0
        };

        // op_label
        static const uint8_t op1b_labels[256] = {
            //      00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
            /* 00 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 10 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 20 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 30 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 40 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 50 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 60 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 70 */ jc1, jc1, jc1, jc1, jc1, jc1, jc1, jc1, jc1, jc1, jc1, jc1, jc1, jc1, jc1, jc1,
            /* 80 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 90 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* A0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* B0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* C0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* D0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* E0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, j2, 0, j1, 0, 0, 0, 0,
            /* F0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        };

        // modrm
        static const uint8_t modrm_2b[256] = {
            //       00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
            /* 00 */ P1,P1,P1,P1,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 10 */ P7,P7,P7,P2,P2,P2,P6,P2,P1,0, 0, 0, 0, 0, 0, P1,
            /* 20 */ P1,P1,P1,P1,0, 0, 0, 0, P2,P2,P7,P2,P7,P7,P2,P2,
            /* 30 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 40 */ P1,P1,P1,P1,P1,P1,P1,P1,P1,P1,P1,P1,P1,P1,P1,P1,
            /* 50 */ P2,P7,P4,P4,P2,P2,P2,P2,P7,P7,P7,P6,P7,P7,P7,P7,
            /* 60 */ P2,P2,P2,P2,P2,P2,P2,P2,P2,P2,P2,P2,O66,O66,P2,P2,
            /* 70 */ P7,P1,P1,P1,P2,P2,P2,P1,P1,P1, 0, 0,P5,P5,P6,P6,
            /* 80 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 90 */ P1,P1,P1,P1,P1,P1,P1,P1,P1,P1,P1,P1,P1,P1,P1,P1,
            /* A0 */ 0, 0, 0, P1,P1,P1, 0, 0, 0, 0, 0,P1,P1,P1,P1,P1,
            /* B0 */ P1,P1,P1,P1,P1,P1,P1,P1,OF3,P1,P1,P1,P4,P4,P1,P1,
            /* C0 */ P1,P1,P7,P1,P2,P2,P2,P1, 0, 0, 0, 0, 0, 0, 0, 0,
            /* D0 */ P5,P2,P2,P2,P2,P2,P8,P2,P2,P2,P2,P2,P2,P2,P2,P2,
            /* E0 */ P2,P2,P2,P2,P2,P2,P8,P2,P2,P2,P2,P2,P2,P2,P2,P2,
            /* F0 */ OF2,P2,P2,P2,P2,P2,P2,P2,P2,P2,P2,P2,P2,P2,P2, 0
        };

        // imm_byte
        static const uint8_t imm_byte_2b[256] = {
            //      00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
            /* 00 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 10 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 20 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 30 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 40 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 50 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 60 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 70 */ b, b, b, b, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 80 */ z, z, z, z, z, z, z, z, z, z, z, z, z, z, z, z,
            /* 90 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* A0 */ 0, 0, 0, 0, b, 0, 0, 0, 0, 0, 0, 0, b, 0, 0, 0,
            /* B0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, b, 0, 0, 0, 0, 0,
            /* C0 */ 0, 0, b, 0, b, b, b, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* D0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* E0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* F0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        };

        // op_label
        static const uint8_t op2b_labels[256] = {
            //      00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
            /* 00 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 10 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 20 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 30 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 40 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 50 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 60 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 70 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 80 */ jc2, jc2, jc2, jc2, jc2, jc2, jc2, jc2, jc2, jc2, jc2, jc2, jc2, jc2, jc2, jc2,
            /* 90 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* A0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* B0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* C0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* D0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* E0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* F0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        };

        static const uint8_t modreg_3b_38[256] = {
            //      00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
            /* 00 */ P2,P2,P2,P2,P2,P2,P2,P2,P2,P2,P2,P2,O66,O66,O66,O66,
            /* 10 */ O66,0,0,O66,O66,O66,O66,O66,0,0,0,0,O66,O66,O66,0,
            /* 20 */ O66,O66,O66,O66,O66,O66,0,0,O66,O66,O66,O66,O66,O66,O66,O66,
            /* 30 */ O66,O66,O66,O66,O66,O66,O66,O66,O66,O66,O66,O66,O66,O66,O66,O66,
            /* 40 */ O66,O66, 0, 0, 0, O66,O66,O66, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 50 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 60 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 70 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 80 */ O66,O66,O66,0, 0, 0, 0, 0, 0, 0, 0, 0,O66,0,O66,0,
            /* 90 */ O66,O66,O66,O66, 0, 0,O66,O66,O66,O66,O66,O66,O66,O66,O66,O66,
            /* A0 */ 0, 0, 0, 0, 0, 0, O66,O66,O66,O66,O66,O66,O66,O66,O66,O66,
            /* B0 */ 0, 0, 0, 0, 0, 0, O66,O66,O66,O66,O66,O66,O66,O66,O66,O66,
            /* C0 */ 0, 0, 0, 0, 0, 0, 0, 0, O66,O66,O66,O66,O66,O66,O66,O66,
            /* D0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, O66, O66, O66, O66, O66,
            /* E0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, O66, O66, O66, O66, O66,
            /* F0 */ OP3, OP3, OE, 0, 0, OP2, OP4, P7, 0, 0, 0, 0, 0, 0, 0, 0
        };

        static const uint8_t imm_byte_3b_38[256] = {
            //      00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
            /* 00 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 10 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 20 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 30 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 40 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 50 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 60 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 70 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 80 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 90 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* A0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* B0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* C0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* D0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* E0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* F0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        };

        static const uint8_t modreg_3b_3A[256] = {
            //      00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
            /* 00 */ O66, O66, O66, 0, O66, O66, O66, 0, O66, O66, O66, O66,O66 , O66, O66, OP5,
            /* 10 */ 0, 0, 0, 0, O66, O66, O66, O66, O66, O66, 0, 0, 0, O66, 0, 0,
            /* 20 */ O66, O66, O66, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 30 */ 0, 0, 0, 0, 0, 0, 0, 0, O66, O66, 0, 0, 0, 0, 0, 0,
            /* 40 */ O66, O66, O66, 0, O66, 0, O66, 0, 0, 0, O66, O66, O66, 0, 0, 0,
            /* 50 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 60 */ O66, O66, O66, O66, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 70 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 80 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 90 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* A0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* B0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* C0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, O66, 0, 0, 0,
            /* D0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, O66,
            /* E0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* F0 */ OF2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        };

        static const uint8_t imm_byte_3b_3A[256] = {
            //      00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
            /* 00 */ 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1,
            /* 10 */ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0,
            /* 20 */ 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 30 */ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
            /* 40 */ 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0,
            /* 50 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 60 */ 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 70 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 80 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* 90 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* A0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* B0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* C0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
            /* D0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
            /* E0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            /* F0 */ 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        };

        static const uint8_t* const imm_table[4] = { nullptr, imm_byte_2b, imm_byte_3b_38, imm_byte_3b_3A };
        static const uint8_t* const modrm_table[4] = { nullptr, modrm_2b, modreg_3b_38, modreg_3b_3A };

#endif

        static bool mca_check_sib(uint8_t mod, uint8_t rm) {
            return mod < 3 && rm == 4;
        }

        static int mca_displacement_size(uint8_t mod, uint8_t rm) {
            if ((mod == 0x02) || (rm == 0x05 && !mod))
                return 4;
            else if (mod == 0x01)
                return 1;
            return 0;
        }

        static int mca_imm_size(instruction* instr, size_t val, supported_architecture arch) {
            switch (val) {
            case b:
                return 1;
            case v:
                if (arch == X64 && instr->set_prefix & OP64)
                    return 8;
                if (instr->set_prefix & OS)
                    return 2;
                return 4;
            case z:
            case z1:
                if (instr->set_prefix & OS)
                    return 2;
                return 4;
            case p:
                if (instr->set_prefix & OS) {
                    if (arch == X86)
                        return 4;
                    return 8;
                }
                return 6;
            case w:
                return 2;
            case wb:
                return 3;
            case gr3b:
                if (!instr->modrm.bits.reg)
                    return 1;
                return 0;
            case gr3z:
                if (!instr->modrm.bits.reg)
                {
                    if (instr->set_prefix & OS)
                        return 2;
                    return 4;
                }
                return 0;

            default:
                return 0;
            }
        }

        static void mca_decode_modrm(instruction* instr, supported_architecture arch, const uint8_t* start_data, const uint8_t* modrm_table, const uint8_t* imm_table, const uint8_t* jcc_table) {
            size_t val;
            if ((val = modrm_table[instr->op])) {
                instr->set_field |= MODRM;

                if (val == 2) // XXX: Label this constant?
                    instr->set_field |= FPU;

                uint8_t curr = *(start_data + instr->length);

                instr->modrm.value = curr;
                instr->length++;

                uint8_t mod_val = instr->modrm.bits.mod, rm_val = instr->modrm.bits.rm;

                if (mca_check_sib(instr->modrm.bits.mod, instr->modrm.bits.rm)) {
                    instr->set_field |= SIB;

                    instr->sib.value = *(start_data + instr->length);
                    instr->length++;

                    if (instr->sib.bits.base == 0x05) {
                        instr->set_field |= DISP;
                        mod_val = instr->modrm.bits.mod;
                        rm_val = instr->sib.bits.base;
                    }
                }

                instr->disp_len = mca_displacement_size(mod_val, rm_val);
                if (instr->disp_len || instr->set_field & DISP) {
                    memcpy(&instr->disp, (start_data + instr->length), instr->disp_len);
                    instr->length += instr->disp_len;
                    instr->set_field |= DISP;
                }
            }

            instr->imm_len = mca_imm_size(instr, imm_table[instr->op], arch);
            if (instr->imm_len) {
                instr->set_field |= IMM;
                memcpy(&instr->imm, (start_data + instr->length), instr->imm_len);
                instr->length += instr->imm_len;
            }

            uint16_t value = 0;
            if (jcc_table != NULL && (value = jcc_table[instr->op])) {
                switch (value) {
                case j1:
                    instr->jcc_type = JMP_SHORT;
                    break;
                case j2:
                    instr->jcc_type = JMP_FAR;
                    break;
                case jc1:
                    instr->jcc_type = JCC_SHORT;
                    break;
                case jc2:
                    instr->jcc_type = JCC_FAR;
                default:
                    break; // avoid compiler warnings
                }

                // 1-byte
                if (value & 0x10)
                    instr->label = (uint64_t)start_data + ((int8_t)instr->imm) + instr->length;
                // 4-byte
                else
                    instr->label = (uint64_t)start_data + ((int64_t)instr->imm) + instr->length;
            }
        }

        static int mca_decode_2b(instruction* instr, supported_architecture arch, const uint8_t* data_src)
        {
            instr->set_prefix |= ESCAPE;
            uint8_t curr = *(data_src + instr->length);

            if (curr == 0x3A || curr == 0x38)
            {
                instr->set_prefix |= OP3B;

                instr->prefixes[instr->prefix_cnt++] = curr;
                instr->length++;
                instr->op = *(data_src + instr->length);
                instr->length++;

                if (curr == 0x3A)
                    mca_decode_modrm(instr, arch, data_src, modreg_3b_3A, imm_byte_3b_3A, NULL);
                else
                    mca_decode_modrm(instr, arch, data_src, modreg_3b_38, imm_byte_3b_38, NULL);

                return instr->length;
            }

            instr->op = curr;
            instr->length++;

            mca_decode_modrm(instr, arch, data_src, modrm_2b, imm_byte_2b, op2b_labels);

            return instr->length;
        }

        static int mca_vex_size(instruction* instr, supported_architecture arch, const uint8_t* data) {
            uint8_t curr_byte = *(data + instr->length);
            uint8_t next_byte = *(data + instr->length + 1);

            // 3-byte VEX prefix
            if ((arch == X86 && curr_byte == 0xC4 && (next_byte >> 6) == 3) || (arch == X64 && curr_byte == 0xC4))
                return 3;
            // 2-byte VEX prefix
            else if ((arch == X86 && curr_byte == 0xC5 && (next_byte & 0x80)) || (arch == X64 && curr_byte == 0xC5))
                return 2;

            return 0;
        }

        static void mca_vex_decode(instruction* instr, supported_architecture arch, const uint8_t* data, int vex_size) {
            memcpy(instr->vex, (data + instr->length), vex_size);
            instr->vex_cnt += vex_size;
            instr->length += vex_size;

            instr->op = *(data + instr->length);
            instr->length++;

            instr->set_prefix |= VEX;

            if (instr->vex[0] == 0xC5) {
#ifdef MCA_ENABLE_VEX_INFO
                instr->_vex.type = instr->vex[0];
                instr->_vex.val5 = instr->vex[1];
#endif
                mca_decode_modrm(instr, arch, data, modrm_2b, imm_byte_2b, NULL);
            }
            else if (instr->vex[0] == 0xC4) {
#ifdef MCA_ENABLE_VEX_INFO
                instr->_vex.type = instr->vex[0];
                memcpy(&instr->_vex.val4, &instr->vex[1], 2);
#endif
                uint8_t index = instr->vex[1] & 0x3;
                mca_decode_modrm(instr, arch, data, modrm_table[index], imm_table[index], NULL);
            }
            // TODO: XOP, 0x8F
        }

    }

    int mca_decode(instruction* instr, supported_architecture arch, const uint8_t* data, int offset) {
        memset(instr, 0, sizeof(instruction));

        const uint8_t* start_data = (data + offset);
        uint8_t curr = *start_data;

        while (impl::x86_64_prefix[curr] & arch)
        {
            switch (curr) {
            case 0x26:
                instr->set_prefix |= ES;
                break;
            case 0x2E:
                instr->set_prefix |= CS;
                break;
            case 0x36:
                instr->set_prefix |= SS;
                break;
            case 0x3E:
                instr->set_prefix |= DS;
                break;
            case 0x48:
            case 0x49:
                if (arch == X64)
                    instr->set_prefix |= OP64;
                break;
            case 0x64:
                instr->set_prefix |= FS;
                break;
            case 0x65:
                instr->set_prefix |= GS;
                break;
            case 0x66:
                instr->set_prefix |= OS;
                break;
            case 0x67:
                instr->set_prefix |= AS;
                break;
            }

            instr->set_field |= PREFIX;
            instr->prefixes[instr->prefix_cnt] = curr;
            instr->prefix_cnt++;
            instr->length++;

            // Rex prefix
            // TODO: 64-bit mode: IF OP == 90h and REX.B == 1,
            //  then the instruction is XCHG r8, rAX
            if (arch == X64 && (curr >= 0x40 && curr <= 0x4F))
            {
                instr->rex.value = curr;
                instr->set_field |= REX;
            }
            else if (curr == 0x0F)
            {
                impl::mca_decode_2b(instr, arch, start_data);
#ifdef MCA_ENABLE_RAW_BYTES
                memcpy(instr->instr, start_data, instr->length);
#endif
                return instr->length;
            }

            curr = *(start_data + instr->length);
        }

        int vex_size = impl::mca_vex_size(instr, arch, start_data);
        if (vex_size)
            impl::mca_vex_decode(instr, arch, start_data, vex_size);
        else
        {
            instr->length++;
            instr->op = curr;
            impl::mca_decode_modrm(instr, arch, start_data, impl::modrm_1b, impl::imm_byte_1b, impl::op1b_labels);
        }

#ifdef MCA_ENABLE_RAW_BYTES
        memcpy(instr->instr, start_data, instr->length);
#endif

        return instr->length;
    }

#endif // MCA_IMPLEMENTATION
}

