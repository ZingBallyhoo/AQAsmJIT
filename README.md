# AQAsmJIT

JIT compiler some for some weird AQA assembly language. 

Spec: https://filestore.aqa.org.uk/resources/computing/AQA-75162-75172-ALI.PDF

# How to build:
* Copy asmjit headers into include/asmjit
* Open in Visual Studio and build
* Go to lib\Release or lib\Debug and copy asmjit.dll to the build output (x86\Release or x64\Release)

**[Prebuilt binaries can be found on the Releases tab](https://github.com/ZingBallyhoo/AQAsmJIT/releases)**

# How to run:
`AQAsmJIT.exe <code file>`

Will execute the code in the file and then print off all register values.

# Code files:
Example code files can be found in [AQAsmJIT\code](https://github.com/ZingBallyhoo/AQAsmJIT/tree/master/AQAsmJIT/code)
* [`example_code.txt`](https://github.com/ZingBallyhoo/AQAsmJIT/blob/master/AQAsmJIT/code/example_code.txt)
  * Official example from AQA. [from a test paper, Q05.2](https://filestore.aqa.org.uk/sample-papers-and-mark-schemes/2017/june/AQA-75172-QP-JUN17.PDF) ([mark scheme](https://filestore.aqa.org.uk/sample-papers-and-mark-schemes/2017/june/AQA-75172-W-MS-JUN17.PDF))
* [`test_registers.txt`](https://github.com/ZingBallyhoo/AQAsmJIT/blob/master/AQAsmJIT/code/test_registers.txt)
  * Tests that all registers are written to the output, and are saved onto and restored from the stack correctly.
* [`test_cmp.txt`](https://github.com/ZingBallyhoo/AQAsmJIT/blob/master/AQAsmJIT/code/test_cmp.txt)
  * Small thing to test that basic comparisions work correctly. This file also tests multi-halt handling

# How it works
* Each of the 13 (0-12) fake registers are mapped to a real x64-16 register. There aren't enough hardware registers on 32bit
* rcx used for memory buffer.
* rdx used for fake register output.
* The AQA instructions are converted to one or two x86-64 instructions and jitted with asmjit.
* All used hardware registers are saved on the stack at the beginning of execution to prevent corruption.
* At the end of execution, registers are restored from the stack and used registers are written into the "register dump" so they can be viewed.

Example of generated code (from `example_code.txt`):

![x86 code from example_code.txt](https://i.imgur.com/FAtm8Wp.png)

# Todo:
* Changing base memory requries recompile. Currently set up for `example_code.txt`.
* Better error handling. Will silently die if no HALT.
* 16 bit registers. I interpreted this from a question so I don't know if this is official or not.
* Label names are limited to 16 bytes.
* [Is `RTN` a real instruction?](https://github.com/TreeveWhite/Assembly-SDK/blob/master/AssemblyEmmulator/bin/Debug/backend.py#L227)
  * Doesn't make sense to me, ambiguous if execution will come back to after BXX or not. e.g how x86 has call and jump, where call comes back and jump doesn't

# Related projects:
* https://github.com/TreeveWhite/Assembly-SDK
* https://github.com/ZanderBrown/aqabler
* https://github.com/jack-parsons/Assembly-Interpreter-AQA

# Dependencies:
* [asmjit](https://github.com/asmjit/asmjit)
