#include "NotQuery.h"

Query operator~(const Query &operand)
{
	return std::shared_ptr<Query_base>(new NotQuery(operand));
}

QueryResult NotQuery::eval(const TextQuery &text) const
{
	//Query �ǿ����ڸ� ���� eval ���� ȣ��
	auto result = query.eval(text);
	//�� ��� set���� �����Ѵ�.
	auto ret_lines = std::make_shared<std::set<line_no>>();
	//�ǿ����ڰ� ���� ���� �ݺ��ؾ� �Ѵ�.
	auto beg = result.begin(), end = result.end();
	//�Է� ���� �� �� �ٿ� ����, �ش� ���� result�� ������ �� �� ��ȣ�� ret_lines�� �߰��Ѵ�.
	auto sz = result.get_file()->size();
	for (std::size_t n = 0; n != sz; ++n) {
		//result �� ��� ���� ó������ �ʾ����� �� ���� �����ϴ��� Ȯ���Ѵ�.
		if (beg == end || *beg != n) {
			ret_lines->insert(n);
		}
		else if (beg != end) {
			++beg;
		}
	}
	return QueryResult(rep(), ret_lines, result.get_file());
}
