#include <iostream>
#include <thread>

void helloFunction() {
	std::cout << "hello from a function." << std::endl;
}
class HelloFunctionObject {
public:
	void operator()() const {
		std::cout << "hello from a function Object." << std::endl;
	}
};
int main() {
	
	std::cout << std::endl;

	std::thread t1(helloFunction);
	HelloFunctionObject helloFunctionObject;
	std::thread t2(helloFunctionObject);
	std::thread t3([]() {std::cout << "hello from a lambda." << std::endl; });

	t1.join();
	t2.join();
	t3.join();

	std::cout << std::endl;
	return 0;
}