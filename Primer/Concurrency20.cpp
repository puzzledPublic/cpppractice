#include <future>
#include <thread>
#include <iostream>
#include <utility>

//std::shared_future ��� ����
//promise�� shared_future�� �ϴ�� �����̴�.
//shared_future�� promise�� �����Ǿ� ����� ���������� ��û�� �� �ִ�.
//�ΰ��� ������� ���� �� �ִ�.
//1. std::future�� fut�� fut.share()�� ȣ��
//2. std::promise�κ��� std::shared_future�� �ʱ�ȭ(ex.. std::shared_future<int> divResult = divPromise.get_future())
//std::future�� �̵������� �ݴ�� std::shared_future�� ����� �� �ִ�.
std::mutex coutMutex;

struct Div {
	void operator()(std::promise<int>&& intPromise, int a, int b) {
		intPromise.set_value(a / b);
	}
};

struct Requestor {
	void operator()(std::shared_future<int> shaFut) {
		//std::cout lock
		std::lock_guard<std::mutex> coutGuard(coutMutex);
		std::cout << "thread id(" << std::this_thread::get_id() << "): ";
		std::cout << "20 / 10 = " << shaFut.get() << std::endl;
	}
};

int main() {
	std::cout << std::endl;
	//promise ����
	std::promise<int> divPromise;
	//shared future �ޱ�
	std::shared_future<int> divResult = divPromise.get_future();
	//������ ������� ��� ���
	Div div;
	std::thread divThread(div, std::move(divPromise), 20, 10);

	Requestor req;
	std::thread sharedThread1(req, divResult);
	std::thread sharedThread2(req, divResult);
	std::thread sharedThread3(req, divResult);
	std::thread sharedThread4(req, divResult);
	std::thread sharedThread5(req, divResult);

	divThread.join();

	sharedThread1.join();
	sharedThread2.join();
	sharedThread3.join();
	sharedThread4.join();
	sharedThread5.join();

	std::cout << std::endl;
	return 0;
}