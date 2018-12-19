#include <iostream>
#include <thread>
#include <mutex>

std::once_flag onceFlag;

//std::call_once, std::once_flag ����
void do_once() {
	std::call_once(onceFlag, []() {std::cout << "Only once." << std::endl; });
}

int main() {
	std::cout << std::endl;
	//4���� �����尡 do_once �Լ��� �����ϳ� do_once���� std::call_once�� �Ѱ��� �����Լ��� �ѹ��� ȣ��ȴ�.
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