#pragma once
#include "Query_base.h"

class WordQuery : public Query_base {
	friend class Query;	//Query에서는 WordQuery생성자를 사용한다.
	WordQuery(const std::string &s) : query_word(s) {}
	//구체 클래스: WordQuery에서는 상속받은 순수 가상 함수를 모두 정의한다.
	QueryResult eval(const TextQuery &t) const {
		return t.query(query_word);
	}
	std::string rep() const {
		return query_word;
	}
	std::string query_word;	//검색할 단어
};