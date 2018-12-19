#include <iostream>
#include <mutex>
//std::call_once, std::once_flag�� �̿��� �̱��� ����
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

//��� ������ ���� ����(���� ��������)�� �̿��� �̱��� ����(���̾ �̱��� ����)
class MySingleton2 {
private:
	MySingleton2();
	~MySingleton2();
	MySingleton2(const MySingleton2&) = delete;
	MySingleton2& operator=(const MySingleton2&) = delete;

public:
	static MySingleton2& getInstance() {
		static MySingleton2 instance;	//���� �����弭 �����ص� ��Ͽ����� ���������� �ѹ��� �ʱ�ȭ���� �����Ѵ�.(�����Ϸ��� �����ؾ���)
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