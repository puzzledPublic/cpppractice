#include "OrQuery.h"

Query operator|(const Query &lhs, const Query &rhs)
{
	//std::shared_ptr<Query_base> tmp(new OrQuery(lhs, rhs));
	//return Query(tmp); 와 같다.
	return std::shared_ptr<Query_base>(new OrQuery(lhs, rhs));	
}

QueryResult OrQuery::eval(const TextQuery &text) const
{
	//Query 멤버인 lhs, rhs를 통한 가상 호출
	//eval을 호출하면 각 피연산자에 대한 QueryResult를 반환한다
	auto right = rhs.eval(text), left = lhs.eval(text);
	//왼쪽 피연산자에서 줄 번호를 복사해 결과 set에 넣는다.
	auto ret_lines = std::make_shared<std::set<line_no>>(left.begin(), left.end());
	//오른쪽 피연산자에 있는 줄 번호를 삽입한다.
	ret_lines->insert(right.begin(), right.end());
	//lhs와 rhs의 합집합을 나타내는 새 QueryResult를 반환한다.
	return QueryResult(rep(), ret_lines, left.get_file());
}
