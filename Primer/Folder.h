#pragma once

#include "Message.h"

class Folder
{
	friend class Message;
	friend void swap(Message&, Message&);
	friend void swap(Folder&, Folder&);
public:
	Folder() = default;
	Folder(const Folder&);
	Folder& operator=(const Folder&);
	~Folder();

	void save(Message&);
	void remove(Message&);
	void print();
private:
	std::set<Message*> messages;
	void addMsg(Message*);
	void remMsg(Message*);
};

void swap(Folder&, Folder&);