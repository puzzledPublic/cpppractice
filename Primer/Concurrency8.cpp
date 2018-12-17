#include <thread>
#include <mutex>
//listing 3.6
class some_big_object {
private:
	int data;
};

void swap(some_big_object& lhs, some_big_object& rhs) {
	using std::swap;
	swap(lhs, rhs);
}

class X {
public:
	X(const some_big_object& sd) : some_detail(sd) {}

	friend void swap(X& lhs, X& rhs) {
		if (&lhs == &rhs) {
			return;
		}
		std::lock(lhs.m, rhs.m);	//lhs와 rhs의 mutex를 동시에 lock 거는걸 보장한다.
		std::lock_guard<std::mutex> lock_a(lhs.m, std::adopt_lock);	//std::adopt_lock은 lock_guard에게 mutex가 이미 lock이 걸렸으며 lock을 걸지 않고 단순히 소유권만 가지게 함을 알린다.
		std::lock_guard<std::mutex> lock_b(rhs.m, std::adopt_lock);
		swap(lhs.some_detail, rhs.some_detail);
	}
private:
	some_big_object some_detail;
	std::mutex m;
};