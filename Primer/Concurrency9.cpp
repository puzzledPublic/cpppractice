#include <thread>
#include <mutex>

//listing 3.7 ~ 3.8
//계층적(hierarchical) 뮤텍스 - 상위 레벨 -> 하위 레벨 순으로 뮤텍스를 얻게끔 강제하여 DeadLock이 발생하지 않도록 하는 기법

//계층적(hierarchical) 뮤텍스 정의
//사용자 정의 mutex를 만들기 위해선 lock, unlock, try_lock을 구현하면 된다.
class hierarchical_mutex {
public:
	explicit hierarchical_mutex(unsigned long value) : hierarchy_value(value), previous_hierarchy_value(0) {}
	void lock() {
		check_for_hierarchy_violation();
		internal_mutex.lock();
		update_hierarchy_value();	//lock을 얻었다면 현재 최소 계층번호를 변경한다.
	}
	void unlock() {
		this_thread_hierarchy_value = previous_hierarchy_value;	//unlock될때 전에 저장한 계층번호를 복구한다.
		internal_mutex.unlock();
	}
	bool try_lock() {
		check_for_hierarchy_violation();
		if (!internal_mutex.try_lock()) {	//try_lock()은 lock 획득을 시도하고 그 여부를 반환한다. 얻을 수 없다면 얻을때까지 기다리지 않고 바로 리턴한다.
			return false;
		}
		update_hierarchy_value();
		return true;
	}
private:
	std::mutex internal_mutex;
	const unsigned long hierarchy_value;
	unsigned long previous_hierarchy_value;
	static thread_local unsigned long this_thread_hierarchy_value;	//현재 스레드가 갖는 '최소' 계층번호, thread_local - hierarchical_mutex 클래스에 하나 존재(static)하고 각 스레드에서 사본을 갖는다

	void check_for_hierarchy_violation() {
		if (this_thread_hierarchy_value <= hierarchy_value) {
			throw std::logic_error("mutex hierarchy violated");
		}
	}
	void update_hierarchy_value() {
		previous_hierarchy_value = this_thread_hierarchy_value;	//현재 스레드가 갖는 계층 번호를 저장한다.
		this_thread_hierarchy_value = hierarchy_value;			//인스턴스 생성시 받아온 계층 번호를 현재 스레드가 갖는 계총 번호로 한다.
	}
};

thread_local unsigned long hierarchical_mutex::this_thread_hierarchy_value(ULONG_MAX);

//계층적(hierarchical) 뮤텍스 사용
hierarchical_mutex high_level_mutex(10000);
hierarchical_mutex low_level_mutex(5000);

int do_low_level_stuff();

int low_level_func() {
	std::lock_guard<hierarchical_mutex> lk(low_level_mutex);
	return do_low_level_stuff();
}

void high_level_stuff(int some_param);

void high_level_func() {
	std::lock_guard<hierarchical_mutex> lk(high_level_mutex);
	high_level_stuff(low_level_func());
}

void thread_a() {	//Good. 상위 레벨(10000) mutex를 얻고 하위 레벨(5000) mutex를 얻는다.
	high_level_func();
}

hierarchical_mutex other_mutex(100);

void do_other_stuff();

void other_stuff() {
	high_level_func();
	do_other_stuff();
}

void thread_b() {	//Error!!! 하위 레벨(100) mutex를 얻고 상위 레벨(10000) mutex를 얻는다.
	std::lock_guard<hierarchical_mutex> lk(other_mutex);
	other_stuff();
}