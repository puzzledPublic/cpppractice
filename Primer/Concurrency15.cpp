#include <mutex>
#include <thread>
#include <queue>
#include <iostream>
#include <condition_variable>

//listing 4.1
//조건 변수(condition_variable) 예제
std::mutex mut;
std::queue<int> data_queue;
std::condition_variable data_cond;

void data_preparation_thread() {	//큐에 데이터를 넣는 스레드
	for (int i = 0; i < 20; i++) {
		//데이터 준비 코드...(여기선 단순히 i로 한다.)
		std::lock_guard<std::mutex> lock(mut);	//데이터 준비가 됐다면 큐를 사용하기위해 먼저 락을 건다.
		data_queue.push(i);	//데이터를 추가한다.
		data_cond.notify_one();	//대기중인 스레드 하나를 깨운다.
	}
}

void data_processing_thread() {		//큐에서 데이터를 소비하는 스레드
	while (true) {
		std::unique_lock<std::mutex> lock(mut);	//먼저 락을 건다.
		//wait()의 predicate(callable object)(여기선 람다함수의 리턴값)가 true라면 락을 유지하고 다음 코드를 실행하고 false라면 락을 풀고 다시 대기상태로 돌아간다.
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