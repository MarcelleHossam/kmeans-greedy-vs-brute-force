
# K-Means: Greedy vs Brute Force

> Comparative analysis of K-Means clustering strategies using C++17, with experimental evaluation of clustering quality, runtime, and computational complexity.

## 📌 Overview

This project investigates the trade-off between two approaches to K-Means clustering:

- **Greedy Iterative K-Means** using KMeans++ initialization and Lloyd's algorithm.
- **Budgeted Brute Force K-Means** using multiple candidate centroid initializations followed by Lloyd's algorithm.

The goal is to evaluate how much solution quality is gained by searching more candidate initializations, and how much additional computational cost that search requires.

The implementation uses controlled synthetic datasets and a shared random seed so that both strategies operate on identical data, making the comparison reproducible and fair.

---

## 🎯 Objectives

- Implement two different K-Means initialization strategies.
- Compare a practical greedy approach against a near-exhaustive baseline.
- Measure clustering quality using **Within-Cluster Sum of Squares (WCSS)**.
- Measure and compare runtime.
- Analyze the theoretical time and space complexity of both approaches.
- Ensure that the experimental comparison is reproducible and fair.
- Address correctness issues such as invalid inputs, empty clusters, integer overflow, and misleading algorithm labeling.

---

## 🧠 Algorithms

### 1. Greedy K-Means — KMeans++ + Lloyd's Algorithm

The primary practical approach uses **KMeans++** to initialize centroids followed by Lloyd's iterative assignment-update process.

The implementation performs multiple independent restarts and selects the best WCSS result.

General workflow:

```text
Generate Dataset
       ↓
KMeans++ Initialization
       ↓
Assign Points
       ↓
Recompute Centroids
       ↓
Reassign Points
       ↓
Converged?
   ↙       ↘
 No        Yes
 ↓          ↓
Repeat    Return Best
````

---

### 2. Budgeted Brute Force

The second strategy evaluates multiple possible centroid initializations.

When the complete number of possible combinations is within the specified budget, the program can enumerate the combinations. Otherwise, it samples a limited number of random `k`-subsets and runs Lloyd's algorithm for each candidate.

The candidate producing the lowest WCSS is selected.

```text
Generate Dataset
       ↓
Check C(n,k)
       ↓
Full Enumeration or
Budgeted Random Sampling
       ↓
Run Lloyd's Algorithm
       ↓
Compute WCSS
       ↓
Best WCSS?
   ↙       ↘
 No        Yes
           ↓
      Update Best
           ↓
      More Trials?
```

---

## 📊 Fair Experimental Comparison

Both approaches share the same:

* Generated dataset
* Random seed
* Distance calculation
* Cluster assignment
* Centroid recomputation
* WCSS calculation

A fixed seed of **7** is used across benchmark experiments so both algorithms operate on identical datasets.

This ensures that observed differences are primarily attributable to the algorithmic strategy rather than differences in the input data.

---

## 🛡️ Correctness Improvements

The implementation includes five documented correctness fixes.

### Fix 1 — Input Validation

The program enforces:

```text
k ≤ n
```

Invalid inputs are rejected before the algorithms are executed.

### Fix 2 — Shared Dataset Seed

Both algorithms operate on the same generated dataset using a shared random seed, ensuring a fair comparison.

### Fix 3 — Empty-Cluster Handling

The implementation handles empty clusters to prevent invalid clustering results and misleading WCSS measurements.

### Fix 4 — Overflow-Safe Combination Counting

The program safely determines whether the number of possible combinations can be evaluated within the available budget without causing integer overflow.

### Fix 5 — Honest Algorithm Labelling

The implementation distinguishes between full enumeration and budgeted sampling instead of describing sampled combinations as complete brute-force enumeration.

---

## ⏱️ Complexity Analysis

Let:

* `n` = number of data points
* `k` = number of clusters
* `i` = number of Lloyd's iterations
* `M` = number of budgeted trials

### Greedy K-Means

Time complexity:

```text
O(nki)
```

Space complexity:

```text
O(n + k)
```

### Budgeted Brute Force

Time complexity:

```text
O(M · nki)
```

Space complexity:

```text
O(n + k)
```

Full enumeration becomes exponential in the number of possible centroid combinations, while budgeted sampling limits the computational cost according to `M`.

### Summary

| Strategy                       | Time Complexity   | Space      | Optimality  |
| ------------------------------ | ----------------- | ---------- | ----------- |
| Greedy / Lloyd's               | `O(nki)`          | `O(n + k)` | Local       |
| KMeans++                       | `O(nk)`           | `O(k)`     | Approximate |
| Brute Force — Full Enumeration | `O(C(n,k) · nki)` | `O(n + k)` | Global      |
| Brute Force — Budgeted         | `O(M · nki)`      | `O(n + k)` | Near-Global |

---

## 🧪 Experimental Setup

The implementation is written in **C++17** and compiled using:

```bash
g++ -O2 -std=c++17
```

The benchmark datasets consist of synthetic 2D Gaussian clusters with:

* Standard deviation: `1.2`
* Cluster centers sampled from `[-10, 10]²`
* Shared random seed: `7`

Runtime is measured using:

```text
std::chrono::high_resolution_clock
```

### Dataset Configurations

| Dataset | Points (n) | Clusters (k) | Std. Dev. | BF Budget |
| ------- | ---------: | -----------: | --------: | --------: |
| Tiny    |         30 |            3 |       1.2 |       200 |
| Small   |        100 |            4 |       1.2 |       300 |
| Medium  |        300 |            5 |       1.2 |       500 |
| Large   |        800 |            6 |       1.2 |       500 |

---

## 📈 Results

The experimental evaluation found that the Greedy approach was substantially faster while maintaining a relatively small WCSS difference.

### Runtime

Greedy was measured to be approximately:

```text
193× – 484× faster
```

across the tested dataset scales.

### WCSS

The measured WCSS gaps were:

| Dataset | WCSS Gap |
| ------- | -------: |
| Tiny    |    0.00% |
| Small   |    3.73% |
| Medium  |  < 0.01% |
| Large   |    0.11% |

Lower WCSS indicates a better clustering objective value.

### Main Finding

The experiments demonstrate the practical trade-off:

```text
Greedy
↓
Much faster
↓
Small quality difference

Budgeted Brute Force
↓
Much higher computational cost
↓
Near-optimal baseline
```

The results show that the Greedy KMeans++ approach provides a strong practical balance between computational efficiency and clustering quality, while the budgeted brute-force strategy can serve as a useful near-optimal baseline for comparison.

---

## 🏗️ System Architecture

```text
Input Layer
    │
    ├── User Parameters
    ├── Dataset Generator
    └── Seed
    │
    ▼
Core Engine
    │
    ├── Input Validation
    └── Shared Seed Management
    │
    ▼
Algorithm Modules
    │
    ├── Greedy KMeans++
    └── Budgeted Brute Force
    │
    ▼
Shared Utilities
    │
    ├── Distance
    ├── WCSS
    ├── Assignment
    └── Centroid Recalculation
    │
    ▼
Output Layer
    │
    ├── Results
    ├── Tables
    └── Charts
```

---

## 🛠️ Technologies

* **C++17**
* **GNU g++**
* **K-Means**
* **KMeans++**
* **Lloyd's Algorithm**
* **Algorithm Analysis**
* **Complexity Analysis**
* **Benchmarking**
* **Synthetic Data Generation**
* **`std::chrono`**

---

## 📁 Repository Structure

```text
kmeans-greedy-vs-brute-force/
│
├── README.md
│
├── src/
│   └── kmeans_comparison.cpp
│
└── docs/
    └── kmeans-algorithm-analysis.pdf
```

---

## 📚 Documentation

The complete technical analysis is available in:

**[K-Means Algorithm Analysis](docs/kmeans-algorithm-analysis.pdf)**

The document contains the methodology, algorithm descriptions, correctness fixes, complexity analysis, experimental setup, results, limitations, and conclusions.

---

## ⚠️ Limitations

The experiments use synthetic 2D Gaussian datasets. Results may differ for high-dimensional, non-spherical, imbalanced, or real-world datasets.

Runtime measurements also depend on the hardware used, so the reported runtimes should primarily be interpreted as relative performance comparisons rather than absolute performance measurements.

---

## 🔮 Future Work

Potential extensions include:

* Testing high-dimensional datasets.
* Evaluating real-world datasets.
* Exploring adaptive budget strategies.
* Investigating additional K-Means initialization strategies.
* Extending benchmarks to larger dataset sizes.

---

## 👤 Project

**K-Means: Greedy vs Brute Force**

A comparative algorithm-analysis project exploring the trade-off between computational efficiency and clustering quality through practical implementation and empirical benchmarking.

````

### One important thing

I deliberately **removed the `results/` directory from the README**, since you decided not to include separate result files. Your repository should now be:

```text
kmeans-greedy-vs-brute-force/
├── README.md
├── src/
│   └── kmeans_comparison.cpp
└── docs/
    └── kmeans-algorithm-analysis.pdf
````

The README's technical claims above are based on your uploaded project paper, including the C++17 implementation, experimental setup, complexity analysis, and reported results.  
