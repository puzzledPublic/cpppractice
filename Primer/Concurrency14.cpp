#include <mutex>
#include <thread>
#include <iostream>

std::mutex mut;
int main() {
	std::thread t1([]() {
		std::unique_lock<std::mutex> ulock(mut);
		for (int i = 0; i < 100; i++) {
			std::cout << "t1" << std::endl;
		}
	});
	std::thread t2([]() {
		std::unique_lock<std::mutex> ulock(mut);
		for (int i = 0; i < 100; i++) {
			std::cout << "t2" << std::endl;
		}
	});

	t1.join();
	t2.join();
	return 0;
}