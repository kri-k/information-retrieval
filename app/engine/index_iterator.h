#pragma once

#include <set>
#include <map>
#include <cassert>
#include <algorithm>
#include <cmath>
#include <unordered_set>
#include "index_loader.h"
#include "../../index_jumps.h"

using namespace std;


Index INDEX;


inline float DFtoIDF(float len, float maxLen) {
    return log(maxLen / len);
}


class IndexIterator {
public:
    virtual ~IndexIterator() {};

    virtual void next() = 0;
    virtual bool end() = 0;
    virtual TID get() = 0;
    virtual unsigned int len() = 0;

    virtual bool isJump() { return false; }
    virtual void jump() {}
    virtual void rollback() {}

    virtual float getRank() { return 0.0; }
};


class SimpleIterator : public IndexIterator {
private:
    TID id;
    IndexRecord rec;
    unsigned int jlen;
    TID curDocId;
    TID prevDocId;
    unsigned int curNum;
    unsigned int prevNum;
    unsigned int prevOffset;
    float IDF;
public:
    SimpleIterator(TID termId) {
        rec = INDEX.get(termId);
        id = termId;
        prevDocId = curDocId = 0;
        prevNum = curNum = 0;
        prevOffset = 0;
        jlen = Jump::jumpLength(rec.length);
        IDF = DFtoIDF(rec.length, MAX_DOC_ID + 1);
    }

    ~SimpleIterator() {
        INDEX.unget(id);
    }

    void next() override {
        curDocId += rec.get();
        if (isJump()) {
            rec.next();
            rec.next();
        }
        curNum++;
        rec.next();
    }

    bool end() override {
        return rec.end();
    }

    TID get() override {
        return curDocId + rec.get();
    }

    unsigned int len() override {
        return rec.length;
    }

    float getRank() override {
        unsigned int tf = INDEX.getTF(get(), id);
        return tf == 0 ? 0 : 1 + log(tf);
        // return log(1 + INDEX.getTF(get(), id)) * IDF;
    }

    bool isJump() override { 
        return Jump::isJump(curNum, jlen, rec.length);
    }

    void jump() override {
        prevOffset = rec.getOffset();
        prevDocId = curDocId;
        prevNum = curNum;

        curDocId += rec.get();
        rec.next();
        curDocId += rec.get();
        rec.next();

        unsigned int tmp = rec.get();
        rec.next();
        rec.setOffset(rec.getOffset() + tmp);

        curNum += jlen;
    }

    void rollback() override {
        rec.setOffset(prevOffset);
        curDocId = prevDocId;
        curNum = prevNum;
    }
};


class NotIterator : public IndexIterator {
private:
    TID id;
    IndexIterator *iter;
    float TF_IDF;
    static const unsigned int NOT_TF = 10;
public:
    NotIterator(IndexIterator *iter) {
        id = 0;
        this->iter = iter;

        while (!iter->end() && id == iter->get()) {
            id++;
            iter->next();
        }

        TF_IDF = NOT_TF * DFtoIDF(iter->len(), MAX_DOC_ID + 1);
    }

    ~NotIterator() {
        if (iter) {
            delete iter;
        }
    }

    void next() override {
        id++;

        while (!iter->end() && id == iter->get()) {
            id++;
            iter->next();
        }
    }

    bool end() override {
        return id > MAX_DOC_ID;
    }

    TID get() override {
        return id;
    }

    unsigned int len() override {
        return max((unsigned int)0, MAX_DOC_ID - iter->len() + 1);
    }

    float getRank() override {
        return TF_IDF;
    }
};


class AndIterator : public IndexIterator {
private:
    IndexIterator *a;
    IndexIterator *b;
public:
    AndIterator(IndexIterator *first, IndexIterator *second) {
        a = first;
        b = second;
        while (!end() && a->get() != b->get()) {
            if (a->get() > b->get()) {
                swap(a, b);
            }
            if (a->isJump()) {
                a->jump();
                if (a->end() || a->get() > b->get()) {
                    a->rollback();
                    a->next();
                }
            } else {
                a->next();
            }
        }
    }

    ~AndIterator() {
        if (a) delete a;
        if (b) delete b;
    }

    void next() override {
        if (end()) {
            return;
        }

        if (a->get() < b->get()) {
            a->next();
        } else {
            b->next();
        }

        while (!end() && a->get() != b->get()) {
            if (a->get() > b->get()) {
                swap(a, b);
            }
            if (a->isJump()) {
                a->jump();
                if (a->end() || a->get() > b->get()) {
                    a->rollback();
                    a->next();
                }
            } else {
                a->next();
            }
        }
    }

    bool end() override {
        return a->end() || b->end();
    }

    TID get() override {
        return a->get();
    }

    unsigned int len() override {
        return min(a->len(), b->len());
    }

    float getRank() override {
        return a->getRank() + b->getRank();
    }
};


class OrIterator : public IndexIterator {
private:
    IndexIterator *a;
    IndexIterator *b;
public:
    OrIterator(IndexIterator *first, IndexIterator *second) {
        a = first;
        b = second;
    }

    ~OrIterator() {
        if (a) delete a;
        if (b) delete b;
    }

    void next() override {
        if (!a->end() && !b->end()) {
            if (a->get() < b->get()) {
                a->next();
            } else if (b->get() < a->get()) {
                b->next();
            } else {
                a->next();
                b->next();
            }
        } 
        else if (!a->end()) a->next();
        else if (!b->end()) b->next();
    }

    bool end() override {
        return a->end() && b->end();
    }

    TID get() override {
        if (!a->end() && !b->end()) {
            if (a->get() < b->get()) {
                return a->get();
            } else {
                return b->get();
            }
        } else if (!a->end()) {
            return a->get();
        } else if (!b->end()) {
            return b->get();
        } else {
            cerr << "ERROR: OrIterator go over bound" << endl;
            return 42;
        }
    }

    unsigned int len() override {
        return min(a->len() + b->len(), MAX_DOC_ID + 1);
    }

    float getRank() override {
        if (!a->end() && !b->end()) {
            TID ida = a->get();
            TID idb = b->get();
            if (ida < idb) {
                return a->getRank();
            } else if (idb < ida) {
                return b->getRank();
            } else {
                return a->getRank() + b->getRank();
            }
        } else if (!a->end()) {
            return a->getRank();
        } else if (!b->end()) {
            return b->getRank();
        } else {
            cerr << "ERROR: OrIterator: getRank go over bound" << endl;
            return 42;
        }
    }
};


class QuoteIterator : public IndexIterator {
private:
    vector<TID> ids;
    set<TID> uniqTerms;
    unsigned int dist;
    IndexIterator *docIter;
    unsigned int lastOkResult;
public:
    QuoteIterator(vector<TID> terms, unsigned int distance) {
        assert(terms.size() >= 2);

        ids = terms;
        dist = distance;

        uniqTerms = set<TID>(terms.begin(), terms.end());
        auto curId = uniqTerms.begin();
        docIter = new SimpleIterator(*curId);
        ++curId;
        while (curId != uniqTerms.end()) {
            docIter = new AndIterator(docIter, new SimpleIterator(*curId));
            ++curId;
        }

        while (!docIter->end()) {
            lastOkResult = ok(docIter->get());
            if (lastOkResult != 0) {
                break;
            }
            docIter->next();
        }
    }

    ~QuoteIterator() {
        if (docIter) delete docIter;
    }

    void next() override {
        docIter->next();
        while (!docIter->end()) {
            lastOkResult = ok(docIter->get());
            if (lastOkResult != 0) {
                break;
            }
            docIter->next();
        }
    }

    bool end() override {
        return docIter->end();
    }

    TID get() override {
        if (docIter->end()) {
            cerr << "ERROR: QuoteIterator go over bound" << endl;
        }
        return docIter->get();
    }

    unsigned int len() override {
        return docIter->len();
    }

    float getRank() override {
        float a = 0.05;
        return a * log(1 + docIter->getRank()) + (1 - a) * log(1 + lastOkResult * ids.size() * 10);
    }

    unsigned int ok(TID docId) {
        unsigned int result = 0;

        auto allPositions = INDEX.getPositions(docId);
        auto &v = allPositions.terms;
        map<TID, unsigned int> pos;

        for (unsigned int i = 0; i < v.size(); ++i) {
            if (uniqTerms.count(v[i].termId)) {
                pos.emplace(v[i].termId, i);
            }
        }

        vector<TermPositions> termPositions;
        termPositions.reserve(ids.size());

        for (TID i : ids) {
            assert(pos.count(i) > 0);
            termPositions.push_back(v[pos[i]]);
            termPositions.back().detach();
        }

        while (true) {
            bool end = false;
            for (int i = 1; i < ids.size(); i++) {
                while (
                    !termPositions[i].end() &&
                    termPositions[i - 1].get() >= termPositions[i].get())
                {
                    termPositions[i].next();
                }

                if (termPositions[i].end()) {
                    end = true;
                    break;
                }
            }

            if (end) break;
            if (termPositions.back().get() -
                termPositions.front().get() + 1 <= dist)
            {
                result++;
            }

            termPositions[0].next();
            if (termPositions[0].end()) {
                break;
            }
        }

        allPositions.clear();
        return result;
    }
};


class RankDecorator {
private:
    const int MAX_RES_NUM = 500;

    vector<pair<TID, float>> result;
    unsigned int pos;

    vector<TID> ids;
    int quoteLen;
    unordered_set<TID> used;

    void initByIndexIterator(IndexIterator *iter) {
        result.clear();
        pos = 0;

        set<pair<float, TID>> s;

        while (!iter->end()) {
            if (used.count(iter->get()) == 0) {
                s.emplace(iter->getRank(), iter->get());
                if (s.size() > MAX_RES_NUM) s.erase(s.begin());
            }
            iter->next();
        }

        delete iter;

        auto i = s.end();

        while (i != s.begin()) {
            i = prev(i);
            result.emplace_back(i->second, i->first);
            used.insert(i->second);
        }
    }
public:
    RankDecorator(IndexIterator *iter) {
        initByIndexIterator(iter);
        quoteLen = -1;
    }

    RankDecorator(const vector<TID> &ids) {
        if (ids.size() == 1) {
            initByIndexIterator(new SimpleIterator(ids.front()));
            quoteLen = -1;
            return;
        }

        this->ids = ids;
        quoteLen = ids.size();

        while (!end() && result.size() == 0) {
            next();
        }
    }

    ~RankDecorator() {}

    void next() {
        ++pos;

        if (pos >= result.size() && quoteLen > -1) {
            IndexIterator *iter = nullptr;
            if (quoteLen == 0) {
                iter = new SimpleIterator(ids.front());
                for (int i = 1; i < ids.size(); i++) {
                    iter = new OrIterator(iter, new SimpleIterator(ids[i]));
                }
            } else if (quoteLen == 1) {
                iter = new SimpleIterator(ids.front());
                for (int i = 1; i < ids.size(); i++) {
                    iter = new AndIterator(iter, new SimpleIterator(ids[i]));
                }
            } else if (quoteLen > 1) {
                for (int start = 0; start + quoteLen - 1 < ids.size(); start++) {
                    vector<TID> quote;
                    for (auto i = start; i < start + quoteLen; i++) {
                        quote.push_back(ids[i]);
                    }

                    auto tmp = new QuoteIterator(quote, quote.size());
                    if (!iter) iter = tmp;
                    else iter = new AndIterator(iter, tmp);
                }
            } else {
                cout << "ERROR: quoteLen out of domain" << endl;
            }

            // cout << "quoteLen = " << quoteLen << endl;
            initByIndexIterator(iter);
            quoteLen--;
        }

        if (!end() && result.size() == 0) {
            next();
        }
    }

    bool end() {
        return pos >= result.size() && quoteLen == -1;
    }

    TID get() {
        // cout << result[pos].first << " has rank " << result[pos].second << endl;
        return result[pos].first;
    }

    unsigned int len() {
        return result.size();
    }
};
