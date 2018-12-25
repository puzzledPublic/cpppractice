#include <future>
#include <iostream>
#include <thread>
#include <utility>

//std::promise, std::future ��� ����
//promise�� future ���̿��� �ϴ��� ���谡 �����Ѵ�.(shared_future�� ����)
void product(std::promise<int>&& intPromise, int a, int b) {
	intPromise.set_value(a * b);
}

struct Div {
	void operator()(std::promise<int>&& intPromise, int a, int b) const {
		intPromise.set_value(a / b);
	}
};

int main() {
	int a = 20, b = 10;

	std::cout << std::endl;
	
	//���ι̽� ����
	std::promise<int> prodPromise;
	std::promise<int> divPromise;

	//ǻó �ޱ�
	std::future<int> prodResult = prodPromise.get_future();
	std::future<int> divResult = divPromise.get_future();

	//������ ������� ��� ���
	std::thread prodThread(product, std::move(prodPromise), a, b);
	Div div;
	std::thread divThread(div, std::move(divPromise), a, b);

	//��� �ޱ�
	std::cout << "20 * 10 = " << prodResult.get() << std::endl;
	std::cout << "20 / 10 = " << divResult.get() << std::endl;

	prodThread.join();
	divThread.join();

	std::cout << std::endl;
}