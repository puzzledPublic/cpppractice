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
		std::lock(lhs.m, rhs.m);	//lhs�� rhs�� mutex�� ���ÿ� lock �Ŵ°� �����Ѵ�.
		std::lock_guard<std::mutex> lock_a(lhs.m, std::adopt_lock);	//std::adopt_lock�� lock_guard���� mutex�� �̹� lock�� �ɷ����� lock�� ���� �ʰ� �ܼ��� �����Ǹ� ������ ���� �˸���.
		std::lock_guard<std::mutex> lock_b(rhs.m, std::adopt_lock);
		swap(lhs.some_detail, rhs.some_detail);
	}
private:
	some_big_object some_detail;
	std::mutex m;
};