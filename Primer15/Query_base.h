#pragma once

#include "TextQuery.h"
//�߻� Ŭ������ ��ü ��ȸ Ÿ�Կ� ���� ���� Ŭ������ �ൿ�ϹǷ� ��� ����� private �̴�.
class Query_base {
	friend class Query;
protected:
	using line_no = TextQuery::line_no;	//eval �Լ����� ����Ѵ�.
	virtual ~Query_base() = default;
private:
	//eval �Լ������� �� Query�� ��ġ�ϴ� QueryResult�� ��ȯ�Ѵ�.
	virtual QueryResult eval(const TextQuery&) const = 0;
	//rep �Լ��� ��ȸ ������ ǥ���� string �̴�.
	virtual std::string rep() const = 0;
};