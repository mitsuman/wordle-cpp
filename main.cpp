#include <stdio.h>
#include <string.h>
#include <vector>
#include <cassert>
#include <random>

constexpr int kWordLen = 5;
constexpr int kCharNum = 26;

int logLevel = 0;
enum {LOG_ERROR, LOG_INFO, LOG_VERBOSE};

#define LOGI(...) {if (logLevel >= LOG_INFO) printf(__VA_ARGS__);}
#define LOGV(...) {if (logLevel >= LOG_VERBOSE) printf(__VA_ARGS__);}
class Dictionary {
public:
	std::vector<char> buf;
	int size_[2];
	int idx_ {0};

	const char* Get(int idx) const {
		assert(idx >= 0 && idx < size_[1]);
		return &buf[(kWordLen + 1) * idx];
	}

	int AnswerSize() const {
		return size_[0];
	}

	int CandidateSize() const {
		return size_[1];
	}

	bool Includes(const char* str) const {
		for (int i = 0 ; i < AnswerSize(); ++i) {
			if (0 == strcmp(str, Get(i)))
			return true;
		}
		return false;
	}

	int Search(const char* str) const {
		for (int i = 0 ; i < CandidateSize(); ++i) {
			if (0 == strcmp(str, Get(i)))
			return i;
		}
		return -1;
	}

	bool Append(const char* fileName) {
		FILE *fp = fopen(fileName, "rb");
		if (!fp) {
			printf("Cannot open %s.\n", fileName);
			return false;
		}
		fseek(fp, 0, SEEK_END);
		const int bufOldSize = buf.size();
		const int fileSize = ftell(fp);
		buf.resize(bufOldSize + fileSize);
		fseek(fp, 0, SEEK_SET);
		int readSize = (int) fread(&buf[bufOldSize], 1, fileSize, fp);
		fclose(fp);

		if (readSize != fileSize) {
			printf("read failed %s, read %d expected %d\n", fileName, readSize, fileSize);
			return false;
		}

		size_[idx_] = buf.size() / (kWordLen+1);
		if (idx_ == 0) size_[1] = size_[0];

		// validation
		char* insertedBuf = &buf[bufOldSize];
		for (int i = (idx_ == 0 ? 0 : size_[0]); i < size_[idx_]; ++i) {
			char* word = &buf[(i) * (kWordLen+1)];
			char* terminal = &word[kWordLen];
			if (*terminal != '\n') {
				printf("parse error %s, invalid delimite at line %d\n", fileName, i + 1);
				return false;
			}
			*terminal = 0;
			for (int j = 0; j < kWordLen; ++j) {
				int c = word[j];
				if ('a' > c || c > 'z') {
					printf("parse error %s, invalid char code '%s' at line %d pos %d\n", fileName, word, i + 1, j + 1);
					return false;
				}
			}
		}
		LOGI("%s : %d words\n", fileName, size_[idx_]);
		idx_++;
		return true;
	}

};

class Master {
public:
	typedef uint8_t ResultType[kWordLen];
	enum { RES_HIT=2, RES_BLOW=1, RES_OUT=0 };
	static const char kResToChar[3];
	static bool Check(ResultType result, const char* answer, const char* input);
	bool Check(ResultType result, const char* input);
	void SetAnswer(const char *answer);
private:
	char answer_[kWordLen+1];
};

const char Master::kResToChar[3] = {'.', '?', 'o'};

void Master::SetAnswer(const char *answer) {
	strncpy(answer_, answer, sizeof answer_);
}

bool Master::Check(ResultType result, const char* input) {
	return Check(result, answer_, input);
}

bool Master::Check(ResultType result, const char* answer, const char* input) {
	int hit = 0;
	for (int i = 0; i < kWordLen; ++i) {
		const char c = input[i];
		if (c == answer[i]) {
			result[i] = RES_HIT;
			hit++;
			continue;
		}

		result[i] = RES_OUT;
		for (int j = 0; j < kWordLen; ++j) {
			if (c == answer[j]) {
				result[i] = RES_BLOW;
				break;
			}
		}
	}
	return hit == kWordLen;
}

class Matcher {
public:
	// 位置に対して確定した文字
	char correct_[kWordLen] {};

	// 位置は分からないが存在することが確定した文字
	uint32_t blow_ {};

	// 位置に対して存在しない事が確定した文字
	// データ量節約のためa-zをbitfieldで表現(a->0bit)
	uint32_t absent_[kWordLen] {};

	void Inspect(const Master::ResultType result, const char* input);
	bool IsMatch(const char* input) const;
};

void Matcher::Inspect(const Master::ResultType result, const char* input) {
	for (int i = 0; i < kWordLen; ++i) {
		const char c = input[i];
		if (result[i] == Master::RES_HIT) {
			correct_[i] = input[i];
		} else {
			absent_[i] |= 1 << (c - 'a');
		}

		if (result[i] == Master::RES_OUT) {
			for (int j = 0; j < kWordLen; ++j) {
				absent_[j] |= 1 << (c - 'a');
			}
		 } else {
			blow_ |= 1 << (c - 'a');
		 }
	}
}

bool Matcher::IsMatch(const char* input) const {
	uint32_t blow = blow_;
	for (int i = 0; i < kWordLen; ++i) {
		const char c = input[i];
		blow &= ~(1 << (c - 'a'));

		if (correct_[i] != 0) {
			if (c != correct_[i])
				return false;
		}

		if (absent_[i] & (1 << (c - 'a')))
			return false;
	}

	// 使われるべき文字が使われていない
	if (blow != 0)
		return false;

	return true;
}

class Solver {
public:
	typedef uint16_t IndexType;
	std::vector<IndexType> groups_;
	Matcher* matcher_;
	std::mt19937* mt_;
	const Dictionary* dict_ {};

	void Import(Matcher& matcher, std::mt19937& mt_, const Dictionary& dict);
	void Screen();
	virtual const char* Select() const;
	int GetSize() const { return groups_.size(); }
};

void Solver::Import(Matcher& matcher, std::mt19937& mt, const Dictionary& dict) {
	matcher_ = &matcher;
	mt_ = &mt;
	dict_ = &dict;
	groups_.clear();
	groups_.resize(dict.AnswerSize());
	for (int i = 0; i < groups_.size(); i++) {
		groups_[i] = i;
	}
}

void Solver::Screen() {
	int grpSize = groups_.size();

	for (int i = 0; i < grpSize; i++) {
		if (matcher_->IsMatch(dict_->Get(groups_[i])))
			continue;

		grpSize--;

		if (0 == grpSize) {
			assert("Detect empty group, should NOT reach here" && false);
			break;
		}

		groups_[i] = groups_[grpSize];
		i--;
	}
	groups_.resize(grpSize);
}

const char* Solver::Select() const {
	uint32_t i = (*mt_)() % groups_.size();
	return dict_->Get(groups_[i]);
}

class SolverEntropy : public Solver {
	const char* Select() const override;
};

template <typename T>
constexpr T calculatePower(T value, unsigned power) {
    return power == 0 ? 1 : value * calculatePower(value, power-1);
}

const char* SolverEntropy::Select() const {
	if (groups_.size() == 1) {
		return dict_->Get(groups_[0]);
	}

	// calc entropy
	constexpr int kPatternNum = calculatePower(3, kWordLen);
	//std::vector<float> entropies(easyWordsNum);
	float maxH = -1;
	int maxWordIdx = 0;

	// input word
	for (int i = 0; i < dict_->CandidateSize(); ++i) {
		int patterns[kPatternNum] {};

		// answer word
		for (int j = 0; j < groups_.size(); ++j) {
			Master::ResultType result;
			Master::Check(result, dict_->Get(groups_[j]), dict_->Get(i));
			int ptn = 0;
			for (int k = 0; k < kWordLen; ++k) {
				ptn = (ptn * 3) + result[k];
			}
			assert(ptn < kPatternNum);
			patterns[ptn]++;
		}

		// calc entropy of input word
		float h = 0;
		const float invSize = 1.0f / groups_.size();
		for (int j = 0; j < kPatternNum; ++j) {
			const int cnt = patterns[j];
			if (cnt == 0) continue;
			const float p = cnt * invSize;
			h += p * (-std::log2f(p));
		}
		if (h > maxH) {
			maxH = h;
			maxWordIdx = i;
		}
	}

	LOGV(" %f [bits] ", maxH);

	return dict_->Get(maxWordIdx);
}

class SolverInteractive : public Solver {
	const char* Select() const override {
		while (true) {
			printf("INPUT: ");
			char buf[kWordLen + 8] {};
			char* ret = fgets(buf, sizeof(buf), stdin);
			(void)ret;
			buf[kWordLen] = 0;
			if (0 == strcmp(buf, "/help")) {
				for (int i = 0; i < groups_.size(); ++i) {
					printf("%s ", dict_->Get(groups_[i]));
				}
				printf("\n");
				continue;
			}
			const int idx = dict_->Search(buf);
			if (idx >= 0)
				return dict_->Get(idx);
			printf("\033[F");
		}
	}
};

void help() {
	printf(
"wordle [--solver (random|entropy|interactive)]\n"
"       [--seed NUM]\n"
"       [--answer ANSWER]\n"
"       [--use-hard]\n"
"       [--log-level (1|2)]\n"
	);
}

int main(int argc, char** argv) {
	Dictionary dict;

	std::random_device seed_gen;
	std::mt19937 mt(seed_gen());

	Master master;
	Matcher matcher;
	Solver* solver = nullptr;

	{
		const char* answer = nullptr;
		bool useHard = false;

		for (int i = 1; i < argc; ++i) {
			const char* a = argv[i];
			if (0 == strcmp(a, "--solver")) {
				i++;
				if (0 == strcmp(argv[i], "random")) {
					//solver = new Solver(matcher, mt);
				} else
				if (0 == strcmp(argv[i], "entropy")) {
					solver = new SolverEntropy();
				} else
				if (0 == strcmp(argv[i], "interactive")) {
					solver = new SolverInteractive();
				}
				else {
					printf("unknown solver\n");
					return -1;
				}
			} else if (0 == strcmp(a, "--seed")) {
				i++;
				mt.seed(atoi(argv[i]));
			} else if (0 == strcmp(a, "--log-level")) {
				i++;
				logLevel = atoi(argv[i]);
			} else if (0 == strcmp(a, "--answer")) {
				i++;
				answer = argv[i];
			} else if (0 == strcmp(a, "--use-hard")) {
				useHard = true;
			} else {
				printf("unknown option %s\n", a);
				help();
				return -2;
			}

		}

		if (!dict.Append("easy.txt")) {
			return -1;
		}

		if (useHard) {
			if (!dict.Append("hard.txt")) {
				return -1;
			}
		}

		if (answer == nullptr) {
			answer = dict.Get(mt() % dict.AnswerSize());
			LOGV("answer:%s\n", answer);
		}

		if (!dict.Includes(answer)) {
			printf("wrong answer %s\n", answer);
			return -3;
		}

		master.SetAnswer(answer);

		if (solver == nullptr) {
			solver = new Solver();
		}

		solver->Import(matcher, mt, dict);
	}

	for (int itr = 0; itr < 16; itr++) {
		const char* challenge = solver->Select();
		printf("%s ", challenge);

		Master::ResultType result;
		const bool hit = master.Check(result, challenge);
		printf("[");
		for (int j = 0; j < kWordLen; ++j) putchar(Master::kResToChar[result[j]]);
		printf("] ");

		matcher.Inspect(result, challenge);
		solver->Screen();
		printf("%3d\n", solver->GetSize());

		if (hit)
			break;
	}
	printf("\n");

	return 0;
}