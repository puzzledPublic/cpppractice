#pragma once

#include "TextQuery.h"
//추상 클래스는 구체 조회 타입에 대한 기초 클래스로 행동하므로 모든 멤버는 private 이다.
class Query_base {
	friend class Query;
protected:
	using line_no = TextQuery::line_no;	//eval 함수에서 사용한다.
	virtual ~Query_base() = default;
private:
	//eval 함수에서는 이 Query와 일치하는 QueryResult를 반환한다.
	virtual QueryResult eval(const TextQuery&) const = 0;
	//rep 함수는 조회 내용을 표현한 string 이다.
	virtual std::string rep() const = 0;
};