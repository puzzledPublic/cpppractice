#include <future>
#include <iostream>
#include <thread>

int main() {
	auto fut = std::async(std::launch::deferred, []() {
		std::cout << "deffered async " << std::this_thread::get_id() << std::endl;
	});
	auto fut2 = std::async(std::launch::async, []() {
		std::cout << "async " << std::this_thread::get_id() << std::endl;
	});
	fut2.get();
	fut.get();
	std::cout << std::this_thread::get_id() << std::endl;
	return 0;
}