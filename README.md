# Dyppyl
A parser generator library written in modern C++.

<img src="promotional/logo.png" alt="logo" width="250">

## Example(s)
NOTE: Dyppyl is under constant developments, so some examples might not match the current state of the code.
```cpp
// define grammar & parser
auto [grammar, lexicon] = (
	dpl::discard |= dpl::kleene{dpl::whitespace},

	"num"t	|= dpl::Lexeme{ dpl::some{dpl::digit}, [](std::string_view str) -> int { return dpl::from_string<int>(str); } },

	"E"nt	|= ("E"nt, !"+"t, "E"nt) & dpl::Assoc::Left, dpl::Prec{5}
			|  ("E"nt, !"*"t, "E"nt) & dpl::Assoc::Left, dpl::Prec{10}
			|  (~"("t, "E"nt, ~")"t)
			|  (!"num"t)
);
dpl::LR1 parser{ grammar, lexicon };

// use parser
dpl::ParseTree tree = parser2.parse("1 + 2 * (3 + 4) * 5");
```

## What is Dyppyl?

Dyppyl is a parser-generator library written with modern C++20 features.

It's meant to be better than other parser generators by being more readable and much easier to learn & use.
Additionally, with Dyppyl being a library, the grammar description happens directly inside your C++ source, so it's very convenient to integrate to your projects. In the future it is planned to support *compile-time parser construction* as to avoid the (usually small) runtime overhead imposed by the library.

## How's progress?

Here's the roadmap for Dyppyl's development (keep in mind it's always changing and evolving!)
- [x] highly customizable lexer
- [ ] various parser types
	- [x] LL(1)
	- [x] LR(0)
	- [x] LR(1)
	- [x] SLR(1)
	- [x] LALR(1)
- [x] embedded grammar specification language
- [ ] compile-time parser generation
- [ ] Parse Tree / AST handling library
	- [x] tree pattern matching
	- [x] tree traversals and visiting
	- [x] parse tree to AST conversion

## How does it work?

The theoretical background that goes into this project is pretty advanced, so you don't need to worry about it.
But if you *do* want to know all the tiniest details, check out [Stanford's CS143 Course](https://web.stanford.edu/class/archive/cs/cs143/cs143.1128/) which is where a lot of my knowledge comes from.
If you have a hard time understanding some concepts feel free to ask for my help on Discord (AradArbel10#3813 or on [the PLT server](https://discord.gg/4Kjt3ZE)) and maybe we'll figure it out together -- I'm still learning too!

## How can I use Dyppyl?
Dyppyl is a fully header-only library, so if you want to use it yourself you just need to get all the headers into your project*.

Notice the code is divided into many different files (for example, different files for different kinds of parsers). If you plan on using only certain features, take only them. But keep in mind files may depend on one another.

*currently there are third-party dependencies that Dyppyl relies on, but some of these likely will be eliminated in the future. you can find them in the `deps/` folder.

## How can I contribute?
The repository doesn't follow any particular formatting style. You are always welcome to contribute examples, fixes, etc... For larger features you should talk to me so we can work together!

## Some Old Examples
These are quite outdated by now, but I keep them here anyway.
#### LL(1) Grammar in-code with generated parse tree
![LL1 example](promotional/LL1.png)

#### LR(1) Grammar with generated parse tree
![LR1 example](promotional/LR1.png)