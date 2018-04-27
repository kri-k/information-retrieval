#pragma once

#include <string>
#include <fstream>
#include <map>
#include <unordered_map>
#include <set>
#include <vector>
#include <iostream>


using namespace std;

using TID = unsigned int;


const string WORK_DIR("/home/forlabs/wiki_index/");
const unsigned int MAX_DOC_ID = 545386;

const size_t MAX_INDEX_FILES_NUM = 100;
const size_t RECORDS_PER_FILE = 100000;

const size_t MAX_POS_FILES_PER_DIR = 5000;
const size_t DOCS_PER_FILE = 10;


struct IndexRecord {
    TID *docId;
    unsigned int length;

    IndexRecord() {
        docId = nullptr;
        length = 0;
    }

    IndexRecord(ifstream &fin) {
        fin.read((char*)&length, sizeof(unsigned int));

        docId = new TID[length];

        for (size_t i = 0; i < length; i++) {
            fin.read((char*)&docId[i], sizeof(TID));
        }
    }

    void clear() {
        if (docId) {
            delete[] docId;
        }
        length = 0;
    }
};


struct TermPositions {
    TID termId;
    unsigned int length;
    unsigned int *positions;

    TermPositions() {
        length = 0;
        positions = nullptr;
    }

    TermPositions(TID termId, unsigned int length, unsigned int *positions) :
        termId(termId), length(length), positions(positions) {}

    TID operator[](unsigned int index) const {
        return positions[index];
    }

    void clear() {
        if (positions) {
            delete[] positions;
            positions = nullptr;
        }
        length = 0;
    }
};


struct DocTermPositions {
    TID docId;
    vector<TermPositions> terms;

    DocTermPositions() {}

    DocTermPositions(TID id, ifstream &fin) {
        docId = id;
        TID termId;
        unsigned int termNum;
        unsigned int len;
        unsigned int n;

        fin.read((char*)&termNum, sizeof(unsigned int));

        terms.reserve(termNum);

        for (int i = 0; i < termNum; i++) {
            fin.read((char*)&termId, sizeof(TID));
            fin.read((char*)&n, sizeof(unsigned int));
            unsigned int *arrPos = new unsigned int[n];
            for (int j = 0; j < n; j++) {
                unsigned int pos;
                fin.read((char*)&pos, sizeof(unsigned int));
                arrPos[j] = pos;
            }

            terms.emplace_back(termId, n, arrPos);
        }
    }

    void clear() {
        for (auto &i : terms) {
            i.clear();
        }
    }

    static void skip(ifstream &fin) {
        TID termId;
        unsigned int termNum;
        unsigned int len;
        unsigned int n;
        unsigned int pos;

        fin.read((char*)&termNum, sizeof(unsigned int));

        for (int i = 0; i < termNum; i++) {
            fin.read((char*)&termId, sizeof(TID));
            fin.read((char*)&n, sizeof(unsigned int));
            for (int j = 0; j < n; j++) {
                fin.read((char*)&pos, sizeof(unsigned int));
            }
        }
    }
};


class Index {
private:
    size_t recordsNum;
    vector<string> indexFiles;
    map<TID, size_t> usageCnt;
    map<TID, IndexRecord> records;

    void loadDocId(unsigned int file) {
        ifstream fin(indexFiles[file]);

        unsigned int n;
        fin.read((char*)&n, sizeof(unsigned int));
        
        TID termId;
        for (unsigned int i = 0; i < n; i++) {
            fin.read((char*)&termId, sizeof(TID));
            IndexRecord rec(fin);

            if (records.find(termId) != records.end()) {
                rec.clear();
                continue;
            }

            records[termId] = rec;
        }

        fin.close();
    }

public:
    Index() {
        recordsNum = 0;

        for (int i = 0; i < MAX_INDEX_FILES_NUM; i++) {
            indexFiles.push_back(WORK_DIR + to_string(i));
        }
    }

    ~Index() {
        for (auto i : records) {
            i.second.clear();
        }
    }


    IndexRecord& get(TID termId) {
        usageCnt[termId]++;

        auto iter = records.find(termId);
        if (iter == records.end()) {
            loadDocId(termId / RECORDS_PER_FILE);
        } else {
            return iter->second;
        }

        return records[termId];
    }

    void unget(TID termId) {
        usageCnt[termId]--;
    }

    DocTermPositions getPositions(TID docId) {
        unsigned int n;
        TID curId = -1;
        string fileName = 
            WORK_DIR + "positions/" + 
            to_string(docId / DOCS_PER_FILE / MAX_POS_FILES_PER_DIR) + "/" + 
            to_string(docId / DOCS_PER_FILE);

        ifstream fin(fileName);

        for (int i = 0; i < DOCS_PER_FILE; i++) {
            fin.read((char*)&curId, sizeof(TID));

            if (curId == docId) {
                DocTermPositions res(docId, fin);
                fin.close();
                return res;
            } else {
                DocTermPositions::skip(fin);
            }
        }

        fin.close();
        if (curId != docId) {
            cerr << "ERROR: Can't find positions for docId " << docId << ", file name is '" << fileName << "'" << endl;
        }
        return DocTermPositions();
    }
};
