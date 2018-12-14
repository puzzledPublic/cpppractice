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
	//std::thread t(update_data_forwidget, 1, data); //������ �Ű������� �Ѱ��ٶ� �����ڸ� �޴� �Լ��� ����Ǿ� �Ѿ�Ƿ� std::ref ����� �ʿ�
	
	std::unique_ptr<widget_data> up = std::make_unique<widget_data>();
	std::cout << up->data << std::endl;

	std::thread t(process_big_object, std::move(up));
	
	t.join();
	//std::cout << up->data << std::endl;	//Error!!! moved�� ��ü�� ���̻� �������� �����ؾ� �ϹǷ� ����ϸ� �ȵȴ�.
	return 0;
}