# Dyppyl
A compiler generator written in modern C++.

<img src="promotional/logo.png" alt="logo" width="250">

## What is Dyppyl?
Dyppyl is a compiler-generator library written with modern C++ features.
Usually you'd hear about parser generators, so what's a *compiler* generator?
At this early stage of development, Dyppyl is a plain parser generator. At the time of writing, it can create parsers of these types:
- LL(1)
- LR(0)
- LR(1)

In the future it will hopefully provide many more useful utilities to be used in all stages of the compiler construction process, including an advanced tokenizer, various more frontend parsers, backend code-generation stage, and even a runtime environment for whatever IR you are omitting.

The cool thing about Dyppyl is that unlike other parser generators, it doesn't spit out a source code in some other language.
Instead, Dyppyl will rely on C++'s compile-time metaprogramming facilities to generate the parser (and other stages) directly as an object file, lib/dll, or even executable if your heart desires so.
This gives an added advantage: Dyppyl can be also used as a good ol' runtime library, generating parsers dynamically at runtime. I'm not sure how many actual uses this may have, but it may still be fun to experiment with.

## How's progress?

At the moment I am mostly working on implementing as many unit tests as I can to make sure nothing breaks completely. There's also a lot of ongoing refactoring going on, and other miscellaneous tasks such as better debugging & error handling tools.

The next actual features that will be implemented (other than refactoring and bug fixes) are even more types of parsers. Specifically SLR(1) and LALR(1). I've considered the idea of supporting GLR, LL(k), and Early parsers too, though these are currently a little too big-scoped and are left for future versions.

For more detailed progress plans see this half-assed [Trello board](https://trello.com/b/u2pzCbZc/dyppyl#).
(cards are ordered left-to-right top-to-bottom from highest priority to lowest).

## How does it work?

The theoretical background that goes into this project is pretty advanced, so you don't need to worry about it.
But if you *do* want to know all the tiniest details, check out [Stanford's CS143 Course](https://web.stanford.edu/class/archive/cs/cs143/cs143.1128/) which Dyppyl is directly based on.  
If you have a hard time understanding some concepts feel free to ask for my help on Discord (AradArbel10#3813) and maybe we'll figure it out together -- I'm still learning too!

## Goals
- Provide utilities for all stages of compilation
    - At the time of writing, Dyppyl can generate a tokenizer, and a parser (LL(1), LR(0), LR(1)).
    I'd like to continue developing more modular stages for both the frontend and backend parts.
    - Currently the tokenizer supports very little customizability.
    For the future I plan on providing support for lexing based on any arbitrary regular expressions, and better integration with parsers.
    - Allow to modularly assemble backends that could export generated code in any format (C, ASM, bytecode, LLVM IR, etc...) and composed with third-party backends.
    - Include type checkers, static analysis stages, and debug info in the compilation process.

- High runtime performance
    - Most common parser generators produce a result that's compiled separately. With Dyppyl I intend to heavily utilize the metaprogramming capabilities of C++ to perform all the prerequired work at the time of compiling the parser itself.
    - Rather than a parser generator, this would work more similarly to a pre-compiled parsing library that you can include in your own projects.

## How can I use Dyppyl?
You can't. Yet.
But stay tuned!

## Examples
#### LL(1) Grammar in-code with generated parse tree
![LL1 example](promotional/LL1.png)

#### LR(1) Grammar with generated parse tree
![LR1 example](promotional/LR1.png)