#include <iostream>>
#include <thread>
#include <memory>

void some_function() {
	for (int i = 0; i < 100; i++) {
		std::cout << "some" << std::endl;
	}
}
void some_other_function() {
	for (int i = 0; i < 100; i++) {
		std::cout << "some_other" << std::endl;
	}
}
//스레드 소유권의 이동
int main() {
	std::thread t1(some_function);
	std::thread t2 = std::move(t1);		//명시적으로 이동
	t1 = std::thread(some_other_function);	//암시적으로 이동한다, 임시객체이므로
	std::thread t3;
	t3 = std::move(t2);
	//t1 = std::move(t3);	//위험!!!	t1에 연관된 스레드(현재 some_other_function을 실행하는 스레드)가 terminate()되고 t3의 스레드가 t1으로 이동한다.
	t1.join();
	t3.join();
}