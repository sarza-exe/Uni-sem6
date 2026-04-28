#pragma once

#include <iostream>
#include <vector>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <random>
#include <chrono>
#include <algorithm>
#include <climits>
#include <omp.h>
#include <atomic>
#include <functional>

using namespace std;


/**
 * param entry is filesystem directory entry of a single .tsp file
 * param coordinates is a reference to a vector of pairs of doubles
 * readFile save locations of cities from entry to coordinates
 */
void readFile(const std::filesystem::directory_entry& entry, vector<pair<double, double>>& coordinates){
    cout << entry << "\n";
    if(entry.path().extension() == ".tsp"){
        ifstream file(entry.path());
        if(file.is_open()){
            string line;
            while(getline(file, line)){ // Skip headers
                if(line.find("NODE_COORD_SECTION") != string::npos) break;
            }

            int id;
            double x, y;
            while(file >> id >> x >> y){
                coordinates.push_back({x,y});
            }
            file.close();
        }
    }
}

/**
 * the cost of travel between cities is specified by the Euclidean distance rounded to the nearest whole number
 */
int EUC_2Dnorm(const pair<double, double>& a, const pair<double, double>& b){
    return static_cast<int>(std::round(std::hypot(a.first - b.first, a.second - b.second)));
}

/**
 * calculates dist matrix that holds distances between all cities in vector coordinates
 * stored as a flat matrix (row-major) of size n*n
 */
vector<int> createDistanceMatrix(vector<pair<double, double>>& coordinates){
    int n = coordinates.size();
    vector<int> dist(n * n);
    for(int i = 0; i < n; i++){
        for(int j = 0; j < n; j++){
            dist[i * n + j] = EUC_2Dnorm(coordinates[i], coordinates[j]);
        }
    }
    return dist;
}

/**
 * returns random permutation of size n
 */
vector<int> randomPermutation(int n){
    vector<int> perm(n);
    for(int i = 0; i < n; i++) perm[i] = i;
    random_device rd;
    mt19937 g(rd());
    shuffle(perm.begin(), perm.end(), g);
    return perm;
}

/**
 * calculates cost of a tour given by perm using distMatrix (flat, row-major n*n)
 */
long long calculateCost(const vector<int>& distMatrix, const vector<int>& perm){
    long long cost = 0;
    const int n = perm.size();
    for(int i = 0; i < n-1; i++)
        cost += distMatrix[perm[i] * n + perm[i+1]];
    cost += distMatrix[perm[n-1] * n + perm[0]];
    return cost;
}

/**
 * @brief Estimates an initial temperature for simulated annealing based on average positive 2-opt move deltas.
 * 
 * @param distMatrix The flat (row-major) distance matrix of the TSP instance.
 * @param samples The number of random 2-opt moves to sample (default: 1000).
 * @return The average positive cost difference (delta) from sampled 2-opt moves.
 */
double getInitialTemperature(const vector<int>& distMatrix, int samples = 1000){
    long long totalDelta = 0;
    int no_samples = 0;
    const int n = static_cast<int>(std::sqrt(distMatrix.size()));
    vector<int> solution = randomPermutation(n);

    mt19937 mt{};
    uniform_int_distribution randN{0, n-1};

    for(int k = 0; k < samples; k++){
        int i = randN(mt);
        int j = randN(mt);
        if(i > j) swap(i,j);
        if(i == j || (i == 0 && j == n - 1)) continue;

        int idx_prev_i = ((i - 1 + n) % n);
        int node_prev_i = solution[idx_prev_i];
        int node_i = solution[i];
        int next_j = (j + 1) % n;
        int node_j = solution[j];
        int node_next_j = solution[next_j];

        long long delta = -distMatrix[node_prev_i * n + node_i] 
                            - distMatrix[node_j * n + node_next_j]
                            + distMatrix[node_prev_i * n + node_j] 
                            + distMatrix[node_i * n + node_next_j];            

        std::reverse(solution.begin() + i, solution.begin() + j + 1);
    
        if(delta > 0){
            totalDelta += delta;
            no_samples++;
        }  
    }
    if(no_samples == 0) return 100.0; 
    return totalDelta / (double)no_samples;
}