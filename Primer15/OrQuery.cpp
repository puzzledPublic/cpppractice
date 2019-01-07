#include "OrQuery.h"

Query operator|(const Query &lhs, const Query &rhs)
{
	//std::shared_ptr<Query_base> tmp(new OrQuery(lhs, rhs));
	//return Query(tmp); �� ����.
	return std::shared_ptr<Query_base>(new OrQuery(lhs, rhs));	
}

QueryResult OrQuery::eval(const TextQuery &text) const
{
	//Query ����� lhs, rhs�� ���� ���� ȣ��
	//eval�� ȣ���ϸ� �� �ǿ����ڿ� ���� QueryResult�� ��ȯ�Ѵ�
	auto right = rhs.eval(text), left = lhs.eval(text);
	//���� �ǿ����ڿ��� �� ��ȣ�� ������ ��� set�� �ִ´�.
	auto ret_lines = std::make_shared<std::set<line_no>>(left.begin(), left.end());
	//������ �ǿ����ڿ� �ִ� �� ��ȣ�� �����Ѵ�.
	ret_lines->insert(right.begin(), right.end());
	//lhs�� rhs�� �������� ��Ÿ���� �� QueryResult�� ��ȯ�Ѵ�.
	return QueryResult(rep(), ret_lines, left.get_file());
}
