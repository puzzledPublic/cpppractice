#include "QueryResult.h"

std::string make_plural(std::size_t ctr, const std::string& word, const std::string& ending) {
	return (ctr > 1) ? word + ending : word;
}
std::ostream & print(std::ostream &os, const QueryResult &qr)
{
	//단어를 찾으면 횟수와 나온 곳 모두 출력한다.
	os << "\n" << qr.sought << " occurs " << qr.lines->size() << "" << make_plural(qr.lines->size(), "time", "s") << std::endl;
	//단어가 나온 각 줄을 출력한다.
	for (auto num : *qr.lines) {	//set의 모든 요소에 대해
		os << "\t(line " << num + 1 << ")" << *(qr.file->begin() + num) << std::endl;	//본문 줄을 0으로 시작해 사용자를 혼란스럽게 하지 않는다.
	}
	return os;
}
