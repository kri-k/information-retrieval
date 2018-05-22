#include <iostream>
#include <cstdlib>
#include <fstream>
#include <experimental/filesystem>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <ctime>
#include <cassert>

#include "../codec.h"
#include "../index_jumps.h"


time_t START_TIME;

#define TIMING(s) START_TIME = time(0); s; std::cout << "* Done. Time: " << (time(0) - START_TIME) << " s" << endl;


const size_t MAX_BLOCK_SIZE = 10000000;
const size_t MAX_INDEX_BLOCK_SIZE = 100000;
const size_t MAX_POS_FILES_PER_DIR = 5000;
const size_t DOCS_PER_FILE = 10;


using namespace std;

namespace fs = std::experimental::filesystem;

using TID = unsigned int;


vector<int8_t> compressedDataBuffer_4bit;
vector<int8_t> compressedDataBuffer_8bit;
vector<int16_t> compressedDataBuffer_16bit;
vector<TID> jumpedDataBuffer;


void systemNoReturn(const char* s) {
    int res = system(s);
    if (res != 0) {
        cerr << "Command '" << s << "' exited with non-zero code" << endl;
        exit(1);
    }
}


void print_help() {
    cout << "specify parameters:\n"; 
    cout << "output_directory" << endl;
    cout << "file with uniq tokens" << endl;
    cout << "file with terms" << endl;
    cout << "paths to directories with docs" << endl;
}


void initTokensTerms(
    const string &tokensFile, 
    const string &termsFile, 
    unordered_map<string, TID> &tokenToTermId, 
    vector<string> &terms) 
{
    TID nextTermId = 0;
    unordered_map<string, TID> termsId;

    ifstream finTokens(tokensFile);
    ifstream finTerms(termsFile);

    string token;
    string term;

    while (!finTokens.eof() && !finTerms.eof()) {
        finTokens >> token;
        finTerms >> term;

        auto iter = termsId.find(term);

        if (iter == termsId.end()) {
            termsId[term] = nextTermId;
            terms.push_back(term);
            tokenToTermId[token] = nextTermId;
            nextTermId++;
        } else {
            tokenToTermId[token] = iter->second;
        }
    }

    if (finTokens.eof() && !finTerms.eof()) {
        cerr << termsFile << " is bigger than " << tokensFile << " but should be equal" << endl;
        string s;
        finTerms >> s;
        cerr << "Next term '" << s << "' size = " << s.size() << endl;
    } else if (!finTokens.eof() && finTerms.eof()) {
        cerr << tokensFile << " is bigger than " << termsFile << " but should be equal" << endl;
        string s;
        finTokens >> s;
        cerr << "Next token '" << s << "' size = " << s.size() << endl;
    }

    finTerms.close();
    finTokens.close();
}


void writeRecords(vector<pair<TID, TID>> &records, const string &outputFile) {
    sort(records.begin(), records.end());

    size_t cnt = 1;
    ofstream fout(outputFile);
    fout << records.front().first << ' ' << records.front().second << '\n';
    auto prev = records.front();

    for (auto &it : records) {
        if (it != prev) {
            fout << it.first << ' ' << it.second << '\n';
            prev = it;
            cnt++;
        }
    }

    fout.close();
    cout << "Wrote " << cnt << " records to " << outputFile << endl;
}


void writePositions(TID docId, map<TID, vector<unsigned int>> &positions, const string &fileName) {
    ofstream fout(fileName, ios_base::binary | ios_base::app);
    fout.write((char*)&docId, sizeof(TID));

    unsigned int n;

    n = positions.size();
    fout.write((char*)&n, sizeof(unsigned int));

    for (auto &i : positions) {
        fout.write((char*)&i.first, sizeof(TID));

        auto &v = i.second;

        unsigned int prev = v[0];
        for (unsigned int j = 1; j < v.size(); j++) {
            v[j] = v[j] - prev;
            prev += v[j];
        }

        compressedDataBuffer_4bit.clear();
        unsigned int n4 = VHB<unsigned int, int8_t>::encode(v, compressedDataBuffer_4bit);
        assert(n4 <= (1 << (sizeof(unsigned int) * 8 - 2)));
        n4 |= 1 << (sizeof(unsigned int) * 8 - 1);

        compressedDataBuffer_8bit.clear();
        unsigned int n8 = VB<unsigned int, int8_t>::encode(v, compressedDataBuffer_8bit);
        assert(n8 <= (1 << (sizeof(unsigned int) * 8 - 2)));

        if (compressedDataBuffer_8bit.size() <= compressedDataBuffer_4bit.size()) {
            fout.write((char*)&n8, sizeof(unsigned int));
            fout.write(
                (char*)compressedDataBuffer_8bit.data(), 
                compressedDataBuffer_8bit.size() * sizeof(int8_t));
        } else {
            fout.write((char*)&n4, sizeof(unsigned int));
            fout.write(
                (char*)compressedDataBuffer_4bit.data(), 
                compressedDataBuffer_4bit.size() * sizeof(int8_t));
        }
    }
    fout.close();
}


void processDocuments(
    char *paths[], 
    int pathsNum, 
    const string &outputDir, 
    const unordered_map<string, TID> &tokenToTermId, 
    vector<string> &documents)
{
    string token;
    TID curDocId = 0;

    vector<pair<TID, TID>> records;

    int curOutputFileNum = 0;
    string positionsDir = outputDir + "/positions/";

    for (int i = 0; i < pathsNum; i++) {
        for (auto& p: fs::recursive_directory_iterator(paths[i])) {
            if (fs::is_directory(p)) continue;

            ifstream fin(p.path());

            map<TID, vector<unsigned int>> positions;
            unsigned int curPos = 0;

            while (!fin.eof()) {
                fin >> token;

                auto it = tokenToTermId.find(token);
                if (it == tokenToTermId.end()) {
                    cerr << "ERROR: Token '" << token << "' from file '" << p << "' not in the dictinonary." << endl;
                    continue;
                }

                positions[it->second].push_back(curPos++);
                records.push_back({it->second, curDocId});

                if (records.size() == MAX_BLOCK_SIZE) {
                    string fileName = outputDir + '/' + to_string(curOutputFileNum);
                    writeRecords(records, fileName);
                    records.clear();
                    curOutputFileNum++;
                }
            }

            fin.close();

            string spath(p.path());
            int pos = spath.find_last_of('/');
            documents.push_back(spath.substr(pos + 1));

            TID curPosFileNum = curDocId / DOCS_PER_FILE;
            string curPosFile = positionsDir + to_string(curPosFileNum / MAX_POS_FILES_PER_DIR);
            if (curDocId % (MAX_POS_FILES_PER_DIR * DOCS_PER_FILE) == 0) {
                systemNoReturn(("mkdir " + curPosFile).c_str());
            }
            curPosFile += "/" + to_string(curPosFileNum);

            writePositions(curDocId, positions, curPosFile);

            curDocId++;
        }
    }

    if (!records.empty()) {
        string fileName = outputDir + '/' + to_string(curOutputFileNum);
        writeRecords(records, fileName);
    }
}


void writeVector(const vector<string> &v, const string &outputFile) {
    ofstream fout(outputFile);

    for (auto &s : v) {
        fout << s << '\n';
    }

    fout.close();
}


void writeIndex(vector<pair<TID, vector<TID>>> &records, const string &outputFile) {
    ofstream fout(outputFile, ios_base::binary);

    unsigned int n = records.size();
    fout.write((char*)&n, sizeof(TID));

    for (size_t i = 0; i < records.size(); i++) {
        fout.write((char*)&records[i].first, sizeof(TID));

        auto &v = records[i].second;
        TID prev = v[0];
        for (unsigned int j = 1; j < v.size(); j++) {
            v[j] = v[j] - prev;
            prev += v[j];
        }

        jumpedDataBuffer.clear();
        Jump::insertJumps<VHB<TID, int8_t>, TID>(v, jumpedDataBuffer);

        compressedDataBuffer_4bit.clear();
        unsigned int n4 = VHB<TID, int8_t>::encode(jumpedDataBuffer, compressedDataBuffer_4bit);
        assert(n4 <= (1 << (sizeof(unsigned int) * 8 - 2)));
        n4 |= 1 << (sizeof(unsigned int) * 8 - 1);

        jumpedDataBuffer.clear();
        Jump::insertJumps<VB<TID, int8_t>, TID>(v, jumpedDataBuffer);

        compressedDataBuffer_8bit.clear();
        unsigned int n8 = VB<TID, int8_t>::encode(jumpedDataBuffer, compressedDataBuffer_8bit);
        assert(n8 <= (1 << (sizeof(unsigned int) * 8 - 2)));

        n = v.size();
        fout.write((char*)&n, sizeof(unsigned int));

        if (compressedDataBuffer_8bit.size() <= compressedDataBuffer_4bit.size()) {
            fout.write((char*)&n8, sizeof(unsigned int));
            fout.write(
                (char*)compressedDataBuffer_8bit.data(), 
                compressedDataBuffer_8bit.size() * sizeof(int8_t));
        } else {
            fout.write((char*)&n4, sizeof(unsigned int));
            fout.write(
                (char*)compressedDataBuffer_4bit.data(), 
                compressedDataBuffer_4bit.size() * sizeof(int8_t));
        }
    }

    fout.close();
    cout << "Wrote " << records.size() << " records to " << outputFile << endl;
}


void buildIndex(const string &outputDir) {
    string f = outputDir + "/merge";
    ifstream fin(f);

    vector<pair<TID, vector<TID>>> index;

    TID prevTerm;

    TID term, doc;

    int curOutputFileNum = 0;

    fin >> term >> doc;
    prevTerm = term;
    index.push_back({term, vector<TID>()});
    index.back().second.push_back(doc);

    while (!fin.eof()) {
        fin >> term >> doc;

        if (term != prevTerm) {
            if (index.size() == MAX_INDEX_BLOCK_SIZE) {
                writeIndex(index, outputDir + '/' + to_string(curOutputFileNum));
                curOutputFileNum++;
                index.clear();
            }

            index.push_back({term, vector<TID>()});
            prevTerm = term;
        }

        index.back().second.push_back(doc);
    }

    if (!index.empty()) {
        writeIndex(index, outputDir + '/' + to_string(curOutputFileNum));
    }

    fin.close();
}


int main(int argc, char *argv[]) {
    if (argc < 5) {
        print_help();
        return 0;
    }

    string outputDir(argv[1]); 
    string tokensFile(argv[2]);
    string termsFile(argv[3]);

    vector<string> terms;
    unordered_map<string, TID> tokenToTermId;

    vector<string> documents;

    string positionsDir = outputDir + "/positions/";

    string cmd;

    cout << "Building token to term dictinonary..." << endl;
    TIMING(initTokensTerms(tokensFile, termsFile, tokenToTermId, terms));

    cmd = "if ! [ -d " + positionsDir + " ]; then mkdir " + positionsDir + "; fi";
    systemNoReturn(cmd.c_str());

    cout << "Processing documents..." << endl;
    TIMING(processDocuments(argv + 4, argc - 4, outputDir, tokenToTermId, documents));

    cout << "Merging index blocks..." << endl;
    cmd = "sort -k 1n -k 2n --merge --unique --buffer-size=30% --parallel=2 --output=";
    cmd += outputDir + "/merge ";
    cmd += "$(find " + outputDir + " -maxdepth 1 -type f)";

    TIMING(systemNoReturn(cmd.c_str()));

    cout << "Deleting tmp files..." << endl;
    cmd = "rm -f " + outputDir + '/';
    for (int i = 0; i < 10; i++) {
        string tmp = cmd + (char)('0' + i) + '*';
        cout << tmp << endl;
        systemNoReturn(tmp.c_str());
    }
    cout << "* Done" << endl;

    cout << "Writing doc titles list..." << endl;
    TIMING(writeVector(documents, outputDir + "/docs"));
    documents.clear();

    cout << "Writing terms list..." << endl;
    TIMING(writeVector(terms, outputDir + "/terms"));
    terms.clear();

    cout << "Building index..." << endl;
    TIMING(buildIndex(outputDir));

    return 0;
}