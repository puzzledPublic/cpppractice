#pragma once

#include <iostream>

class MyString
{
public:
	MyString(const char* = "hello");
	MyString(const MyString&);
	MyString& operator=(const MyString&);

	MyString(MyString&&);
	MyString& operator=(MyString&&);
	void print();
	~MyString();

private:
	int len;
	char* buf;
};
