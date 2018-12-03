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
	add_to_Folders(m);	//m을 가리키는 Folder에 이 Message를 추가
}

Message & Message::operator=(const Message &rhs)
{
	remove_from_Folders();		//기존 Folder를 갱신
	contents = rhs.contents;	//rhs에서 메시지 본문을 복사
	folders = rhs.folders;		//rhs에서 Folder 포인터를 복사
	add_to_Folders(rhs);		//그 Folder에 이 Message를 추가
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
	folders.insert(&f);	//Folder 목록에 지정 Folder를 추가
	f.addMsg(this);		//f의 Message set에 이 Message를 추가
}

void Message::remove(Folder &f)
{
	folders.erase(&f);	//Folders 목록에서 지정 Folder를 제거
	f.remMsg(this);		//f의 Message set에서 이 Message를 제거
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
	for (auto f : m.folders) {	//m을 담고 있는 각 Folder에 대해
		f->addMsg(this);		//그 Folder에 이 Message에 대한 포인터를 추가
	}
}

void Message::remove_from_Folders()
{
	for (auto f : folders) {	//folders 내 각 포인터에 대해
		f->remMsg(this);		//해당 Folder에서 이 Message를 제거
	}
	folders.clear();			//이 Message를 가리키는 Folder는 없음
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
