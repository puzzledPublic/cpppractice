#include "Query.h"

std::ostream & operator<<(std::ostream & os, const Query & query)
{
	//Query::rep������ rep()�� ���� �ڽ��� Query_base �����͸� ���� ���� ȣ���Ѵ�.
	return os << query.rep();
}
