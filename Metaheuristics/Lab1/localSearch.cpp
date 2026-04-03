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

using namespace std;

// g++ -std=c++17 -O3 -fopenmp localSearch.cpp -o localSearch.exe

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
 * runs local search with invert neighborhood on the given solution and distance matrix
 * returns the cost of the final solution and the number of improvement steps taken
 * modifies the solution vector in place to represent the final tour
 */
pair<long long, int> localSearch(const vector<int>& distMatrix, vector<int>& solution){
    //1. Wyznacz wartość funkcji celu wszystkich sąsiadów rozwiązania aktualnego (dla otoczenia invert)
    //Jako kandydata do poprawy wybierz najlepszego z ocenionych sąsiadów.
    //jeśli kandydat nie jest lepszy od aktualnego rozwiązania, to zakończ algorytm
    //Zastąp aktualne rozwiązanie kandydatem i przejdź do kroku 1
    int noSteps = 0;
    long long currCost = calculateCost(distMatrix, solution);
    const int n = solution.size();

    for(;; noSteps++){
        long long bestDelta = 0;
        int newI = -1; int newJ = -1;

        for (int i = 0; i < n; i++) { // maybe get the first negative delta instead of the best one to speed up?
            int idx_prev_i = ((i - 1 + n) % n);
            int node_prev_i = solution[idx_prev_i];
            int node_i = solution[i];

            for (int j = i + 1; j < n; j++) {
                if (i == 0 && j == n - 1) continue; // full reversal = same tour
                int next_j = (j + 1) % n;
                int node_j = solution[j];
                int node_next_j = solution[next_j];

                long long delta = -distMatrix[node_prev_i * n + node_i] 
                                  - distMatrix[node_j * n + node_next_j]
                                  + distMatrix[node_prev_i * n + node_j] 
                                  + distMatrix[node_i * n + node_next_j];

                if (delta < bestDelta) {
                    bestDelta = delta;
                    newI = i; newJ = j;
                }
            }
        }

        if(bestDelta >= 0) break;
        currCost += bestDelta;
        reverse(solution.begin() + newI, solution.begin() + newJ + 1);
    }
    return {currCost,noSteps};
}


int main(){
    namespace fs = std::filesystem;

    for(const auto& entry : fs::directory_iterator("data/")){
        vector<pair<double, double>> coordinates;
        readFile(entry, coordinates);
        if(coordinates.empty()) continue;
        if(entry.path() == "data/ca4663.tsp") continue;
        int n = coordinates.size();

        vector<int> dist = createDistanceMatrix(coordinates);

        // Local Search
        vector<long long> costs;
        vector<int> steps;
        long long bestCost = LLONG_MAX;
        vector<int> bestPerm(n);

        #pragma omp parallel num_threads(8)
        {
            vector<long long> localCosts;
            vector<int> localSteps;
            long long localBestCost = LLONG_MAX;
            vector<int> localBestPerm(n);
            int no_runs = ceil(sqrt(n));
            if(entry.path() == "data/eg7146.tsp") no_runs = 4;
            if(entry.path() == "data/ei8246.tsp") no_runs = 4;
            if(entry.path() == "data/tz6117.tsp") no_runs = 8;

            //#pragma omp for nowait
            #pragma omp for schedule(dynamic) nowait
            for (int i = 0; i < no_runs; ++i) {
                auto perm = randomPermutation(n);
                auto [costR, stepR] = localSearch(dist, perm);
                localCosts.push_back(costR);
                localSteps.push_back(stepR);
                if (costR < localBestCost) {
                    localBestCost = costR;
                    localBestPerm = perm;
                }
                #pragma omp critical(logging)
                {
                    cout << "Run " << i+1 << "/" << no_runs << " done (Thread " << omp_get_thread_num() << ")\n";
                }
            }

            #pragma omp critical
            {
                costs.insert(costs.end(), localCosts.begin(), localCosts.end());
                steps.insert(steps.end(), localSteps.begin(), localSteps.end());
                if (localBestCost < bestCost) {
                    bestCost = localBestCost;
                    bestPerm = localBestPerm;
                }
            }
        }

        long long avgCost = reduce(costs.begin(), costs.end(), 0LL) / costs.size();
        double avgSteps = static_cast<double>(reduce(steps.begin(), steps.end(), 0)) / steps.size();

        cout << "File: " << entry.path().filename() << "\n";

        cout << "Local Search \n";
        cout << "Average Cost: " << avgCost << "\n";
        cout << "Average Steps: " << avgSteps << "\n";
        //for data file .sol
        string solFilename = "dataSol/" + entry.path().stem().string() + "localSearch.sol";
        ofstream solFile(solFilename);
        if(solFile.is_open()){
            solFile << bestCost << "\n";
            for(auto c : bestPerm) solFile << coordinates[c].first << " " << coordinates[c].second << "\n";
            solFile.close();
        }
        else cerr <<"Error opening file for writing: " << solFilename << "\n";
    }
    return 0;
}