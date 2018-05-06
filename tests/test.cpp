#include <iostream>
#include <fstream>
#include <ctime>
#include "test.h"

using namespace std;

const string sampleDir("./samples/");
const string resultDir("./results/");

time_t TOTAL_TIME;
time_t START_TIME;
time_t FINISH_TIME;
#define TIMING(s) START_TIME = time(0); s; FINISH_TIME = time(0); \
					std::cout << "* Done. Time: " << (FINISH_TIME - START_TIME) << " s" << endl; \
					TOTAL_TIME += FINISH_TIME - START_TIME;



int main() {
	for (auto &t : tests) {
		cout << "Start test: [" << t.testName << "]" << endl;
		ofstream fout(resultDir + t.sampleFileName);
		TIMING(t.f(fout));
		fout.close();
	}

	cout << "====================" << endl;

	for (auto &t : tests) {
		string a = sampleDir + t.sampleFileName;
		string b = resultDir + t.sampleFileName;
		cout << "Test [" << t.testName << "]: ";
		cout.flush();
		string cmd = "cmp -s " + a + " " + b + 
			"; if [ $? -eq 0 ]; then echo \"OK\"; else echo \"FAIL\"; fi";
		int res = system(cmd.c_str());
	}

	cout << "====================" << endl;
	cout << "Total run time: " << TOTAL_TIME << " s" << endl;

	return 0;
}