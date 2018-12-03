#pragma once
#include <iostream>
#include <set>
#include <string>

#include "Folder.h"
class Message
{
	friend class Folder;
	friend void swap(Message&, Message&);
	friend void swap(Folder&, Folder&);
public:
	explicit Message(const std::string &str = "") :contents(str) {}		//암시적으로 folders를 빈 set으로 초기화
	Message(const Message&);	//복사 생성자
	Message& operator=(const Message&);	//복사 대입
	Message(Message&&);			//이동 생성자
	Message& operator=(Message&&);	//이동 대입
	~Message();	//소멸자

	//지정한 Folder의 메시지 set에서 이 Message를 추가, 제거
	void save(Folder&);	
	void remove(Folder&);
	void print();
private:
	std::string contents;	//실제 메시지 본문
	std::set<Folder*> folders;	//이 Message를 담고 있는 Folder

	//복사 생성자, 대입, 소멸자에서 사용하는 편의 함수
	void add_to_Folders(const Message&);	//해당 매기변수를 가리키는 Folder에 이 Message를 추가
	void remove_from_Folders();		//folders의 모든 Folder에서 이 Message를 제거
	
	void addFdr(Folder*);
	void remFdr(Folder*);

	void move_Folders(Message*);
};

void swap(Message&, Message&);