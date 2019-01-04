#include "QueryResult.h"

std::string make_plural(std::size_t ctr, const std::string& word, const std::string& ending) {
	return (ctr > 1) ? word + ending : word;
}
std::ostream & print(std::ostream &os, const QueryResult &qr)
{
	//�ܾ ã���� Ƚ���� ���� �� ��� ����Ѵ�.
	os << "\n" << qr.sought << " occurs " << qr.lines->size() << "" << make_plural(qr.lines->size(), "time", "s") << std::endl;
	//�ܾ ���� �� ���� ����Ѵ�.
	for (auto num : *qr.lines) {	//set�� ��� ��ҿ� ����
		os << "\t(line " << num + 1 << ")" << *(qr.file->begin() + num) << std::endl;	//���� ���� 0���� ������ ����ڸ� ȥ�������� ���� �ʴ´�.
	}
	return os;
}
