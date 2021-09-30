# Dyppyl
A compiler generator written in modern C++.


# Examples
#### LL(1) Grammar in-code with generated parse tree
![LL1 example](promotional/LL1.png)

#### LR(1) Grammar with generated parse tree
![LR1 example](promotional/LR1.png)


# Goals
- Provide utilities for all stages of compilation
    - At the time of writing, Dyppyl can generate a tokenizer, and a parser (LL(1), LR(0), LR(1)).
    I'd like to continue developing more modular stages for both the frontend and parts of the backend.
    - Currently the tokenizer supports very little customizability.
    For the future I plan on providing support for lexing based on any arbitrary regular expressions.
    - Allow to modularly assemble backends that could export generated code in any format (C, ASM, bytecode, LLVM IR, etc...) and composed with third-party backends.

- High runtime performance
    - Most common parser generators produce a result that's compiled separately. With Dyppyl I intend to heavily utilize the metaprogramming capabilities of C++ to perform all the prerequired work at the time of compiling the parser itself.
    - Rather than a parser generator, this would work more similarly to a pre-compiled parsing library that you can include in your own projects.