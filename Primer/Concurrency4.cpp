#include <iostream>
#include <thread>
#include <memory>

//std::thread가 가진 Move 연산자를 통해 thread를 보호하는 클래스
class scoped_thread {
public:
	explicit scoped_thread(std::thread _t) : t(std::move(_t)) {
		if (!t.joinable()) {
			throw std::logic_error("No thread");
		}
	}
	~scoped_thread() {
		std::cout << "scoped thread 소멸자 호출" << std::endl;
		t.join();
	}
	scoped_thread(scoped_thread const&) = delete;
	scoped_thread& operator=(scoped_thread const&) = delete;
private:
	std::thread t;
};

void func() {
	for (int i = 0; i < 100; i++) {
		std::cout << "scoped thread" << std::endl;
	}
}
int main() {
	scoped_thread t(std::move(std::thread(func)));
	
	for (int i = 0; i < 100; i++) {
		std::cout << "main thread" << std::endl;
	}
	//main이 끝나며 t의 소멸자가 호출될때 std::thread의 join()도 호출된다.
	return 0;
}