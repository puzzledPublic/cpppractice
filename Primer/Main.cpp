#include <iostream>
#include <utility>

//#include "StrVec.h"
#include "MoveTest.h"
int main() {

	MyString ms("hellow");
	MyString ms2(ms);
	
	MyString ms3;

	ms3 = std::move(ms2);

	ms.print();
	ms2.print();
	ms3.print();
	return 0;
}
