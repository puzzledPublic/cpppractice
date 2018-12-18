#include <thread>
#include <mutex>
//listing 3.6, 3.9
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
	//unique_lock은 mutex 상태의 flag가 필요하여 lock_guard보다 공간이 더 필요하며 추가 연산이 더 필요해 약간의 성능차가 난다.
	//하지만 lock_guard 보다 유연성이 있다.
	friend void unique_lock_swap(X& lhs, X& rhs) {
		if (&lhs == &rhs) {
			return;
		}
		std::unique_lock<std::mutex> lock_a(lhs.m, std::defer_lock);	//std::defer_lock은 생성시 mutex가 unlocked 상태로 남아있어야함을 알린다.
		std::unique_lock<std::mutex> lock_b(rhs.m, std::defer_lock);	
		std::lock(lock_a, lock_b);	//unique_lock에 lock을 걸려면 해당 인스턴스의 lock()을 호출하던가 std::lock()의 파라미터로 넘겨줄 수 있다.
		swap(lhs.some_detail, rhs.some_detail);
	}
private:
	some_big_object some_detail;
	std::mutex m;
};
