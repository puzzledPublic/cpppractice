#include <exception>
#include <thread>
#include <memory>
#include <mutex>
#include <stack>
#include <assert.h>
//listing 3.5
struct empty_stack : std::exception {
	const char* what() const throw();
};

//thread safe«— stack
template<typename T>
class threadsafe_stack {
public:
	threadsafe_stack() {}
	threadsafe_stack(const threadsafe_stack& other) {
		std::lock_guard<std::mutex> lock(other.m);
		data = other.data;
	}
	threadsafe_stack& operator=(const threadsafe_stack&) = delete;

	void push(T new_value) {
		std::lock_guard<std::mutex> lock(m);
		data.push(new_value);
	}
	std::shared_ptr<T> pop() {
		std::lock_guard<std::mutex> lock(m);
		if (data.empty()) {
			throw empty_stack();
		}
		const std::shared_ptr<T> res(std::make_shared<T>(data.top()));
		data.pop();
		return res;
	}
	void pop(T& value) {
		std::lock_guard<std::mutex> lock(m);
		if (data.empty()) {
			throw empty_stack();
		}
		value = data.top();
		data.pop();
	}
	bool empty() const {
		std::lock_guard<std::mutex> lock(m);
		return data.empty();
	}
private:
	std::stack<T> data;
	mutable std::mutex m;
};

int main() {
	threadsafe_stack<char> stack;

	std::thread t1([&]() {
		for (int i = 0; i < 20; i++) {
			stack.push('a');
		}
		for (int i = 0; i < 20; i++) {
			stack.pop();
		}
	});
	std::thread t2([&]() {
		for (int i = 0; i < 20; i++) {
			stack.push('b');
		}
		for (int i = 0; i < 20; i++) {
			stack.pop();
		}
	});

	t1.join();
	t2.join();
	
	assert(stack.empty());

	return 0;
}

const char * empty_stack::what() const throw()
{
	return "empty exception";
}
