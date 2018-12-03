#pragma once

#include <iostream>
#include <string>
#include <memory>  // allocator, uninitialized_copy
#include <initializer_list>

//프로젝트 설정 -> C/C++ -> SDL검사 -> 아니오
//Visual studio서 보안문제 때문에 uninitialized_copy 함수 호출을 막음
class StrVec {
public:
	typedef size_t size_type;
	StrVec() : first_elem(nullptr), first_free(nullptr), cap(nullptr) { }
	StrVec(std::initializer_list<std::string>);

	StrVec(const StrVec &);
	StrVec &operator=(const StrVec &);

	StrVec(StrVec &&) noexcept;
	StrVec& operator=(StrVec&&) noexcept;

	~StrVec();
	
	void push_back(const std::string &s);
	void pop_back();

	void reserve(size_type);
	void resize(size_type, const std::string & = std::string());

	bool empty() const { return cbegin() == cend(); }
	size_type size() const { return first_free - first_elem; }
	size_type capacity() const { return cap - first_elem; }

	std::string *begin() { return first_elem; }
	std::string *end() { return first_free; }
	const std::string *begin() const { return first_elem; }
	const std::string *end() const { return first_free; }
	const std::string *cbegin() const { return begin(); }
	const std::string *cend() const { return end(); }

private:
	static std::allocator<std::string> alloc;

	std::string *first_elem;
	std::string *first_free;
	std::string *cap;

	void chk_n_alloc() { if (size() == capacity()) reallocate(); }
	std::pair<std::string *, std::string *>
		alloc_n_copy(const std::string *, const std::string *);
	void reallocate();
	void free();
};


void print(const StrVec&);

void testStrVec();