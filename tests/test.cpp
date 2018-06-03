#include <iostream>
#include <fstream>
#include <ctime>
#include <chrono>
#include <fstream>
#include <vector>
#include <algorithm>
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

int TOTAL_ERR_NUM = 0;

bool eq(const string &fileA, const string &fileB) {
    vector<string> a;
    vector<string> b;
    string s;

    ifstream finA(fileA);
    while (finA >> s) {
        a.push_back(s);
    }
    finA.close();
    sort(a.begin(), a.end());

    ifstream finB(fileB);
    while (finB >> s) {
        b.push_back(s);
    }
    finB.close();
    sort(b.begin(), b.end());

    return a == b;
}


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
        if (eq(a, b)) {
            cout << "OK";
        } else {
            cout << "FAIL";
            TOTAL_ERR_NUM++;
        }
        cout << endl;
    }

    cout << "====================" << endl;
    cout << "Total run time: " << TOTAL_TIME << " ms" << endl;
    cout << "Errors: " << TOTAL_ERR_NUM << "/" << tests.size() << endl;
    return 0;
}