#pragma once

#include <string>
#include <iostream>

namespace dpl {

	class GenericDFA {
	public:

		virtual void step(char c) = 0;
		virtual bool isAccepted() const = 0;

		void setAlive() {
			alive = true;
			current_state = 0;
		}

		bool isAlive() const {
			return alive;
		}

		int getAge() const {
			return age;
		}

		void kill() {
			current_state = -1;
			age = 0;
			alive = false;
		}

	protected:

		int current_state = 0;
		bool alive = false;
		int age = 0;

	};

	class LinearDFA : public GenericDFA { // regex = specificword
	public:

		LinearDFA(const std::string& s_) : states(s_) { }
		LinearDFA(const char* s_) : states(s_) { }

		bool isAccepted() const override {
			return current_state == states.size();
		}

		void step(char c) override {
			if (!isAlive()) return;

			if (current_state < states.size()) {
				if (states[current_state] == c) {
					age++;
					current_state++;
				} else {
					kill();
				}
			} else {
				kill();
			}

			//if (current_state == 0) { // waiting for init
			//	if (isLetter(c) || c == '_') {
			//		age++;
			//		current_state++;
			//	} else {
			//		kill();
			//	}
			//} else if (current_state == 1) { // waiting for init / any
			//	if (isLetter(c) || c == '_' || isDigit(c)) {
			//		age++;
			//	} else {
			//		current_state = 2;
			//	}
			//} else if (current_state == 2) {
			//	kill();
			//}
		}

		friend std::ostream& operator<<(std::ostream& os, const LinearDFA& dfa) {
			for (int i = 0; i < dfa.states.size(); i++) {
				if (dfa.current_state == i) os << '[';
				os << dfa.states[i];
				if (dfa.current_state == i) os << ']';
			}

			os << "\t\t" << dfa.current_state << "/" << dfa.states.size();

			return os;
		}

	private:

		std::string states = "";

	};


	class IdentifierDFA : public GenericDFA { // regex = [a-z,A-Z,_][a-z,A-Z,_,0-9]*
	public:

		IdentifierDFA() { }

		bool isAccepted() const override {
			return current_state == 2;
		}

		void step(char c) override {
			if (!isAlive()) return;

			if (current_state == 0) { // waiting for init
				if (isLetter(c) || c == '_' || c == '\'') {
					age++;
					current_state++;
				} else {
					kill();
				}
			} else if (current_state == 1) { // waiting for init / any
				if (isLetter(c) || c == '_' || c == '\'' || isDigit(c)) {
					age++;
				} else {
					current_state = 2;
				}
			} else if (current_state == 2) {
				kill();
			}
		}

		static bool isLetter(char c) {
			//65-90, 97-122
			return (c >= 65 && c <= 90) || (c >= 97 && c <= 122);
		}

		static bool isDigit(char c) {
			//48-57
			return c >= 48 && c <= 57;
		}
	};

	// string DFA = "letters numbers ,!./\n \\ \""
	// number DFA = +-digits.digits
	// symbol DFA = basially the same as the identifier DFA but even simpler
	//				or --- use linear ones for that, just hard-code possible operators & delimeters
	//				(note: rumtime tokenizer will still have to use a soft-coded symbol lexer (but maybe still not I'll need to try it out first))

}

std::unique_ptr<dpl::LinearDFA> operator"" _ldfa(const char* c_, std::size_t s) {
	return std::make_unique<dpl::LinearDFA>(c_);
}