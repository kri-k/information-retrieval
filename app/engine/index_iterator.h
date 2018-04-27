#pragma once

#include <set>
#include <map>
#include <cassert>
#include "index_loader.h"


using namespace std;


Index INDEX;


class IndexIterator {
public:
    virtual ~IndexIterator() {};
    virtual void next() = 0;
    virtual bool end() = 0;
    virtual TID get() = 0;
    virtual unsigned int len() = 0;
};


class SimpleIterator : public IndexIterator {
private:
    unsigned int pos;
    IndexRecord rec;
public:
    SimpleIterator(TID termId) {
        rec = INDEX.get(termId);
        pos = 0;
    }

    ~SimpleIterator() {}

    void next() override {
        pos++;
    }

    bool end() override {
        return pos >= rec.length;
    }

    TID get() override {
        return rec.docId[pos];
    }

    unsigned int len() override {
        return rec.length;
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
            if (a->get() < b->get()) {
                a->next();
            } else {
                b->next();
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
            if (a->get() < b->get()) {
                a->next();
            } else {
                b->next();
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
        map<TID, TermPositions> pos;

        for (auto &p : allPositions.terms) {
            if (uniqTerms.count(p.termId)) {
                pos.emplace(p.termId, p);
            }
        }

        vector<unsigned int> offset(id.size(), 0);
        vector<TermPositions> termPositions;
        termPositions.reserve(id.size());

        for (TID i : id) {
            termPositions.push_back(pos[i]);
        }

        while (true) {
            bool end = false;
            for (int i = 1; i < id.size(); i++) {
                while (
                    offset[i] < termPositions[i].length &&
                    termPositions[i - 1][offset[i - 1]] >= termPositions[i][offset[i]])
                {
                    offset[i]++;
                }

                if (offset[i] >= termPositions[i].length) {
                    end = true;
                    break;
                }
            }

            if (end) break;
            if (termPositions.back()[offset.back()] -
                termPositions.front()[offset.front()] + 1 <= dist)
            {
                result = true;
                break;
            }

            offset[0]++;
            if (offset[0] >= termPositions[0].length) {
                break;
            }
        }

        allPositions.clear();
        return result;
    }
};
