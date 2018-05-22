#pragma once

#include <set>
#include <map>
#include <cassert>
#include "index_loader.h"
#include "../../index_jumps.h"


using namespace std;


Index INDEX;


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
public:
    SimpleIterator(TID termId) {
        rec = INDEX.get(termId);
        id = termId;
        prevDocId = curDocId = 0;
        prevNum = curNum = 0;
        prevOffset = 0;
        jlen = Jump::jumpLength(rec.length);
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
public:
    NotIterator(IndexIterator *iter) {
        id = 0;
        this->iter = iter;

        while (!iter->end() && id == iter->get()) {
            id++;
            iter->next();
        }
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
};


class QuoteIterator : public IndexIterator {
private:
    vector<TID> id;
    set<TID> uniqTerms;
    unsigned int dist;
    IndexIterator *docIter;
public:
    QuoteIterator(vector<TID> terms, unsigned int distance) {
        assert(terms.size() >= 2);

        id = terms;
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
            if (ok(docIter->get())) {
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
            if (ok(docIter->get())) {
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

    bool ok(TID docId) {
        bool result = false;

        auto allPositions = INDEX.getPositions(docId);
        auto &v = allPositions.terms;
        map<TID, unsigned int> pos;

        for (unsigned int i = 0; i < v.size(); ++i) {
            if (uniqTerms.count(v[i].termId)) {
                pos.emplace(v[i].termId, i);
            }
        }

        vector<TermPositions> termPositions;
        termPositions.reserve(id.size());

        for (TID i : id) {
            assert(pos.count(i) > 0);
            termPositions.push_back(v[pos[i]]);
            termPositions.back().detach();
        }

        while (true) {
            bool end = false;
            for (int i = 1; i < id.size(); i++) {
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
                result = true;
                break;
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
