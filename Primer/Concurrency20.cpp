#include <future>
#include <thread>
#include <iostream>
#include <utility>

//std::shared_future 사용 예제
//promise와 shared_future는 일대다 관계이다.
//shared_future는 promise와 연동되어 결과를 독리벅으로 요청할 수 있다.
//두가지 방법으로 만들 수 있다.
//1. std::future인 fut에 fut.share()을 호출
//2. std::promise로부터 std::shared_future를 초기화(ex.. std::shared_future<int> divResult = divPromise.get_future())
//std::future는 이동하지만 반대로 std::shared_future는 복사될 수 있다.
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
	//promise 정의
	std::promise<int> divPromise;
	//shared future 받기
	std::shared_future<int> divResult = divPromise.get_future();
	//별도의 스레드로 결과 계산
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