# Dyppyl
A compiler generator written in modern C++.

<img src="promotional/logo.png" alt="logo" width="250">

## Example(s)
NOTE: Dyppyl is under constant developments, so some examples might not match the current state of the code.
```cpp
// define grammar & parser
auto [grammar, lexicon] = (
	dpl::discard |= dpl::kleene{dpl::whitespace},

	"num"t	|= dpl::Lexeme{ dpl::some{dpl::digit}, [](std::string_view str) -> double { return dpl::stod(str); } },

	"E"nt	|= ("E"nt, "+"t, "E"nt) & dpl::Assoc::Left, dpl::Prec{5}
			|  ("E"nt, "*"t, "E"nt) & dpl::Assoc::Left, dpl::Prec{10}
			|  ("("t, "E"nt, ")"t)
			|  ("num"t)
);
dpl::LR1 parser{ grammar, lexicon };

// use parser
dpl::ParseTree tree = parser2.parse("1 + 2 * (3 + 4) * 5");

// transform from parse tree to AST
tree.replace_with(
	dpl::ParseTree{ "E", {{"("_sym}, {}, {")"_sym}}},
	[](const std::vector<dpl::ParseTree>& cs) { return cs[0]; }
);

tree.replace_with(
	dpl::ParseTree{ "E", {{}, {}, {}} },
	[](const std::vector<dpl::ParseTree>& cs) {
		return dpl::ParseTree{ cs[1].value, {
			(cs[0].match({"E"}) ? cs[0][0].value : cs[0]),
			(cs[2].match({"E"}) ? cs[2][0].value : cs[2])
		}};
	},
	dpl::TraverseOrder::BottomUp
);
```

## What is Dyppyl?

Dyppyl is a compiler-generator library written with modern C++ features.
Usually you'd hear about parser generators, so what's a *compiler* generator?
Dyppyl is mostly just a parser generator, but also includes a suite of tools dedicated specifically for compiler construction.

## How's progress?

Here's the roadmap for Dyppyl's development (keep in mind it's always changing and evolving!)
- [x] highly customizable lexer
- [ ] various parser types
	- [x] LL(1)
	- [x] LR(0)
	- [x] LR(1)
	- [x] SLR(1)
	- [ ] LALR(1)
	- [ ] LL(k)
	- [ ] GLR
- [x] embedded grammar specification language
- [ ] compile-time parser generation
- [ ] tree handling library
	- [x] tree pattern matching
	- [x] tree traversals and visiting
	- [ ] parse tree to AST conversion
	- [ ] tree scanning, folding, transformation
- [ ] semantic analysis
	- [ ] type checking and elaboration passes
	- [ ] scope analysis
- [ ] code generation utilities
	- [ ] intermediate representation
	- [ ] register allocators
	- [ ] optimization passes

For detailed progress plans see this very half-assed [Trello board](https://trello.com/b/u2pzCbZc/dyppyl#).
(cards are ordered left-to-right top-to-bottom from highest priority to lowest).

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
These are a little outdated by now, but I keep them here anyway.
#### LL(1) Grammar in-code with generated parse tree
![LL1 example](promotional/LL1.png)

#### LR(1) Grammar with generated parse tree
![LR1 example](promotional/LR1.png)