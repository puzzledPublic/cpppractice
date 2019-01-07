#pragma once

#include "Query_base.h"
#include "Query.h"

class BinaryQuery : public Query_base {
protected:
	BinaryQuery(const Query &l, const Query &r, std::string s) : lhs(l), rhs(r), opSym(s) {}
	//�߻� Ŭ���� : BinaryQuery������ eval�� �������� �ʴ´�.
	std::string rep() const {
		return "(" + lhs.rep() + " " + opSym + " " + rhs.rep() + ")";
	}
	Query lhs, rhs;	//�����ʰ� ���� �ǿ�����
	std::string opSym;	//������ �̸�
};