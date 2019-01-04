#include "Query.h"

std::ostream & operator<<(std::ostream & os, const Query & query)
{
	//Query::rep에서는 rep()에 대한 자신의 Query_base 포인터를 통해 가상 호출한다.
	return os << query.rep();
}
