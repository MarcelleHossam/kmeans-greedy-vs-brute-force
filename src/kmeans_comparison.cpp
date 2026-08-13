#include <iostream>
#include <vector>
#include <cmath>
#include <limits>
#include <algorithm>
#include <random>
#include <chrono>
#include <iomanip>
#include <numeric>
#include <cassert>
#include <string>
#include <sstream>
#include <set>

using namespace std;
using namespace std::chrono;

struct Point {
    double x, y;
    int cluster = -1;
    Point(double x = 0, double y = 0) : x(x), y(y) {}
};

struct ClusterResult {
    vector<Point> points;
    vector<Point> centroids;
    double        wcss = 0.0;
    long long     duration_us = 0;
    int           iterations = 0;
    string        algorithm;
    bool          fullEnumeration = false;
};

//  UTILITY
inline double distSq(const Point& a, const Point& b) {
    double dx = a.x - b.x, dy = a.y - b.y;
    return dx * dx + dy * dy;
}

double computeWCSS(const vector<Point>& pts, const vector<Point>& cen) {
    double w = 0;
    for (const auto& p : pts) {
        w += distSq(p, cen[p.cluster]);
    }
    return w;
}

bool assignClusters(vector<Point>& pts, const vector<Point>& cen) {
    bool changed = false;
    for (auto& p : pts) {
        int best = 0;
        double bd = distSq(p, cen[0]);
        for (size_t k = 1; k < cen.size(); ++k) {
            double d = distSq(p, cen[k]);
            if (d < bd) { bd = d; best = k; }
        }
        if (p.cluster != best) { p.cluster = best; changed = true; }
    }
    return changed;
}

vector<Point> recomputeCentroids(const vector<Point>& pts, int k,
    const vector<Point>& oldCentroids) {
    vector<double> sx(k, 0), sy(k, 0);
    vector<int> cnt(k, 0);
    for (const auto& p : pts) {
        sx[p.cluster] += p.x;
        sy[p.cluster] += p.y;
        cnt[p.cluster]++;
    }
    vector<Point> newCen(k);
    set<int> used;
    for (int i = 0; i < k; ++i) {
        if (cnt[i] > 0) {
            newCen[i] = { sx[i] / cnt[i], sy[i] / cnt[i] };
        }
        else {
            double maxDist = -1;
            int worst = 0;
            for (size_t j = 0; j < pts.size(); ++j) {
                if (used.count(j)) continue;
                double d = distSq(pts[j], oldCentroids[pts[j].cluster]);
                if (d > maxDist) { maxDist = d; worst = j; }
            }
            used.insert(worst);
            newCen[i] = pts[worst];
        }
    }
    return newCen;
}

vector<Point> generateDataset(int n, int trueK, unsigned seed) {
    mt19937 rng(seed);
    uniform_real_distribution<double> cd(-10.0, 10.0);
    normal_distribution<double> nd(0.0, 1.2);
    vector<Point> centers(trueK);
    for (auto& c : centers) c = { cd(rng), cd(rng) };
    vector<Point> data;
    data.reserve(n);
    uniform_int_distribution<int> cp(0, trueK - 1);
    for (int i = 0; i < n; ++i) {
        int ci = cp(rng);
        data.push_back({ centers[ci].x + nd(rng), centers[ci].y + nd(rng) });
    }
    return data;
}

//  GREEDY (KMeans++ + Lloyd)
vector<Point> kmeansppInit(const vector<Point>& pts, int k, mt19937& rng) {
    uniform_int_distribution<int> pick(0, pts.size() - 1);
    vector<Point> cen;
    cen.push_back(pts[pick(rng)]);
    for (int c = 1; c < k; ++c) {
        vector<double> dists(pts.size());
        double total = 0;
        for (size_t i = 0; i < pts.size(); ++i) {
            double minD = numeric_limits<double>::max();
            for (const auto& cc : cen)
                minD = min(minD, distSq(pts[i], cc));
            dists[i] = minD;
            total += minD;
        }
        uniform_real_distribution<double> wheel(0, total);
        double r = wheel(rng), cum = 0;
        for (size_t i = 0; i < pts.size(); ++i) {
            cum += dists[i];
            if (cum >= r) { cen.push_back(pts[i]); break; }
        }
        if ((int)cen.size() < c + 1) cen.push_back(pts.back());
    }
    return cen;
}

ClusterResult greedyKMeans(vector<Point> pts, int k, int maxIter = 300, unsigned seed = 42) {
    mt19937 rng(seed);
    auto t0 = high_resolution_clock::now();
    vector<Point> cen = kmeansppInit(pts, k, rng);
    assignClusters(pts, cen);
    int iter = 0;
    for (; iter < maxIter; ++iter) {
        vector<Point> newCen = recomputeCentroids(pts, k, cen);
        if (!assignClusters(pts, newCen)) break;
        cen = move(newCen);
    }
    auto t1 = high_resolution_clock::now();
    return { pts, cen, computeWCSS(pts, cen),
            duration_cast<microseconds>(t1 - t0).count(), iter,
            "Greedy (Lloyd's + KMeans++)" };
}

//  BRUTE FORCE with budget
ClusterResult lloydFromCentroids(const vector<Point>& pts, vector<Point> cen, int k, int maxIter = 100) {
    vector<Point> p = pts;
    assignClusters(p, cen);
    int iter = 0;
    for (; iter < maxIter; ++iter) {
        vector<Point> newCen = recomputeCentroids(p, k, cen);
        if (!assignClusters(p, newCen)) break;
        cen = move(newCen);
    }
    return { p, cen, computeWCSS(p, cen), 0, iter, "" };
}

bool combosWithinLimit(int n, int k, long long limit, long long& out) {
    if (k > n) { out = 0; return false; }
    if (k == 0 || k == n) { out = 1; return true; }
    long long comb = 1;
    for (int i = 1; i <= k; ++i) {
        if (comb > limit / (n - i + 1)) return false;
        comb = comb * (n - i + 1) / i;
        if (comb > limit) return false;
    }
    out = comb;
    return true;
}

ClusterResult bruteForceKMeans(vector<Point> pts, int k, int maxCombinations = 500, unsigned seed = 42) {
    int n = pts.size();
    auto t0 = high_resolution_clock::now();
    vector<int> idx(n);
    iota(idx.begin(), idx.end(), 0);
    double bestW = numeric_limits<double>::max();
    ClusterResult best;
    int combos = 0;
    bool fullEnum = false;
    long long totalCombos = 0;
    bool within = combosWithinLimit(n, k, maxCombinations, totalCombos);

    if (within && totalCombos <= maxCombinations) {
        fullEnum = true;
        vector<int> sel(k);
        iota(sel.begin(), sel.end(), 0);
        auto nextCombo = [&]() -> bool {
            int i = k - 1;
            while (i >= 0 && sel[i] == n - k + i) --i;
            if (i < 0) return false;
            ++sel[i];
            for (int j = i + 1; j < k; ++j) sel[j] = sel[j - 1] + 1;
            return true;
            };
        do {
            vector<Point> ic(k);
            for (int i = 0; i < k; ++i) ic[i] = pts[sel[i]];
            auto r = lloydFromCentroids(pts, ic, k);
            if (r.wcss < bestW) { bestW = r.wcss; best = r; }
            ++combos;
        } while (nextCombo());
    }
    else {
        mt19937 rng(seed);
        for (int t = 0; t < maxCombinations; ++t) {
            shuffle(idx.begin(), idx.end(), rng);
            vector<Point> ic(k);
            for (int i = 0; i < k; ++i) ic[i] = pts[idx[i]];
            auto r = lloydFromCentroids(pts, ic, k);
            if (r.wcss < bestW) { bestW = r.wcss; best = r; }
            ++combos;
        }
    }
    auto t1 = high_resolution_clock::now();
    best.duration_us = duration_cast<microseconds>(t1 - t0).count();
    best.fullEnumeration = fullEnum;
    best.algorithm = fullEnum ? "BF (full enumeration)" : "BF (budgeted, " + to_string(combos) + " trials)";
    return best;
}

//  BULLETPROOF SUMMARY TABLE (no broken borders)
void printSummary(const ClusterResult& bf, const ClusterResult& gr) {
    double diff = (gr.wcss - bf.wcss) / bf.wcss * 100.0;
    double speedup = (double)bf.duration_us / max(1LL, gr.duration_us);

    cout << fixed << setprecision(2);
    cout << "\n  ==================================================\n";
    cout << "    Brute Force WCSS        : " << setw(12) << bf.wcss << "\n";
    cout << "    Greedy WCSS             : " << setw(12) << gr.wcss << "\n";
    cout << "    --------------------------------------------------\n";
    cout << "    Brute Force Time (us)   : " << setw(12) << bf.duration_us << "\n";
    cout << "    Greedy Time (us)        : " << setw(12) << gr.duration_us << "\n";
    cout << "    --------------------------------------------------\n";
    cout << "    Brute Force Iterations  : " << setw(12) << bf.iterations << "\n";
    cout << "    Greedy Iterations       : " << setw(12) << gr.iterations << "\n";
    cout << "    --------------------------------------------------\n";
    cout << "    Greedy WCSS worse by    : " << setw(10) << setprecision(2) << diff << " %\n";
    cout << "    Greedy is faster by     : " << setw(10) << setprecision(1) << speedup << " x\n";
    cout << "    BF mode                 : " << bf.algorithm << "\n";
    cout << "  ==================================================\n";
    cout << resetiosflags(ios::fixed) << defaultfloat;
}

//  CUSTOM EXPERIMENT
void runCustom() {
    int n = 0, k = 0;
    cout << "\n  Number of points (10-2000): ";
    cin >> n;
    if (n < 10 || n>2000) { cout << "  Invalid.\n"; return; }
    cout << "  Number of clusters (2-10): ";
    cin >> k;
    if (k < 2 || k>10 || k > n) { cout << "  Invalid.\n"; return; }
    int budget = (n <= 50) ? 200 : (n <= 200) ? 300 : 500;
    unsigned seed;
    cout << "  Random seed (1-9999): ";
    cin >> seed;
    if (seed < 1 || seed>9999) seed = 42;

    auto data = generateDataset(n, k, seed);
    cout << "\n  Running Brute Force (budget=" << budget << ") ...\n";
    auto bf = bruteForceKMeans(data, k, budget, seed);
    cout << "  Running Greedy (5 restarts) ...\n";
    ClusterResult bestGr;
    bestGr.wcss = numeric_limits<double>::max();
    long long totalTime = 0;
    for (int r = 0; r < 5; ++r) {
        auto g = greedyKMeans(data, k, 300, seed + r * 17);
        totalTime += g.duration_us;
        if (g.wcss < bestGr.wcss) bestGr = g;
    }
    bestGr.duration_us = totalTime / 5;
    printSummary(bf, bestGr);
}

//  BENCHMARK
void runBenchmark() {
    cout << "\n  === BENCHMARK (seed=7) ===\n";
    struct Test { int n, k, budget; string name; };
    vector<Test> tests = { {30,3,200,"Tiny"},{100,4,300,"Small"},{300,5,500,"Medium"},{800,6,500,"Large"} };
    for (auto& t : tests) {
        cout << "\n  -- " << t.name << " (n=" << t.n << ", k=" << t.k << ") --\n";
        auto data = generateDataset(t.n, t.k, 7);
        auto bf = bruteForceKMeans(data, t.k, t.budget, 7);
        ClusterResult bestGr;
        bestGr.wcss = numeric_limits<double>::max();
        long long tt = 0;
        for (int r = 0; r < 5; ++r) {
            auto g = greedyKMeans(data, t.k, 300, 7 + r * 13);
            tt += g.duration_us;
            if (g.wcss < bestGr.wcss) bestGr = g;
        }
        bestGr.duration_us = tt / 5;
        printSummary(bf, bestGr);
    }
}

void printComplexity() {
    cout << "\n  === COMPLEXITY ===\n";
    cout << "  Greedy       : O(n*k*i) time, O(n+k) space\n";
    cout << "  Brute force  : O(M*n*k*i) time, O(n+k) space\n";
    cout << "  (M = budget, i = iterations)\n";
}

int main() {
    cout << "\n=== K-MEANS: GREEDY vs BRUTE FORCE ===\n";
    while (true) {
        cout << "\n  1. Custom experiment\n  2. Run benchmark\n  3. Complexity\n  0. Exit\n  Choice: ";
        int ch; cin >> ch;
        if (ch == 0) break;
        if (ch == 1) runCustom();
        else if (ch == 2) runBenchmark();
        else if (ch == 3) printComplexity();
        else cout << "  Invalid.\n";
    }
    return 0;
}