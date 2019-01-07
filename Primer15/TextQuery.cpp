#include "TextQuery.h"

//�Է� ������ �а� �� ������ �� ��ȣ�� ����� map�� �����.
TextQuery::TextQuery(std::ifstream &is) : file(new std::vector<std::string>)
{
	std::string text;
	while (std::getline(is, text)) {	//���� �� �� �ٿ� ����
		file->push_back(text);			//�ش� ���� ������ ����Ѵ�.
		int n = file->size() - 1;		//���� �� ��ȣ
		std::istringstream line(text);	//�� ������ �ܾ�� �и��Ѵ�.
		std::string word;
		while (line >> word) {			//�ش� ���� �� �ܾ ����
			erase_special_symbol(word);
			auto& lines = wm[word];		//lines�� shared_ptr�̴�.
			if (!lines) {				//word �� �ܾ ó���̸� �� �����ʹ� null�̴�.
				lines.reset(new std::set<line_no>);		//�� set�� �Ҵ��Ѵ�.
			}
			lines->insert(n);			//�� ��ȣ�� �����Ѵ�.
		}
	}
}

QueryResult TextQuery::query(const std::string &sought) const
{
	//sought�� �� ã���� �� set�� ���� �����͸� ��ȯ�Ѵ�.
	static std::shared_ptr<std::set<line_no>> nodata(new std::set<line_no>);
	//wm�� �ܾ �߰����� �ʵ��� ÷�� ������ �ƴ� find�� ����Ѵ�.
	auto loc = wm.find(sought);
	if (loc == wm.end()) {
		return QueryResult(sought, nodata, file);	//��ã��
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
