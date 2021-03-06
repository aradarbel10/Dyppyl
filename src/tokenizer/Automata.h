#pragma once

#include <string>
#include <cctype>
#include <iostream>

namespace dpl {

	class GenericDFA {
	public:

		virtual void step(char c) = 0;
		virtual bool isAccepted() const = 0;

		virtual void setAlive() {
			current_state = 0;
		}

		virtual bool isAlive() const {
			return current_state != -1;
		}

		virtual int getAge() const {
			return age;
		}

		virtual void kill() {
			current_state = -1;
			age = 0;
		}

	protected:

		int current_state = 0;
		int age = 0;

	};

	class LinearDFA : public GenericDFA { // regex = specificword
	public:

		LinearDFA() { }
		LinearDFA(const char* s_) : states(s_) { }
		LinearDFA& operator=(std::string_view s_) { states = s_; return *this; }

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

		std::string_view getStates() const { return states; }

	private:

		std::string_view states;

	};


	class IdentifierDFA : public GenericDFA { // regex = [a-z,A-Z,_][a-z,A-Z,_,0-9]*
	public:

		bool isAccepted() const override {
			return current_state == 2;
		}

		void step(char c) override {
			if (!isAlive()) return;

			if (current_state == 0) { // waiting for init
				if (std::isalpha(c) || c == '_' || c == '\'') {
					age++;
					current_state++;
				} else {
					kill();
				}
			} else if (current_state == 1) { // waiting for init / any
				if (std::isalpha(c) || c == '_' || c == '\'' || std::isdigit(c)) {
					age++;
				} else {
					current_state = 2;
				}
			} else if (current_state == 2) { // accept
				kill();
			}
		}
	};

	class NumberDFA : public GenericDFA { // regex = [+,-]?[0-9]+(.[0-9]+)?
	public:

		bool isAccepted() const override {
			return current_state == 4;
		}

		void step(char c) override {
			if (!isAlive()) return;

			if (current_state == 0) {
				if (c == '+' || c == '-') {
					age++;
					current_state = 1;
				} else if (std::isdigit(c)) {
					age++;
					current_state = 2;
				} else {
					kill();
				}
			} else if (current_state == 1) {
				if (std::isdigit(c)) {
					age++;
					current_state = 2;
				} else {
					kill();
				}
			} else if (current_state == 2) {
				if (std::isdigit(c)) {
					age++;
				} else if (c == '.') {
					age++;
					current_state = 3;
				} else {
					current_state = 4;
				}
			} else if (current_state == 3) {
				if (std::isdigit(c)) {
					age++;
				} else {
					current_state = 4;
				}
			} else if (current_state == 4) {
				kill();
			}
		}

	};

	class StringDFA : public GenericDFA {
	public:

		bool isAccepted() const override {
			return current_state == 3;
		}

		void step(char c) override {
			if (!isAlive()) return;

			if (current_state == 0) {
				recent_string.clear();

				if (c == '\"') {
					age++;
					current_state = 1;
				} else {
					kill();
				}
			} else if (current_state == 1) {
				if (c == '\"') {
					age++;
					current_state = 3;
				} else if (c == '\\') {
					age++;
					current_state = 2;
				} else {
					age++;
					recent_string += c;
				}
			} else if (current_state == 2) {
				age++;
				current_state = 1;

				switch (c) {
				case 'n':
					recent_string += '\n';
					break;
				case 't':
					recent_string += '\t';
					break;
				case '\"':
					recent_string += '\"';
					break;
				case '\'':
					recent_string += '\'';
					break;
				case '\\':
					recent_string += '\\';
					break;
				case '0':
					recent_string += '\0';
					break;
				default:
					break;
				}
			} else if (current_state == 3) {
				kill();
			}
		}

		// #TASK : what the fuck is this aweful design.. fix fast fix soon
		inline static std::string recent_string = "";

	};
}