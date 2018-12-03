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
	explicit Message(const std::string &str = "") :contents(str) {}		//�Ͻ������� folders�� �� set���� �ʱ�ȭ
	Message(const Message&);	//���� ������
	Message& operator=(const Message&);	//���� ����
	Message(Message&&);			//�̵� ������
	Message& operator=(Message&&);	//�̵� ����
	~Message();	//�Ҹ���

	//������ Folder�� �޽��� set���� �� Message�� �߰�, ����
	void save(Folder&);	
	void remove(Folder&);
	void print();
private:
	std::string contents;	//���� �޽��� ����
	std::set<Folder*> folders;	//�� Message�� ��� �ִ� Folder

	//���� ������, ����, �Ҹ��ڿ��� ����ϴ� ���� �Լ�
	void add_to_Folders(const Message&);	//�ش� �ű⺯���� ����Ű�� Folder�� �� Message�� �߰�
	void remove_from_Folders();		//folders�� ��� Folder���� �� Message�� ����
	
	void addFdr(Folder*);
	void remFdr(Folder*);

	void move_Folders(Message*);
};

void swap(Message&, Message&);