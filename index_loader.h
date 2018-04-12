#pragma once

#include <string>
#include <fstream>
#include <map>
#include <unordered_map>
#include <set>
#include <vector>


using namespace std;

using TID = unsigned int;


const string WORK_DIR("/home/forlabs/wiki_index/");
const unsigned int MAX_DOC_ID = 545386;

const int MAX_INDEX_FILES_NUM = 100;
const int RECORDS_PER_FILE = 100000;


struct IndexRecord {
    TID *docId;
    unsigned int length;

    IndexRecord() {
        docId = nullptr;
        length = 0;
    }

    IndexRecord(ifstream &fin) {
        fin.read((char*) &length, sizeof(unsigned int));

        docId = new TID[length];

        for (size_t i = 0; i < length; i++) {
            fin.read((char*) &docId[i], sizeof(TID));
        }
    }

    void clear() {
        if (docId) {
            delete[] docId;
        }
        length = 0;
    }
};


struct Index {
    size_t recordsNum;
    vector<string> indexFiles;
    map<TID, size_t> usageCnt;
    map<TID, IndexRecord> records;

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

    void load(unsigned int file) {
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

    IndexRecord& get(TID termId) {
        usageCnt[termId]++;

        auto iter = records.find(termId);
        if (iter == records.end()) {
            load(termId / RECORDS_PER_FILE);
        } else {
            return iter->second;
        }

        return records[termId];
    }

    void unget(TID termId) {
        usageCnt[termId]--;
    }
};