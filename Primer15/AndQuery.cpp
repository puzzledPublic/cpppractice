#include "AndQuery.h"
#include <algorithm>
#include <iterator>

Query operator&(const Query &lhs, const Query &rhs)
{
	return std::shared_ptr<Query_base>(new AndQuery(lhs, rhs));
}

QueryResult AndQuery::eval(const TextQuery &text) const
{
	//해당 피연산자에 대한 결과 set을 얻기 위해 Query 피연산자를 통해 가상 호출한다.
	auto left = lhs.eval(text), right = rhs.eval(text);
	//left와 right의 교집합을 담는 set
	auto ret_lines = std::make_shared<std::set<line_no>>();
	//두 범위의 교집합을 목적지 반복자에 기록한다.
	//이 호출에서 목적지 반복자는 ret에 요소를 추가한다.
	std::set_intersection(left.begin(), left.end(), right.begin(), right.end(), std::inserter(*ret_lines, ret_lines->begin()));
	return QueryResult(rep(), ret_lines, left.get_file());
}
