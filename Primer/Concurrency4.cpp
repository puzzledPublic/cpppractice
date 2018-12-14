#include <iostream>
#include <thread>
#include <memory>

//std::thread�� ���� Move �����ڸ� ���� thread�� ��ȣ�ϴ� Ŭ����
class scoped_thread {
public:
	explicit scoped_thread(std::thread _t) : t(std::move(_t)) {
		if (!t.joinable()) {
			throw std::logic_error("No thread");
		}
	}
	~scoped_thread() {
		std::cout << "scoped thread �Ҹ��� ȣ��" << std::endl;
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
	//main�� ������ t�� �Ҹ��ڰ� ȣ��ɶ� std::thread�� join()�� ȣ��ȴ�.
	return 0;
}