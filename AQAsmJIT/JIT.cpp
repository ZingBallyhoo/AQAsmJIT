#include "pch.h"
#include "JIT.h"

std::unordered_map<std::string, RegisteredInstruction*> JIT::InstructionMap = {
	REGISTER_JIT_INSTRUCTION2_2(LDR, AssembleLDR, Register, MemoryAddress),
	REGISTER_JIT_INSTRUCTION2_2(STR, AssembleSTR, Register, MemoryAddress),

	REGISTER_JIT_INSTRUCTION3_2(ADD, AssembleADD, Register, Register, Dynamic),
	REGISTER_JIT_INSTRUCTION3_2(SUB, AssembleSUB, Register, Register, Dynamic),
	REGISTER_JIT_INSTRUCTION3_2(AND, AssembleAND, Register, Register, Dynamic),
	REGISTER_JIT_INSTRUCTION3_2(ORR, AssembleORR, Register, Register, Dynamic),
	REGISTER_JIT_INSTRUCTION3_2(EOR, AssembleEOR, Register, Register, Dynamic),
	REGISTER_JIT_INSTRUCTION3_2(LSL, AssembleLSL, Register, Register, Dynamic),
	REGISTER_JIT_INSTRUCTION3_2(LSR, AssembleLSR, Register, Register, Dynamic),
	REGISTER_JIT_INSTRUCTION2_2(MVN, AssembleMVN, Register, Dynamic),

	REGISTER_JIT_INSTRUCTION2_2(MOV, AssembleMOV, Register, Dynamic),
	REGISTER_JIT_INSTRUCTION2_2(CMP, AssembleCMP, Register, Dynamic),

	REGISTER_JIT_INSTRUCTION1_2(B, AssembleB, Label),
	REGISTER_JIT_INSTRUCTION1_2(BEQ, AssembleBEQ, Label),
	REGISTER_JIT_INSTRUCTION1_2(BNE, AssembleBNE, Label),
	REGISTER_JIT_INSTRUCTION1_2(BGT, AssembleBGT, Label),
	REGISTER_JIT_INSTRUCTION1_2(BLT, AssembleBLT, Label),

	REGISTER_JIT_INSTRUCTION0_2(HALT, AssembleHALT),

	// not real
	REGISTER_JIT_INSTRUCTION1_2(BIND_LABEL, BindLabel, Label)
};
std::vector<asmjit::X86Gp> JIT::X64Registers = { asmjit::x86::rax, asmjit::x86::rbx, asmjit::x86::rsi, asmjit::x86::rdi, asmjit::x86::rbp, asmjit::x86::r8, asmjit::x86::r9, asmjit::x86::r10, asmjit::x86::r11, asmjit::x86::r12, asmjit::x86::r13, asmjit::x86::r14, asmjit::x86::r15 };
std::vector<asmjit::X86Gp> JIT::X16Registers = { asmjit::x86::ax, asmjit::x86::bx, asmjit::x86::si, asmjit::x86::di, asmjit::x86::bp, asmjit::x86::r8w, asmjit::x86::r9w, asmjit::x86::r10w, asmjit::x86::r11w, asmjit::x86::r12w, asmjit::x86::r13w, asmjit::x86::r14w, asmjit::x86::r15w };

void JIT::BeginFunc()
{
	if (_assembler != nullptr) Reset();
	_codeHolder.init(_runtime.getCodeInfo()); // Initialize to the same arch as JIT runtime.
	_assembler = new asmjit::X86Assembler(&_codeHolder); // Create and attach X86Assembler to `code`.

	_enterLabel = _assembler->newLabel();
	_exitLabel = _assembler->newLabel();
	_codeLabel = _assembler->newLabel();
	_assembler->jmp(_enterLabel);
	_assembler->bind(_codeLabel);
}

void JIT::Reset()
{
	_labels.clear();
	_usedRegisters.clear();
	delete _assembler;
	_assembler = nullptr;
}

AQAFunc JIT::FinishFunc()
{
	if (!_hasHalted) throw std::exception("didn't halt at end of code");

	// exit
	_assembler->bind(_exitLabel);
	int i = 0;
	for (auto usedRegister : _usedRegisters)
	{
		_assembler->mov(asmjit::x86::ptr(GetDumpRegister(), usedRegister * 2), GetX16Reg(usedRegister));  // write to dump
		_assembler->mov(GetX64Reg(usedRegister), asmjit::x86::ptr(asmjit::x86::rsp, -8 * (i + 1)));  // restore from stack
		i++;
	}
	_assembler->ret();

	// enter
	_assembler->bind(_enterLabel);
	int j = 0;
	for (auto usedRegister : _usedRegisters)
	{
		_assembler->mov(asmjit::x86::ptr(asmjit::x86::rsp, -8 * (j + 1)), GetX64Reg(usedRegister));  // write to stack
		j++;
	}
	_assembler->jmp(_codeLabel);

	AQAFunc func;
	const auto err = _runtime.add(&func, &_codeHolder);
	if (err) return nullptr;
	return func;
}

void JIT::ReleaseFunc(const AQAFunc func)
{
	_runtime.release(func);
}

asmjit::Operand JIT::GetJitOperand(ParsedParam* parameter)
{
	switch (parameter->Type)
	{
	case ParsedParamType::Register:
		{
			_usedRegisters.insert(parameter->Data.register_idx);
			return GetX16Reg(parameter->Data.register_idx);
		}
	case ParsedParamType::MemoryAddress:
		return asmjit::x86::ptr(GetMemoryRegister(), parameter->Data.memory_address * sizeof(short));
	case ParsedParamType::Constant:
		return asmjit::imm(parameter->Data.constant_value);
	case ParsedParamType::Label:
		return GetLabel(reinterpret_cast<LabelParam*>(parameter));
	default:
		throw std::exception("JIT::GetJitOperand: unhandled ParsedParamType");
	}
}

void JIT::CallAssemble(RegisteredInstruction* inst, const std::vector<ParsedParam>& params)
{
	CheckHalt();
	// todo: this sucks
	const auto count = params.size();
	if (count == 0) CallAssemble0(inst);
	else if (count == 1) CallAssemble1(inst, &params[0]);
	else if (count == 2) CallAssemble2(inst, &params[0], &params[1]);
	else if (count == 3) CallAssemble3(inst, &params[0], &params[1], &params[2]);
	else
	{
		throw std::exception("unable to assemble instruction, too many parameters");
	}
}

void JIT::CheckHalt()
{
	// if we have more than one exit, explicitly jump to exit handler
	if (_hasHalted)
	{
		_assembler->jmp(_exitLabel);
		_hasHalted = false;
	}
}

asmjit::Label JIT::GetLabel(const std::string& name)
{
	if (_labels.count(name) != 0)
	{
		return _labels[name];
	}
	auto label = _assembler->newLabel();
	_labels[name] = label;
	return label;
}

void JIT::EmitSmartMove(RegisterParam* dest, RegisterParam* source)
{
	if (dest->Data.register_idx == source->Data.register_idx) return;
	Emit2(asmjit::X86Inst::kIdMov, dest, source);
}

void JIT::EmitMovAndOp(const asmjit::X86Inst::Id inst, RegisterParam* dest, RegisterParam* a, ParsedParam* b)
{
	EmitSmartMove(dest, a);
	Emit2(inst, dest, b);
}
