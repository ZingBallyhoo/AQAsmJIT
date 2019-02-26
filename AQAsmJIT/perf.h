#pragma once
#include "windows.h"

namespace perf
{
	struct Token final
	{
		LARGE_INTEGER Start;
	};

	inline void start(Token* token)
	{
		QueryPerformanceCounter(&token->Start);
	}

	inline Token* start()
	{
		const auto token = new Token();
		start(token);
		return token;
	}

	inline double get_elapsed_ms(const Token* token)
	{
		LARGE_INTEGER end;
		QueryPerformanceCounter(&end);

		LARGE_INTEGER frequency;  // ticks per second
		QueryPerformanceFrequency(&frequency); // get ticks per second

		// compute the elapsed time in millisec
		const auto elapsedTime = (end.QuadPart - token->Start.QuadPart) * 1000.0 / frequency.QuadPart;
		return elapsedTime;
	}
}