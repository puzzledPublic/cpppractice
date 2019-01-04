#include <iostream>
#include <chrono>
#include <condition_variable>
#include <mutex>

std::mutex mut;
std::condition_variable cond;
bool ready;

void waitFunc() {
	const auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(2);
	while (!ready) {
		std::unique_lock<std::mutex> lk(mut);
		if (cond.wait_until(lk, timeout) == std::cv_status::timeout) {
			std::cout << "timeout!!" << std::endl;
			break;
		}
		else {
			std::cout << "waiting..." << std::endl;
		}
		ready = false;
	}
}
void readyFunc() {
	for (int i = 0; i < 200; i++) {
		std::lock_guard<std::mutex> guard(mut);
		ready = true;
		std::cout << "ready " << i << std::endl;
		cond.notify_one();
		std::this_thread::sleep_for(std::chrono::microseconds(100));
	}
}
int main() {
	std::thread t1(readyFunc);
	std::thread t2(waitFunc);

	t1.join();
	t2.join();
}

