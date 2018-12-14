#include <iostream>
#include <thread>
#include <memory>

typedef int widget_id;
class widget_data{
public:
	int data = 1;
};
void update_data_forwidget(widget_id w, widget_data& data) {
	data.data = 3;
	std::cout << "in the other thread data address = " << (&data) << std::endl;
}
void process_big_object(std::unique_ptr<widget_data> up) {
	up->data = 3;
	std::cout << "moved unique_ptr" << std::endl;
	std::cout << up->data << std::endl;
}
int main() {

	widget_data data;
	std::cout << "in main thread data address = " << (&data) << std::endl;
	//std::thread t(update_data_forwidget, 1, std::ref(data));	
	//std::thread t(update_data_forwidget, 1, data); //스레드 매개변수를 넘겨줄때 참조자를 받는 함수라도 복사되어 넘어가므로 std::ref 사용이 필요
	
	std::unique_ptr<widget_data> up = std::make_unique<widget_data>();
	std::cout << up->data << std::endl;

	std::thread t(process_big_object, std::move(up));
	
	t.join();
	//std::cout << up->data << std::endl;	//Error!!! moved된 객체는 더이상 사용안함을 보장해야 하므로 사용하면 안된다.
	return 0;
}