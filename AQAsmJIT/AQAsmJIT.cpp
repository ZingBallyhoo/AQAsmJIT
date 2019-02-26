#include <fstream>
#include <vector>
#include "trim.h"
#include "parser.h"
#include "JIT.h"
#include "perf.h"

int main(const int argc, char *argv[])
{
	if (argc == 1) {
		printf("missing required argument: code file");
		return 1;
	}
    auto stream = std::ifstream(argv[1]);
	if (!stream.good())
	{
		printf("code file doesn't exist");
		return 1;
	}

	const auto perfToken = perf::start();

	auto parsed = parse_all(JIT::InstructionMap, stream);
    auto jit = JIT();
	jit.BeginFunc();
	for (auto && parsedInstruction : parsed)
	{
		jit.CallAssemble(parsedInstruction.Instruction, parsedInstruction.Params);
	}

	const auto function = jit.FinishFunc();

	const auto mem = reinterpret_cast<short*>(malloc(200 * sizeof(short)));
	mem[100] = 10;
	mem[101] = 50;
	mem[102] = 80;
	auto dump = RegisterDump();
	function(mem, &dump);
	free(mem);

	const auto elapsedTime = perf::get_elapsed_ms(perfToken);
	printf("execution finished in %fms\n", elapsedTime);
    for (int i = 0; i <= 12; ++i)
    {
		printf("R%d: %d\n", i, *(reinterpret_cast<short*>(&dump) + i));
    }

	stream.close();
	return 0;
}
