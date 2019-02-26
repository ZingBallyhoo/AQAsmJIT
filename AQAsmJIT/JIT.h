#pragma once
#include <unordered_map>
#include <unordered_set>
#include <asmjit/asmjit.h>
#include "parser.h"
#include "partial_func.h"

#define PRIMITIVE_CAT(a, ...) a ## __VA_ARGS__
#define __SOURCE_TO_PARSED(c) PRIMITIVE_CAT(__SOURCE_TO_PARSED_, c)
#define __SOURCE_TO_PARSED_Register (RegisterParam*)
#define __SOURCE_TO_PARSED_MemoryAddress (MemoryAddressParam*)
#define __SOURCE_TO_PARSED_Dynamic 
#define __SOURCE_TO_PARSED_Label (LabelParam*)

// macro hell to make defining instructions easy
// could just make one macro but that would be 10000 times worse

/*#define REGISTER_JIT_INSTRUCTION0(name, func) \
	{auto *l = static_cast<void(*)(JIT*)>([](JIT* jit) {jit->func(); }); \
	JIT::InstructionMap[#name] = new RegisteredInstruction(#name, (void*)l); }
#define REGISTER_JIT_INSTRUCTION1(name, func, type_a) \
	{auto *l = static_cast<void(*)(JIT*, ParsedParam*)>([](JIT* jit, ParsedParam* a) {jit->func(__SOURCE_TO_PARSED(type_a)(a)); }); \
	JIT::InstructionMap[#name] = new RegisteredInstruction(#name, (void*)l, std::vector<SourceParamType> {SourceParamType::type_a}); }
#define REGISTER_JIT_INSTRUCTION2(name, func, type_a, type_b) \
	{auto *l = static_cast<void(*)(JIT*, ParsedParam*, ParsedParam*)>([](JIT* jit, ParsedParam* a, ParsedParam* b) {jit->func(__SOURCE_TO_PARSED(type_a)(a), __SOURCE_TO_PARSED(type_b)(b)); }); \
	JIT::InstructionMap[#name] = new RegisteredInstruction(#name, (void*)l, std::vector<SourceParamType> {SourceParamType::type_a, SourceParamType::type_b}); }
#define REGISTER_JIT_INSTRUCTION3(name, func, type_a, type_b, type_c) \
	{auto *l = static_cast<void(*)(JIT*, ParsedParam*, ParsedParam*, ParsedParam*)>([](JIT* jit, ParsedParam* a, ParsedParam* b, ParsedParam* c) {jit->func(__SOURCE_TO_PARSED(type_a)(a), __SOURCE_TO_PARSED(type_b)(b), __SOURCE_TO_PARSED(type_c)(c)); }); \
	JIT::InstructionMap[#name] = new RegisteredInstruction(#name, (void*)l, std::vector<SourceParamType> {SourceParamType::type_a, SourceParamType::type_b, SourceParamType::type_c}); }*/

#define REGISTER_JIT_INSTRUCTION0_2(name, func) \
	{#name, new RegisteredInstruction(#name, (void*)*static_cast<void(*)(JIT*)>([](JIT* jit) {jit->func(); }))}
#define REGISTER_JIT_INSTRUCTION1_2(name, func, type_a) \
	{#name, new RegisteredInstruction(#name, (void*)*static_cast<void(*)(JIT*, ParsedParam*)>([](JIT* jit, ParsedParam* a) {jit->func(__SOURCE_TO_PARSED(type_a)(a)); }), std::vector<SourceParamType> {SourceParamType::type_a})}
#define REGISTER_JIT_INSTRUCTION2_2(name, func, type_a, type_b) \
	{#name, new RegisteredInstruction(#name, (void*)*static_cast<void(*)(JIT*, ParsedParam*, ParsedParam*)>([](JIT* jit, ParsedParam* a, ParsedParam* b) {jit->func(__SOURCE_TO_PARSED(type_a)(a), __SOURCE_TO_PARSED(type_b)(b)); }), std::vector<SourceParamType> {SourceParamType::type_a, SourceParamType::type_b}) }
#define REGISTER_JIT_INSTRUCTION3_2(name, func, type_a, type_b, type_c) \
	{#name, new RegisteredInstruction(#name, (void*)*static_cast<void(*)(JIT*, ParsedParam*, ParsedParam*, ParsedParam*)>([](JIT* jit, ParsedParam* a, ParsedParam* b, ParsedParam* c) {jit->func(__SOURCE_TO_PARSED(type_a)(a), __SOURCE_TO_PARSED(type_b)(b), __SOURCE_TO_PARSED(type_c)(c)); }), std::vector<SourceParamType> {SourceParamType::type_a, SourceParamType::type_b, SourceParamType::type_c}) }


struct RegisterDump final
{
	short R0{}, R1{}, R2{}, R3{}, R4{}, R5{}, R6{}, R7{}, R8{}, R9{}, R10{}, R11{}, R12{};
};

typedef void(*AQAFunc)(void* memory, RegisterDump* registerDump);

class JIT final
{
public:
	// rcx = memory
	// rdx = register output

	static std::unordered_map<std::string, RegisteredInstruction*> InstructionMap;
	static std::vector<asmjit::X86Gp> X64Registers;
	static std::vector<asmjit::X86Gp> X16Registers;

	void BeginFunc();

	void Reset();

	~JIT()
	{
		delete _assembler;
	}

	void CallAssemble(RegisteredInstruction* inst, const std::vector<ParsedParam>& params);
	AQAFunc FinishFunc();
	void ReleaseFunc(AQAFunc func);

private:
	bool _hasHalted = false;
	std::unordered_set<int> _usedRegisters;
	std::unordered_map<std::string, asmjit::Label> _labels;

	asmjit::JitRuntime _runtime;
	asmjit::CodeHolder _codeHolder;
	asmjit::X86Assembler* _assembler = nullptr;

	asmjit::Label _codeLabel;
	asmjit::Label _enterLabel;
	asmjit::Label _exitLabel;

	static asmjit::X86Gp GetMemoryRegister() { return asmjit::x86::rcx; }
	static asmjit::X86Gp GetDumpRegister() { return asmjit::x86::rdx; }
	static asmjit::X86Gp GetX16Reg(const int index) { return X16Registers[index]; }
	static asmjit::X86Gp GetX64Reg(const int index) { return X64Registers[index]; }

	asmjit::Operand GetJitOperand(ParsedParam* parameter);

	void CallAssemble0(RegisteredInstruction* inst) { call_partial_function<void>(inst->Func, this); }
	void CallAssemble1(RegisteredInstruction* inst, const ParsedParam* a) { call_partial_function<void>(inst->Func, this, a); }
	void CallAssemble2(RegisteredInstruction* inst, const ParsedParam* a, const ParsedParam* b) { call_partial_function<void>(inst->Func, this, a, b); }
	void CallAssemble3(RegisteredInstruction* inst, const ParsedParam* a, const ParsedParam* b, const ParsedParam* c) { call_partial_function<void>(inst->Func, this, a, b, c); }

	void Emit0(const asmjit::X86Inst::Id inst) const { _assembler->emit(inst); }
	void Emit1(const asmjit::X86Inst::Id inst, ParsedParam* a) { _assembler->emit(inst, GetJitOperand(a)); }
	void Emit2(const asmjit::X86Inst::Id inst, ParsedParam* a, ParsedParam* b) { _assembler->emit(inst, GetJitOperand(a), GetJitOperand(b)); }
	void Emit3(const asmjit::X86Inst::Id inst, ParsedParam* a, ParsedParam* b, ParsedParam* c) { _assembler->emit(inst, GetJitOperand(a), GetJitOperand(b), GetJitOperand(c)); }

	void CheckHalt();

	asmjit::Label GetLabel(const std::string& name);
	asmjit::Label GetLabel(LabelParam* label) { return GetLabel(label->Data.label_id); }

	// ---- helpers ----

	void EmitSmartMove(RegisterParam* dest, RegisterParam* source);
	void EmitMovAndOp(asmjit::X86Inst::Id inst, RegisterParam* dest, RegisterParam* a, ParsedParam* b);

	// ---- x86 instruction creation ----

	// ReSharper disable CppInconsistentNaming
	void AssembleLDR(RegisterParam* dest, MemoryAddressParam* source) { Emit2(asmjit::X86Inst::kIdMov, dest, source); }
	void AssembleSTR(RegisterParam* source, MemoryAddressParam* dest) { Emit2(asmjit::X86Inst::kIdMov, dest, source); }

	void AssembleADD(RegisterParam* dest, RegisterParam* a, ParsedParam* b) { EmitMovAndOp(asmjit::X86Inst::kIdAdd, dest, a, b); }
	void AssembleSUB(RegisterParam* dest, RegisterParam* a, ParsedParam* b) { EmitMovAndOp(asmjit::X86Inst::kIdSub, dest, a, b); }
	void AssembleAND(RegisterParam* dest, RegisterParam* a, ParsedParam* b) { EmitMovAndOp(asmjit::X86Inst::kIdAnd, dest, a, b); }
	void AssembleORR(RegisterParam* dest, RegisterParam* a, ParsedParam* b) { EmitMovAndOp(asmjit::X86Inst::kIdOr, dest, a, b); }
	void AssembleEOR(RegisterParam* dest, RegisterParam* a, ParsedParam* b) { EmitMovAndOp(asmjit::X86Inst::kIdXor, dest, a, b); }
	void AssembleLSL(RegisterParam* dest, RegisterParam* a, ParsedParam* b) { EmitMovAndOp(asmjit::X86Inst::kIdShl, dest, a, b); }
	void AssembleLSR(RegisterParam* dest, RegisterParam* a, ParsedParam* b) { EmitMovAndOp(asmjit::X86Inst::kIdShr, dest, a, b); }
	void AssembleMVN(RegisterParam* dest, ParsedParam* source) { Emit2(asmjit::X86Inst::kIdNot, dest, source); }

	void AssembleMOV(RegisterParam* dest, ParsedParam* source) { Emit2(asmjit::X86Inst::kIdMov, dest, source); }
	void AssembleCMP(RegisterParam* a, ParsedParam* b) { Emit2(asmjit::X86Inst::kIdCmp, a, b); }

	void AssembleB(LabelParam* label) { Emit1(asmjit::X86Inst::kIdJmp, label); }
	void AssembleBEQ(LabelParam* label) { Emit1(asmjit::X86Inst::kIdJe, label); }
	void AssembleBNE(LabelParam* label) { Emit1(asmjit::X86Inst::kIdJne, label); }
	void AssembleBGT(LabelParam* label) { Emit1(asmjit::X86Inst::kIdJg, label); }
	void AssembleBLT(LabelParam* label) { Emit1(asmjit::X86Inst::kIdJl, label); }

	void AssembleHALT() { _hasHalted = true; } // any work that needs to be done is done by CheckHalt

	void BindLabel(LabelParam* label) { _assembler->bind(GetLabel(label)); }
	// ReSharper restore CppInconsistentNaming
};
