#include <iostream>
#include <mutex>
//std::call_once, std::once_flag를 이용한 싱글턴 패턴
class MySingleton {
private:
	static std::once_flag initInstanceFlag;
	static MySingleton* instance;
	MySingleton() = default;
	~MySingleton() = default;
	static void initSingleton() {
		instance = new MySingleton();
	}

public:
	MySingleton(const MySingleton&) = delete;
	MySingleton& operator=(const MySingleton&) = delete;
	static MySingleton* getInstance() {
		std::call_once(initInstanceFlag, MySingleton::initSingleton);
		return instance;
	}
};

MySingleton* MySingleton::instance = nullptr;
std::once_flag MySingleton::initInstanceFlag;

//블록 영역의 정적 변수(지역 정적변수)를 이용한 싱글턴 패턴(마이어스 싱글턴 패턴)
class MySingleton2 {
private:
	MySingleton2();
	~MySingleton2();
	MySingleton2(const MySingleton2&) = delete;
	MySingleton2& operator=(const MySingleton2&) = delete;

public:
	static MySingleton2& getInstance() {
		static MySingleton2 instance;	//여러 스레드서 접근해도 블록영역내 정적변수는 한번만 초기화됨을 보장한다.(컴파일러가 지원해야함)
		return instance;
	}
};

MySingleton2::MySingleton2() = default;
MySingleton2::~MySingleton2() = default;

int main() {
	std::cout << std::endl;

	std::cout << "MySingleton::getInstance(): " << MySingleton::getInstance() << std::endl;
	std::cout << "MySingleton::getInstance(): " << MySingleton::getInstance() << std::endl;

	std::cout << std::endl;
}