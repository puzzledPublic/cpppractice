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
		threads.push_back(std::thread(func, (char)(i + 'a')));	//vector�� ��������ʰ� �̵��Ѵ�.
	}
	std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));	//vector�� ��ȸ�ϸ� �� thread�� join() ȣ��
}