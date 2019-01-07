#pragma once

#include "Query_base.h"
#include "Query.h"

class NotQuery : public Query_base {
	friend Query operator~(const Query&);
	NotQuery(const Query &q) :query(q) {}
	//��ü Ŭ���� : NotQuery������ ��ӹ��� ���� �Լ��� ��� �����Ѵ�.
	std::string rep() const {
		return "~(" + query.rep() + ")";
	}
	QueryResult eval(const TextQuery&) const;
	Query query;
};