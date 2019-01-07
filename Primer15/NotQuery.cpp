#include "NotQuery.h"

Query operator~(const Query &operand)
{
	return std::shared_ptr<Query_base>(new NotQuery(operand));
}

QueryResult NotQuery::eval(const TextQuery &text) const
{
	//Query 피연산자를 통한 eval 가상 호출
	auto result = query.eval(text);
	//빈 결과 set으로 시작한다.
	auto ret_lines = std::make_shared<std::set<line_no>>();
	//피연산자가 나온 줄을 반복해야 한다.
	auto beg = result.begin(), end = result.end();
	//입력 파일 내 각 줄에 대해, 해당 줄이 result에 없으면 그 줄 번호를 ret_lines에 추가한다.
	auto sz = result.get_file()->size();
	for (std::size_t n = 0; n != sz; ++n) {
		//result 내 모든 줄을 처리하지 않아으면 이 줄이 존재하는지 확인한다.
		if (beg == end || *beg != n) {
			ret_lines->insert(n);
		}
		else if (beg != end) {
			++beg;
		}
	}
	return QueryResult(rep(), ret_lines, result.get_file());
}
