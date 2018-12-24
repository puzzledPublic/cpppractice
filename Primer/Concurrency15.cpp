#include <mutex>
#include <thread>
#include <queue>
#include <iostream>
#include <condition_variable>

//listing 4.1
//���� ����(condition_variable) ����
std::mutex mut;
std::queue<int> data_queue;
std::condition_variable data_cond;

void data_preparation_thread() {	//ť�� �����͸� �ִ� ������
	for (int i = 0; i < 20; i++) {
		//������ �غ� �ڵ�...(���⼱ �ܼ��� i�� �Ѵ�.)
		std::lock_guard<std::mutex> lock(mut);	//������ �غ� �ƴٸ� ť�� ����ϱ����� ���� ���� �Ǵ�.
		data_queue.push(i);	//�����͸� �߰��Ѵ�.
		data_cond.notify_one();	//������� ������ �ϳ��� �����.
	}
}

void data_processing_thread() {		//ť���� �����͸� �Һ��ϴ� ������
	while (true) {
		std::unique_lock<std::mutex> lock(mut);	//���� ���� �Ǵ�.
		//wait()�� predicate(callable object)(���⼱ �����Լ��� ���ϰ�)�� true��� ���� �����ϰ� ���� �ڵ带 �����ϰ� false��� ���� Ǯ�� �ٽ� �����·� ���ư���.
		data_cond.wait(lock, []() { return !data_queue.empty(); });
		int data = data_queue.front();
		data_queue.pop();
		lock.unlock();
		std::cout << data << std::endl;
		if (data == 19) {
			break;
		}
	}
}
int main() {

	std::thread t1(data_preparation_thread);
	std::thread t2(data_processing_thread);

	t1.join();
	t2.join();
	return 0;
}