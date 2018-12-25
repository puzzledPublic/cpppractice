#include <future>
#include <iostream>
#include <thread>
#include <utility>

//std::promise, std::future 사용 예제
//promise와 future 사이에는 일대일 관계가 성립한다.(shared_future는 예외)
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
	
	//프로미스 정의
	std::promise<int> prodPromise;
	std::promise<int> divPromise;

	//퓨처 받기
	std::future<int> prodResult = prodPromise.get_future();
	std::future<int> divResult = divPromise.get_future();

	//별도의 스레드로 결과 계산
	std::thread prodThread(product, std::move(prodPromise), a, b);
	Div div;
	std::thread divThread(div, std::move(divPromise), a, b);

	//결과 받기
	std::cout << "20 * 10 = " << prodResult.get() << std::endl;
	std::cout << "20 / 10 = " << divResult.get() << std::endl;

	prodThread.join();
	divThread.join();

	std::cout << std::endl;
}