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
		"1.t"
	},
	{
		"что && где",
		[](ostream &cout) {
			IndexIterator *it = new AndIterator(new SimpleIterator(3098799), new SimpleIterator(1369257));
			getAll(it, cout);
			delete it;
		},
		"2.t"
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
		"3.t"
	},
	{
		"\"москва слезам верит\" 4",
		[](ostream &cout) {
			IndexIterator *it = new QuoteIterator({2069902, 2720450, 1255913}, 4);
			getAll(it, cout);
			delete it;
		},
		"4.t"
	},
	{
		"\"москва слезам не верит\" 4",
		[](ostream &cout) {
			IndexIterator *it = new QuoteIterator({2069902, 2720450, 2125081, 1255913}, 4);
			getAll(it, cout);
			delete it;
		},
		"5.t"
	},
	{
		"\"что где когда\"",
		[](ostream &cout) {
			IndexIterator *it = new QuoteIterator({3098799, 1369257, 1799149}, 3);
			getAll(it, cout);
			delete it;
		},
		"6.t"
	},
	{
		"\"быть или не\"",
		[](ostream &cout) {
			IndexIterator *it = new QuoteIterator({1219117, 1668634, 2125081}, 4);
			getAll(it, cout);
			delete it;
		},
		"7.t"
	},
	{
		"\"быть или не быть\"",
		[](ostream &cout) {
			IndexIterator *it = new QuoteIterator({1219117, 1668634, 2125081, 1219117}, 4);
			getAll(it, cout);
			delete it;
		},
		"8.t"
	}
};
