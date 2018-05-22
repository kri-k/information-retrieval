#pragma once

#include <functional>
#include <iostream>
#include <vector>
#include <string>
#include "../app/engine/index_iterator.h"

using namespace std;

struct TestItem {
    string testName;
    function<void(ostream&)> f;
    string sampleFileName;
};


void getAll(IndexIterator *it, ostream &cout) {
    while (!it->end()) {
        cout << it->get() << endl;
        it->next();
    }
}


vector<TestItem> tests = {
    {
        "москва",
        [](ostream &cout) {
            IndexIterator *it = new SimpleIterator(2069902);
            getAll(it, cout);
            delete it;
        },
        "1.test"
    },
    {
        "что && где",
        [](ostream &cout) {
            IndexIterator *it = new AndIterator(new SimpleIterator(3098799), new SimpleIterator(1369257));
            getAll(it, cout);
            delete it;
        },
        "2.test"
    },
    {
        "что && где || что && когда",
        [](ostream &cout) {
            IndexIterator *a = new AndIterator(new SimpleIterator(3098799), new SimpleIterator(1369257));
            IndexIterator *b = new AndIterator(new SimpleIterator(3098799), new SimpleIterator(1799149));
            IndexIterator *it = new OrIterator(a, b);
            getAll(it, cout);
            delete it;
        },
        "3.test"
    },
    {
        "\"москва слезам верит\" 4",
        [](ostream &cout) {
            IndexIterator *it = new QuoteIterator({2069902, 2720450, 1255913}, 4);
            getAll(it, cout);
            delete it;
        },
        "4.test"
    },
    {
        "\"москва слезам не верит\" 4",
        [](ostream &cout) {
            IndexIterator *it = new QuoteIterator({2069902, 2720450, 2125081, 1255913}, 4);
            getAll(it, cout);
            delete it;
        },
        "5.test"
    },
    {
        "\"что где когда\"",
        [](ostream &cout) {
            IndexIterator *it = new QuoteIterator({3098799, 1369257, 1799149}, 3);
            getAll(it, cout);
            delete it;
        },
        "6.test"
    },
    {
        "\"быть или не\"",
        [](ostream &cout) {
            IndexIterator *it = new QuoteIterator({1219117, 1668634, 2125081}, 4);
            getAll(it, cout);
            delete it;
        },
        "7.test"
    },
    {
        "\"быть или не быть\"",
        [](ostream &cout) {
            IndexIterator *it = new QuoteIterator({1219117, 1668634, 2125081, 1219117}, 4);
            getAll(it, cout);
            delete it;
        },
        "8.test"
    },
    {
        "слезам",
        [](ostream &cout) {
            IndexIterator *it = new SimpleIterator(2720450);
            getAll(it, cout);
            delete it;
        },
        "9.test"
    },
    {
        "не",
        [](ostream &cout) {
            IndexIterator *it = new SimpleIterator(2125081);
            getAll(it, cout);
            delete it;
        },
        "10.test"
    },
    {
        "верит",
        [](ostream &cout) {
            IndexIterator *it = new SimpleIterator(1255913);
            getAll(it, cout);
            delete it;
        },
        "11.test"
    },
    {
        "москва && слезам",
        [](ostream &cout) {
            IndexIterator *it = new AndIterator(new SimpleIterator(2069902), new SimpleIterator(2720450));
            getAll(it, cout);
            delete it;
        },
        "12.test"
    },
    {
        "москва && не",
        [](ostream &cout) {
            IndexIterator *it = new AndIterator(new SimpleIterator(2069902), new SimpleIterator(2125081));
            getAll(it, cout);
            delete it;
        },
        "13.test"
    },
    {
        "слезам && не",
        [](ostream &cout) {
            IndexIterator *it = new AndIterator(new SimpleIterator(2720450), new SimpleIterator(2125081));
            getAll(it, cout);
            delete it;
        },
        "14.test"
    },
    {
        "москва && слезам && не",
        [](ostream &cout) {
            IndexIterator *a = new AndIterator(new SimpleIterator(2069902), new SimpleIterator(2720450));
            IndexIterator *it = new AndIterator(a, new SimpleIterator(2125081));
            getAll(it, cout);
            delete it;
        },
        "15.test"
    },
    {
        "москва && слезам && верит",
        [](ostream &cout) {
            IndexIterator *a = new AndIterator(new SimpleIterator(2069902), new SimpleIterator(2720450));
            IndexIterator *it = new AndIterator(a, new SimpleIterator(1255913));
            getAll(it, cout);
            delete it;
        },
        "16.test"
    },
    {
        "москва && слезам && не && верит",
        [](ostream &cout) {
            IndexIterator *a = new AndIterator(new SimpleIterator(2069902), new SimpleIterator(2720450));
            IndexIterator *b = new AndIterator(new SimpleIterator(2125081), new SimpleIterator(1255913));
            IndexIterator *it = new AndIterator(a, b);
            getAll(it, cout);
            delete it;
        },
        "17.test"
    },
    {
        "москва && москва",
        [](ostream &cout) {
            IndexIterator *it = new AndIterator(new SimpleIterator(2069902), new SimpleIterator(2069902));
            getAll(it, cout);
            delete it;
        },
        "18.test"
    },
    {
        "(! \"москва слезам не верит\" 4 || (где || когда)) && что",
        [](ostream &cout) {
            IndexIterator *a = new NotIterator(new QuoteIterator({2069902, 2720450, 2125081, 1255913}, 4));
            IndexIterator *b = new OrIterator(new SimpleIterator(1369257), new SimpleIterator(1799149));
            IndexIterator *it = new OrIterator(a, b);
            it = new AndIterator(it, new SimpleIterator(3098799));
            getAll(it, cout);
            delete it;
        },
        "19.test"
    },
    {
        "(быть && не) && (или && что) && (где && когда)",
        [](ostream &cout) {
            IndexIterator *a = new AndIterator(new SimpleIterator(1219117), new SimpleIterator(2125081));
            IndexIterator *b = new AndIterator(new SimpleIterator(1668634), new SimpleIterator(3098799));
            IndexIterator *c = new AndIterator(new SimpleIterator(1369257), new SimpleIterator(1799149));
            IndexIterator *it = new AndIterator(a, b);
            it = new AndIterator(it, c);
            getAll(it, cout);
            delete it;
        },
        "20.test"
    },
    {
        "((((быть && не) && или) && что) && где) && когда",
        [](ostream &cout) {
            IndexIterator *it = new AndIterator(new SimpleIterator(1219117), new SimpleIterator(2125081));
            it = new AndIterator(it, new SimpleIterator(1668634));
            it = new AndIterator(it, new SimpleIterator(3098799));
            it = new AndIterator(it, new SimpleIterator(1369257));
            it = new AndIterator(it, new SimpleIterator(1799149));
            getAll(it, cout);
            delete it;
        },
        "21.test"
    },
    {
        "и && медведь",
        [](ostream &cout) {
            IndexIterator *it = new AndIterator(new SimpleIterator(1643361), new SimpleIterator(2003227));
            getAll(it, cout);
            delete it;
        },
        "22.test"
    },
    {
        "википедия && аквамариновый",
        [](ostream &cout) {
            IndexIterator *it = new AndIterator(new SimpleIterator(1271737), new SimpleIterator(1011528));
            getAll(it, cout);
            delete it;
        },
        "23.test"
    },
    {
        "аквамариновый && медведь",
        [](ostream &cout) {
            IndexIterator *it = new AndIterator(new SimpleIterator(1011528), new SimpleIterator(2003227));
            getAll(it, cout);
            delete it;
        },
        "24.test"
    },
    {
        "(аквамариновый && медведь) && википедия",
        [](ostream &cout) {
            IndexIterator *it = new AndIterator(new SimpleIterator(1011528), new SimpleIterator(2003227));
            it = new AndIterator(it, new SimpleIterator(1271737));
            getAll(it, cout);
            delete it;
        },
        "25.test"
    },
    {
        "аквамариновый && (медведь && википедия)",
        [](ostream &cout) {
            IndexIterator *it = new AndIterator(new SimpleIterator(2003227), new SimpleIterator(1271737));
            it = new AndIterator(it, new SimpleIterator(1011528));
            getAll(it, cout);
            delete it;
        },
        "26.test"
    },
    {
        "(и && в) && (а && на)",
        [](ostream &cout) {
            IndexIterator *a = new AndIterator(new SimpleIterator(1643361), new SimpleIterator(1222741));
            IndexIterator *b = new AndIterator(new SimpleIterator(978768), new SimpleIterator(2092054));
            IndexIterator *it = new AndIterator(a, b);
            getAll(it, cout);
            delete it;
        },
        "27.test"
    },
    {
        "википедия && чеитина",
        [](ostream &cout) {
            IndexIterator *it = new AndIterator(new SimpleIterator(1271737), new SimpleIterator(3082065));
            getAll(it, cout);
            delete it;
        },
        "28.test"
    },
    {
        "википедия && медведь",
        [](ostream &cout) {
            IndexIterator *it = new AndIterator(new SimpleIterator(1271737), new SimpleIterator(2003227));
            getAll(it, cout);
            delete it;
        },
        "29.test"
    },
    {
        "википедия && и",
        [](ostream &cout) {
            IndexIterator *it = new AndIterator(new SimpleIterator(1271737), new SimpleIterator(1643361));
            getAll(it, cout);
            delete it;
        },
        "30.test"
    }
};
