#include <iostream>
#include <thread>
#include <vector>
#include <memory>
#include <algorithm>

void func(char s) {
	for (int i = 0; i < 10; i++) {
		std::cout << s << std::endl;
	}
}
//listing 2.7
int main() {
	std::vector<std::thread> threads;
	for (int i = 0; i < 20; i++) {
		threads.push_back(std::thread(func, (char)(i + 'a')));	//vector로 복사되지않고 이동한다.
	}
	std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));	//vector를 순회하며 각 thread에 join() 호출
}