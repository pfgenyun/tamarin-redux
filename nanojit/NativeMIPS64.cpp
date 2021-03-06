/* -*- Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*- */
/* vi: set ts=4 sw=4 expandtab: (add to ~/.vimrc: set modeline modelines=5) */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nanojit.h"
#include "stdint.h"

#if defined FEATURE_NANOJIT && defined NANOJIT_MIPS64

namespace nanojit
{
#ifdef NJ_VERBOSE
    const char *regNames[] = {
        "$zr", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
        "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
        "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
        "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra",

        "$f0",  "$f1",  "$f2",  "$f3",  "$f4",  "$f5",  "$f6",  "$f7",
        "$f8",  "$f9",  "$f10", "$f11", "$f12", "$f13", "$f14", "$f15",
        "$f16", "$f17", "$f18", "$f19", "$f20", "$f21", "$f22", "$f23",
        "$f24", "$f25", "$f26", "$f27", "$f28", "$f29", "$f30", "$f31"
    };

    const char *cname[16] = {
        "f",    "un",   "eq",   "ueq",
        "olt",  "ult",  "ole",  "ule",
        "sf",   "ngle", "seq",  "ngl",
        "lt",   "nge",  "le",   "ngt"
    };

    const char *fname[32] = {
        "resv", "resv", "resv", "resv",
        "resv", "resv", "resv", "resv",
        "resv", "resv", "resv", "resv",
        "resv", "resv", "resv", "resv",
        "s",    "d",    "resv", "resv",
        "w",    "l",    "ps",   "resv",
        "resv", "resv", "resv", "resv",
        "resv", "resv", "resv", "resv",
    };

    const char *oname[64] = {
        "special", "regimm", "j",    "jal",   "beq",      "bne",  "blez",  "bgtz",
        "addi",    "addiu",  "slti", "sltiu", "andi",     "ori",  "xori",  "lui",
        "cop0",    "cop1",   "cop2", "cop1x", "beql",     "bnel", "blezl", "bgtzl",
        "resv",    "resv",   "resv", "resv",  "special2", "jalx", "resv",  "special3",
        "lb",      "lh",     "lwl",  "lw",    "lbu",      "lhu",  "lwr",   "resv",
        "sb",      "sh",     "swl",  "sw",    "resv",     "resv", "swr",   "cache",
        "ll",      "lwc1",   "lwc2", "pref",  "resv",     "ldc1", "ldc2",  "ld",
        "sc",      "swc1",   "swc2", "resv",  "resv",     "sdc1", "sdc2",  "sd",
    };
#endif

    const Register RegAlloc::argRegs[] = { A0, A1, A2, A3, T0, T1, T2, T3 };
    const Register RegAlloc::retRegs[] = { V0, V1 };
    const Register RegAlloc::savedRegs[] = {
        S0, S1, S2, S3, S4, S5, S6, S7,
#ifdef FPCALLEESAVED
        FS0, FS1, FS2, FS3, FS4, FS5, FS6, FS7
#endif
    };

#define USE(x) (void)x
#define BADOPCODE(op) NanoAssertMsgf(false, "unexpected opcode %s", lirNames[op])

    // This function will get will get optimised by the compiler into a known value
    static inline bool isLittleEndian(void)
    {
        const union {
            uint32_t      ival;
            unsigned char cval[4];
        } u = { 1 };
        return u.cval[0] == 1;
    }

    // offsets to most/least significant parts of 64bit data in memory
    // These functions will get optimised by the compiler into a known value
    static inline int mswoff(void) {
        return isLittleEndian() ? 4 : 0;
    }

    static inline int lswoff(void) {
        return isLittleEndian() ? 0 : 4;
    }

    static inline Register mswregpair(Register r) {
        return isLittleEndian() ? r+1 : r;
    }

    static inline Register lswregpair(Register r) {
        return isLittleEndian() ? r : r+1;
    }

    static inline bool isInS16(int64_t imm) {
        return imm >= INT16_MIN && imm <= INT16_MAX; 
    }

    static inline bool isInU16(int64_t imm) {
        return imm <= UINT16_MAX;
    }

    static inline bool isInS32(int64_t imm) {
        return imm >= INT32_MIN && imm <= INT32_MAX; 
    }

    static inline bool isInU32(int64_t imm) {
        return imm <= UINT32_MAX;
    }

// These variables affect the code generator
// They can be defined as constants and the compiler will remove
// the unused paths through dead code elimination
// Alternatively they can be defined as variables which will allow
// the exact code generated to be determined at runtime
//
//  cpu_has_fpu        CPU has fpu
//  cpu_has_movn       CPU has movn
//  cpu_has_cmov       CPU has movf/movn instructions
//  cpu_has_lsdc1      CPU has ldc1/sdc1 instructions
//  cpu_has_lsxdc1     CPU has ldxc1/sdxc1 instructions
//  cpu_has_fpuhazard  hazard between c.xx.xx & bc1[tf]
//
// Currently the values are initialised bases on preprocessor definitions

#ifdef DEBUG
    // Don't allow the compiler to eliminate dead code for debug builds
    #define _CONST
#else
    #define _CONST const
#endif

#if NJ_SOFTFLOAT_SUPPORTED
    _CONST bool cpu_has_fpu = false;
#else
    _CONST bool cpu_has_fpu = true;
#endif

#if (__mips==4 || __mips==32 || __mips==64)
    _CONST bool cpu_has_cmov = true;
#else
    _CONST bool cpu_has_cmov = false;
#endif

#if __mips != 1
    _CONST bool cpu_has_lsdc1 = true;
#else
    _CONST bool cpu_has_lsdc1 = false;
#endif

#if (__mips==32 || __mips==64) && __mips_isa_rev>=2
    _CONST bool cpu_has_lsdxc1 = true;
#else
    _CONST bool cpu_has_lsdxc1 = false;
#endif

#if (__mips==1 || __mips==2 || __mips==3)
    _CONST bool cpu_has_fpuhazard = true;
#else
    _CONST bool cpu_has_fpuhazard = false;
#endif
#undef _CONST

    /* Support routines */

    debug_only (
                // break to debugger when generating code to this address
                static NIns *breakAddr;
                static void codegenBreak(NIns *genAddr)
                {
                    NanoAssert (breakAddr != genAddr);
                }
    )

    // Equivalent to assembler %hi(), %lo()
    uint16_t hi(uint32_t v)
    {
        uint16_t r = v >> 16;
        if ((int16_t)(v) < 0)
            r += 1;
        return r;
    }

    int16_t lo(uint32_t v)
    {
        int16_t r = v;
        return r;
    }

    void Assembler::asm_li32(Register r, int32_t imm)
    {
        // general case generating a full 32-bit load
        ORI(r, r, imm & 0xffff);
        LUI(r, (imm>>16) & 0xffff);
    }

    void Assembler::asm_li(Register r, int32_t imm)
    {
#if !PEDANTIC
        if (isU16(imm)) {
            ORI(r, ZERO, imm);
            return;
        }
        if (isS16(imm)) {
            DADDIU(r, ZERO, imm);
            return;
        }
        if ((imm & 0xffff) == 0) {
            LUI(r, uint32_t(imm) >> 16);
            return;
        }
#endif
        asm_li32(r, imm);
    }

    void Assembler::asm_liAddress(Register r, int64_t imm)
    {
        ORI(r, r, imm & 0xffff);
        DSLL(r, r, 16);
        ORI(r, r, (imm>>16) & 0xffff);
        DSLL(r, r, 16);
        ORI(r, r, (imm>>32) & 0xffff);
        LUI(r, (imm>>48) & 0xffff);
    }

    void Assembler::asm_li64(Register r, int64_t imm)
    {
        if (isInS16(imm)) {
            DADDIU(r, ZERO, imm);
        } else if (isInS32(imm)) {
            ORI(r, r, imm & 0xffff);
            LUI(r, (imm>>16) & 0xffff); 
        } else {
            asm_liAddress(r, imm);
        }
    }

    // 64 bit immediate load to a register pair
    void Assembler::asm_li_d(Register r, int32_t msw, int32_t lsw)
    {
        if (IsFpReg(r)) {
            NanoAssert(cpu_has_fpu);
            // li   $at,lsw         # iff lsw != 0
            // mtc1 $at,$r          # may use $0 instead of $at
            // li   $at,msw         # iff (msw != 0) && (msw != lsw)
            // mtc1 $at,$(r+1)      # may use $0 instead of $at
            if (msw == 0)
                MTHC1(ZERO, r);
            else {
                MTHC1(AT, r);
                // If the MSW & LSW values are different, reload AT
                if (msw != lsw)
                    asm_li(AT, msw);
            }
            if (lsw == 0)
                MTC1(ZERO, r);
            else {
                MTC1(AT, r);
                asm_li(AT, lsw);
            }
        }
        else {
            /*
             * li $r.lo, lsw
             * li $r.hi, msw   # will be converted to move $f.hi,$f.lo if (msw==lsw)
             */
            NanoAssert(false);  // soft fpu simulte
            if (msw == lsw)
                MOVE(mswregpair(r), lswregpair(r));
            else
                asm_li(mswregpair(r), msw);
            asm_li(lswregpair(r), lsw);
        }
    }

    void Assembler::asm_move(Register d, Register s)
    {
        MOVE(d, s);
    }

    // General load/store operation
    void Assembler::asm_ldst(int op, Register rt, int dr, Register rbase)
    {
#if !PEDANTIC
        if (isS16(dr)) {
            if (IsGpReg(rt))
                LDSTGPR(op, rt, dr, rbase);
            else
                LDSTFPR(op, rt, dr, rbase);
            return;
        }
#endif

        // lui AT,hi(d)
        // daddu AT,rbase
        // ldst rt,lo(d)(AT)
        if (IsGpReg(rt))
            LDSTGPR(op, rt, lo(dr), AT);
        else
            LDSTFPR(op, rt, lo(dr), AT);
        DADDU(AT, AT, rbase);
        LUI(AT, hi(dr));
    }

    void Assembler::asm_ldst64(bool store, Register r, int dr, Register rbase)
    {
#if !PEDANTIC
        if (isS16(dr)) {
            if (IsGpReg(r)) {
                LDSTGPR(store ? OP_SD : OP_LD, r,   dr,   rbase);
                return;
            }
            else {
                NanoAssert(cpu_has_fpu);
                // NanoAssert((dr & 7) == 0);
                if (cpu_has_lsdc1 && ((dr & 7) == 0)) {
                    // lsdc1 $fr,dr($rbase)
                    LDSTFPR(store ? OP_SDC1 : OP_LDC1, r, dr, rbase);
                    return;
                }
                else if(isS16(dr+4)) {
                    // lswc1 $fr,  dr+LSWOFF($rbase)
                    // lswc1 $fr+1,dr+MSWOFF($rbase)
                    LDSTFPR(store ? OP_SWC1 : OP_LWC1, r+1, dr+mswoff(), rbase);
                    LDSTFPR(store ? OP_SWC1 : OP_LWC1, r,   dr+lswoff(), rbase);
                    return;
                }
            }
        }
#endif

        if (IsGpReg(r)) {
            // lui   $at,%hi(d)
            // addu  $at,$rbase
            // ldsw  $r,  %lo(d)($at)
            // ldst  $r+1,%lo(d+4)($at)
            LDSTGPR(store ? OP_SD : OP_LD, r,   dr & 0xffff, AT);
            DADDU(AT, AT, rbase);
            LUI(AT, (dr>>16) & 0xffff);
        }
        else {
            NanoAssert(cpu_has_fpu);
            if (cpu_has_lsdxc1) {
                // li     $at,dr
                // lsdcx1 $r,$at($rbase)
                if (store)
                    SDXC1(r, AT, rbase);
                else
                    LDXC1(r, AT, rbase);
                asm_li(AT, dr);
            }
            else if (cpu_has_lsdc1) {
                // lui    $at,%hi(dr)
                // addu   $at,$rbase
                // lsdc1  $r,%lo(dr)($at)
                LDSTFPR(store ? OP_SDC1 : OP_LDC1, r, dr & 0xffff, AT);
                DADDU(AT, AT, rbase);
                LUI(AT, (dr >> 16) & 0xffff);
            }
            else {
                // lui   $at,%hi(d)
                // addu  $at,$rbase
                // lswc1 $r,  %lo(d+LSWOFF)($at)
                // lswc1 $r+1,%lo(d+MSWOFF)($at)
                LDSTFPR(store ? OP_SWC1 : OP_LWC1, r+1, lo(dr+mswoff()), AT);
                LDSTFPR(store ? OP_SWC1 : OP_LWC1, r,   lo(dr+lswoff()), AT);
                DADDU(AT, AT, rbase);
                LUI(AT, (dr >> 16) & 0xffff);
            }
        }
    }

    void Assembler::asm_store_imm64(LIns *value, int dr, Register rbase)
    {
        NanoAssert(value->isImmD() || value->isImmQ());
        NanoAssert(isS16(dr) && isS16(dr+4));
        
        if (value->isImmQ()) {
            SD(AT, dr, rbase);
            asm_li64(AT, value->immQ());
            return;
        }

        int32_t msw = value->immDhi();
        int32_t lsw = value->immDlo();

        // li $at,lsw                   # iff lsw != 0
        // sw $at,off+LSWOFF($rbase)    # may use $0 instead of $at
        // li $at,msw                   # iff (msw != 0) && (msw != lsw)
        // sw $at,off+MSWOFF($rbase)    # may use $0 instead of $at

        if (lsw == 0)
            SW(ZERO, dr+lswoff(), rbase);
        else {
            SW(AT, dr+lswoff(), rbase);
            if (msw != lsw)
                asm_li(AT, lsw);
        }
        if (msw == 0)
            SW(ZERO, dr+mswoff(), rbase);
        else {
            SW(AT, dr+mswoff(), rbase);
            asm_li(AT, msw);
        }
    }

    void Assembler::asm_regarg(ArgType ty, LIns* p, Register r)
    {
        NanoAssert(deprecated_isKnownReg(r));
        if (ty == ARGTYPE_I || ty == ARGTYPE_UI || ty == ARGTYPE_Q) {
            // arg goes in specific register
            if (p->isImmI())
                asm_li64(r, p->immI());
            else if (p->isImmQ())
                asm_li64(r, p->immQ());
            else {
                if (p->isExtant()) {
                    if (!p->deprecated_hasKnownReg()) {
                        // load it into the arg reg
                        int d = findMemFor(p);
                        if (p->isop(LIR_allocp))
                            DADDIU(r, FP, d);
                        else
                            asm_ldst((ty == ARGTYPE_Q )? OP_LD:OP_LW, r, d, FP);
                    }
                    else
                        // it must be in a saved reg
                        MOVE(r, p->deprecated_getReg());
                }
                else {
                    // this is the last use, so fine to assign it
                    // to the scratch reg, it's dead after this point.
                    findSpecificRegFor(p, r);
                }
            }
        }
        else {
            // Other argument types unsupported
            NanoAssert(false);
        }
    }

    void Assembler::asm_stkarg(LIns* arg, int stkd)
    {
        bool isF64 = arg->isD();
        Register rr;
        if (arg->isExtant() && (rr = arg->deprecated_getReg(), deprecated_isKnownReg(rr))) {
            // The argument resides somewhere in registers, so we simply need to
            // push it onto the stack.
            if (!cpu_has_fpu || !isF64) {
                NanoAssert(IsGpReg(rr));
                SD(rr, stkd, SP);
            }
            else {
                NanoAssert(cpu_has_fpu);
                NanoAssert(IsFpReg(rr));
                NanoAssert((stkd & 7) == 0);
                asm_ldst64(true, rr, stkd, SP);
            }
        }
        else {
            // The argument does not reside in registers, so we need to get some
            // memory for it and then copy it onto the stack.
            int d = findMemFor(arg);
            if (!isF64) {
                SD(AT, stkd, SP);
                if (arg->isop(LIR_allocp))
                    DADDIU(AT, FP, d);
                else
                    LD(AT, d, FP);
            }
            else {
                NanoAssert((stkd & 7) == 0);
                SD(AT, stkd,   SP);
                LD(AT, d,      FP);
            }
        }
    }

    // Encode a 64-bit floating-point argument using the appropriate ABI.
    // This function operates in the same way as asm_arg, except that it will only
    // handle arguments where (ArgType)ty == ARGTYPE_D.
    void
    Assembler::asm_arg_64(LIns* arg, Register& r, Register& fr, int& stkd)
    {
        // The stack offset always be at least aligned to 16 bytes.
        NanoAssert((stkd & 7) == 0);
#if NJ_SOFTFLOAT_SUPPORTED
        NanoAssert(arg->isop(LIR_ii2d));
#else
        NanoAssert(cpu_has_fpu);
#endif

        if (stkd < 64) {
            NanoAssert(fr == FA0 || fr == FA1 || fr == FA2 || fr == FA3 || fr == FA4 || fr == FA5 ||
                       fr == FA6 || fr == FA7);
            findSpecificRegFor(arg, fr);
            r = r + 1;
            fr = fr + 1;
        }
        else
            asm_stkarg(arg, stkd);

        stkd += 8;
    }

    /* Required functions */

#define FRAMESIZE        16
#define RA_OFFSET        8
#define FP_OFFSET        0

    void Assembler::asm_store32(LOpcode op, LIns *value, int dr, LIns *base)
    {
        Register rt, rbase;
        getBaseReg2(GpRegs, value, rt, GpRegs, base, rbase, dr);

        switch (op) {
        case LIR_sti:
            asm_ldst(OP_SW, rt, dr, rbase);
            break;
        case LIR_sti2s:
            asm_ldst(OP_SH, rt, dr, rbase);
            break;
        case LIR_sti2c:
            asm_ldst(OP_SB, rt, dr, rbase);
            break;
        default:
            BADOPCODE(op);
        }

        TAG("asm_store32(value=%p{%s}, dr=%d, base=%p{%s})",
            value, lirNames[value->opcode()], dr, base, lirNames[base->opcode()]);
    }

    void Assembler::asm_ui2d(LIns *ins)
    {
        Register fr = deprecated_prepResultReg(ins, FpRegs);
        Register v = findRegFor(ins->oprnd1(), GpRegs);
        Register ft = _allocator.allocTempReg(FpRegs & ~(rmask(fr)));    // allocate temporary register for constant

        // todo: support int value in memory, as per x86
        NanoAssert(deprecated_isKnownReg(v));

        // mtc1       $v,$ft
        // bgez       $v,1f
        //  cvt.d.w $fr,$ft
        // lui       $at,0x41f0    # (double)0x10000000LL = 0x41f0000000000000
        // mtc1    $0,$ft
        // mtc1    $at,$ft+1
        // add.d   $fr,$fr,$ft
        // 1:

        underrunProtect(6*4);   // keep branch and destination together
        NIns *here = _nIns;
        ADD_D(fr,fr,ft);
        MTHC1(AT,ft);
        MTC1(ZERO,ft);
        LUI(AT,0x41f0);
        CVT_D_W(fr,ft);            // branch delay slot
        BGEZ(v,here);
        MTC1(v,ft);

        TAG("asm_ui2d(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_d2i(LIns* ins)
    {
        NanoAssert(cpu_has_fpu);

        Register rr = deprecated_prepResultReg(ins, GpRegs);
        Register sr = findRegFor(ins->oprnd1(), FpRegs);
        // trunc.w.d $sr,$sr
        // mfc1 $rr,$sr
        MFC1(rr,sr);
        TRUNC_W_D(sr,sr);
        TAG("asm_d2i(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_ui2f(LIns *ins) {
        (void)ins;
        NanoAssertMsg(0, "LIR_ui2f not yet supported for this architecture");
    }
    void Assembler::asm_i2f(LIns *ins) {
        (void)ins;
        NanoAssertMsg(0, "LIR_i2f not yet supported for this architecture");
    }
    void Assembler::asm_f2i(LIns *ins) {
        (void)ins;
        NanoAssertMsg(0, "LIR_f2i not yet supported for this architecture");
    }
    void Assembler::asm_f2d(LIns *ins) {
        (void)ins;
        NanoAssertMsg(0, "LIR_f2d not yet supported for this architecture");
    }
    void Assembler::asm_d2f(LIns *ins) {
        (void)ins;
        NanoAssertMsg(0, "LIR_d2f not yet supported for this architecture");
    }
    void Assembler::asm_immf(LIns *ins) {
        (void)ins;
        NanoAssertMsg(0, "LIR_immf not yet supported for this architecture");
    }
    void Assembler::asm_immf4(LIns *ins) {
        (void)ins;
        NanoAssertMsg(0, "LIR_immf4 not yet supported for this architecture");
    }
    void Assembler::asm_f2f4(LIns *ins) {
        (void)ins;
        NanoAssertMsg(0, "LIR_f2f4 not yet supported for this architecture");
    }
    void Assembler::asm_ffff2f4(LIns *ins) {
        (void)ins;
        NanoAssertMsg(0, "LIR_ffff2f4 not yet supported for this architecture");
    }
    void Assembler::asm_f4comp(LIns *ins) {
        (void)ins;
        NanoAssertMsg(0, "LIR_f4comp not yet supported for this architecture");
    }
    void Assembler::asm_condf4(LIns *ins) {
        (void)ins;
        NanoAssertMsg(0, "asm_condf4 not yet supported for this architecture");
    }
    void
        Assembler::asm_load128(LIns* ins)
    {
        (void)ins;
        NanoAssertMsg(0, "asm_load128 not yet supported for this architecture");
    }

    void
        Assembler::asm_store128(LOpcode op, LIns* value, int dr, LIns* base)
    {
        (void)op; (void)value;(void)dr;(void)base;
        NanoAssertMsg(0, "asm_store128 not yet supported for this architecture");
    }

    void Assembler::asm_recip_sqrt(LIns*) {
        NanoAssert(!"not implemented");
    }

    void Assembler::asm_fop(LIns *ins)
    {
        NanoAssert(cpu_has_fpu);
        if (cpu_has_fpu) {
            LIns* lhs = ins->oprnd1();
            LIns* rhs = ins->oprnd2();
            LOpcode op = ins->opcode();

            // rr = ra OP rb

            Register rr = deprecated_prepResultReg(ins, FpRegs);
            Register ra = findRegFor(lhs, FpRegs);
            Register rb = (rhs == lhs) ? ra : findRegFor(rhs, FpRegs & ~rmask(ra));

            switch (op) {
            case LIR_addd: ADD_D(rr, ra, rb); break;
            case LIR_subd: SUB_D(rr, ra, rb); break;
            case LIR_muld: MUL_D(rr, ra, rb); break;
            case LIR_divd: DIV_D(rr, ra, rb); break;
            default:
                BADOPCODE(op);
            }
        }
        TAG("asm_fop(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_neg_abs(LIns *ins)
    {
        NanoAssert(cpu_has_fpu);
        if (cpu_has_fpu) {
            LIns* lhs = ins->oprnd1();
            Register rr = deprecated_prepResultReg(ins, FpRegs);
            Register sr = ( !lhs->isInReg()
                            ? findRegFor(lhs, FpRegs)
                            : lhs->deprecated_getReg() );
            NEG_D(rr, sr);
        }
        TAG("asm_neg_abs(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_immd(LIns *ins)
    {
        int d = deprecated_disp(ins);
        Register rr = ins->deprecated_getReg();

        deprecated_freeRsrcOf(ins);

        if (cpu_has_fpu && deprecated_isKnownReg(rr)) {
            if (d)
                asm_spill(rr, d, 2 /*words*/);
            asm_li_d(rr, ins->immDhi(), ins->immDlo());
        }
        else {
            NanoAssert(d);
            asm_store_imm64(ins, d, FP);
        }
        TAG("asm_immd(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

#ifdef NANOJIT_64BIT
    void
    Assembler::asm_q2i(LIns *ins)
    {
        LIns* a = ins->oprnd1();
        RegisterMask allow = GpRegs;
        Register rr = deprecated_prepResultReg(ins, allow);
        Register ra = a->isInReg() ? a->deprecated_getReg():rr;

        NanoAssert(deprecated_isKnownReg(rr));
        NanoAssert(deprecated_isKnownReg(ra));
        allow &= ~rmask(rr);
        allow &= ~rmask(ra);

        if (ra != rr)
           SLL(rr, ra, 0);

        if (!a->isInReg()) {
           NanoAssert(ra == rr);
           findSpecificRegForUnallocated(a, ra);
        }
    }

    void Assembler::asm_ui2uq(LIns *ins)
    {
        LIns* a = ins->oprnd1();
        RegisterMask allow = GpRegs;
        Register rr = deprecated_prepResultReg(ins, allow);
        Register ra = a->isInReg() ? a->deprecated_getReg():rr;

        NanoAssert(deprecated_isKnownReg(rr));
        NanoAssert(deprecated_isKnownReg(ra));
        allow &= ~rmask(rr);
        allow &= ~rmask(ra);

        if (ins->isop(LIR_ui2uq)) {
            DSRL32(rr, rr, 0);
            DSLL32(rr, ra, 0);  // set upper 32 bits to zero
        } else {
            NanoAssert(ins->isop(LIR_i2q));
            SLL(rr, ra, 0);  // sign extend 32 -> 64
        }

        if (!a->isInReg()) {
           NanoAssert(ra == rr);
           findSpecificRegForUnallocated(a, ra);
        }
        TAG("asm_ui2uq(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }
#endif

    void Assembler::asm_load64(LIns *ins)
    {
        NanoAssert(ins->isD() || ins->isQ());

        if (ins->isQ()) {
            LIns*       base = ins->oprnd1();
            Register    rn = findRegFor(base, GpRegs);
            int         offset = ins->disp();
            Register    dd = prepareResultReg(ins, GpRegs);

            if (isInS16(offset)) {
                LD(dd, offset, rn);
            } else {
                LD(dd, 0, AT);
                DADDU(AT, AT, rn);
                asm_li32(AT, offset); 
            }
            freeResourcesOf(ins);
            TAG("asm_load64(ins=%p{%s})", ins, lirNames[ins->opcode()]);
        
            return;
        }
        if (cpu_has_fpu) {
            Register    dd;
            LIns*       base = ins->oprnd1();
            Register    rn = findRegFor(base, GpRegs);
            int         offset = ins->disp();

            if (ins->isInReg()) {
                dd = prepareResultReg(ins, FpRegs);
            }
            else {
                // If the result isn't already in a register, allocate a temporary
                // register for the result and store it directly into memory.
                NanoAssert(ins->isInAr());
                int d = arDisp(ins);
                dd = _allocator.allocTempReg(FpRegs);
                asm_ldst64(true, dd, d, FP);
            }

            switch (ins->opcode()) {
            case LIR_ldd:
                asm_ldst64(false, dd, offset, rn);
                break;
            case LIR_ldf2d:
                CVT_D_S(dd, dd);
                asm_ldst(OP_LWC1, dd, offset, rn);
                break;
            default:
                NanoAssertMsg(0, "LIR opcode unsupported by asm_load64.");
                break;
            }
        }
        else {
            NanoAssert(ins->isInAr());
            int         d = arDisp(ins);

            LIns*       base = ins->oprnd1();
            Register    rn = findRegFor(base, GpRegs);
            int         offset = ins->disp();

            switch (ins->opcode()) {
            case LIR_ldd:
                SD(AT, d,   FP);
                LD(AT, offset,   rn);
                break;
            case LIR_ldf2d:
                NanoAssertMsg(0, "LIR_ldf2d is not yet implemented for soft-float.");
                break;
            default:
                NanoAssertMsg(0, "LIR opcode unsupported by asm_load64.");
                break;
            }
        }

        freeResourcesOf(ins);
        TAG("asm_load64(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_cond(LIns *ins)
    {
        Register r = deprecated_prepResultReg(ins, GpRegs);
        LOpcode op = ins->opcode();
        LIns *a = ins->oprnd1();
        LIns *b = ins->oprnd2();

        asm_cmp(op, a, b, r);

        TAG("asm_cond(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

#if NJ_SOFTFLOAT_SUPPORTED
    void Assembler::asm_qhi(LIns *ins)
    {
        Register rr = deprecated_prepResultReg(ins, GpRegs);
        LIns *q = ins->oprnd1();
        int d = findMemFor(q);
        LW(rr, d+mswoff(), FP);
        TAG("asm_qhi(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_qlo(LIns *ins)
    {
        Register rr = deprecated_prepResultReg(ins, GpRegs);
        LIns *q = ins->oprnd1();
        int d = findMemFor(q);
        LW(rr, d+lswoff(), FP);
        TAG("asm_qlo(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_qjoin(LIns *ins)
    {
        int d = findMemFor(ins);
        NanoAssert(d && isS16(d));
        LIns* lo = ins->oprnd1();
        LIns* hi = ins->oprnd2();

        Register r = findRegFor(hi, GpRegs);
        SW(r, d+mswoff(), FP);
        r = findRegFor(lo, GpRegs);             // okay if r gets recycled.
        SW(r, d+lswoff(), FP);
        deprecated_freeRsrcOf(ins);             // if we had a reg in use, flush it to mem

        TAG("asm_qjoin(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

#endif

    void Assembler::asm_neg_not(LIns *ins)
    {
        LOpcode op = ins->opcode();
        Register rr = deprecated_prepResultReg(ins, GpRegs);

        LIns* lhs = ins->oprnd1();
        // If this is the last use of lhs in reg, we can re-use result reg.
        // Else, lhs already has a register assigned.
        Register ra = !lhs->isInReg() ? findSpecificRegFor(lhs, rr) : lhs->deprecated_getReg();
        if (op == LIR_noti)
            NOT(rr, ra);
        else
            NEGU(rr, ra);
        TAG("asm_neg_not(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_immi(LIns *ins)
    {
        Register rr = deprecated_prepResultReg(ins, GpRegs);
        asm_li(rr, ins->immI());
        TAG("asm_immi(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_cmov(LIns *ins)
    {
        LIns* condval = ins->oprnd1();
        LIns* iftrue  = ins->oprnd2();
        LIns* iffalse = ins->oprnd3();
        RegisterMask allow = ins->isD() ? FpRegs : GpRegs;

        NanoAssert(condval->isCmp());
        NanoAssert((ins->isop(LIR_cmovi) && iftrue->isI() && iffalse->isI()) ||
                   (ins->isop(LIR_cmovq) && iftrue->isQ() && iffalse->isQ()) ||
                   (ins->isop(LIR_cmovd) && iftrue->isD() && iffalse->isD()));

        Register rd = prepareResultReg(ins, allow);

        // Try to re-use the result register for one of the arguments.
        Register rt = iftrue->isInReg() ? iftrue->getReg() : rd;
        Register rf = iffalse->isInReg() ? iffalse->getReg() : rd;
        // Note that iftrue and iffalse may actually be the same, though it
        // shouldn't happen with the LIR optimizers turned on.
        if ((rt == rf) && (iftrue != iffalse)) {
            // We can't re-use the result register for both arguments, so force one
            // into its own register.
            rf = findRegFor(iffalse, allow & ~rmask(rd));
            NanoAssert(iffalse->isInReg());
        }

        if (ins->isI() || ins->isQ()) {
            if (rd == rf)
                MOVN(rd, rt, AT);
            else if (rd == rt)
                MOVZ(rd, rf, AT);
            else {
                MOVZ(rd, rf, AT);
                MOVE(rd, rt);
            }
        } else if (ins->isD()) {
            NanoAssert(cpu_has_fpu);
            if (cpu_has_fpu) {
                if (rd == rf)
                    MOVT_D(rd, rt, 0);
                else if (rd == rt)
                    MOVF_D(rd, rf, 0);
                else {
                    MOVF_D(rd, rf, 0);
                    MOV_D(rd, rt);
                }
            }
        }

        freeResourcesOf(ins);

        // If we re-used the result register, mark it as active for either iftrue
        // or iffalse (or both in the corner-case where they're the same).
        if (rt == rd) {
            NanoAssert(!iftrue->isInReg());
            findSpecificRegForUnallocated(iftrue, rd);
        } else if (rf == rd) {
            NanoAssert(!iffalse->isInReg());
            findSpecificRegForUnallocated(iffalse, rd);
        } else {
            NanoAssert(iffalse->isInReg());
            NanoAssert(iftrue->isInReg());
        }

        asm_cmp(condval->opcode(), condval->oprnd1(), condval->oprnd2(), AT);
        TAG("asm_cmov(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_condd(LIns *ins)
    {
        NanoAssert(cpu_has_fpu);
        if (cpu_has_fpu) {
            Register r = deprecated_prepResultReg(ins, GpRegs);
            LOpcode op = ins->opcode();
            LIns *a = ins->oprnd1();
            LIns *b = ins->oprnd2();

            if (cpu_has_cmov) {
                // c.xx.d  $a,$b
                // li      $r,1
                // movf    $r,$0,$fcc0
                MOVF(r, ZERO, 0);
                ORI(r, ZERO, 1);
            }
            else {
                // c.xx.d  $a,$b
                // [nop]
                // bc1t    1f
                //  li      $r,1
                // move    $r,$0
                // 1:
                NIns *here = _nIns;
                verbose_only(verbose_outputf("%p:", here);)
                underrunProtect(3*4);
                MOVE(r, ZERO);
                ORI(r, ZERO, 1);        // branch delay slot
                BC1T(here);
                if (cpu_has_fpuhazard)
                    NOP();
            }
            asm_cmp(op, a, b, r);
        }
        TAG("asm_condd(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_i2d(LIns *ins)
    {
        NanoAssert(cpu_has_fpu);
        if (cpu_has_fpu) {
            Register fr = deprecated_prepResultReg(ins, FpRegs);
            Register v = findRegFor(ins->oprnd1(), GpRegs);

            // mtc1    $v,$fr
            // cvt.d.w $fr,$fr
            CVT_D_W(fr,fr);
            MTC1(v,fr);
        }
        TAG("asm_i2d(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_ret(LIns *ins)
    {
        genEpilogue();

        releaseRegisters();
        assignSavedRegs();

        LIns *value = ins->oprnd1();
        if (ins->isop(LIR_retq)) {
            findSpecificRegFor(value, V0);
        }
        else {
            NanoAssert(ins->isop(LIR_retd));
#if NJ_SOFTFLOAT_SUPPORTED
            NanoAssert(value->isop(LIR_ii2d));
            findSpecificRegFor(value->oprnd1(), V0); // lo
            findSpecificRegFor(value->oprnd2(), V1); // hi
#else
            findSpecificRegFor(value, FV0);
#endif
        }
        TAG("asm_ret(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_load32(LIns *ins)
    {
        LOpcode op = ins->opcode();
        LIns* base = ins->oprnd1();
        int d = ins->disp();

        Register rres = deprecated_prepResultReg(ins, GpRegs);
        Register rbase = getBaseReg(base, d, GpRegs);

        switch (op) {
        case LIR_lduc2ui:          // 8-bit integer load, zero-extend to 32-bit
            asm_ldst(OP_LBU, rres, d, rbase);
            break;
        case LIR_ldus2ui:          // 16-bit integer load, zero-extend to 32-bit
            asm_ldst(OP_LHU, rres, d, rbase);
            break;
        case LIR_ldc2i:          // 8-bit integer load, sign-extend to 32-bit
            asm_ldst(OP_LB, rres, d, rbase);
            break;
        case LIR_lds2i:          // 16-bit integer load, sign-extend to 32-bit
            asm_ldst(OP_LH, rres, d, rbase);
            break;
        case LIR_ldi:            // 32-bit integer load
            asm_ldst(OP_LW, rres, d, rbase);
            break;
        default:
            BADOPCODE(op);
        }

        TAG("asm_load32(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_param(LIns *ins)
    {
        uint32_t a = ins->paramArg();
        uint32_t kind = ins->paramKind();

        if (kind == 0) {
            // ordinary param
            // first 8 args A0..A7
            if (a < 8) {
                // incoming arg in register
                deprecated_prepResultReg(ins, rmask(RegAlloc::argRegs[a]));
            } else {
                // incoming arg is on stack
                Register r = deprecated_prepResultReg(ins, GpRegs);
                int d = FRAMESIZE + a * sizeof(intptr_t);
                LD(r, d, FP);
            }
        }
        else {
            // saved param
            deprecated_prepResultReg(ins, rmask(RegAlloc::savedRegs[a]));
        }
        TAG("asm_param(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_arith(LIns *ins)
    {
        LOpcode op = ins->opcode();
        LIns* lhs = ins->oprnd1();
        LIns* rhs = ins->oprnd2();

        RegisterMask allow = GpRegs;

        // We always need the result register and the first operand register.
        Register rr = deprecated_prepResultReg(ins, allow);

        // If this is the last use of lhs in reg, we can re-use the result reg.
        // Else, lhs already has a register assigned.
        Register ra = !lhs->isInReg() ? findSpecificRegFor(lhs, rr) : lhs->deprecated_getReg();
        Register rb, t;

        // Don't re-use the registers we've already allocated.
        NanoAssert(deprecated_isKnownReg(rr));
        NanoAssert(deprecated_isKnownReg(ra));
        allow &= ~rmask(rr);
        allow &= ~rmask(ra);

        if (rhs->isImmI()) {
            int32_t rhsc = rhs->immI();
            if (isS16(rhsc)) {
                // MIPS arith immediate ops sign-extend the imm16 value
                switch (op) {
                case LIR_addxovi:
                case LIR_addjovi:
                    // add with overflow result into $at
                    // overflow is indicated by ((sign(rr)^sign(ra)) & (sign(rr)^sign(rhsc))

                    // [move $t,$ra]            if (rr==ra)
                    // addiu $rr,$ra,rhsc
                    // [xor  $at,$rr,$ra]       if (rr!=ra)
                    // [xor  $at,$rr,$t]        if (rr==ra)
                    // [not  $t,$rr]            if (rhsc < 0)
                    // [and  $at,$at,$t]        if (rhsc < 0)
                    // [and  $at,$at,$rr]       if (rhsc >= 0)
                    // srl   $at,$at,31

                    t = _allocator.allocTempReg(allow);
                    SRL(AT, AT, 31);
                    if (rhsc < 0) {
                        AND(AT, AT, t);
                        NOT(t, rr);
                    }
                    else
                        AND(AT, AT, rr);
                    if (rr == ra)
                        XOR(AT, rr, t);
                    else
                        XOR(AT, rr, ra);
                    ADDIU(rr, ra, rhsc);
                    if (rr == ra)
                        MOVE(t, ra);
                    goto done;
                case LIR_addi:
                    ADDIU(rr, ra, rhsc);
                    goto done;
                case LIR_subxovi:
                case LIR_subjovi:
                    // subtract with overflow result into $at
                    // overflow is indicated by (sign(ra)^sign(rhsc)) & (sign(rr)^sign(ra))

                    // [move $t,$ra]            if (rr==ra)
                    // addiu $rr,$ra,-rhsc
                    // [xor  $at,$rr,$ra]       if (rr!=ra)
                    // [xor  $at,$rr,$t]        if (rr==ra)
                    // [and  $at,$at,$ra]       if (rhsc >= 0 && rr!=ra)
                    // [and  $at,$at,$t]        if (rhsc >= 0 && rr==ra)
                    // [not  $t,$ra]            if (rhsc < 0 && rr!=ra)
                    // [not  $t,$t]             if (rhsc < 0 && rr==ra)
                    // [and  $at,$at,$t]        if (rhsc < 0)
                    // srl   $at,$at,31
                    if (isS16(-rhsc)) {
                        t = _allocator.allocTempReg(allow);
                        SRL(AT,AT,31);
                        if (rhsc < 0) {
                            AND(AT, AT, t);
                            if (rr == ra)
                                NOT(t, t);
                            else
                                NOT(t, ra);
                        }
                        else {
                            if (rr == ra)
                                AND(AT, AT, t);
                            else
                                AND(AT, AT, ra);
                        }
                        if (rr == ra)
                            XOR(AT, rr, t);
                        else
                            XOR(AT, rr, ra);
                        ADDIU(rr, ra, -rhsc);
                        if (rr == ra)
                            MOVE(t, ra);
                        goto done;
                    }
                    break;
                case LIR_subi:
                    if (isS16(-rhsc)) {
                        ADDIU(rr, ra, -rhsc);
                        goto done;
                    }
                    break;
                case LIR_mulxovi:
                case LIR_muljovi:
                case LIR_muli:
                    // FIXME: optimise constant multiply by 2^n
                    // if ((rhsc & (rhsc-1)) == 0)
                    //    SLL(rr, ra, ffs(rhsc)-1);
                    //goto done;
                    break;
                default:
                    break;
                }
            }
            if (isU16(rhsc)) {
                // MIPS logical immediate zero-extend the imm16 value
                switch (op) {
                case LIR_ori:
                    ORI(rr, ra, rhsc);
                    goto done;
                case LIR_andi:
                    ANDI(rr, ra, rhsc);
                    goto done;
                case LIR_xori:
                    XORI(rr, ra, rhsc);
                    goto done;
                default:
                    break;
                }
            }

            // LIR shift ops only use last 5bits of shift const
            switch (op) {
            case LIR_lshi:
                SLL(rr, ra, rhsc&31);
                goto done;
            case LIR_rshui:
                SRL(rr, ra, rhsc&31);
                goto done;
            case LIR_rshi:
                SRA(rr, ra, rhsc&31);
                goto done;
            default:
                break;
            }
        }

        // general case, put rhs in register
        rb = (rhs == lhs) ? ra : findRegFor(rhs, allow);
        NanoAssert(deprecated_isKnownReg(rb));
        allow &= ~rmask(rb);

        // The register allocator will have set up one of these 4 cases
        // rr==ra && ra==rb              r0 = r0 op r0
        // rr==ra && ra!=rb              r0 = r0 op r1
        // rr!=ra && ra==rb              r0 = r1 op r1
        // rr!=ra && ra!=rb && rr!=rb    r0 = r1 op r2
        NanoAssert(ra == rb || rr != rb);

        switch (op) {
            case LIR_addxovi:
            case LIR_addjovi:
                t = _allocator.allocTempReg(allow);
                XORI(AT, AT, 1);
                SLTIU(AT, AT, 1);
                XOR(AT, AT, rr);
                DADDU(AT, rr == ra ? t : ra, rb);
                ADDU(rr, ra, rb);
                if (rr == ra)
                    MOVE(t, ra);
                goto done;
            case LIR_addjovq:
                // add with overflow result into $at
                // overflow is indicated by (sign(rr)^sign(ra)) & (sign(rr)^sign(rb))

                // [move $t,$ra]        if (rr==ra)
                // addu  $rr,$ra,$rb
                // ; Generate sign($rr)^sign($ra)
                // [xor  $at,$rr,$t]    sign($at)=sign($rr)^sign($t) if (rr==ra)
                // [xor  $at,$rr,$ra]   sign($at)=sign($rr)^sign($ra) if (rr!=ra)
                // ; Generate sign($rr)^sign($rb) if $ra!=$rb
                // [xor  $t,$rr,$rb]    if (ra!=rb)
                // [and  $at,$t]        if (ra!=rb)
                // srl   $at,31

                t = ZERO;
                if (rr == ra || ra != rb)
                    t = _allocator.allocTempReg(allow);
                DSRL32(AT, AT, 31);
                if (ra != rb) {
                    AND(AT, AT, t);
                    XOR(t, rr, rb);
                }
                if (rr == ra)
                    XOR(AT, rr, t);
                else
                    XOR(AT, rr, ra);
                DADDU(rr, ra, rb);
                if (rr == ra)
                    MOVE(t, ra);
                break;
            case LIR_addi:
                ADDU(rr, ra, rb);
                break;
            case LIR_addq:
                DADDU(rr, ra, rb);
                break;
            case LIR_andi:
                AND(rr, ra, rb);
                break;
            case LIR_ori:
                OR(rr, ra, rb);
                break;
            case LIR_orq:
                OR(rr, ra, rb);
                break;
            case LIR_xori:
            case LIR_xorq:
                XOR(rr, ra, rb);
                break;
            case LIR_subxovi:
            case LIR_subjovi:
                // subtract with overflow result into $at
                // overflow is indicated by (sign(ra)^sign(rb)) & (sign(rr)^sign(ra))

                // [move $t,$ra]        if (rr==ra)
                // ; Generate sign($at)=sign($ra)^sign($rb)
                // xor   $at,$ra,$rb
                // subu  $rr,$ra,$rb
                // ; Generate sign($t)=sign($rr)^sign($ra)
                // [xor  $t,$rr,$ra]    if (rr!=ra)
                // [xor  $t,$rr,$t]     if (rr==ra)
                // and   $at,$at,$t
                // srl   $at,$at,31

                if (ra == rb) {
                    // special case for (ra == rb) which can't overflow
                    MOVE(AT, ZERO);
                    SUBU(rr, ra, rb);
                }
                else {
                    t = _allocator.allocTempReg(allow);
                    SRL(AT, AT, 31);
                    AND(AT, AT, t);
                    if (rr == ra)
                        XOR(t, rr, t);
                    else
                        XOR(t, rr, ra);
                    SUBU(rr, ra, rb);
                    XOR(AT, ra, rb);
                    if (rr == ra)
                        MOVE(t, ra);
                }
                break;
            case LIR_subi:
                SUBU(rr, ra, rb);
                break;
            case LIR_subq:
                DSUBU(rr, ra, rb);
                break;
            case LIR_lshi:
                // SLLV uses the low-order 5 bits of rb for the shift amount so no masking required
                SLLV(rr, ra, rb);
                break;
            case LIR_lshq:
                // SLLV uses the low-order 5 bits of rb for the shift amount so no masking required
                DSLLV(rr, ra, rb);
                break;
            case LIR_rshi:
                // SRAV uses the low-order 5 bits of rb for the shift amount so no masking required
                SRAV(rr, ra, rb);
                break;
            case LIR_rshq:
                DSRAV(rr, ra, rb);
                break;
            case LIR_rshui:
                SRLV(rr, ra, rb);
                break;
            case LIR_rshuq:
                // SRLV uses the low-order 5 bits of rb for the shift amount so no masking required
                DSRLV(rr, ra, rb);
                break;
            case LIR_mulxovi:
            case LIR_muljovi:
                t = _allocator.allocTempReg(allow);
                // Overflow indication required
                // Do a 32x32 signed multiply generating a 64 bit result
                // Compare bit31 of the result with the high order bits
                // mult $ra,$rb
                // mflo $rr             # result to $rr
                // sra  $t,$rr,31       # $t = 0x00000000 or 0xffffffff
                // mfhi $at
                // xor  $at,$at,$t      # sets $at to nonzero if overflow
                XOR(AT, AT, t);
                MFHI(AT);
                SRA(t, rr, 31);
                MFLO(rr);
                MULT(ra, rb);
                break;
            case LIR_muli:
                MUL(rr, ra, rb);
                break;
            case LIR_andq:
                AND(rr, ra, rb);
                break;
            default:
                BADOPCODE(op);
        }
    done:
        TAG("asm_arith(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_store64(LOpcode op, LIns *value, int dr, LIns *base)
    {
        // NanoAssert((dr & 7) == 0);
#ifdef NANOJIT_64BIT
        NanoAssert (op == LIR_stq || op == LIR_std2f || op == LIR_std);
#else
        NanoAssert (op == LIR_std2f || op == LIR_std);
#endif

        switch (op) {
            case LIR_stq:
                if (1) {
                    Register rt, rbase;
                    getBaseReg2(GpRegs, value, rt, GpRegs, base, rbase, dr);
                    asm_ldst(OP_SD, rt, dr, rbase);
                }
                break;

            case LIR_std:
                if (cpu_has_fpu) {
                    Register rbase = findRegFor(base, GpRegs);

                    if (value->isImmD())
                        asm_store_imm64(value, dr, rbase);
                    else {
                        Register fr = findRegFor(value, FpRegs);
                        asm_ldst64(true, fr, dr, rbase);
                    }
                }
                else {
                    Register rbase = findRegFor(base, GpRegs);
                    // *(uint64_t*)(rb+dr) = *(uint64_t*)(FP+da)

                    int ds = findMemFor(value);

                    // lw $at,ds(FP)
                    // sw $at,dr($rbase)
                    // lw $at,ds+4(FP)
                    // sw $at,dr+4($rbase)
                    SW(AT, dr+4, rbase);
                    LW(AT, ds+4, FP);
                    SW(AT, dr,   rbase);
                    LW(AT, ds,   FP);
                }
                break;

            case LIR_std2f:
                if (cpu_has_fpu) {
                    Register fr = value->isInReg() ? value->getReg() : findRegFor(value, FpRegs);
                    Register rbase = getBaseReg(base, dr, GpRegs);
                    Register ft = _allocator.allocTempReg(FpRegs & ~(rmask(fr)));
                    asm_ldst(OP_SWC1, ft, dr, rbase);
                    CVT_S_D(ft, fr);
                }
                else {
                    TODO(Soft-float implementation of LIR_std2f);
                }
                break;

            default:
                BADOPCODE(op);
                return;
        }

        TAG("asm_store64(value=%p{%s}, dr=%d, base=%p{%s})",
            value, lirNames[value->opcode()], dr, base, lirNames[base->opcode()]);
    }

    bool canRematALU(LIns *ins)
    {
        // Return true if we can generate code for this instruction that neither
        // sets CCs, clobbers an input register, nor requires allocating a register.
        switch (ins->opcode()) {
            case LIR_addi:
            case LIR_subi:
            case LIR_andi:
            case LIR_ori:
            case LIR_xori:
                return ins->oprnd1()->isInReg() && ins->oprnd2()->isImmI();
            default:
                ;
        }
        return false;
    }

    bool RegAlloc::canRemat(LIns* ins)
    {
        return ins->isImmI() || ins->isop(LIR_allocp) || ins->isop(LIR_ldi) || canRematALU(ins);
    }

    void Assembler::asm_restore(LIns *i, Register r)
    {
        int d;
        if (i->isop(LIR_allocp)) {
            d = deprecated_disp(i);
            if (isS16(d))
                DADDIU(r, FP, d);
            else {
                DADDU(r, FP, AT);
                asm_li(AT, d);
            }
        } else if (i->isImmI()) {
            asm_li(r, i->immI());
        } else if (i->isImmQ()) {
            asm_li64(r, i->immQ());
        } else {
            d = findMemFor(i);
            if (IsFpReg(r)) {
                asm_ldst64(false, r, d, FP);
            } else if (i->isI()) {
                asm_ldst(OP_LW, r, d, FP);
            } else {
                NanoAssert(i->isQ());
                asm_ldst(OP_LD, r, d, FP);
            }
        }
        TAG("asm_restore(i=%p{%s}, r=%d)", i, lirNames[i->opcode()], r);
    }

    void Assembler::asm_cmp(LOpcode condop, LIns *a, LIns *b, Register cr)
    {
        RegisterMask allow = isCmpDOpcode(condop) ? FpRegs : GpRegs;
        Register ra = findRegFor(a, allow);
        Register rb = (b==a) ? ra : findRegFor(b, allow & ~rmask(ra));

        // FIXME: Use slti if b is small constant

        /* Generate the condition code */
        switch (condop) {
        case LIR_eqi:
        case LIR_eqq:
            SLTIU(cr,cr,1);
            XOR(cr,ra,rb);
            break;
        case LIR_lti:
        case LIR_ltq:
            SLT(cr,ra,rb);
            break;
        case LIR_gti:
        case LIR_gtq:
            SLT(cr,rb,ra);
            break;
        case LIR_lei:
        case LIR_leq:
            XORI(cr,cr,1);
            SLT(cr,rb,ra);
            break;
        case LIR_gei:
        case LIR_geq:
            XORI(cr,cr,1);
            SLT(cr,ra,rb);
            break;
        case LIR_ltui:
        case LIR_ltuq:
            SLTU(cr,ra,rb);
            break;
        case LIR_gtui:
        case LIR_gtuq:
            SLTU(cr,rb,ra);
            break;
        case LIR_leui:
        case LIR_leuq:
            XORI(cr,cr,1);
            SLTU(cr,rb,ra);
            break;
        case LIR_geui:
        case LIR_geuq:
            XORI(cr,cr,1);
            SLTU(cr,ra,rb);
            break;
        case LIR_eqd:
            C_EQ_D(ra,rb);
            break;
        case LIR_ltd:
            C_LT_D(ra,rb);
            break;
        case LIR_gtd:
            C_LT_D(rb,ra);
            break;
        case LIR_led:
            C_LE_D(ra,rb);
            break;
        case LIR_ged:
            C_LE_D(rb,ra);
            break;
        default:
            debug_only(outputf("%s",lirNames[condop]);)
            TODO(asm_cond);
        }
    }

#define SEG(addr) (uint64_t(addr) & 0xf0000000)
#define SEGOFFS(addr) (uint64_t(addr) & 0x0fffffff)


    // Check that the branch target is in range
    // Generate a trampoline if it isn't
    // Emits the branch delay slot instruction
    NIns* Assembler::asm_branchtarget(NIns * const targ)
    {
        bool inrange;
        NIns *btarg = targ;

        // do initial underrun check here to ensure that inrange test is correct
        // allow
        if (targ)
            underrunProtect(2 * 4);    // branch + delay slot

        // MIPS offsets are based on the address of the branch delay slot
        // which is the next instruction that will be generated
        ptrdiff_t bd = BOFFSET(targ-1);

#if PEDANTIC
        inrange = false;
#else
        inrange = (targ && isS16(bd));
#endif

        // If the branch target is known and in range we can just generate a branch
        // Otherwise generate a branch to a trampoline that will be stored in the
        // literal area
        if (inrange)
            NOP();
        else {
            NIns *tramp = _nSlot;
            if (targ) {
                // Can the target be reached by a jump instruction?
                if (SEG(targ) == SEG(tramp)) {
                    //  [linkedinstructions]
                    //  bxxx trampoline
                    //   nop
                    //  ...
                    // trampoline:
                    //  j targ
                    //   nop

                    underrunProtect(4 * 4);             // keep bxx and trampoline together
                    tramp = _nSlot;
                    NOP();                              // delay slot

                    // NB trampoline code is emitted in the correct order
                    trampJ(targ);
                    trampNOP();                         // trampoline delay slot

                }
                else {
                    
                    if (uint64_t(targ) >> 32 == 0) {
                        underrunProtect(5 * 4);             // keep bxx and trampoline together
    
                        tramp = _nSlot;
                        LUI(RA,(uint64_t(targ)>> 16) & 0xffff);         // delay slot
    
                        // NB trampoline code is emitted in the correct order
                        trampORI(RA, RA, uint64_t(targ) & 0xffff);
                        trampJR(RA);
                        trampNOP();                         // trampoline delay slot
                    } else {
                        underrunProtect(7 * 4);
                        tramp = _nSlot;
                        LUI(RA,(uint64_t(targ)>>32) & 0xffff);

                        trampORI(RA, RA, (uint64_t(targ)>>16) & 0xffff);
                        trampDSLL(RA, RA, 16);
                        trampORI(RA, RA, uint64_t(targ) & 0xffff);
                        trampJR(RA);
                        trampNOP();
                    }
                }
            }
            else {
                // Worst case is bxxx,lui addiu;jr;nop as above
                // Best case is branch to trampoline can be replaced
                // with branch to target in which case the trampoline will be abandoned
                // Fixup handled in nPatchBranch

                underrunProtect(7 * 4);                 // keep bxx and trampoline together
                tramp = _nSlot;
                NOP();                                  // delay slot

                trampNOP();
                trampNOP();
                trampNOP();
                trampNOP();
                trampNOP();

            }
            btarg = tramp;
        }

        return btarg;
    }


    NIns* Assembler::asm_bxx(bool branchOnFalse, LOpcode condop, Register ra, Register rb, NIns * const targ)
    {
        NIns *patch = NULL;
        NIns *btarg = asm_branchtarget(targ);

        if (cpu_has_fpu && isCmpDOpcode(condop)) {
            // c.xx.d $ra,$rb
            // bc1x   btarg
            switch (condop) {
            case LIR_eqd:
                if (branchOnFalse)
                    BC1F(btarg);
                else
                    BC1T(btarg);
                patch = _nIns;
                if (cpu_has_fpuhazard)
                    NOP();
                C_EQ_D(ra, rb);
                break;
            case LIR_ltd:
                if (branchOnFalse)
                    BC1F(btarg);
                else
                    BC1T(btarg);
                patch = _nIns;
                if (cpu_has_fpuhazard)
                    NOP();
                C_LT_D(ra, rb);
                break;
            case LIR_gtd:
                if (branchOnFalse)
                    BC1F(btarg);
                else
                    BC1T(btarg);
                patch = _nIns;
                if (cpu_has_fpuhazard)
                    NOP();
                C_LT_D(rb, ra);
                break;
            case LIR_led:
                if (branchOnFalse)
                    BC1F(btarg);
                else
                    BC1T(btarg);
                patch = _nIns;
                if (cpu_has_fpuhazard)
                    NOP();
                C_LE_D(ra, rb);
                break;
            case LIR_ged:
                if (branchOnFalse)
                    BC1F(btarg);
                else
                    BC1T(btarg);
                patch = _nIns;
                if (cpu_has_fpuhazard)
                    NOP();
                C_LE_D(rb, ra);
                break;
            default:
                BADOPCODE(condop);
                break;
            }
        }
        else {
            // general case
            // s[lg]tu?   $at,($ra,$rb|$rb,$ra)
            // b(ne|eq)z  $at,btarg
            switch (condop) {
            case LIR_eqi:
            case LIR_eqq:
                // special case
                // b(ne|eq)  $ra,$rb,btarg
                if (branchOnFalse)
                    BNE(ra, rb, btarg);
                else {
                    if (ra == rb)
                        B(btarg);
                    else
                        BEQ(ra, rb, btarg);
                }
                patch = _nIns;
                break;
            case LIR_lti:
            case LIR_ltq:
                if (branchOnFalse)
                    BEQ(AT, ZERO, btarg);
                else
                    BNE(AT, ZERO, btarg);
                patch = _nIns;
                SLT(AT, ra, rb);
                break;
            case LIR_gti:
            case LIR_gtq:
                if (branchOnFalse)
                    BEQ(AT, ZERO, btarg);
                else
                    BNE(AT, ZERO, btarg);
                patch = _nIns;
                SLT(AT, rb, ra);
                break;
            case LIR_lei:
            case LIR_leq:
                if (branchOnFalse)
                    BNE(AT, ZERO, btarg);
                else
                    BEQ(AT, ZERO, btarg);
                patch = _nIns;
                SLT(AT, rb, ra);
                break;
            case LIR_gei:
            case LIR_geq:
                if (branchOnFalse)
                    BNE(AT, ZERO, btarg);
                else
                    BEQ(AT, ZERO, btarg);
                patch = _nIns;
                SLT(AT, ra, rb);
                break;
            case LIR_ltui:
            case LIR_ltuq:
                if (branchOnFalse)
                    BEQ(AT, ZERO, btarg);
                else
                    BNE(AT, ZERO, btarg);
                patch = _nIns;
                SLTU(AT, ra, rb);
                break;
            case LIR_gtui:
            case LIR_gtuq:
                if (branchOnFalse)
                    BEQ(AT, ZERO, btarg);
                else
                    BNE(AT, ZERO, btarg);
                patch = _nIns;
                SLTU(AT, rb, ra);
                break;
            case LIR_leui:
            case LIR_leuq:
                if (branchOnFalse)
                    BNE(AT, ZERO, btarg);
                else
                    BEQ(AT, ZERO, btarg);
                patch = _nIns;
                SLTU(AT, rb, ra);
                break;
            case LIR_geui:
            case LIR_geuq:
                if (branchOnFalse)
                    BNE(AT, ZERO, btarg);
                else
                    BEQ(AT, ZERO, btarg);
                patch = _nIns;
                SLTU(AT, ra, rb);
                break;
            default:
                BADOPCODE(condop);
            }
        }
        TAG("asm_bxx(branchOnFalse=%d, condop=%s, ra=%s rb=%s targ=%p)",
            branchOnFalse, lirNames[condop], gpn(ra), gpn(rb), targ);
        return patch;
    }

    NIns* Assembler::asm_branch_ov(LOpcode op, NIns* target)
    {
        USE(op);
        NanoAssert(target != NULL);

        NIns* patch = asm_bxx(true, LIR_eqi, AT, ZERO, target);

        TAG("asm_branch_ov(op=%s, target=%p)", lirNames[op], target);
        return patch;
    }

    Branches Assembler::asm_branch(bool branchOnFalse, LIns *cond, NIns * const targ)
    {
        NanoAssert(cond->isCmp());
        LOpcode condop = cond->opcode();
        RegisterMask allow = isCmpDOpcode(condop) ? FpRegs : GpRegs;
        LIns *a = cond->oprnd1();
        LIns *b = cond->oprnd2();
        Register ra = findRegFor(a, allow);
        Register rb = (b==a) ? ra : findRegFor(b, allow & ~rmask(ra));

        return Branches(asm_bxx(branchOnFalse, condop, ra, rb, targ));
    }

    void Assembler::asm_j(NIns * const targ, bool bdelay)
    {
        if (targ == NULL) {
            // target is unknown - asm_bxx wiill generate tramopline code
            NanoAssert(bdelay);
            (void) asm_bxx(false, LIR_eqi, ZERO, ZERO, targ);
        }
        else if (SEG(targ) == SEG(_nIns)) {
            // target is known and in same segment
            if (bdelay) {
                underrunProtect(2*4);    // j + delay
                NOP();
            }
            J(targ);
        }
        else {
            // target is known but in different segment
            // generate register jump using $ra
            // lui $ra,%hi(targ)
            // ori $ra,%lo(targ) # will be omitted if (targ & 0xffff)==0
            // jr $ra
            //  [nop]
            underrunProtect(8*4); // worst case to prevent underrunProtect from reinvoking asm_j
            if (bdelay)
                NOP();
            JR(RA);
            asm_liAddress(RA, (int64_t)targ);
        }
        TAG("asm_j(targ=%p) bdelay=%d", targ);
    }

    void
    Assembler::asm_spill(Register rr, int d, int8_t nWords)
    {
        USE(nWords);
        NanoAssert(d);
        if (IsFpReg(rr)) {
            NanoAssert(nWords == 2);
            asm_ldst64(true, rr, d, FP);
        }
        else {
            NanoAssert(nWords == 1 || nWords == 2);
            asm_ldst(nWords == 1 ? OP_SW:OP_SD, rr, d, FP);
        }
        TAG("asm_spill(rr=%d, d=%d, quad=%d)", rr, d, nWords == 2);
    }

    RegisterMask 
    RegAlloc::nRegCopyCandidates(Register r, RegisterMask allow) {
        // MIPS doesn't support any GPR<->FPR moves
        if(rmask(r) & GpRegs)
            return allow & GpRegs;
        if(rmask(r) & FpRegs)
            return allow & FpRegs;
        NanoAssert(false); // How did we get here?
        return RegisterMask(0);
    }
    
    void
    Assembler::asm_nongp_copy(Register dst, Register src)
    {
        NanoAssert ((rmask(dst) & FpRegs) && (rmask(src) & FpRegs));
        MOV_D(dst, src);
        TAG("asm_nongp_copy(dst=%d src=%d)", dst, src);
    }

    /*
     * asm_arg will encode the specified argument according to the current ABI, and
     * will update r and stkd as appropriate so that the next argument can be
     * encoded.
     *
     * - doubles are 64-bit aligned.  both in registers and on the stack.
     *   If the next available argument register is A1, it is skipped
     *   and the double is placed in A2:A3.  If A0:A1 or A2:A3 are not
     *   available, the double is placed on the stack, 64-bit aligned.
     * - 32-bit arguments are placed in registers and 32-bit aligned
     *   on the stack.
     */
    void
    Assembler::asm_arg(ArgType ty, LIns* arg, Register& r, Register& fr, int& stkd)
    {
        // The stack offset must always be at least aligned to 8 bytes.
        NanoAssert((stkd & 7) == 0);

        if (ty == ARGTYPE_D) {
            // This task is fairly complex and so is delegated to asm_arg_64.
            asm_arg_64(arg, r, fr, stkd);
        } else {
            NanoAssert(ty == ARGTYPE_I || ty == ARGTYPE_UI || ty == ARGTYPE_Q);
            if (stkd < 64) {
                asm_regarg(ty, arg, r);
                fr = fr + 1;
                r = r + 1;
            }
            else
                asm_stkarg(arg, stkd);
            // The o32 ABI calling convention is that if the first arguments
            // is not a double, subsequent double values are passed in integer registers
            //fr = r;
            stkd += 8;
        }
    }

    void 
    Assembler::asm_pushstate(void)
    {
        SD(RA, 128,  SP);
        SD(FP, 120,  SP);
        SD(FP, 112,  SP);  // sp
        SD(S7, 104,  SP);
        SD(S6, 96,  SP);
        SD(S5, 88,  SP);
        SD(S4, 80,  SP);
        SD(S3, 72,  SP);
        SD(S2, 64,  SP);
        SD(S1, 56,  SP);
        SD(S0, 48,  SP);
        SD(A3, 40, SP);
        SD(A2, 32, SP);
        SD(A1, 24, SP);
        SD(A0, 16, SP);
        SD(V1, 8,  SP);
        SD(V0, 0,  SP);
        DADDIU(SP, SP, -136);
    }

    void 
    Assembler::asm_popstate(void)
    {
        DADDIU(SP, SP, 136);
        LD(RA, 128,  SP);
        LD(FP, 120,  SP);
        LD(FP, 112,  SP);  // sp
        LD(S7, 104,  SP);
        LD(S6, 96,  SP);
        LD(S5, 88,  SP);
        LD(S4, 80,  SP);
        LD(S3, 72,  SP);
        LD(S2, 64,  SP);
        LD(S1, 56,  SP);
        LD(S0, 48,  SP);
        LD(A3, 40, SP);
        LD(A2, 32, SP);
        LD(A1, 24, SP);
        LD(A0, 16, SP);
        LD(V1, 8,  SP);
        LD(V0, 0,  SP);
    }

    void 
    Assembler::asm_savepc(void)
    {
        NanoAssert(false);
    }

    void 
    Assembler::asm_restorepc(void)
    {
        NOP();
        JR(T9);
        LD(T9, -8, SP);
        DADDIU(SP, SP, 16);
    }

    void 
    Assembler::asm_discardpc(void)
    {
        NanoAssert(false);
    }

    void
    Assembler::asm_brsavpc_impl(LIns* flag, NIns* target)
    {
        Register r = findRegFor(flag, GpRegs);
        NIns* skipTarget = _nIns;
        underrunProtect(16 * 4);

        NOP();
        JR(T9);
        asm_liAddress(T9, (int64_t)target);

        SD(RA, 8, SP);
        DADDIU(SP, SP, -16);

        DADDIU(RA, RA, 10 * 4);  // Delayslot
        BGEZAL(ZERO, _nIns+2);  // get pc

        NOP();
        J(skipTarget);
        NOP();
        BNE(r, ZERO, _nIns + 4);
    }

    void Assembler::asm_memfence()
    {}

    void
    Assembler::asm_call(LIns* ins)
    {
        if (!ins->isop(LIR_callv)) {
            Register rr;
            LOpcode op = ins->opcode();

            switch (op) {
            case LIR_calli:
            case LIR_callq:
                rr = RegAlloc::retRegs[0];
                break;
            case LIR_calld:
                NanoAssert(cpu_has_fpu);
                rr = FV0;
                break;
            default:
                BADOPCODE(op);
                return;
            }

            deprecated_prepResultReg(ins, rmask(rr));
        }

        // Do this after we've handled the call result, so we don't
        // force the call result to be spilled unnecessarily.
        evictScratchRegsExcept(0);

        const CallInfo* ci = ins->callInfo();
        ArgType argTypes[MAXARGS];
        uint32_t argc = ci->getArgTypes(argTypes);
        bool indirect = ci->isIndirect();

        // FIXME: Put one of the argument moves into the BDS slot

        underrunProtect(2*4);    // jalr+delay
        NOP();
        JALR(T9);

        if (!indirect)
            // FIXME: If we can tell that we are calling non-PIC
            // (ie JIT) code, we could call direct instead of using t9
            asm_liAddress(T9, ci->_address);
        else
            // Indirect call: we assign the address arg to t9
            // which matches the o32 ABI for calling functions
            asm_regarg(ARGTYPE_P, ins->arg(--argc), T9);

        // Encode the arguments, starting at A0 and with an empty argument stack.
        Register    r = A0, fr = FA0;
        int         stkd = 0;

        // Iterate through the argument list and encode each argument according to
        // the ABI.
        // Note that we loop through the arguments backwards as LIR specifies them
        // in reverse order.
        while(argc--)
            asm_arg(argTypes[argc], ins->arg(argc), r, fr, stkd);

        if (stkd > max_out_args)
            max_out_args = stkd;
        TAG("asm_call(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void
    Assembler::asm_immq(LIns* ins)
    {
        Register rr = deprecated_prepResultReg(ins, GpRegs);
        asm_li64(rr, ins->immQ());
        TAG("asm_immq(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void
    Assembler::asm_q2d(LIns* ins)
    {
        NanoAssert(cpu_has_fpu);
        if (cpu_has_fpu) {
            Register fr = deprecated_prepResultReg(ins, FpRegs);
            Register v = findRegFor(ins->oprnd1(), GpRegs);

            // dmtc1    $v,$fr
            // cvt.d.l $fr,$fr
            CVT_D_L(fr,fr);
            DMTC1(v,fr);
        }
        TAG("asm_q2d(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void
    Assembler::asm_dasq(LIns* ins)
    {
        Register rr = deprecated_prepResultReg(ins, GpRegs);
        Register ra = findRegFor(ins->oprnd1(), FpRegs);
        DMFC1(rr, ra);
        TAG("asm_dasq(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void
    Assembler::asm_qasd(LIns* ins)
    {
        Register rr = deprecated_prepResultReg(ins, FpRegs);
        Register ra = findRegFor(ins->oprnd1(), GpRegs);
        DMTC1(ra, rr);
        TAG("asm_qasd(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    Register
    RegAlloc::nRegisterAllocFromSet(RegisterMask set)
    {
        int n;

        // note, deliberate truncation of 64->32 bits
        if (set & 0xffffffff) {
            // gp reg
            n = ffs(int(set));
            NanoAssert(n != 0);
            n = n - 1;
        }
        else {
            // fp reg
            NanoAssert(cpu_has_fpu);
            n = ffs(int(set >> 32));
            NanoAssert(n != 0);
            n = 32 + n - 1;
        }
        Register r = { n };
        TAG("nRegisterAllocFromSet(set=%016llx) => %s", set, gpn(r));
        return r;
    }

    RegisterMask RegAlloc::nInitManagedRegisters() {
        RegisterMask retval = GpRegs;
        if (cpu_has_fpu)
            retval |= FpRegs;
        return retval;
    }


#define signextend16(s) ((int32_t(s)<<16)>>16)

    void
    Assembler::nPatchBranch(NIns* branch, NIns* target)
    {
        uint32_t op = (branch[0] >> 26) & 0x3f;
        uint32_t bdoffset = target-(branch+1);

        if (op == OP_BEQ || op == OP_BNE ||
            ((branch[0] & 0xfffe0000) == ((OP_COP1 << 26) | (COP1_BC << 21)))) {
            if (isS16(bdoffset)) {
                // The branch is in range, so just replace the offset in the instruction
                // The trampoline that was allocated is redundant and will remain unused
                branch[0] = (branch[0]  & 0xffff0000) | (bdoffset & 0xffff);
            }
            else {
                // The branch is pointing to a trampoline. Find out where that is
                NIns *tramp = branch + 1 + (signextend16(branch[0] & 0xffff));
                if (SEG(branch) == SEG(target)) {
                    *tramp = J_FORMAT(OP_J,JINDEX(target));
                }
                else {
                    // Full 32-bit jump
                    // bxx tramp
                    //  lui $at,(target>>16)>0xffff
                    // ..
                    // tramp:
                    // ori $at,target & 0xffff
                    // jr $at
                    //  nop
                    branch[1] = U_FORMAT(OP_LUI, 0, GPR(AT), ((int64_t)target >> 32) & 0xffff);
                    tramp[0] = U_FORMAT(OP_ORI, GPR(AT), GPR(AT), ((int64_t)target>>16) & 0xffff);
                    tramp[1] = R_FORMAT(OP_SPECIAL, 0, GPR(AT), GPR(AT), 16, SPECIAL_DSLL);
                    tramp[2] = U_FORMAT(OP_ORI, GPR(AT), GPR(AT), (int64_t)target & 0xffff);
                    tramp[3] = R_FORMAT(OP_SPECIAL, GPR(AT), 0, 0, 0, SPECIAL_JR);
                }
            }
        }
        else if (op == OP_J) {
            NanoAssert (SEG(branch) == SEG(target));
            branch[0] = J_FORMAT(OP_J,JINDEX(target));
        }
        else
            TODO(unknown_patch);
        // TAG("nPatchBranch(branch=%p target=%p)", branch, target);
    }

    void
    Assembler::nFragExit(LIns *guard)
    {
        SideExit *exit = guard->record()->exit;
        Fragment *frag = exit->target;
        bool destKnown = (frag && frag->fragEntry);

        // Generate jump to epilogue and initialize lr.

        // If the guard already exists, use a simple jump.
        if (destKnown) {
            // j     _fragEntry
            //  move $v0,$zero
            underrunProtect(2 * 4);     // j + branch delay
            MOVE(V0, ZERO);
            asm_j(frag->fragEntry, false);
        }
        else {
            // Target doesn't exist. Jump to an epilogue for now.
            // This can be patched later.
            if (!_epilogue)
                _epilogue = genEpilogue();
            GuardRecord *lr = guard->record();
            // lui    $v0,%hi(lr)
            // j      _epilogue
            //  addiu $v0,%lo(lr)
            underrunProtect(2 * 4);     // j + branch delay
            ORI(V0, V0, int64_t(lr) & 0xffff);
            asm_j(_epilogue, false);
            DSLL(V0, V0, 16);
            ORI(V0, V0, (int64_t(lr) >> 16) & 0xffff);
            LUI(V0, (int64_t(lr) >> 32) & 0xffff);
            lr->jmp = _nIns;
        }

        // profiling for the exit
        verbose_only(
            if (_logc->lcbits & LC_FragProfile) {
                // lui   $fp,%hi(profCount)
                // lw    $at,%lo(profCount)(fp)
                // addiu $at,1
                // sw    $at,%lo(profCount)(fp)
                // uint32_t profCount = uint32_t(&guard->record()->profCount);
                // SW(AT, lo(profCount), FP);
                // ADDIU(AT, AT, 1);
                // LW(AT, lo(profCount), FP);
                // LUI(FP, hi(profCount));
                uint64_t profCount = uint64_t(&guard->record()->profCount);
                SD(AT, profCount & 0xffff, FP);
                DADDIU(AT, AT, 1);
                LD(AT, profCount & 0xffff, FP);
                DSLL(FP, FP, 16);
                ORI(FP, FP, (profCount >> 16) & 0xffff);
                DSLL(FP, FP, 16);
                ORI(FP, FP, (profCount >> 32) & 0xffff);
                LUI(FP, (profCount >> 48) & 0xffff);
            }
        )

        // Pop the stack frame.
        MOVE(SP, FP);

        // return value is GuardRecord*
        TAG("nFragExit(guard=%p{%s})", guard, lirNames[guard->opcode()]);
    }

    void Assembler::nBeginAssembly()
    {
        max_out_args = 64;        // Always reserve space for a0-a3
    }

    // Increment the 32-bit profiling counter at pCtr, without
    // changing any registers.
    verbose_only(
    void Assembler::asm_inc_m32(uint32_t* /*pCtr*/)
    {
        // TODO: implement this
    }
    )

    void
    Assembler::nativePageReset(void)
    {
        _nSlot = 0;
        _nExitSlot = 0;
        TAG("nativePageReset()");
    }

    void
    Assembler::asm_qbinop(LIns *ins)
    {
        asm_arith(ins);
    }

    void
    Assembler::nativePageSetup(void)
    {
        NanoAssert(!_inExit);
        if (!_nIns)
            codeAlloc(codeStart, codeEnd, _nIns verbose_only(, codeBytes));
        if (!_nExitIns)
            codeAlloc(exitStart, exitEnd, _nExitIns verbose_only(, exitBytes));

        // constpool starts at bottom of page and moves up
        // code starts at top of page and goes down,

        if (!_nSlot)
            _nSlot = codeStart;
        if (!_nExitSlot)
            _nExitSlot = exitStart;

        TAG("nativePageSetup()");
    }


    NIns*
    Assembler::genPrologue(void)
    {
        /*
         * Use a non standard fp because we don't know the final framesize until now
         * addiu   $sp,-FRAMESIZE
         * sw      $ra,RA_OFFSET($sp)
         * sw      $fp,FP_OFFSET($sp)
         * move    $fp,$sp
         * addu    $sp,-stackNeeded
         */

        uint32_t stackNeeded = max_out_args + STACK_GRANULARITY * _activation.stackSlotsNeeded();
        uint32_t amt = alignUp(stackNeeded, NJ_ALIGN_STACK);

        if (amt) {
            if (isS16(-amt))
                DADDIU(SP, SP, -amt);
            else {
                DADDU(SP, SP, AT);
                asm_li(AT, -amt);
            }
        }

        NIns *patchEntry = _nIns; // FIXME: who uses this value and where should it point?

        MOVE(FP, SP);
        SD(FP, FP_OFFSET, SP);
        underrunProtect(2 * 4);         // code page switch could change $ra
        SD(RA, RA_OFFSET, SP);
        DADDIU(SP, SP, -FRAMESIZE);

        TAG("genPrologue()");

        return patchEntry;
    }

    NIns*
    Assembler::genEpilogue(void)
    {
        /*
         * move    $sp,$fp
         * lw      $fp,FP_OFFSET($sp)
         * lw      $ra,RA_OFFSET($sp)
         * j       $ra
         * addiu   $sp,FRAMESIZE
         */
        underrunProtect(3*4);   // lw $ra,RA_OFFSET($sp);j $ra; addiu $sp,FRAMESIZE
        DADDIU(SP, SP, FRAMESIZE);
        JR(RA);
        LD(RA, RA_OFFSET, SP);
        LD(FP, FP_OFFSET, SP);
        MOVE(SP, FP);

        TAG("genEpilogue()");

        return _nIns;
    }

    const RegisterMask PREFER_SPECIAL = ~ ((RegisterMask)0);
    // Init per-opcode register hint table.  Defaults to no hints for all instructions 
    // (initialized to 0 )
    static bool nHintsInit(RegisterMask Hints[])
    {
        VMPI_memset(Hints,0,sizeof(RegisterMask)*LIR_sentinel );
        Hints[LIR_calli]  = rmask(V0);
#if NJ_SOFTFLOAT_SUPPORTED
        Hints[LIR_hcalli] = rmask(V1);
#endif
        Hints[LIR_calld]  = rmask(FV0);
        Hints[LIR_paramp] = PREFER_SPECIAL;
        return true;
    }

    RegisterMask
    RegAlloc::nHint(LIns* ins)
    {
        static RegisterMask  Hints[LIR_sentinel+1]; // effectively const, save for the initialization
        static bool initialized = nHintsInit(Hints); (void)initialized; 
        
        RegisterMask prefer = Hints[ins->opcode()];
        if(prefer != PREFER_SPECIAL) return prefer;

        NanoAssert(ins->isop(LIR_paramp));
        // FIXME: FLOAT parameters?
        if (ins->paramKind() == 0)
            if (ins->paramArg() < 8)
                prefer = rmask(RegAlloc::argRegs[ins->paramArg()]);
        return prefer;
    }

    void
    Assembler::underrunProtect(int bytes)
    {
        NanoAssertMsg(bytes<=LARGEST_UNDERRUN_PROT, "constant LARGEST_UNDERRUN_PROT is too small");
        NanoAssert(_nSlot != 0);
        uintptr_t top = uintptr_t(_nSlot);
        uintptr_t pc = uintptr_t(_nIns);
        if (pc - bytes < top) {
            verbose_only(verbose_outputf("        %p:", _nIns);)
            NIns* target = _nIns;
            codeAlloc(codeStart, codeEnd, _nIns verbose_only(, codeBytes));

            _nSlot = codeStart;

            // _nSlot points to the first empty position in the new code block
            // _nIns points just past the last empty position.
            asm_j(target, true);
        }
    }

    void
    Assembler::swapCodeChunks() {
        if (!_nExitIns)
            codeAlloc(exitStart, exitEnd, _nExitIns verbose_only(, exitBytes));
        if (!_nExitSlot)
            _nExitSlot = exitStart;
        SWAP(NIns*, _nIns, _nExitIns);
        SWAP(NIns*, _nSlot, _nExitSlot);
        SWAP(NIns*, codeStart, exitStart);
        SWAP(NIns*, codeEnd, exitEnd);
        verbose_only( SWAP(size_t, codeBytes, exitBytes); )
    }

    void
    Assembler::asm_insert_random_nop() {
        NanoAssert(0); // not supported
    }

    void
    Assembler::asm_label() {
        // do nothing right now
    }

}

#endif // FEATURE_NANOJIT && NANOJIT_MIPS
