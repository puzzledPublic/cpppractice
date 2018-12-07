#include "MoveTest.h"

void MyString::test()
{
	MyString ms("hellow");
	MyString ms2(ms);

	MyString ms3;

	ms3 = std::move(ms2);

	ms.print();
	ms2.print();
	ms3.print();
}

MyString::MyString(const char *c) : len(std::strlen(c))
{
	std::cout << "MyString(const char*) ������" << "\n";
	buf = new char[len + 1];
	std::memcpy(buf, c, len + 1);
	*(buf + len) = '\0';
}

MyString::MyString(const MyString &ms) : len(ms.len)
{
	std::cout << "MyString(const MyString&) ���� ������" << "\n";
	buf = new char[len + 1];
	std::memcpy(buf, ms.buf, len + 1);
}

MyString & MyString::operator=(const MyString &ms)
{
	std::cout << "MyString ���� ���� ������" << "\n";
	delete buf;
	len = ms.len;
	buf = new char[len + 1];
	std::memcpy(buf, ms.buf, len + 1);
	
	return *this;
}

MyString::MyString(MyString &&ms)
{
	std::cout << "MyString(MyString&&) �̵� ������" << "\n";
	len = ms.len;
	buf = ms.buf;
	ms.buf = nullptr;
}

MyString & MyString::operator=(MyString &&ms)
{
	std::cout << "MyString �̵� ���� ������" << "\n";
	if (this != &ms) {
		delete buf;
		len = ms.len;
		buf = ms.buf;
		ms.buf = nullptr;
	}
	return *this;
}

void MyString::print()
{
	if (buf == nullptr) {
		std::cout << "null" << "\n";
		return;
	}
	std::cout << buf << "\n";
}

MyString::~MyString()
{
	std::cout << "MyString �Ҹ���" << "\n";
	delete buf;
}
