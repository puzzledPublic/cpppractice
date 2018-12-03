#include "Message.h"

void swap(Message &lhs, Message &rhs) {
	using std::swap;

	for (auto f : lhs.folders) {
		f->remMsg(&lhs);
	}
	for (auto f : rhs.folders) {
		f->remMsg(&rhs);
	}

	swap(lhs.folders, rhs.folders);
	swap(lhs.contents, rhs.contents);

	for (auto f : lhs.folders) {
		f->addMsg(&lhs);
	}
	for (auto f : rhs.folders) {
		f->addMsg(&rhs);
	}
}
Message::Message(const Message &m) : contents(m.contents), folders(m.folders)
{
	add_to_Folders(m);	//m�� ����Ű�� Folder�� �� Message�� �߰�
}

Message & Message::operator=(const Message &rhs)
{
	remove_from_Folders();		//���� Folder�� ����
	contents = rhs.contents;	//rhs���� �޽��� ������ ����
	folders = rhs.folders;		//rhs���� Folder �����͸� ����
	add_to_Folders(rhs);		//�� Folder�� �� Message�� �߰�
	return *this;
}

Message::Message(Message &&m): contents(std::move(m.contents))
{
	move_Folders(&m);
}

Message & Message::operator=(Message &&rhs)
{
	if (this != &rhs) {
		remove_from_Folders();
		contents = std::move(rhs.contents);
		move_Folders(&rhs);
	}
	return *this;
}

Message::~Message()
{
	remove_from_Folders();
}

void Message::save(Folder &f)
{
	folders.insert(&f);	//Folder ��Ͽ� ���� Folder�� �߰�
	f.addMsg(this);		//f�� Message set�� �� Message�� �߰�
}

void Message::remove(Folder &f)
{
	folders.erase(&f);	//Folders ��Ͽ��� ���� Folder�� ����
	f.remMsg(this);		//f�� Message set���� �� Message�� ����
}

void Message::print()
{
	std::cout << "Message <" << this << ">:<" << contents << "> in "
		<< folders.size() << " folders:\n";
	for (const auto &f : folders)
		std::cout << "\t<" << f << ">\n";
}

void Message::add_to_Folders(const Message &m)
{
	for (auto f : m.folders) {	//m�� ��� �ִ� �� Folder�� ����
		f->addMsg(this);		//�� Folder�� �� Message�� ���� �����͸� �߰�
	}
}

void Message::remove_from_Folders()
{
	for (auto f : folders) {	//folders �� �� �����Ϳ� ����
		f->remMsg(this);		//�ش� Folder���� �� Message�� ����
	}
	folders.clear();			//�� Message�� ����Ű�� Folder�� ����
}

void Message::addFdr(Folder *f)
{
	folders.insert(f);
}

void Message::remFdr(Folder *f)
{
	folders.erase(f);
}

void Message::move_Folders(Message *m)
{
	folders = std::move(m->folders);
	for (auto f : folders) {
		f->remMsg(m);
		f->addMsg(this);
	}
	m->folders.clear();
}
