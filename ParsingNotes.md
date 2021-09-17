Breadth First Search
	- create a queue initialized with a nonterminal representing all programs
	- for every element in the queue:
    	- expand all nonterminals and push new results to the queue
    	- continue until you match the entire sentence.
	this is slow and inefficient... yuch

Leftmost Breadth First Search
	- consider only leftmost derivations while searching
	- prune sentences that start with terminals that don't match the target
	reduced expantion rate -> good for us

	BUT exponential time & memory to resolve left-recursion

Leftmost Depth First Search
	- a lot more efficient for most simple grammars
	- linear memory

	BUT breaks completely on left-recursion

Predictive Parsers
	- much more efficient (can reach linear time)

	BUT less grammars work with it
	- more lookahead == more grammars

	BUT can be much more complicated to implement

LL(1) Predictive Parser
	- top-down parsing
	- fairly efficient for simple grammars
	- shift-reduce concept
	- trivially detect syntax errors


	parsing alg: [03, 214]
	- initialize stack with S (starting symbol) followed by a unique EOF marker
	- until the stack is empty:
    	- if the top of the stack is a terminal
        	- if it doesn't match the next token, report an error
        	- otherwise consume the matching terminals
      	- if the top of the stack is a nonterminal
        	- if there is no prediction defined for the current inputs, report an error
        	- otherwise use prediction
	

	definition of FIRST sets: [03, 234]
	forall nonterminal A.
	FIRST(A) = { all terminals that A could eventually begin with }

	computing FIRST sets: [03, 235]
	- for all productions that begin with a terminal, add that terminal to the set
	- then repeat until no changes occur:
    	- for each production that begins with a nonterminal, add elements of FIRST of that non-terminal to the set


	alg for constructing LL(1) prediction table (epsilon-productions not allowed): [03, 278]
	- compute FIRST sets for all nonterminals
	- for each nonterminal (row) in the table
    	- for each production rule that begins with a terminal, put the production rule in the terminal's column
    	- for each production rule that begins with a non-terminal, put the production rule in all the columns of the non-terminal's FIRST elements
	

	computing FIRST sets with epsilon: [03, 302]
	- for all productions that begin with a terminal, add that terminal to the set
	- for all nonterminals with an epsilon-production, add epsilon to the set
	- then repeat until no changes occur:
    	- for each production which is a string of nonterminals, which all have epsilon in FIRST, add epsilon to the set
    	- for each production which begins with a string of nonterminals, which all have epsilon in FIRST
        	- if the following is a terminal, add it to the set
        	- if the following is a nonterminal, add to the list the non-epsilon elements of its FIRST


	defining FIRST* for strings: [03, 303]
	- FIRST* of epsilon is epsilon
	- FIRST* of a string that begins with a terminal is that terminal
	- FIRST* of a string that begins with a nonterminal
    	- if epsilon isn't in that nonterminal's FIRST, FIRST* is FIRST of that nonterminal
    	- but if it is in that nonterminal's FIRST, FIRST* is FIRST of that nonterminal, minus epsilon, plus FIRST* of the remaining string


	the FIRST computation alg (with epsilon) can be re-written as: [03, 304]
	- for all productions that begin with a terminal, add that terminal to the set
	- for all nonterminals with an epsilon-production, add epsilon to the set
	- then repeat until no changes occur:
    	- for each production, add the elements of FIRST* of the resulting string to the set


	when constructing LL(1) table with epsilon-productions:
	- add extra column for the EOF marker


	defining FOLLOW sets: [03, 359]
	set of terminal that might come after a given nonterminal,

	FOLLOW = {t | recursive production rules S ->* aAtw where
		a sentence
		w sentence
		A the nonterminal
		t a terminal
	}


	computing FOLLOW sets: [04A, 32]


	final LL(1) alg:
	- compute FIRST and FOLLOW for all nonterminals