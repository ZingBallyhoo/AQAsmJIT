#pragma once

// https://stackoverflow.com/questions/46010878/c-call-function-pointer-with-any-amount-of-variables
template <typename R, typename... Args>
R call_partial_function(void* function, Args... args)
{
	typedef R(*Function)(Args...);
	const auto typed_function = static_cast<Function>(function);
	return typed_function(args...);
}