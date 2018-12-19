#include <iostream>
#include <string>
#include <mutex>
#include <thread>

std::mutex coutMutex;
//thread_local - 스레드마다 따로 생성, 동작은 정적 데이터와 비슷하다. 스레드의 라이프사이클 동안 바인딩되고 처음 사용되는 시점에 생성된다.
//스레드마다 하나씩 독점적으로 만들어진다. (싱글 스레드 프로그램을 멀티 스레드 환경으로 포팅하는데 사용될 수 있다)
thread_local std::string s("hello from ");

void addThreadLocal(const std::string& s2) {
	s += s2;
	std::lock_guard<std::mutex> guard(coutMutex);
	std::cout << s << std::endl;
	std::cout << "&s: " << &s << std::endl;
	std::cout << std::endl;
}

int main() {
	std::cout << std::endl;

	std::thread t1(addThreadLocal, "t1");
	std::thread t2(addThreadLocal, "t1");
	std::thread t3(addThreadLocal, "t1");
	std::thread t4(addThreadLocal, "t1");

	t1.join();
	t2.join();
	t3.join();
	t4.join();
}