#include <iostream>
#include <dirent.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <fstream>

using namespace std;


inline bool endsWith(const string &s, const string &suff) {
    int a = s.size() - 1;
    int b = suff.size() - 1;
    for (int i = 0; i < min(s.size(), suff.size()); i++) {
        if (s[a] != suff[b]) return false;
        a--; b--;
    }
    if (b > 0) return false;
    return true;
}


int main(int argc, char* argv[]) {
    if (argc < 3) return 0;

    map<string, unsigned int> tokens;
    string token;

    DIR *dirp;
    struct dirent *directory;

    for (int i = 2; i < argc; i++) {
        dirp = opendir(argv[i]);
        if (dirp) {
            while ((directory = readdir(dirp)) != NULL) {
                string file_path = string(argv[i]) + '/' + string(directory->d_name);

                if (endsWith(file_path, "/.") || 
                    endsWith(file_path, "/..") || 
                    endsWith(file_path, "/./") || 
                    endsWith(file_path, "/../")) continue;

                ifstream fin(file_path);
                
                while (fin >> token) {
                    if (!token.empty()) tokens[token]++;
                }

                fin.close();
            }

            cout << argv[i] << ": done" << endl;

            closedir(dirp);
        }
    }

    vector<pair<unsigned int, string>> v;
    v.reserve(tokens.size());

    for (auto &i : tokens) {
        v.emplace_back(i.second, i.first);
    }
    sort(v.begin(), v.end());

    ofstream fout(argv[1]);
    for (auto &i : v) {
        fout << i.second << endl;
    }
    fout.close();

    return 0;
}
