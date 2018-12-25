#include <future>
#include <iostream>
#include <thread>
#include <deque>
#include <utility>
//std::packagd_task는 callable의 비동기 호출을 위한 wrapper이다.	std::async에서 내부적으로도 사용한다.
//get_future로 연동된 퓨처를 생성하며, 호출 연산자를 통해 callable이 실행된다.
//크게 4단계로 수행한다.
//1. 작업(task)을 래핑한다.
//2. 퓨처를 생성한다.
//3. 계산을 수행한다.
//4. 결과를 조회한다.
class SumUp {
public:
	int operator()(int beg, int end) {
		long long int sum{ 0 };
		for (int i = beg; i < end; ++i) {
			sum += i;
		}
		return sum;
	}
};

int main() {
	std::cout << std::endl;
	//Callable 객체
	SumUp sumUp1;
	SumUp sumUp2;
	SumUp sumUp3;
	SumUp sumUp4;

	//1. Task Wrapping
	//int(int, int) = int 2개를 파라미터로, int를 리턴으로 하는 callable 객체를 받는다는것을 의미
	std::packaged_task<int(int, int)> sumTask1(sumUp1);
	std::packaged_task<int(int, int)> sumTask2(sumUp2);
	std::packaged_task<int(int, int)> sumTask3(sumUp3);
	std::packaged_task<int(int, int)> sumTask4(sumUp4);

	//2. 퓨처 생성
	std::future<int> fut1 = sumTask1.get_future();
	std::future<int> fut2 = sumTask2.get_future();
	std::future<int> fut3 = sumTask3.get_future();
	std::future<int> fut4 = sumTask4.get_future();

	//컨테이너에 태스크 넣기
	std::deque<std::packaged_task<int(int, int)>> allTasks;
	allTasks.push_back(std::move(sumTask1));
	allTasks.push_back(std::move(sumTask2));
	allTasks.push_back(std::move(sumTask3));
	allTasks.push_back(std::move(sumTask4));

	int begin{ 1 };
	int increment{ 2500 };
	int end = begin + increment;

	//3. 계산을 수행한다.(여기선 개별 스레드에서 계산을 수행한다)
	while (!allTasks.empty()) {
		std::packaged_task<int(int, int)> myTask = std::move(allTasks.front());
		allTasks.pop_front();
		std::thread sumThread(std::move(myTask), begin, end);
		begin = end;
		end = end + increment;
		sumThread.detach();
	}

	//4. 결과를 조회한다.
	auto sum = fut1.get() + fut2.get() + fut3.get() + fut4.get();

	std::cout << "Sum of 0 .. 10000 = " << sum << std::endl;
	std::cout << std::endl;
}