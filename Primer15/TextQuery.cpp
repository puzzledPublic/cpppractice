#include "TextQuery.h"

//입력 파일을 읽고 줄 내용을 줄 번호에 사상한 map을 만든다.
TextQuery::TextQuery(std::ifstream &is) : file(new std::vector<std::string>)
{
	std::string text;
	while (std::getline(is, text)) {	//파일 내 각 줄에 대해
		file->push_back(text);			//해당 줄의 본문을 기억한다.
		int n = file->size() - 1;		//현재 줄 번호
		std::istringstream line(text);	//줄 내용을 단어로 분리한다.
		std::string word;
		while (line >> word) {			//해당 줄의 각 단어에 대해
			erase_special_symbol(word);
			auto& lines = wm[word];		//lines는 shared_ptr이다.
			if (!lines) {				//word 내 단어가 처음이면 이 포인터는 null이다.
				lines.reset(new std::set<line_no>);		//새 set을 할당한다.
			}
			lines->insert(n);			//줄 번호를 삽입한다.
		}
	}
}

QueryResult TextQuery::query(const std::string &sought) const
{
	//sought를 못 찾으면 이 set에 대한 포인터를 반환한다.
	static std::shared_ptr<std::set<line_no>> nodata(new std::set<line_no>);
	//wm에 단어를 추가하지 않도록 첨자 연산이 아닌 find를 사용한다.
	auto loc = wm.find(sought);
	if (loc == wm.end()) {
		return QueryResult(sought, nodata, file);	//못찾음
	}
	else {
		return QueryResult(sought, loc->second, file);
	}
}

void erase_special_symbol(std::string &word)
{
	std::string temp;
	for (auto i = word.begin(); i != word.end(); i++) {
		if (('a' <= *i && *i <= 'z') || ('A' <= *i && *i <= 'Z')) {
			temp.push_back(*i);
		}
	}
	word = temp;
}
