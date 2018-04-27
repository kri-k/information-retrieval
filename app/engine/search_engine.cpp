#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <cassert>
#include "index_iterator.h"


using namespace std;


const char* REQUEST_PIPE = "../pipe_A";
const char* RESPONSE_PIPE = "../pipe_B";

const char NEW_REQ = 0;
const char EXIST_REQ = 1;

const char OK = 0; 
const char OK_PAYLOAD = 1;
const char BAD = 2;

const int RESPONSE_BLOCK_SIZE = 50;


char *BUFFER = new char[1000000];
vector<IndexIterator*> requests;


void clearVector(vector<IndexIterator*> &v) {
    for (int i = 0; i < v.size(); i++) {
        delete v[i];
    }
    v.clear();
}


IndexIterator* getQuoteIterator(stringstream &ss) {
    string s;
    vector<TID> terms;
    unsigned int dist = 0;
    while (ss >> s) {
        if (s == "\"") {
            break;
        }

        if (s[0] == '/') {
            dist = stoi(s.substr(1));
        } else {
            terms.push_back(stoi(s));
        }
    }
    assert(terms.size() > 0);
    if (terms.size() == 1) {
        return new SimpleIterator(terms[0]);
    }
    if (dist < terms.size()) dist = terms.size();
    return new QuoteIterator(terms, dist);
}


IndexIterator* getIterator(const string &expr) {
    vector<IndexIterator*> stack;
    stringstream ss(expr);
    string s;
    while (ss >> s) {
        if (s == "!") {
            if (stack.empty()) {
                clearVector(stack);
                return nullptr;
            }
            stack[stack.size() - 1] = new NotIterator(stack[stack.size() - 1]);
        } else if (s == "&") {
            if (stack.size() < 2) {
                clearVector(stack);
                return nullptr;
            }

            IndexIterator* a = stack.back();
            stack.pop_back();

            IndexIterator* b = stack.back();
            stack.pop_back();

            stack.push_back(new AndIterator(a, b));
        } else if (s == "|") {
            if (stack.size() < 2) {
                clearVector(stack);
                return nullptr;
            }
            
            IndexIterator* a = stack.back();
            stack.pop_back();
            
            IndexIterator* b = stack.back();
            stack.pop_back();

            stack.push_back(new OrIterator(a, b));
        } else if (s == "\"") {
            stack.push_back(getQuoteIterator(ss));
        } else {
            stack.push_back(new SimpleIterator(stoi(s)));
        }
    }
    if (stack.size() != 1) {
        clearVector(stack);
        return nullptr;
    }
    return stack[0];
}


void sendNextDocId(IndexIterator *iter) {
    vector<TID> v;

    while (!iter->end() && v.size() < RESPONSE_BLOCK_SIZE) {
        v.push_back(iter->get());
        iter->next();
    } 

    unsigned int n = v.size();

    ofstream fout(RESPONSE_PIPE, ios::binary);

    fout.write((char*)&OK_PAYLOAD, sizeof(char));
    fout.write((char*)&n, sizeof(unsigned int));
    for (TID id : v) {
        cout << id << ' ';
        fout.write((char*)&id, sizeof(TID));
    }
    cout << endl;

    fout.close();
}


int main() {
    cout << "Started listening to pipe..." << endl;

    while (true) {
        ifstream fin(REQUEST_PIPE, ios::binary);

        char cmd;
        fin.read(&cmd, sizeof(char));

        if (cmd == NEW_REQ) {
            cout << "New request" << endl;

            unsigned int length;
            fin.read((char*)&length, sizeof(unsigned int));

            fin.read(BUFFER, sizeof(char) * length);
            BUFFER[length] = '\0';

            fin.close();

            string expr = string(BUFFER);

            cout << "LEN = " << length << ", STR = " << expr << endl;

            IndexIterator *iter = getIterator(expr);

            ofstream fout(RESPONSE_PIPE, ios::binary);

            if (iter == nullptr) {
                fout.write((char*)&BAD, sizeof(char));
            } else {
                fout.write((char*)&OK, sizeof(char));
            }

            fout.close();

            requests.push_back(iter);

            cout << "ID = " << requests.size() - 1 << endl;

        } else if (cmd == EXIST_REQ) {
            cout << "Exist request" << endl;

            unsigned int id;
            fin.read((char*)&id, sizeof(unsigned int));

            cout << "ID = " << id << endl;

            fin.close();

            if (id < requests.size()) {
                sendNextDocId(requests[id]);
            } else {
                ofstream fout(RESPONSE_PIPE, ios::binary);
                fout.write((char*)&BAD, sizeof(char));
                fout.close();
            }
        } else {
            cerr << "Get bad command '" << cmd << "' with code " << ((int)cmd) << endl;
            fin.close();

            ofstream fout(RESPONSE_PIPE, ios::binary);
            fout.write((char*)&BAD, sizeof(char));
            fout.close();
        }
    }

    return 0;
}