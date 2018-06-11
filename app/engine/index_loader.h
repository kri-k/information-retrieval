#pragma once

#include <string>
#include <cstdio>
#include <map>
#include <unordered_map>
#include <set>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include "compressed_data_stream.h"

using namespace std;

using TID = unsigned int;


const string WORK_DIR("/home/krik-standard/wiki_index/");
const TID MAX_DOC_ID = 1476244;

const size_t MAX_INDEX_FILES_NUM = 100;
const size_t RECORDS_PER_FILE = 100000;

const size_t MAX_POS_FILES_PER_DIR = 5000;
const size_t DOCS_PER_FILE = 10;

const string TF_FILE_PATH = "/tf";
const string TF_OFFSET_FILE_PATH = "/tf_offsets";
const string POSITIONS_DIR_PATH = "/positions/";
const string EXTERNAL_IDS_FILE_PATH = "/docs";


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

    static const int MAX_DOC_TF_CACHE_SIZE = 100000;
    vector<unsigned int> TFOffsets;
    map<TID, map<TID, unsigned int>> docTermTF;
    FILE *finTF;

    vector<TID> externalIds;

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

    void loadDocTF(TID docId) {
        if (docTermTF.size() == MAX_DOC_TF_CACHE_SIZE) {
            docTermTF.erase(docTermTF.begin());
        }
        fseek(finTF, sizeof(int8_t) * TFOffsets[docId], SEEK_SET);
        unsigned int length = TFOffsets[docId + 1] - TFOffsets[docId];
        int8_t *buf = new int8_t[length];
        fread(buf, sizeof(int8_t), length, finTF);

        VB<unsigned int, int8_t> vb(buf, length);
        TID termId;
        unsigned int tf;

        vector<pair<TID, unsigned int>> res;

        while (!vb.end()) {
            termId = vb.decodeNext();
            tf = vb.decodeNext();
            res.emplace_back(termId, tf);
        }

        delete[] buf;

        docTermTF[docId].insert(res.begin(), res.end());
    }
public:
    Index() {
        recordsNum = 0;

        for (int i = 0; i < MAX_INDEX_FILES_NUM; i++) {
            indexFiles.push_back(WORK_DIR + to_string(i));
        }

        unsigned int tmp;
        FILE *fin = fopen((WORK_DIR + TF_OFFSET_FILE_PATH).c_str(), "rb");
        for (TID i = 0; i <= MAX_DOC_ID; ++i) {
            fread(&tmp, sizeof(unsigned int), 1, fin);
            TFOffsets.push_back(tmp);
        }
        fclose(fin);

        finTF = fopen((WORK_DIR + TF_FILE_PATH).c_str(), "rb");

        ifstream finExternalIds(WORK_DIR + EXTERNAL_IDS_FILE_PATH);
        string externalIdString;
        while (finExternalIds >> externalIdString) {
            externalIds.push_back(stoll(externalIdString));
        }
    }

    ~Index() {
        for (auto i : records) {
            i.second.clear();
        }
        if (finTF) fclose(finTF);
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
            WORK_DIR + POSITIONS_DIR_PATH + 
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

    unsigned int getTF(TID docId, TID termId) {
        if (docTermTF.count(docId) == 0) {
            loadDocTF(docId);
        }
        return docTermTF[docId][termId];
    }

    TID getExternalId(TID internalId) {
        return externalIds[internalId];
    }
};
