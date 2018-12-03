#include "Folder.h"

void swap(Folder& lhs, Folder& rhs) {
	using std::swap;
	
	for (auto &m : lhs.messages) {
		m->remFdr(&lhs);
	}
	for (auto &m : rhs.messages) {
		m->remFdr(&rhs);
	}

	swap(lhs.messages, rhs.messages);

	for (auto &m : lhs.messages) {
		m->addFdr(&lhs);
	}
	for (auto &m : rhs.messages) {
		m->addFdr(&rhs);
	}
}
Folder::Folder(const Folder &f) : messages(f.messages)
{
	for (auto &m : messages) {
		m->addFdr(this);
	}
}

Folder & Folder::operator=(const Folder &rhs)
{
	for (auto &m : messages) {
		m->remFdr(this);
	}

	messages = rhs.messages;

	for (auto &m : messages) {
		m->addFdr(this);
	}
	return *this;
}

Folder::~Folder()
{
	for (auto &m : messages) {
		m->remFdr(this);
	}
}

void Folder::save(Message &m)
{
	messages.insert(&m);
	m.addFdr(this);
}

void Folder::remove(Message &m)
{
	messages.erase(&m);
	m.remFdr(this);
}

void Folder::print()
{
	std::cout << "Folder <" << this << "> contains " << messages.size() << " messages:\n";
	for (const auto &m : messages) {
		std::cout << "\t<" << m << ">:<" << m->contents << ">\n";
	}
}

void Folder::addMsg(Message *m)
{
	messages.insert(m);
}

void Folder::remMsg(Message *m)
{
	messages.erase(m);
}
