#pragma once

#include <string>
#include <cstdio>
#include <map>
#include <unordered_map>
#include <set>
#include <vector>
#include <iostream>
#include "compressed_data_stream.h"

using namespace std;

using TID = unsigned int;


const string WORK_DIR("/home/forlabs/wiki_index_jump/");
const unsigned int MAX_DOC_ID = 545386;

const size_t MAX_INDEX_FILES_NUM = 100;
const size_t RECORDS_PER_FILE = 100000;

const size_t MAX_POS_FILES_PER_DIR = 5000;
const size_t DOCS_PER_FILE = 10;


class IndexRecord {
private:
    CompressedDataStream<TID> *stream;
public:
    unsigned int length;

    IndexRecord() {
        stream = nullptr;
    }

    IndexRecord(FILE *fin) {
        fread(&length, sizeof(unsigned int), 1, fin);

        unsigned int bitCnt;
        fread(&bitCnt, sizeof(unsigned int), 1, fin);

        int codecType = bitCnt >> (sizeof(unsigned int) * 8 - 1);
        bitCnt &= (1 << (sizeof(unsigned int) * 8 - 1)) - 1;

        unsigned int size = (bitCnt + sizeof(int8_t) * 8 - 1) / (sizeof(int8_t) * 8);

        int8_t *data = new int8_t[size];
        fread(data, sizeof(int8_t), size, fin);

        if (codecType == 0) {
            stream = new VBDataStream<TID>(data, size);
        } else {
            stream = new VHBDataStream<TID>(data, size);
        }
    }

    IndexRecord(const IndexRecord &other) {
        stream = other.stream->copy();
        length = other.length;
    }

    ~IndexRecord() {
        if (stream) delete stream;
    }

    IndexRecord& operator=(const IndexRecord &other) {
        if (stream) {
            delete stream;
        }
        stream = other.stream->copy();
        length = other.length;
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

    unsigned int getOffset() {
        return stream->getOffset();
    }

    void setOffset(unsigned int offset) {
        stream->setOffset(offset);
    }
};


class TermPositions {
private:
    CompressedDataStream<unsigned int> *stream;
    unsigned int curPos;
    bool detached;
public:
    TID termId;

    TermPositions() : stream(nullptr), curPos(0), detached(false) {}

    TermPositions(TID termId, CompressedDataStream<unsigned int> *stream) :
        stream(stream), curPos(0), detached(false), termId(termId) {}

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

    DocTermPositions(TID id, FILE *fin) {
        docId = id;
        TID termId;
        unsigned int termNum;
        unsigned int bitCnt;
        unsigned int size;
        int codecType;
        CompressedDataStream<unsigned int> *stream;

        fread(&termNum, sizeof(unsigned int), 1, fin);

        terms.reserve(termNum);

        for (int i = 0; i < termNum; i++) {
            fread(&termId, sizeof(TID), 1, fin);
            fread(&bitCnt, sizeof(unsigned int), 1, fin);

            codecType = bitCnt >> (sizeof(unsigned int) * 8 - 1);
            bitCnt &= (1 << (sizeof(unsigned int) * 8 - 1)) - 1;
            size = (bitCnt + sizeof(int8_t) * 8 - 1) / (sizeof(int8_t) * 8);

            int8_t *data = new int8_t[size];
            fread(data, sizeof(int8_t), size, fin);

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

    static void skip(FILE *fin) {
        TID termId;
        unsigned int termNum;
        unsigned int bitCnt;
        unsigned int size;

        fread(&termNum, sizeof(unsigned int), 1, fin);

        for (int i = 0; i < termNum; i++) {
            fread(&termId, sizeof(TID), 1, fin);
            fread(&bitCnt, sizeof(unsigned int), 1, fin);

            bitCnt &= (1 << (sizeof(unsigned int) * 8 - 1)) - 1;
            size = (bitCnt + sizeof(int8_t) * 8 - 1) / (sizeof(int8_t) * 8);

            fseek(fin, sizeof(int8_t) * size, SEEK_CUR);
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
        FILE *fin = fopen(indexFiles[file].c_str(), "rb");

        unsigned int n;
        fread(&n, sizeof(unsigned int), 1, fin);

        TID termId;
        for (unsigned int i = 0; i < n; i++) {
            fread(&termId, sizeof(TID), 1, fin);

            IndexRecord rec(fin);

            if (records.find(termId) != records.end()) {
                rec.clear();
                continue;
            }

            records[termId] = rec;
        }

        fclose(fin);
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

        FILE *fin = fopen(fileName.c_str(), "rb");

        for (int i = 0; i < DOCS_PER_FILE; i++) {
            fread(&curId, sizeof(TID), 1, fin);
            if (curId == docId) {
                DocTermPositions res(docId, fin);
                fclose(fin);
                return res;
            } else {
                DocTermPositions::skip(fin);
            }
        }

        fclose(fin);
        if (curId != docId) {
            cerr << "ERROR: Can't find positions for docId " << docId << ", file name is '" << fileName << "'" << endl;
        }
        return DocTermPositions();
    }
};
