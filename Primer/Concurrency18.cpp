#include <future>
#include <iostream>
#include <thread>
#include <deque>
#include <utility>
//std::packagd_task�� callable�� �񵿱� ȣ���� ���� wrapper�̴�.	std::async���� ���������ε� ����Ѵ�.
//get_future�� ������ ǻó�� �����ϸ�, ȣ�� �����ڸ� ���� callable�� ����ȴ�.
//ũ�� 4�ܰ�� �����Ѵ�.
//1. �۾�(task)�� �����Ѵ�.
//2. ǻó�� �����Ѵ�.
//3. ����� �����Ѵ�.
//4. ����� ��ȸ�Ѵ�.
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
	//Callable ��ü
	SumUp sumUp1;
	SumUp sumUp2;
	SumUp sumUp3;
	SumUp sumUp4;

	//1. Task Wrapping
	//int(int, int) = int 2���� �Ķ���ͷ�, int�� �������� �ϴ� callable ��ü�� �޴´ٴ°��� �ǹ�
	std::packaged_task<int(int, int)> sumTask1(sumUp1);
	std::packaged_task<int(int, int)> sumTask2(sumUp2);
	std::packaged_task<int(int, int)> sumTask3(sumUp3);
	std::packaged_task<int(int, int)> sumTask4(sumUp4);

	//2. ǻó ����
	std::future<int> fut1 = sumTask1.get_future();
	std::future<int> fut2 = sumTask2.get_future();
	std::future<int> fut3 = sumTask3.get_future();
	std::future<int> fut4 = sumTask4.get_future();

	//�����̳ʿ� �½�ũ �ֱ�
	std::deque<std::packaged_task<int(int, int)>> allTasks;
	allTasks.push_back(std::move(sumTask1));
	allTasks.push_back(std::move(sumTask2));
	allTasks.push_back(std::move(sumTask3));
	allTasks.push_back(std::move(sumTask4));

	int begin{ 1 };
	int increment{ 2500 };
	int end = begin + increment;

	//3. ����� �����Ѵ�.(���⼱ ���� �����忡�� ����� �����Ѵ�)
	while (!allTasks.empty()) {
		std::packaged_task<int(int, int)> myTask = std::move(allTasks.front());
		allTasks.pop_front();
		std::thread sumThread(std::move(myTask), begin, end);
		begin = end;
		end = end + increment;
		sumThread.detach();
	}

	//4. ����� ��ȸ�Ѵ�.
	auto sum = fut1.get() + fut2.get() + fut3.get() + fut4.get();

	std::cout << "Sum of 0 .. 10000 = " << sum << std::endl;
	std::cout << std::endl;
}