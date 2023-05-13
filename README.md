# The lisa programming language


### What is *lisa*?
*lisa* is an experimental programming language and compiler architecture designed for the simplest and easiest implementation of high performance AI/ML systems.

### The Compiler
The *lisa* compiler is currently only a frontend for the LLVM compiler infrastructure. It is used to generate LLVM IR code from the `.lisa` source files. The compiler is written in C++ and uses the LLVM C++ APIs. Future versions of the compiler will include advance features like JIT compiling, MLIR, machine-learning-based optimizations and so on.

### The Language
The *lisa* programming language is a simple and easy-to-use language designed for the development of machine learning software. It integrates commonly used tools and libraries in AI/ML systems development into its standard library, which will make it fast and easy to implement an intelligent system for research and production alike. It is designed with performance in mind, with powerful commands like `@GPU` or `@CPU(mode: "multithread")`, *lisa* is designed to provide simple tags to supercharge your code instantly.

---

<img height="65" alt="llvm" src="https://github.com/MiracleFactory/lisa/assets/89094576/e194ddff-48b7-421d-b470-c2f185ca3350">
