#include <mutex>

//listing 3.12
//std::call_once()�� ����� Thread safe�� Ŭ���� ����� ���� �ʱ�ȭ
//std::call_once() - ���� �����尡 �ϳ��� �����͸� �ѹ��� �ʱ�ȭ���� ������
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
	//send_data() �Ǵ� receive_data�� ȣ��� open_connection�� �ѹ��� �ʱ�ȭ �ȴ�.
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
	std::once_flag connection_init_flag;	//std::mutex�� ���� not movable, copyable
	
	void open_connection() {
		connection = connection_manager::open(connection_details);
	}
};