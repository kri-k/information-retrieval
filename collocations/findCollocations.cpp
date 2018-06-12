#include <bits/stdc++.h>
#include <experimental/filesystem>

using namespace std;

#define forn(i, n) for (int i = 0; i < n; ++i)
#define all(x) (x).begin(), (x).end()

#define mp(a, b) make_pair(a, b)

using ll = long long;
using TCollocation = pair<string, string>;

namespace fs = std::experimental::filesystem;


void doTtest(ll total, ll docs, map<TCollocation, ll> &both, map<string, ll> &single, vector<pair<double, TCollocation>> &result) {
    for (auto &i : both) {
        double mean_1 = (static_cast<double>(single[i.first.first]) / total) * (static_cast<double>(single[i.first.second]) / total);
        double mean_2 = static_cast<double>(i.second) / (total - docs);
        double S = mean_2 * (1.0 - mean_2);
        double t = (mean_2 - mean_1) / sqrt(S / (total - docs));
        if (t > 3.291) {
            result.emplace_back(-t, i.first);
        }
    }
}


void doChiTest(
    ll total,
    ll docs,
    map<TCollocation, ll> &both,
    map<string, ll> &firstWord,
    map<string, ll> &secondWord,
    vector<pair<double, TCollocation>> &result)
{
    for (auto &i : both) {
        double a11, a12, a21, a22;
        a11 = i.second;
        a12 = firstWord[i.first.first] - a11;
        a21 = secondWord[i.first.second] - a11;
        a22 = total - docs - a12 - a21 + a11;
        double x = (total - docs) * pow(a11 * a22 - a12 * a21, 2.0);
        x /= (a11 + a12) * (a11 + a21) * (a12 + a22) * (a21 + a22);
        if (x > 10.83) {
            if (a11 > 50 && a12 > 20 && a21 > 20)
                result.emplace_back(-x, i.first);
        }
    }
}


int main(int argc, char *argv[]) {
    ios_base::sync_with_stdio(false);
    cin.tie(0);

    ll docs = 0;
    ll total = 0;
    map<TCollocation, ll> both;
    map<string, ll> single;
    map<string, ll> firstWord;
    map<string, ll> secondWord;

    if (argc < 2) return 1;

    for (int i = 1; i < argc; i++) {
        cerr << "Processing '" << argv[i] << "'..." << endl;
        for (auto& p: fs::recursive_directory_iterator(argv[i])) {
            if (fs::is_directory(p)) continue;

            docs++;

            ifstream fin(p.path());
            
            string a;
            string b;
            fin >> a;
            single[a]++;
            total++;

            while (fin >> b) {
                total++;
                both[mp(a, b)]++;
                firstWord[a]++;
                secondWord[b]++;
                single[b]++;
                a = b;
            }

            fin.close();
        }
    }

    vector<pair<double, TCollocation>> result;

    doTtest(total, docs, both, single, result);
    sort(all(result));
    
    cout << "Number of processed documents: " << docs << endl;
    cout << "Total bigrams: " << total - docs << endl << endl;

    cout << "---- T test ----" << endl;
    cout << "Accepted: " << result.size() << endl;
    cout << "TOP 150 collocations:\n" << endl;
    forn(i, min(150, (int)result.size())) {
        // cout << result[i].second.first << ' ' << result[i].second.second << '\t' << -result[i].first << endl;
        cout << result[i].second.first << ' ' << result[i].second.second << endl;
    }

    result.clear();

    doChiTest(total, docs, both, firstWord, secondWord, result);
    sort(all(result));

    cout << "\n---- Chi test ----" << endl;
    cout << "Accepted: " << result.size() << endl;
    cout << "TOP 150 collocations:\n" << endl;
    forn(i, min(150, (int)result.size())) {
        // cout << result[i].second.first << ' ' << result[i].second.second << '\t' << -result[i].first << endl;
        cout << result[i].second.first << ' ' << result[i].second.second << endl;
    }

    return 0;
}
