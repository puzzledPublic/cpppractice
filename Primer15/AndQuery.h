#pragma once

#include "BinaryQuery.h"

class AndQuery : public BinaryQuery {
	friend Query operator&(const Query&, const Query&);
	AndQuery(const Query& left, const Query& right) : BinaryQuery(left, right, "&") {}
	//구체 클래스 : AndQuery에서는 rep을 상속받고 나머지 순수 가상 함수는 정의한다.
	QueryResult eval(const TextQuery&) const;
};