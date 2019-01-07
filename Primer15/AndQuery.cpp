#include "AndQuery.h"
#include <algorithm>
#include <iterator>

Query operator&(const Query &lhs, const Query &rhs)
{
	return std::shared_ptr<Query_base>(new AndQuery(lhs, rhs));
}

QueryResult AndQuery::eval(const TextQuery &text) const
{
	//�ش� �ǿ����ڿ� ���� ��� set�� ��� ���� Query �ǿ����ڸ� ���� ���� ȣ���Ѵ�.
	auto left = lhs.eval(text), right = rhs.eval(text);
	//left�� right�� �������� ��� set
	auto ret_lines = std::make_shared<std::set<line_no>>();
	//�� ������ �������� ������ �ݺ��ڿ� ����Ѵ�.
	//�� ȣ�⿡�� ������ �ݺ��ڴ� ret�� ��Ҹ� �߰��Ѵ�.
	std::set_intersection(left.begin(), left.end(), right.begin(), right.end(), std::inserter(*ret_lines, ret_lines->begin()));
	return QueryResult(rep(), ret_lines, left.get_file());
}
