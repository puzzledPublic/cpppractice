#pragma once

#include "Query_base.h"
#include "Query.h"

class NotQuery : public Query_base {
	friend Query operator~(const Query&);
	NotQuery(const Query &q) :query(q) {}
	//구체 클래스 : NotQuery에서는 상속받은 가상 함수를 모두 정의한다.
	std::string rep() const {
		return "~(" + query.rep() + ")";
	}
	QueryResult eval(const TextQuery&) const;
	Query query;
};