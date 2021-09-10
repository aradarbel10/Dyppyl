#pragma once

#include <string>
#include <iostream>

namespace dpl {

	class LinearDFA {
	public:

		LinearDFA() : states("") {
			
		}

		LinearDFA(const std::string& s_) : states(s_) {
			states = s_;
		}

		void step(char c) {
			if (isAlive()) {
				if (states[current_state] == c) {
					current_state++;
					age++;
				} else {
					current_state = -1;
					age = 0;
				}
			}
		}

		bool isAccepted() const {
			return current_state == states.size();
		}

		void setAlive() {
			current_state = 0;
		}

		bool isAlive() const {
			return current_state != -1 && !isAccepted();
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

		std::string states = "";
		int current_state = -1;
		int age = 0;

	};

}