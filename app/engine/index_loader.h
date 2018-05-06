#pragma once

#include <string>
#include <fstream>
#include <map>
#include <unordered_map>
#include <set>
#include <vector>
#include <iostream>
#include "compressed_data_stream.h"

using namespace std;

using TID = unsigned int;


const string WORK_DIR("/home/forlabs/wiki_index_compress/");
const unsigned int MAX_DOC_ID = 545386;

const size_t MAX_INDEX_FILES_NUM = 100;
const size_t RECORDS_PER_FILE = 100000;

const size_t MAX_POS_FILES_PER_DIR = 5000;
const size_t DOCS_PER_FILE = 10;


class IndexRecord {
private:
    CompressedDataStream<TID> *stream;

public:
    IndexRecord() {
        stream = nullptr;
    }

    IndexRecord(ifstream &fin) {
        unsigned int bitCnt;
        fin.read((char*)&bitCnt, sizeof(unsigned int));

        int codecType = bitCnt >> (sizeof(unsigned int) * 8 - 1);
        bitCnt &= (1 << (sizeof(unsigned int) * 8 - 1)) - 1;

        unsigned int size = (bitCnt + sizeof(int8_t) * 8 - 1) / (sizeof(int8_t) * 8);

        int8_t *data = new int8_t[size];
        fin.read((char*)data, sizeof(int8_t) * size);

        if (codecType == 0) {
            stream = new VBDataStream<TID>(data, size);
        } else {
            stream = new VHBDataStream<TID>(data, size);
        }
    }

    IndexRecord(const IndexRecord &other) {
        stream = other.stream->copy();
    }

    ~IndexRecord() {
        if (stream) delete stream;
    }

    IndexRecord& operator=(const IndexRecord &other) {
        if (stream) {
            delete stream;
        }
        stream = other.stream->copy();
        return *this;
    }

    TID get() {
        return stream->get();
    }

    void next() {
        stream->next();
    }

    bool end() {
        return stream->end();
    }

    void clear() {
        if (stream) {
            stream->clear();
            delete stream;
            stream = nullptr;
        }
    }
};


class TermPositions {
private:
    CompressedDataStream<unsigned int> *stream;
    unsigned int curPos;
    bool detached;
public:
    TID termId;

    TermPositions() : curPos(0), stream(nullptr), detached(false) {}

    TermPositions(TID termId, CompressedDataStream<unsigned int> *stream) :
        termId(termId), stream(stream), curPos(0), detached(false) {}

    ~TermPositions() {
        if (detached && stream) delete stream;
    }

    unsigned int get() {
        return curPos + stream->get();
    }

    void next() {
        curPos += stream->get();
        stream->next();
    }

    bool end() {
        return stream->end();
    }

    void clear() {
        if (stream) {
            stream->clear();
            delete stream;
            stream = nullptr;
        }
    }

    void detach() {
        stream = stream->copy();
        curPos = 0;
        detached = true;
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
        unsigned int bitCnt;
        unsigned int size;
        int codecType;
        CompressedDataStream<unsigned int> *stream;

        fin.read((char*)&termNum, sizeof(unsigned int));

        terms.reserve(termNum);

        for (int i = 0; i < termNum; i++) {
            fin.read((char*)&termId, sizeof(TID));

            fin.read((char*)&bitCnt, sizeof(unsigned int));

            codecType = bitCnt >> (sizeof(unsigned int) * 8 - 1);
            bitCnt &= (1 << (sizeof(unsigned int) * 8 - 1)) - 1;
            size = (bitCnt + sizeof(int8_t) * 8 - 1) / (sizeof(int8_t) * 8);

            int8_t *data = new int8_t[size];
            fin.read((char*)data, sizeof(int8_t) * size);

            if (codecType == 0) {
                stream = new VBDataStream<unsigned int>(data, size);
            } else {
                stream = new VHBDataStream<unsigned int>(data, size);
            }

            terms.emplace_back(termId, stream);
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
        unsigned int bitCnt;
        unsigned int size;
        int8_t pos;

        fin.read((char*)&termNum, sizeof(unsigned int));

        for (int i = 0; i < termNum; i++) {
            fin.read((char*)&termId, sizeof(TID));
            fin.read((char*)&bitCnt, sizeof(unsigned int));

            bitCnt &= (1 << (sizeof(unsigned int) * 8 - 1)) - 1;
            size = (bitCnt + sizeof(int8_t) * 8 - 1) / (sizeof(int8_t) * 8);

            for (int j = 0; j < size; j++) {
                fin.read((char*)&pos, sizeof(int8_t));
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
