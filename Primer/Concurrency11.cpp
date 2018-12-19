#include <iostream>
#include <thread>
#include <mutex>

std::once_flag onceFlag;

//std::call_once, std::once_flag 예제
void do_once() {
	std::call_once(onceFlag, []() {std::cout << "Only once." << std::endl; });
}

int main() {
	std::cout << std::endl;
	//4개의 스레드가 do_once 함수를 실행하나 do_once내의 std::call_once에 넘겨준 람다함수는 한번만 호출된다.
	std::thread t1(do_once);
	std::thread t2(do_once);
	std::thread t3(do_once);
	std::thread t4(do_once);

	t1.join();
	t2.join();
	t3.join();
	t4.join();

	std::cout << std::endl;
}