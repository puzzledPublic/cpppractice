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
//������ �������� �̵�
int main() {
	std::thread t1(some_function);
	std::thread t2 = std::move(t1);		//��������� �̵�
	t1 = std::thread(some_other_function);	//�Ͻ������� �̵��Ѵ�, �ӽð�ü�̹Ƿ�
	std::thread t3;
	t3 = std::move(t2);
	//t1 = std::move(t3);	//����!!!	t1�� ������ ������(���� some_other_function�� �����ϴ� ������)�� terminate()�ǰ� t3�� �����尡 t1���� �̵��Ѵ�.
	t1.join();
	t3.join();
}