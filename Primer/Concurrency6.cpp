#include <iostream>
#include <thread>
#include <memory>
#include <numeric>
#include <vector>
#include <algorithm>

//listing 2.8
//병렬 합산기 구현
template<typename Iterator, typename T>
struct accumulate_block {
	void operator()(Iterator first, Iterator last, T& result) {
		result = std::accumulate(first, last, result);
	}
};

template<typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init) {
	const unsigned long length = std::distance(first, last);
	if (!length) {	//빈 배열이면 초기값 리턴
		return init;
	}
	const unsigned long min_per_thread = 25;	//하나의 스레드당 처리할 임의의 elements 갯수
	const unsigned long max_threads = (length + min_per_thread - 1) / min_per_thread;	//받은 리스트의 개수로 최대 가능한 스레드 갯수 계산
	const unsigned long hardware_threads = std::thread::hardware_concurrency();			//하드웨어가 동시에 실행 처리 가능한 갯수, 0으로 나올 수도 있다.
	const unsigned long num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);	//하드웨어의 동시처리 갯수, elements 갯수 기반의 스레드 개수 중 최소값을 생성 스레드 개수로 한다.
	const unsigned long block_size = length / num_threads;	//실질적으로 하나의 스레드가 처리할 연속된 elements의 크기

	std::vector<T> results(num_threads);	//각각의 thread가 처리한 결과 값을(중간 결과) 담을 vector
	std::vector<std::thread> threads(num_threads - 1);	//스레드를 넣어놓을 vector
	
	Iterator block_start = first;
	for (unsigned long i = 0; i < (num_threads - 1); ++i) {	//현재 스레드(main)도 계산에 참여하므로 계산해 놓은 스레드 갯수값에서 하나 뺀 만큼만 생성한다.
		Iterator block_end = block_start;
		std::advance(block_end, block_size);
		threads[i] = std::thread(accumulate_block<Iterator, T>(), block_start, block_end, std::ref(results[i]));
		block_start = block_end;
	}
	//현재 스레드가 마지막 elements(last)까지 합을 만든다.
	accumulate_block<Iterator, T>()(block_start, last, results[num_threads - 1]);

	std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));	//각 스레드에 join 호출

	//각각의 스레드가 모두 계산이 끝난 상태.

	return std::accumulate(results.begin(), results.end(), init);	//중간 결과 값을 모두 더하면 최종 결과 값.
}

int main() {

	std::vector<int> vec(200);
	for (int i = 1; i <= 200; i++) {	//1 ~ 200까지의 합
		vec.push_back(i);
	}
	int result = parallel_accumulate(vec.begin(), vec.end(), 0); //1~ n까지 합을 병렬로 실행
	std::cout << ((200 * 201) / 2) << std::endl;	// n * (n + 1) / 2;
	std::cout << result << std::endl;				//병렬 실행한 값
	return 0;
}