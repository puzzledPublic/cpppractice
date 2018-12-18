#include <mutex>

//listing 3.12
//std::call_once()를 사용한 Thread safe한 클래스 멤버의 늦은 초기화
//std::call_once() - 여러 스레드가 하나의 포인터를 한번만 초기화함을 보장함
class data_packet {};
class connection_info {};
class connection_handle {
public:
	void send_data(data_packet data) {}
	data_packet receive_data() {}
};
class connection_manager {
public:
	static connection_handle open(connection_info&) {};
};

class X {
public:
	X(const connection_info& connection_details_) : connection_details(connection_details_) { }
	//send_data() 또는 receive_data의 호출로 open_connection이 한번만 초기화 된다.
	void send_data(const data_packet& data) {
		std::call_once(connection_init_flag, &X::open_connection, this);
		connection.send_data(data);
	}
	data_packet receive_data() {
		std::call_once(connection_init_flag, &X::open_connection, this);
		return connection.receive_data();
	}
private:
	connection_info connection_details;
	connection_handle connection;
	std::once_flag connection_init_flag;	//std::mutex와 같이 not movable, copyable
	
	void open_connection() {
		connection = connection_manager::open(connection_details);
	}
};