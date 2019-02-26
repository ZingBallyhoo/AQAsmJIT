#pragma once
#include <utility>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include "trim.h"

enum class SourceParamType
{
	Register,
	MemoryAddress,
	Dynamic, // <operand2>, constant or register
	Label
};

enum class ParsedParamType
{
	Register,
	MemoryAddress,
	Constant,
	Label
};

class RegisteredInstruction final
{
public:
	std::string Name;
	void* Func{};
	std::vector<SourceParamType> Params;

	RegisteredInstruction() = default;
	RegisteredInstruction(std::string name, void* func) : Name(std::move(name)), Func(func) {};
	RegisteredInstruction(std::string name, void* func, std::vector<SourceParamType> params) : Name(std::move(name)), Func(func), Params(std::move(params)) {}
};

struct ParsedParam
{
	union
	{
		char register_idx;
		short memory_address;
		short constant_value;
		char label_id[16];
	} Data;
	ParsedParamType Type;

	ParsedParam() = default;

	explicit ParsedParam(const ParsedParamType type) : Data(), Type(type) {}

	static void FromRegister(ParsedParam& param, const char index)
	{
		param.Type = ParsedParamType::Register;
		param.Data.register_idx = index;
	}

	static void FromConstant(ParsedParam& param, const short value)
	{
		param.Type = ParsedParamType::Constant;
		param.Data.constant_value = value;
	}

	static void FromMemoryAddress(ParsedParam& param, const short address)
	{
		param.Type = ParsedParamType::MemoryAddress;
		param.Data.constant_value = address;
	}

	static void FromLabel(ParsedParam& param, std::string& id)
	{
		param.Type = ParsedParamType::Label;
		strcpy_s(param.Data.label_id, id.c_str());
	}
};

class ParsedInstruction final
{
public:
	RegisteredInstruction* Instruction = nullptr;
	std::vector<ParsedParam> Params;

	explicit ParsedInstruction(RegisteredInstruction* inst)
	{
		Instruction = inst;
		Params = std::vector<ParsedParam>(inst->Params.size());
	}
};

struct RegisterParam : ParsedParam {
	explicit RegisterParam() : ParsedParam(ParsedParamType::Register) {}
};
struct MemoryAddressParam : ParsedParam
{
	explicit MemoryAddressParam() : ParsedParam(ParsedParamType::MemoryAddress) {}
};
struct ConstantParam : ParsedParam
{
	explicit ConstantParam() : ParsedParam(ParsedParamType::Constant) {}
};
struct LabelParam : ParsedParam
{
	explicit LabelParam() : ParsedParam(ParsedParamType::Label) {}
};

inline std::vector<std::string> split_args(const std::string& str)
{
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(str);
	while (std::getline(tokenStream, token, ','))
	{
		trim(token); // remove space
		tokens.push_back(token);
	}
	return tokens;
}

static void parse_register(ParsedParam& param, std::string& string)
{
	// R10
	const auto number = string.substr(1, string.length() - 1);
	const auto index = std::stoi(number);
	return ParsedParam::FromRegister(param, index);
}

static void parse_constant(ParsedParam& param, std::string& string)
{
	// #1234
	return ParsedParam::FromConstant(param, std::stoi(string.substr(1, string.length())));
}

static void parse_memory_address(ParsedParam& param, std::string& string)
{
	// 100
	return ParsedParam::FromMemoryAddress(param, std::stoi(string));
}

static void parse_dynamic(ParsedParam& param, std::string& string)
{
	// #100
	// R10
	if (string._Starts_with("#"))
	{
		return parse_constant(param, string);
	}
	if (string._Starts_with("R"))
	{
		return parse_register(param, string);
	}
	throw std::exception("unhandled dynamic param");
}

static std::vector<ParsedInstruction> parse_all(std::unordered_map<std::string, RegisteredInstruction*> instructionMap, std::ifstream& stream)
{
	std::vector<ParsedInstruction> output;
	for (std::string line; getline(stream, line); )
	{
		trim(line);
		auto spaceIdx = static_cast<int>(line.find_first_of(' '));
		std::vector<std::string> params;

		std::string instName;
		if (spaceIdx == -1 && line.back() == ':')
		{
			params.push_back(line.substr(0, line.length() - 1));
			instName = "BIND_LABEL";
		}
		else
		{
			if (spaceIdx == -1) spaceIdx = static_cast<int>(line.length());
			else
			{
				auto argString = line.substr(spaceIdx + 1, line.length() - spaceIdx);
				params = split_args(argString);
			}
			instName = line.substr(0, spaceIdx);
		}

		if (instructionMap.count(instName) == 0) throw std::exception("unimplemented instruction");
		auto inst = instructionMap[instName];

		ParsedInstruction parsedInst = ParsedInstruction(inst);
		for (auto i = 0; i < static_cast<int>(params.size()); ++i)
		{
			const auto type = inst->Params[i];
			auto param = params[i];
			auto parsedParam = ParsedParam();

			if (type == SourceParamType::Register)
			{
				parse_register(parsedParam, param);
			}
			else if (type == SourceParamType::MemoryAddress)
			{
				parse_memory_address(parsedParam, param);
			}
			else if (type == SourceParamType::Dynamic)
			{
				parse_dynamic(parsedParam, param);
			}
			else if (type == SourceParamType::Label)
			{
				ParsedParam::FromLabel(parsedParam, param);
			}
			else
			{
				throw std::exception("unhandled SourceParamType");
			}
			parsedInst.Params[i] = parsedParam;
		}
		
		output.push_back(parsedInst);
	}
	return output;
}