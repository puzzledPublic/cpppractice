#pragma once

#include "TextQuery.h"
#include "Query_base.h"
#include "WordQuery.h"

class Query {
	friend Query operator~(const Query&);
	friend Query operator|(const Query&, const Query&);
	friend Query operator&(const Query&, const Query&);
public:
	inline Query(const std::string &s) : q(new WordQuery(s)) {}
	QueryResult eval(const TextQuery& t) const {
		return q->eval(t);
	}
	std::string rep() const {
		return q->rep();
	}
private:
	Query(std::shared_ptr<Query_base> query) : q(query) {}
	std::shared_ptr<Query_base> q;
};

std::ostream& operator<<(std::ostream &os, const Query& query);