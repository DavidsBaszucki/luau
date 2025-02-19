// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
#include "Luau/AssemblyBuilderX64.h"
#include "Luau/StringUtils.h"

#include "doctest.h"

#include <string.h>

using namespace Luau::CodeGen;

std::string bytecodeAsArray(const std::vector<uint8_t>& bytecode)
{
    std::string result = "{";

    for (size_t i = 0; i < bytecode.size(); i++)
        Luau::formatAppend(result, "%s0x%02x", i == 0 ? "" : ", ", bytecode[i]);

    return result.append("}");
}

class AssemblyBuilderX64Fixture
{
public:
    void check(void (*f)(AssemblyBuilderX64& build), std::vector<uint8_t> code, std::vector<uint8_t> data = {})
    {
        AssemblyBuilderX64 build(/* logText= */ false);

        f(build);

        build.finalize();

        if (build.code != code)
        {
            printf("Expected code: %s\nReceived code: %s\n", bytecodeAsArray(code).c_str(), bytecodeAsArray(build.code).c_str());
            CHECK(false);
        }

        if (build.data != data)
        {
            printf("Expected data: %s\nReceived data: %s\n", bytecodeAsArray(data).c_str(), bytecodeAsArray(build.data).c_str());
            CHECK(false);
        }
    }
};

TEST_SUITE_BEGIN("x64Assembly");

#define SINGLE_COMPARE(inst, ...) \
    check( \
        [](AssemblyBuilderX64& build) { \
            build.inst; \
        }, \
        {__VA_ARGS__})

TEST_CASE_FIXTURE(AssemblyBuilderX64Fixture, "BaseBinaryInstructionForms")
{
    // reg, reg
    SINGLE_COMPARE(add(rax, rcx), 0x48, 0x03, 0xc1);
    SINGLE_COMPARE(add(rsp, r12), 0x49, 0x03, 0xe4);
    SINGLE_COMPARE(add(r14, r10), 0x4d, 0x03, 0xf2);

    // reg, imm
    SINGLE_COMPARE(add(rax, 0), 0x48, 0x83, 0xc0, 0x00);
    SINGLE_COMPARE(add(rax, 0x7f), 0x48, 0x83, 0xc0, 0x7f);
    SINGLE_COMPARE(add(rax, 0x80), 0x48, 0x81, 0xc0, 0x80, 0x00, 0x00, 0x00);
    SINGLE_COMPARE(add(r10, 0x7fffffff), 0x49, 0x81, 0xc2, 0xff, 0xff, 0xff, 0x7f);

    // reg, [reg]
    SINGLE_COMPARE(add(rax, qword[rax]), 0x48, 0x03, 0x00);
    SINGLE_COMPARE(add(rax, qword[rbx]), 0x48, 0x03, 0x03);
    SINGLE_COMPARE(add(rax, qword[rsp]), 0x48, 0x03, 0x04, 0x24);
    SINGLE_COMPARE(add(rax, qword[rbp]), 0x48, 0x03, 0x45, 0x00);
    SINGLE_COMPARE(add(rax, qword[r10]), 0x49, 0x03, 0x02);
    SINGLE_COMPARE(add(rax, qword[r12]), 0x49, 0x03, 0x04, 0x24);
    SINGLE_COMPARE(add(rax, qword[r13]), 0x49, 0x03, 0x45, 0x00);

    SINGLE_COMPARE(add(r12, qword[rax]), 0x4c, 0x03, 0x20);
    SINGLE_COMPARE(add(r12, qword[rbx]), 0x4c, 0x03, 0x23);
    SINGLE_COMPARE(add(r12, qword[rsp]), 0x4c, 0x03, 0x24, 0x24);
    SINGLE_COMPARE(add(r12, qword[rbp]), 0x4c, 0x03, 0x65, 0x00);
    SINGLE_COMPARE(add(r12, qword[r10]), 0x4d, 0x03, 0x22);
    SINGLE_COMPARE(add(r12, qword[r12]), 0x4d, 0x03, 0x24, 0x24);
    SINGLE_COMPARE(add(r12, qword[r13]), 0x4d, 0x03, 0x65, 0x00);

    // reg, [base+imm8]
    SINGLE_COMPARE(add(rax, qword[rax + 0x1b]), 0x48, 0x03, 0x40, 0x1b);
    SINGLE_COMPARE(add(rax, qword[rbx + 0x1b]), 0x48, 0x03, 0x43, 0x1b);
    SINGLE_COMPARE(add(rax, qword[rsp + 0x1b]), 0x48, 0x03, 0x44, 0x24, 0x1b);
    SINGLE_COMPARE(add(rax, qword[rbp + 0x1b]), 0x48, 0x03, 0x45, 0x1b);
    SINGLE_COMPARE(add(rax, qword[r10 + 0x1b]), 0x49, 0x03, 0x42, 0x1b);
    SINGLE_COMPARE(add(rax, qword[r12 + 0x1b]), 0x49, 0x03, 0x44, 0x24, 0x1b);
    SINGLE_COMPARE(add(rax, qword[r13 + 0x1b]), 0x49, 0x03, 0x45, 0x1b);

    SINGLE_COMPARE(add(r12, qword[rax + 0x1b]), 0x4c, 0x03, 0x60, 0x1b);
    SINGLE_COMPARE(add(r12, qword[rbx + 0x1b]), 0x4c, 0x03, 0x63, 0x1b);
    SINGLE_COMPARE(add(r12, qword[rsp + 0x1b]), 0x4c, 0x03, 0x64, 0x24, 0x1b);
    SINGLE_COMPARE(add(r12, qword[rbp + 0x1b]), 0x4c, 0x03, 0x65, 0x1b);
    SINGLE_COMPARE(add(r12, qword[r10 + 0x1b]), 0x4d, 0x03, 0x62, 0x1b);
    SINGLE_COMPARE(add(r12, qword[r12 + 0x1b]), 0x4d, 0x03, 0x64, 0x24, 0x1b);
    SINGLE_COMPARE(add(r12, qword[r13 + 0x1b]), 0x4d, 0x03, 0x65, 0x1b);

    // reg, [base+imm32]
    SINGLE_COMPARE(add(rax, qword[rax + 0xabab]), 0x48, 0x03, 0x80, 0xab, 0xab, 0x00, 0x00);
    SINGLE_COMPARE(add(rax, qword[rbx + 0xabab]), 0x48, 0x03, 0x83, 0xab, 0xab, 0x00, 0x00);
    SINGLE_COMPARE(add(rax, qword[rsp + 0xabab]), 0x48, 0x03, 0x84, 0x24, 0xab, 0xab, 0x00, 0x00);
    SINGLE_COMPARE(add(rax, qword[rbp + 0xabab]), 0x48, 0x03, 0x85, 0xab, 0xab, 0x00, 0x00);
    SINGLE_COMPARE(add(rax, qword[r10 + 0xabab]), 0x49, 0x03, 0x82, 0xab, 0xab, 0x00, 0x00);
    SINGLE_COMPARE(add(rax, qword[r12 + 0xabab]), 0x49, 0x03, 0x84, 0x24, 0xab, 0xab, 0x00, 0x00);
    SINGLE_COMPARE(add(rax, qword[r13 + 0xabab]), 0x49, 0x03, 0x85, 0xab, 0xab, 0x00, 0x00);

    SINGLE_COMPARE(add(r12, qword[rax + 0xabab]), 0x4c, 0x03, 0xa0, 0xab, 0xab, 0x00, 0x00);
    SINGLE_COMPARE(add(r12, qword[rbx + 0xabab]), 0x4c, 0x03, 0xa3, 0xab, 0xab, 0x00, 0x00);
    SINGLE_COMPARE(add(r12, qword[rsp + 0xabab]), 0x4c, 0x03, 0xa4, 0x24, 0xab, 0xab, 0x00, 0x00);
    SINGLE_COMPARE(add(r12, qword[rbp + 0xabab]), 0x4c, 0x03, 0xa5, 0xab, 0xab, 0x00, 0x00);
    SINGLE_COMPARE(add(r12, qword[r10 + 0xabab]), 0x4d, 0x03, 0xa2, 0xab, 0xab, 0x00, 0x00);
    SINGLE_COMPARE(add(r12, qword[r12 + 0xabab]), 0x4d, 0x03, 0xa4, 0x24, 0xab, 0xab, 0x00, 0x00);
    SINGLE_COMPARE(add(r12, qword[r13 + 0xabab]), 0x4d, 0x03, 0xa5, 0xab, 0xab, 0x00, 0x00);

    // reg, [index*scale]
    SINGLE_COMPARE(add(rax, qword[rax * 2]), 0x48, 0x03, 0x04, 0x45, 0x00, 0x00, 0x00, 0x00);
    SINGLE_COMPARE(add(rax, qword[rbx * 2]), 0x48, 0x03, 0x04, 0x5d, 0x00, 0x00, 0x00, 0x00);
    SINGLE_COMPARE(add(rax, qword[rbp * 2]), 0x48, 0x03, 0x04, 0x6d, 0x00, 0x00, 0x00, 0x00);
    SINGLE_COMPARE(add(rax, qword[r10 * 2]), 0x4a, 0x03, 0x04, 0x55, 0x00, 0x00, 0x00, 0x00);
    SINGLE_COMPARE(add(rax, qword[r12 * 2]), 0x4a, 0x03, 0x04, 0x65, 0x00, 0x00, 0x00, 0x00);
    SINGLE_COMPARE(add(rax, qword[r13 * 2]), 0x4a, 0x03, 0x04, 0x6d, 0x00, 0x00, 0x00, 0x00);

    SINGLE_COMPARE(add(r12, qword[rax * 2]), 0x4c, 0x03, 0x24, 0x45, 0x00, 0x00, 0x00, 0x00);
    SINGLE_COMPARE(add(r12, qword[rbx * 2]), 0x4c, 0x03, 0x24, 0x5d, 0x00, 0x00, 0x00, 0x00);
    SINGLE_COMPARE(add(r12, qword[rbp * 2]), 0x4c, 0x03, 0x24, 0x6d, 0x00, 0x00, 0x00, 0x00);
    SINGLE_COMPARE(add(r12, qword[r10 * 2]), 0x4e, 0x03, 0x24, 0x55, 0x00, 0x00, 0x00, 0x00);
    SINGLE_COMPARE(add(r12, qword[r12 * 2]), 0x4e, 0x03, 0x24, 0x65, 0x00, 0x00, 0x00, 0x00);
    SINGLE_COMPARE(add(r12, qword[r13 * 2]), 0x4e, 0x03, 0x24, 0x6d, 0x00, 0x00, 0x00, 0x00);

    // reg, [base+index*scale+imm]
    SINGLE_COMPARE(add(rax, qword[rax + rax * 2]), 0x48, 0x03, 0x04, 0x40);
    SINGLE_COMPARE(add(rax, qword[rax + rbx * 2 + 0x1b]), 0x48, 0x03, 0x44, 0x58, 0x1b);
    SINGLE_COMPARE(add(rax, qword[rax + rbp * 2]), 0x48, 0x03, 0x04, 0x68);
    SINGLE_COMPARE(add(rax, qword[rax + rbp + 0xabab]), 0x48, 0x03, 0x84, 0x28, 0xAB, 0xab, 0x00, 0x00);
    SINGLE_COMPARE(add(rax, qword[rax + r12 + 0x1b]), 0x4a, 0x03, 0x44, 0x20, 0x1b);
    SINGLE_COMPARE(add(rax, qword[rax + r12 * 4 + 0xabab]), 0x4a, 0x03, 0x84, 0xa0, 0xab, 0xab, 0x00, 0x00);
    SINGLE_COMPARE(add(rax, qword[rax + r13 * 2 + 0x1b]), 0x4a, 0x03, 0x44, 0x68, 0x1b);
    SINGLE_COMPARE(add(rax, qword[rax + r13 + 0xabab]), 0x4a, 0x03, 0x84, 0x28, 0xab, 0xab, 0x00, 0x00);
    SINGLE_COMPARE(add(r12, qword[rax + r12 * 2]), 0x4e, 0x03, 0x24, 0x60);
    SINGLE_COMPARE(add(r12, qword[rax + r13 + 0xabab]), 0x4e, 0x03, 0xA4, 0x28, 0xab, 0xab, 0x00, 0x00);
    SINGLE_COMPARE(add(r12, qword[rax + rbp * 2 + 0x1b]), 0x4c, 0x03, 0x64, 0x68, 0x1b);

    // reg, [imm32]
    SINGLE_COMPARE(add(rax, qword[0]), 0x48, 0x03, 0x04, 0x25, 0x00, 0x00, 0x00, 0x00);
    SINGLE_COMPARE(add(rax, qword[0xabab]), 0x48, 0x03, 0x04, 0x25, 0xab, 0xab, 0x00, 0x00);

    // [addr], reg
    SINGLE_COMPARE(add(qword[rax], rax), 0x48, 0x01, 0x00);
    SINGLE_COMPARE(add(qword[rax + rax * 4 + 0xabab], rax), 0x48, 0x01, 0x84, 0x80, 0xab, 0xab, 0x00, 0x00);
    SINGLE_COMPARE(add(qword[rbx + rax * 2 + 0x1b], rax), 0x48, 0x01, 0x44, 0x43, 0x1b);
    SINGLE_COMPARE(add(qword[rbx + rbp * 2 + 0x1b], rax), 0x48, 0x01, 0x44, 0x6b, 0x1b);
    SINGLE_COMPARE(add(qword[rbp + rbp * 4 + 0xabab], rax), 0x48, 0x01, 0x84, 0xad, 0xab, 0xab, 0x00, 0x00);
    SINGLE_COMPARE(add(qword[rbp + r12 + 0x1b], rax), 0x4a, 0x01, 0x44, 0x25, 0x1b);
    SINGLE_COMPARE(add(qword[r12], rax), 0x49, 0x01, 0x04, 0x24);
    SINGLE_COMPARE(add(qword[r13 + rbx + 0xabab], rax), 0x49, 0x01, 0x84, 0x1d, 0xab, 0xab, 0x00, 0x00);
    SINGLE_COMPARE(add(qword[rax + r13 * 2 + 0x1b], rsi), 0x4a, 0x01, 0x74, 0x68, 0x1b);
    SINGLE_COMPARE(add(qword[rbp + rbx * 2], rsi), 0x48, 0x01, 0x74, 0x5d, 0x00);
    SINGLE_COMPARE(add(qword[rsp + r10 * 2 + 0x1b], r10), 0x4e, 0x01, 0x54, 0x54, 0x1b);

    // [addr], imm
    SINGLE_COMPARE(add(byte[rax], 2), 0x80, 0x00, 0x02);
    SINGLE_COMPARE(add(dword[rax], 2), 0x83, 0x00, 0x02);
    SINGLE_COMPARE(add(dword[rax], 0xabcd), 0x81, 0x00, 0xcd, 0xab, 0x00, 0x00);
    SINGLE_COMPARE(add(qword[rax], 2), 0x48, 0x83, 0x00, 0x02);
    SINGLE_COMPARE(add(qword[rax], 0xabcd), 0x48, 0x81, 0x00, 0xcd, 0xab, 0x00, 0x00);
}

TEST_CASE_FIXTURE(AssemblyBuilderX64Fixture, "BaseUnaryInstructionForms")
{
    SINGLE_COMPARE(div(rcx), 0x48, 0xf7, 0xf1);
    SINGLE_COMPARE(idiv(qword[rax]), 0x48, 0xf7, 0x38);
    SINGLE_COMPARE(mul(qword[rax + rbx]), 0x48, 0xf7, 0x24, 0x18);
    SINGLE_COMPARE(imul(r9), 0x49, 0xf7, 0xe9);
    SINGLE_COMPARE(neg(r9), 0x49, 0xf7, 0xd9);
    SINGLE_COMPARE(not_(r12), 0x49, 0xf7, 0xd4);
}

TEST_CASE_FIXTURE(AssemblyBuilderX64Fixture, "FormsOfMov")
{
    SINGLE_COMPARE(mov(rcx, 1), 0x48, 0xb9, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
    SINGLE_COMPARE(mov64(rcx, 0x1234567812345678ll), 0x48, 0xb9, 0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);
    SINGLE_COMPARE(mov(ecx, 2), 0xb9, 0x02, 0x00, 0x00, 0x00);
    SINGLE_COMPARE(mov(cl, 2), 0xb1, 0x02);
    SINGLE_COMPARE(mov(rcx, qword[rdi]), 0x48, 0x8b, 0x0f);
    SINGLE_COMPARE(mov(dword[rax], 0xabcd), 0xc7, 0x00, 0xcd, 0xab, 0x00, 0x00);
    SINGLE_COMPARE(mov(r13, 1), 0x49, 0xbd, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
    SINGLE_COMPARE(mov64(r13, 0x1234567812345678ll), 0x49, 0xbd, 0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);
    SINGLE_COMPARE(mov(r13d, 2), 0x41, 0xbd, 0x02, 0x00, 0x00, 0x00);
    SINGLE_COMPARE(mov(r13, qword[r12]), 0x4d, 0x8b, 0x2c, 0x24);
    SINGLE_COMPARE(mov(dword[r13], 0xabcd), 0x41, 0xc7, 0x45, 0x00, 0xcd, 0xab, 0x00, 0x00);
    SINGLE_COMPARE(mov(qword[rdx], r9), 0x4c, 0x89, 0x0a);
    SINGLE_COMPARE(mov(byte[rsi], 0x3), 0xc6, 0x06, 0x03);
    SINGLE_COMPARE(mov(byte[rsi], al), 0x88, 0x06);
}

TEST_CASE_FIXTURE(AssemblyBuilderX64Fixture, "FormsOfMovExtended")
{
    SINGLE_COMPARE(movsx(eax, byte[rcx]), 0x0f, 0xbe, 0x01);
    SINGLE_COMPARE(movsx(r12, byte[r10]), 0x4d, 0x0f, 0xbe, 0x22);
    SINGLE_COMPARE(movsx(ebx, word[r11]), 0x41, 0x0f, 0xbf, 0x1b);
    SINGLE_COMPARE(movsx(rdx, word[rcx]), 0x48, 0x0f, 0xbf, 0x11);
    SINGLE_COMPARE(movzx(eax, byte[rcx]), 0x0f, 0xb6, 0x01);
    SINGLE_COMPARE(movzx(r12, byte[r10]), 0x4d, 0x0f, 0xb6, 0x22);
    SINGLE_COMPARE(movzx(ebx, word[r11]), 0x41, 0x0f, 0xb7, 0x1b);
    SINGLE_COMPARE(movzx(rdx, word[rcx]), 0x48, 0x0f, 0xb7, 0x11);
}

TEST_CASE_FIXTURE(AssemblyBuilderX64Fixture, "FormsOfTest")
{
    SINGLE_COMPARE(test(al, 8), 0xf6, 0xc0, 0x08);
    SINGLE_COMPARE(test(eax, 8), 0xf7, 0xc0, 0x08, 0x00, 0x00, 0x00);
    SINGLE_COMPARE(test(rax, 8), 0x48, 0xf7, 0xc0, 0x08, 0x00, 0x00, 0x00);
    SINGLE_COMPARE(test(rcx, 0xabab), 0x48, 0xf7, 0xc1, 0xab, 0xab, 0x00, 0x00);
    SINGLE_COMPARE(test(rcx, rax), 0x48, 0x85, 0xc8);
    SINGLE_COMPARE(test(rax, qword[rcx]), 0x48, 0x85, 0x01);
}

TEST_CASE_FIXTURE(AssemblyBuilderX64Fixture, "FormsOfShift")
{
    SINGLE_COMPARE(shl(al, 1), 0xd0, 0xe0);
    SINGLE_COMPARE(shl(al, cl), 0xd2, 0xe0);
    SINGLE_COMPARE(shr(al, 4), 0xc0, 0xe8, 0x04);
    SINGLE_COMPARE(shr(eax, 1), 0xd1, 0xe8);
    SINGLE_COMPARE(sal(eax, cl), 0xd3, 0xe0);
    SINGLE_COMPARE(sal(eax, 4), 0xc1, 0xe0, 0x04);
    SINGLE_COMPARE(sar(rax, 4), 0x48, 0xc1, 0xf8, 0x04);
    SINGLE_COMPARE(sar(r11, 1), 0x49, 0xd1, 0xfb);
}

TEST_CASE_FIXTURE(AssemblyBuilderX64Fixture, "FormsOfLea")
{
    SINGLE_COMPARE(lea(rax, qword[rdx + rcx]), 0x48, 0x8d, 0x04, 0x0a);
    SINGLE_COMPARE(lea(rax, qword[rdx + rax * 4]), 0x48, 0x8d, 0x04, 0x82);
    SINGLE_COMPARE(lea(rax, qword[r13 + r12 * 4 + 4]), 0x4b, 0x8d, 0x44, 0xa5, 0x04);
}

TEST_CASE_FIXTURE(AssemblyBuilderX64Fixture, "FormsOfAbsoluteJumps")
{
    SINGLE_COMPARE(jmp(rax), 0x48, 0xff, 0xe0);
    SINGLE_COMPARE(jmp(r14), 0x49, 0xff, 0xe6);
    SINGLE_COMPARE(jmp(qword[r14 + rdx * 4]), 0x49, 0xff, 0x24, 0x96);
    SINGLE_COMPARE(call(rax), 0x48, 0xff, 0xd0);
    SINGLE_COMPARE(call(r14), 0x49, 0xff, 0xd6);
    SINGLE_COMPARE(call(qword[r14 + rdx * 4]), 0x49, 0xff, 0x14, 0x96);
}

TEST_CASE_FIXTURE(AssemblyBuilderX64Fixture, "FormsOfImul")
{
    SINGLE_COMPARE(imul(ecx, esi), 0x0f, 0xaf, 0xce);
    SINGLE_COMPARE(imul(r12, rax), 0x4c, 0x0f, 0xaf, 0xe0);
    SINGLE_COMPARE(imul(r12, qword[rdx + rdi]), 0x4c, 0x0f, 0xaf, 0x24, 0x3a);
    SINGLE_COMPARE(imul(ecx, edx, 8), 0x6b, 0xca, 0x08);
    SINGLE_COMPARE(imul(ecx, r9d, 0xabcd), 0x41, 0x69, 0xc9, 0xcd, 0xab, 0x00, 0x00);
    SINGLE_COMPARE(imul(r8d, eax, -9), 0x44, 0x6b, 0xc0, 0xf7);
    SINGLE_COMPARE(imul(rcx, rdx, 17), 0x48, 0x6b, 0xca, 0x11);
    SINGLE_COMPARE(imul(rcx, r12, 0xabcd), 0x49, 0x69, 0xcc, 0xcd, 0xab, 0x00, 0x00);
    SINGLE_COMPARE(imul(r12, rax, -13), 0x4c, 0x6b, 0xe0, 0xf3);
}

TEST_CASE_FIXTURE(AssemblyBuilderX64Fixture, "ControlFlow")
{
    // Jump back
    check(
        [](AssemblyBuilderX64& build) {
            Label start = build.setLabel();
            build.add(rsi, 1);
            build.cmp(rsi, rdi);
            build.jcc(Condition::Equal, start);
        },
        {0x48, 0x83, 0xc6, 0x01, 0x48, 0x3b, 0xf7, 0x0f, 0x84, 0xf3, 0xff, 0xff, 0xff});

    // Jump back, but the label is set before use
    check(
        [](AssemblyBuilderX64& build) {
            Label start;
            build.add(rsi, 1);
            build.setLabel(start);
            build.cmp(rsi, rdi);
            build.jcc(Condition::Equal, start);
        },
        {0x48, 0x83, 0xc6, 0x01, 0x48, 0x3b, 0xf7, 0x0f, 0x84, 0xf7, 0xff, 0xff, 0xff});

    // Jump forward
    check(
        [](AssemblyBuilderX64& build) {
            Label skip;

            build.cmp(rsi, rdi);
            build.jcc(Condition::Greater, skip);
            build.or_(rdi, 0x3e);
            build.setLabel(skip);
        },
        {0x48, 0x3b, 0xf7, 0x0f, 0x8f, 0x04, 0x00, 0x00, 0x00, 0x48, 0x83, 0xcf, 0x3e});

    // Regular jump
    check(
        [](AssemblyBuilderX64& build) {
            Label skip;

            build.jmp(skip);
            build.and_(rdi, 0x3e);
            build.setLabel(skip);
        },
        {0xe9, 0x04, 0x00, 0x00, 0x00, 0x48, 0x83, 0xe7, 0x3e});
}

TEST_CASE_FIXTURE(AssemblyBuilderX64Fixture, "LabelCall")
{
    check(
        [](AssemblyBuilderX64& build) {
            Label fnB;

            build.and_(rcx, 0x3e);
            build.call(fnB);
            build.ret();

            build.setLabel(fnB);
            build.lea(rax, qword[rcx + 0x1f]);
            build.ret();
        },
        {0x48, 0x83, 0xe1, 0x3e, 0xe8, 0x01, 0x00, 0x00, 0x00, 0xc3, 0x48, 0x8d, 0x41, 0x1f, 0xc3});
}

TEST_CASE_FIXTURE(AssemblyBuilderX64Fixture, "AVXBinaryInstructionForms")
{
    SINGLE_COMPARE(vaddpd(xmm8, xmm10, xmm14), 0xc4, 0x41, 0xa9, 0x58, 0xc6);
    SINGLE_COMPARE(vaddpd(xmm8, xmm10, xmmword[r9]), 0xc4, 0x41, 0xa9, 0x58, 0x01);
    SINGLE_COMPARE(vaddpd(ymm8, ymm10, ymm14), 0xc4, 0x41, 0xad, 0x58, 0xc6);
    SINGLE_COMPARE(vaddpd(ymm8, ymm10, ymmword[r9]), 0xc4, 0x41, 0xad, 0x58, 0x01);
    SINGLE_COMPARE(vaddps(xmm8, xmm10, xmm14), 0xc4, 0x41, 0xa8, 0x58, 0xc6);
    SINGLE_COMPARE(vaddps(xmm8, xmm10, xmmword[r9]), 0xc4, 0x41, 0xa8, 0x58, 0x01);
    SINGLE_COMPARE(vaddsd(xmm8, xmm10, xmm14), 0xc4, 0x41, 0xab, 0x58, 0xc6);
    SINGLE_COMPARE(vaddsd(xmm8, xmm10, qword[r9]), 0xc4, 0x41, 0xab, 0x58, 0x01);
    SINGLE_COMPARE(vaddss(xmm8, xmm10, xmm14), 0xc4, 0x41, 0xaa, 0x58, 0xc6);
    SINGLE_COMPARE(vaddss(xmm8, xmm10, dword[r9]), 0xc4, 0x41, 0xaa, 0x58, 0x01);

    SINGLE_COMPARE(vaddps(xmm1, xmm2, xmm3), 0xc4, 0xe1, 0xe8, 0x58, 0xcb);
    SINGLE_COMPARE(vaddps(xmm9, xmm12, xmmword[r9 + r14 * 2 + 0x1c]), 0xc4, 0x01, 0x98, 0x58, 0x4c, 0x71, 0x1c);
    SINGLE_COMPARE(vaddps(ymm1, ymm2, ymm3), 0xc4, 0xe1, 0xec, 0x58, 0xcb);
    SINGLE_COMPARE(vaddps(ymm9, ymm12, ymmword[r9 + r14 * 2 + 0x1c]), 0xc4, 0x01, 0x9c, 0x58, 0x4c, 0x71, 0x1c);

    // Coverage for other instructions that follow the same pattern
    SINGLE_COMPARE(vsubsd(xmm8, xmm10, xmm14), 0xc4, 0x41, 0xab, 0x5c, 0xc6);
    SINGLE_COMPARE(vmulsd(xmm8, xmm10, xmm14), 0xc4, 0x41, 0xab, 0x59, 0xc6);
    SINGLE_COMPARE(vdivsd(xmm8, xmm10, xmm14), 0xc4, 0x41, 0xab, 0x5e, 0xc6);

    SINGLE_COMPARE(vxorpd(xmm8, xmm10, xmm14), 0xc4, 0x41, 0xa9, 0x57, 0xc6);
}

TEST_CASE_FIXTURE(AssemblyBuilderX64Fixture, "AVXUnaryMergeInstructionForms")
{
    SINGLE_COMPARE(vsqrtpd(xmm8, xmm10), 0xc4, 0x41, 0xf9, 0x51, 0xc2);
    SINGLE_COMPARE(vsqrtpd(xmm8, xmmword[r9]), 0xc4, 0x41, 0xf9, 0x51, 0x01);
    SINGLE_COMPARE(vsqrtpd(ymm8, ymm10), 0xc4, 0x41, 0xfd, 0x51, 0xc2);
    SINGLE_COMPARE(vsqrtpd(ymm8, ymmword[r9]), 0xc4, 0x41, 0xfd, 0x51, 0x01);
    SINGLE_COMPARE(vsqrtps(xmm8, xmm10), 0xc4, 0x41, 0xf8, 0x51, 0xc2);
    SINGLE_COMPARE(vsqrtps(xmm8, xmmword[r9]), 0xc4, 0x41, 0xf8, 0x51, 0x01);
    SINGLE_COMPARE(vsqrtsd(xmm8, xmm10, xmm14), 0xc4, 0x41, 0xab, 0x51, 0xc6);
    SINGLE_COMPARE(vsqrtsd(xmm8, xmm10, qword[r9]), 0xc4, 0x41, 0xab, 0x51, 0x01);
    SINGLE_COMPARE(vsqrtss(xmm8, xmm10, xmm14), 0xc4, 0x41, 0xaa, 0x51, 0xc6);
    SINGLE_COMPARE(vsqrtss(xmm8, xmm10, dword[r9]), 0xc4, 0x41, 0xaa, 0x51, 0x01);

    // Coverage for other instructions that follow the same pattern
    SINGLE_COMPARE(vcomisd(xmm8, xmm10), 0xc4, 0x41, 0xf9, 0x2f, 0xc2);
    SINGLE_COMPARE(vucomisd(xmm1, xmm4), 0xc4, 0xe1, 0xf9, 0x2e, 0xcc);
}

TEST_CASE_FIXTURE(AssemblyBuilderX64Fixture, "AVXMoveInstructionForms")
{
    SINGLE_COMPARE(vmovsd(qword[r9], xmm10), 0xc4, 0x41, 0xfb, 0x11, 0x11);
    SINGLE_COMPARE(vmovsd(xmm8, qword[r9]), 0xc4, 0x41, 0xfb, 0x10, 0x01);
    SINGLE_COMPARE(vmovsd(xmm8, xmm10, xmm14), 0xc4, 0x41, 0xab, 0x10, 0xc6);
    SINGLE_COMPARE(vmovss(dword[r9], xmm10), 0xc4, 0x41, 0xfa, 0x11, 0x11);
    SINGLE_COMPARE(vmovss(xmm8, dword[r9]), 0xc4, 0x41, 0xfa, 0x10, 0x01);
    SINGLE_COMPARE(vmovss(xmm8, xmm10, xmm14), 0xc4, 0x41, 0xaa, 0x10, 0xc6);
    SINGLE_COMPARE(vmovapd(xmm8, xmmword[r9]), 0xc4, 0x41, 0xf9, 0x28, 0x01);
    SINGLE_COMPARE(vmovapd(xmmword[r9], xmm10), 0xc4, 0x41, 0xf9, 0x29, 0x11);
    SINGLE_COMPARE(vmovapd(ymm8, ymmword[r9]), 0xc4, 0x41, 0xfd, 0x28, 0x01);
    SINGLE_COMPARE(vmovaps(xmm8, xmmword[r9]), 0xc4, 0x41, 0xf8, 0x28, 0x01);
    SINGLE_COMPARE(vmovaps(xmmword[r9], xmm10), 0xc4, 0x41, 0xf8, 0x29, 0x11);
    SINGLE_COMPARE(vmovaps(ymm8, ymmword[r9]), 0xc4, 0x41, 0xfc, 0x28, 0x01);
    SINGLE_COMPARE(vmovupd(xmm8, xmmword[r9]), 0xc4, 0x41, 0xf9, 0x10, 0x01);
    SINGLE_COMPARE(vmovupd(xmmword[r9], xmm10), 0xc4, 0x41, 0xf9, 0x11, 0x11);
    SINGLE_COMPARE(vmovupd(ymm8, ymmword[r9]), 0xc4, 0x41, 0xfd, 0x10, 0x01);
    SINGLE_COMPARE(vmovups(xmm8, xmmword[r9]), 0xc4, 0x41, 0xf8, 0x10, 0x01);
    SINGLE_COMPARE(vmovups(xmmword[r9], xmm10), 0xc4, 0x41, 0xf8, 0x11, 0x11);
    SINGLE_COMPARE(vmovups(ymm8, ymmword[r9]), 0xc4, 0x41, 0xfc, 0x10, 0x01);
}

TEST_CASE_FIXTURE(AssemblyBuilderX64Fixture, "AVXConversionInstructionForms")
{
    SINGLE_COMPARE(vcvttsd2si(ecx, xmm0), 0xc4, 0xe1, 0x7b, 0x2c, 0xc8);
    SINGLE_COMPARE(vcvttsd2si(r9d, xmmword[rcx + rdx]), 0xc4, 0x61, 0x7b, 0x2c, 0x0c, 0x11);
    SINGLE_COMPARE(vcvttsd2si(rdx, xmm0), 0xc4, 0xe1, 0xfb, 0x2c, 0xd0);
    SINGLE_COMPARE(vcvttsd2si(r13, xmmword[rcx + rdx]), 0xc4, 0x61, 0xfb, 0x2c, 0x2c, 0x11);
    SINGLE_COMPARE(vcvtsi2sd(xmm5, xmm10, ecx), 0xc4, 0xe1, 0x2b, 0x2a, 0xe9);
    SINGLE_COMPARE(vcvtsi2sd(xmm6, xmm11, dword[rcx + rdx]), 0xc4, 0xe1, 0x23, 0x2a, 0x34, 0x11);
    SINGLE_COMPARE(vcvtsi2sd(xmm5, xmm10, r13), 0xc4, 0xc1, 0xab, 0x2a, 0xed);
    SINGLE_COMPARE(vcvtsi2sd(xmm6, xmm11, qword[rcx + rdx]), 0xc4, 0xe1, 0xa3, 0x2a, 0x34, 0x11);
}

TEST_CASE_FIXTURE(AssemblyBuilderX64Fixture, "AVXTernaryInstructionForms")
{
    SINGLE_COMPARE(vroundsd(xmm7, xmm12, xmm3, 9), 0xc4, 0xe3, 0x99, 0x0b, 0xfb, 0x09);
    SINGLE_COMPARE(vroundsd(xmm8, xmm13, xmmword[r13 + rdx], 9), 0xc4, 0x43, 0x91, 0x0b, 0x44, 0x15, 0x00, 0x09);
    SINGLE_COMPARE(vroundsd(xmm9, xmm14, xmmword[rcx + r10], 1), 0xc4, 0x23, 0x89, 0x0b, 0x0c, 0x11, 0x01);
}

TEST_CASE_FIXTURE(AssemblyBuilderX64Fixture, "MiscInstructions")
{
    SINGLE_COMPARE(int3(), 0xcc);
}

TEST_CASE("LogTest")
{
    AssemblyBuilderX64 build(/* logText= */ true);

    build.push(r12);
    build.add(rax, rdi);
    build.add(rcx, 8);
    build.sub(dword[rax], 0x1fdc);
    build.and_(dword[rcx], 0x37);
    build.mov(rdi, qword[rax + rsi * 2]);
    build.vaddss(xmm0, xmm0, dword[rax + r14 * 2 + 0x1c]);

    Label start = build.setLabel();
    build.cmp(rsi, rdi);
    build.jcc(Condition::Equal, start);

    build.jmp(qword[rdx]);
    build.vaddps(ymm9, ymm12, ymmword[rbp + 0xc]);
    build.vaddpd(ymm2, ymm7, build.f64(2.5));
    build.neg(qword[rbp + r12 * 2]);
    build.mov64(r10, 0x1234567812345678ll);
    build.vmovapd(xmmword[rax], xmm11);
    build.movzx(eax, byte[rcx]);
    build.movsx(rsi, word[r12]);
    build.imul(rcx, rdx);
    build.imul(rcx, rdx, 8);
    build.vroundsd(xmm1, xmm2, xmm3, 5);
    build.pop(r12);
    build.ret();
    build.int3();

    build.finalize();

    bool same = "\n" + build.text == R"(
 push        r12
 add         rax,rdi
 add         rcx,8
 sub         dword ptr [rax],1FDCh
 and         dword ptr [rcx],37h
 mov         rdi,qword ptr [rax+rsi*2]
 vaddss      xmm0,xmm0,dword ptr [rax+r14*2+01Ch]
.L1:
 cmp         rsi,rdi
 je          .L1
 jmp         qword ptr [rdx]
 vaddps      ymm9,ymm12,ymmword ptr [rbp+0Ch]
 vaddpd      ymm2,ymm7,qword ptr [.start-8]
 neg         qword ptr [rbp+r12*2]
 mov         r10,1234567812345678h
 vmovapd     xmmword ptr [rax],xmm11
 movzx       eax,byte ptr [rcx]
 movsx       rsi,word ptr [r12]
 imul        rcx,rdx
 imul        rcx,rdx,8
 vroundsd    xmm1,xmm2,xmm3,5
 pop         r12
 ret
 int3
)";
    CHECK(same);
}

TEST_CASE_FIXTURE(AssemblyBuilderX64Fixture, "Constants")
{
    // clang-format off
    check(
        [](AssemblyBuilderX64& build) {
            build.xor_(rax, rax);
            build.add(rax, build.i64(0x1234567887654321));
            build.vmovss(xmm2, build.f32(1.0f));
            build.vmovsd(xmm3, build.f64(1.0));
            build.vmovaps(xmm4, build.f32x4(1.0f, 2.0f, 4.0f, 8.0f));
            char arr[16] = "hello world!123";
            build.vmovupd(xmm5, build.bytes(arr, 16, 8));
            build.ret();
        },
        {
            0x48, 0x33, 0xc0,
            0x48, 0x03, 0x05, 0xee, 0xff, 0xff, 0xff,
            0xc4, 0xe1, 0xfa, 0x10, 0x15, 0xe1, 0xff, 0xff, 0xff,
            0xc4, 0xe1, 0xfb, 0x10, 0x1d, 0xcc, 0xff, 0xff, 0xff,
            0xc4, 0xe1, 0xf8, 0x28, 0x25, 0xab, 0xff, 0xff, 0xff,
            0xc4, 0xe1, 0xf9, 0x10, 0x2d, 0x92, 0xff, 0xff, 0xff,
            0xc3
        },
        {
            'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd', '!', '1', '2', '3', 0x0,
            0x00, 0x00, 0x80, 0x3f,
            0x00, 0x00, 0x00, 0x40,
            0x00, 0x00, 0x80, 0x40,
            0x00, 0x00, 0x00, 0x41,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // padding to align f32x4
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f,
            0x00, 0x00, 0x00, 0x00, // padding to align f64
            0x00, 0x00, 0x80, 0x3f,
            0x21, 0x43, 0x65, 0x87, 0x78, 0x56, 0x34, 0x12,
        });
    // clang-format on
}

TEST_CASE("ConstantStorage")
{
    AssemblyBuilderX64 build(/* logText= */ false);

    for (int i = 0; i <= 3000; i++)
        build.vaddss(xmm0, xmm0, build.f32(1.0f));

    build.finalize();

    LUAU_ASSERT(build.data.size() == 12004);

    for (int i = 0; i <= 3000; i++)
    {
        LUAU_ASSERT(build.data[i * 4 + 0] == 0x00);
        LUAU_ASSERT(build.data[i * 4 + 1] == 0x00);
        LUAU_ASSERT(build.data[i * 4 + 2] == 0x80);
        LUAU_ASSERT(build.data[i * 4 + 3] == 0x3f);
    }
}

TEST_SUITE_END();
