#pragma once

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
public:
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
        if (pos >= rec.length) return true;
        return false;
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
        while (!a->end() && !b->end() && a->get() != b->get()) {
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
        if (a->get() < b->get()) {
            a->next();
        } else {
            b->next();
        }

        while (a->get() != b->get() && !a->end() && !b->end()) {
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