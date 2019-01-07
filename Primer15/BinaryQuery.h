#pragma once

#include "Query_base.h"
#include "Query.h"

class BinaryQuery : public Query_base {
protected:
	BinaryQuery(const Query &l, const Query &r, std::string s) : lhs(l), rhs(r), opSym(s) {}
	//추상 클래스 : BinaryQuery에서는 eval을 정의하지 않는다.
	std::string rep() const {
		return "(" + lhs.rep() + " " + opSym + " " + rhs.rep() + ")";
	}
	Query lhs, rhs;	//오른쪽과 왼쪽 피연산자
	std::string opSym;	//연산자 이름
};