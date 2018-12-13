#include <iostream>
#include <thread>

void hello() {
	std::cout << "hello concurrent world" << std::endl;
}
class background_task {
public:
	void operator()() const{
		std::cout << "background_task" << std::endl;
	}
};
int main() {
	std::thread t(hello);
	
	//std::thread t2{ background_task() };	//== std::thread t2((background_task()));
	std::thread t2([]() { std::cout << "lambda expresion" << std::endl; });
	t.join();
	t2.join();
}