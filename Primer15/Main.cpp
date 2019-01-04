#include <iostream>

#include "TextQuery.h"

int main() {
	std::ifstream file("./Text.txt");
	TextQuery tq(file);
	while (true) {
		std::cout << "enter word to look for, or q to quit: ";
		std::string s;
		if (!(std::cin >> s) || s == "q") {
			break;
		}
		print(std::cout, tq.query(s)) << std::endl;
	}
	return 0;
}