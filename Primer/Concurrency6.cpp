#include <iostream>
#include <thread>
#include <memory>
#include <numeric>
#include <vector>
#include <algorithm>

//listing 2.8
//���� �ջ�� ����
template<typename Iterator, typename T>
struct accumulate_block {
	void operator()(Iterator first, Iterator last, T& result) {
		result = std::accumulate(first, last, result);
	}
};

template<typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init) {
	const unsigned long length = std::distance(first, last);
	if (!length) {	//�� �迭�̸� �ʱⰪ ����
		return init;
	}
	const unsigned long min_per_thread = 25;	//�ϳ��� ������� ó���� ������ elements ����
	const unsigned long max_threads = (length + min_per_thread - 1) / min_per_thread;	//���� ����Ʈ�� ������ �ִ� ������ ������ ���� ���
	const unsigned long hardware_threads = std::thread::hardware_concurrency();			//�ϵ��� ���ÿ� ���� ó�� ������ ����, 0���� ���� ���� �ִ�.
	const unsigned long num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);	//�ϵ������ ����ó�� ����, elements ���� ����� ������ ���� �� �ּҰ��� ���� ������ ������ �Ѵ�.
	const unsigned long block_size = length / num_threads;	//���������� �ϳ��� �����尡 ó���� ���ӵ� elements�� ũ��

	std::vector<T> results(num_threads);	//������ thread�� ó���� ��� ����(�߰� ���) ���� vector
	std::vector<std::thread> threads(num_threads - 1);	//�����带 �־���� vector
	
	Iterator block_start = first;
	for (unsigned long i = 0; i < (num_threads - 1); ++i) {	//���� ������(main)�� ��꿡 �����ϹǷ� ����� ���� ������ ���������� �ϳ� �� ��ŭ�� �����Ѵ�.
		Iterator block_end = block_start;
		std::advance(block_end, block_size);
		threads[i] = std::thread(accumulate_block<Iterator, T>(), block_start, block_end, std::ref(results[i]));
		block_start = block_end;
	}
	//���� �����尡 ������ elements(last)���� ���� �����.
	accumulate_block<Iterator, T>()(block_start, last, results[num_threads - 1]);

	std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));	//�� �����忡 join ȣ��

	//������ �����尡 ��� ����� ���� ����.

	return std::accumulate(results.begin(), results.end(), init);	//�߰� ��� ���� ��� ���ϸ� ���� ��� ��.
}

int main() {

	std::vector<int> vec(200);
	for (int i = 1; i <= 200; i++) {	//1 ~ 200������ ��
		vec.push_back(i);
	}
	int result = parallel_accumulate(vec.begin(), vec.end(), 0); //1~ n���� ���� ���ķ� ����
	std::cout << ((200 * 201) / 2) << std::endl;	// n * (n + 1) / 2;
	std::cout << result << std::endl;				//���� ������ ��
	return 0;
}