#include <utility>  // move
#include "StrVec.h"

std::allocator<std::string> StrVec::alloc;

StrVec::StrVec(std::initializer_list<std::string> il) : StrVec() {
	for (const auto &s : il)
		push_back(s);
}
std::pair<std::string *, std::string *>
StrVec::alloc_n_copy(const std::string *b, const std::string *e) {
	auto b2 = alloc.allocate(e - b);
	return{ b2, std::uninitialized_copy(b, e, b2) };
}

void StrVec::reallocate() {
	auto new_cap_sz = size() ? 2 * size() : 1;
	auto new_first_elem = alloc.allocate(new_cap_sz);
	auto new_first_free = new_first_elem;
	auto old_first_elem = first_elem;
	for (size_type i = 0; i != size(); ++i)
		alloc.construct(new_first_free++, std::move(*old_first_elem++));
	free();
	first_elem = new_first_elem;
	first_free = new_first_free;
	cap = first_elem + new_cap_sz;
}

void StrVec::free() {
	if (first_elem) {
		for (auto p = first_free; p != first_elem; alloc.destroy(--p)) {}
		alloc.deallocate(first_elem, cap - first_elem);
		//first_elem = first_free = cap = nullptr;
	}
}

StrVec::StrVec(const StrVec &rhs) {
	auto data = alloc_n_copy(rhs.begin(), rhs.end());
	first_elem = data.first;
	first_free = cap = data.second;
}

StrVec::~StrVec() {
	free();
}

StrVec &StrVec::operator=(const StrVec &rhs) {
	auto data = alloc_n_copy(rhs.begin(), rhs.end());
	free();
	first_elem = data.first;
	first_free = cap = data.second;
	return *this;
}

StrVec::StrVec(StrVec &&s) noexcept : first_elem(s.first_elem), first_free(s.first_free), cap(s.cap)
{
	s.first_elem = s.first_free = s.cap = nullptr;
}

StrVec & StrVec::operator=(StrVec &&rhs) noexcept
{
	if (this != &rhs) {
		free();
		first_elem = rhs.first_elem;
		first_free = rhs.first_free;
		cap = rhs.cap;
		rhs.first_elem = rhs.first_free = rhs.cap = nullptr;
	}
	return *this;
}


void StrVec::push_back(const std::string &s) {
	chk_n_alloc();
	alloc.construct(first_free++, s);
}

void StrVec::pop_back() {
	if (size() > 0)
		alloc.destroy(--first_free);
}

void StrVec::reserve(size_type n) {
	if (n > capacity()) {
		auto new_first_elem = alloc.allocate(n);
		auto new_first_free = new_first_elem;
		auto old_first_elem = first_elem;
		for (size_type i = 0; i != size(); ++i)
			alloc.construct(new_first_free++, std::move(*old_first_elem++));
		free();
		first_elem = new_first_elem;
		first_free = new_first_free;
		cap = first_elem + n;
	}
}

void StrVec::resize(size_type n, const std::string &s) {
	if (n > size()) {
		for (int i = n - size(); i != 0; --i)
			push_back(s);
	}
	else if (n < size()) {
		for (int i = size() - n; i != 0; --i)
			pop_back();
	}
}
void print(const StrVec& sv) {
	std::cout << "<" << &sv << "> size: " << sv.size() << " capacity: " << sv.capacity() << " contents:";
	for (const auto &s : sv) {
		std::cout << " <" << s << ">";
	}
	std::cout << std::endl;
}

void testStrVec() {
	StrVec sv;
	print(sv);

	sv.push_back("s1"); print(sv);
	sv.push_back("s2"); print(sv);
	sv.push_back("s3"); print(sv);
	sv.push_back("s4"); print(sv);
	sv.push_back("s5"); print(sv);

	{
		StrVec sv2(sv); print(sv2);
		sv2.push_back("s6"); print(sv); print(sv2);
		sv.pop_back(); print(sv); print(sv2);
		sv = sv2; print(sv); print(sv2);
	}

	sv.reserve(sv.capacity() / 2); print(sv);
	sv.reserve(sv.capacity() * 2); print(sv);

	sv.resize(sv.size() + 2); print(sv);
	sv.resize(sv.size() + 2, "s7"); print(sv);
	sv.resize(sv.size() - 2); print(sv);
	sv.resize(sv.size() - 2, "s7"); print(sv);

	StrVec sv2{ "il1", "il2", "il3", "il4", "il5" }; print(sv2);
	StrVec sv3 = { "il1", "il2", "il3", "il4", "il5" }; print(sv3);
}