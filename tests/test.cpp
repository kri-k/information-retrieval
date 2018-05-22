#include <iostream>
#include <fstream>
#include <ctime>
#include <chrono>
#include "test.h"

using namespace std;
using namespace chrono;

const string sampleDir("./samples/");
const string resultDir("./results/");

double TOTAL_TIME;
time_point<system_clock> START_TIME;
time_point<system_clock> FINISH_TIME;
#define TIMING(s) START_TIME = system_clock::now(); s; FINISH_TIME = system_clock::now(); \
                    cout << "* Done. Time: " << duration_cast<milliseconds>(FINISH_TIME - START_TIME).count() << " ms" << endl; \
                    TOTAL_TIME += duration_cast<milliseconds>(FINISH_TIME - START_TIME).count();



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
    cout << "Total run time: " << TOTAL_TIME << " ms" << endl;

    return 0;
}