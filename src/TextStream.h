#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

namespace dpl{
	class TextStream {
	public:

		virtual char fetchNext() = 0;
		virtual bool closed() = 0;

	};

	class StringStream : public TextStream {
	public:

		StringStream(std::string_view str_) : str(str_) { }

		char fetchNext() override {
			++pos;
			if (pos - 1 < str.length()) return str[pos - 1];
			else if (pos - 1 == str.length()) return '\n';
			else return '\0';
		}

		bool closed() override {
			return pos > str.length() + 1;
		}

	//private:

		std::string_view str;
		size_t pos = 0;

	};

	class FileStream : public TextStream {
	public:

		FileStream(std::filesystem::path dir) : file(dir) {
			file >> std::noskipws;

			if (!file.is_open()) std::cerr << "Can't open file at " << dir << '\n';
			else std::cout << "Successfully opened file " << dir << '\n';
		}

		char fetchNext() override {
			char out;
			file >> out;

			if (is_eof) {
				is_closed = true;
				return '\0';
			} else if (file.eof()) {
				is_eof = true;
				return '\n';
			} else return out;
		}

		bool closed() override {
			return is_closed;
		}

		void close() { file.close(); }
		~FileStream() { this->close(); }

	private:

		std::ifstream file;
		bool is_eof = false;
		bool is_closed = false;

	};
}