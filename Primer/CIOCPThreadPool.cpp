#include <iostream>
#include "CIOCPThreadPool.h"

void WINAPI test(PVOID pParam) {
	for (int i = 0; i < 100; i++) {
		std::cout << "hello" << (LONG)pParam << "\n";
	}
}
int main() {
	IocpThreadPool itp;
	itp.Init(2, 2);

	itp.Enqueue(test, (PVOID)2);
	itp.Enqueue(test, (PVOID)3);
	itp.Enqueue(test, (PVOID)4);

	itp.Uninit();
	return 0;
}