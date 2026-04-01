#include <iostream>
#include <vector>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <random>
#include <chrono>
#include <algorithm>
#include <climits>

using namespace std;
// compile with g++ -std=c++17 -O3 localSearch.cpp -o localSearch

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
 */
vector<vector<int>> createDistanceMatrix(vector<pair<double, double>>& coordinates){
    int n = coordinates.size();
    vector<vector<int>> dist(n, vector<int>(n));
    for(int i = 0; i < n; i++){
        for(int j = 0; j < n; j++){
            dist[i][j] = EUC_2Dnorm(coordinates[i], coordinates[j]);
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

long long calculateCost(const vector<vector<int>>& distMatrix, const vector<int>& perm){
    long long cost = 0;
    const int n = perm.size();
    for(int i = 0; i < n-1; i++)
        cost += distMatrix[perm[i]][perm[i+1]];
    cost += distMatrix[perm[n-1]][perm[0]];
    return cost;
}

long long getInvertDelta(const vector<vector<int>>& dist, const vector<int>& perm, int i, int j) {
    int n = perm.size();
    if (i == 0 && j == n - 1) return 0; // full reversal = same tour

    int prev_i = (i - 1 + n) % n;
    int next_j = (j + 1) % n;

    return -dist[perm[prev_i]][perm[i]] - dist[perm[j]][perm[next_j]]
           + dist[perm[prev_i]][perm[j]] + dist[perm[i]][perm[next_j]];
}

// local search przyjmuje początkowy cykl (losowy albo z mst)
// zwraca koszt uzyskanego rozwiązania i liczbę kroków poprawy a także rozwiązanie (permutacje)
// solution to roziwiązanie początkowe a po wykonaniu algorytmu - końcowe
pair<long long, int> localSearch(const vector<vector<int>>& distMatrix, vector<int>& solution){
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
        for(int i = 0; i < n; i++){
            for(int j = i+1; j < n; j++){
                long long delta = getInvertDelta(distMatrix, solution, i, j);
                if(delta < bestDelta) {
                    bestDelta = delta;
                    newI = i; newJ = j;
                }
            }
        }
        if(bestDelta >= 0) break;
        if(newI < 0) break; // safety guard
        currCost += bestDelta;
        cout << bestDelta << " at cost " << currCost << "\n";
        reverse(solution.begin() + newI, solution.begin() + newJ + 1);
        if(noSteps > n * 10) break; // safety stop in case
    }
    return {currCost,noSteps};
}




int main(){
    namespace fs = std::filesystem;

    for(const auto& entry : fs::directory_iterator("data/")){
        vector<pair<double, double>> coordinates;
        readFile(entry, coordinates);
        if(coordinates.empty()) continue;
        if(entry.path() != "data/dj38.tsp") continue;
        int n = coordinates.size();

        vector<vector<int>> dist = createDistanceMatrix(coordinates);

        auto perm = randomPermutation(n);
        auto [costF, stepF] = localSearch(dist, perm);
        cout<<"best new cost is "<<costF<<" after " << stepF << "steps\n";
        
        // // For full local search
        // vector<long long> costsFull;
        // vector<int> stepsFull;
        // long long bestCostFull = LLONG_MAX;
        
        // // For random local search
        // vector<long long> costsRandom;
        // vector<int> stepsRandom;
        // long long bestCostRandom = LLONG_MAX;
        
        // for(int run = 0; run < n; ++run){
        //     auto perm = randomPermutation(n);
            
        //     // // Full
        //     // auto [costF, stepF] = localSearchFull(dist, perm);
        //     // costsFull.push_back(costF);
        //     // stepsFull.push_back(stepF);
        //     // if(costF < bestCostFull) bestCostFull = costF;
            
        //     // // Random
        //     // perm = randomPermutation(n); // new random start
        //     // auto [costR, stepR] = localSearchRandom(dist, perm);
        //     // costsRandom.push_back(costR);
        //     // stepsRandom.push_back(stepR);
        //     // if(costR < bestCostRandom) bestCostRandom = costR;
        // }
        
        // // Calculate averages
        // double avgCostFull = 0, avgStepsFull = 0;
        // for(auto c : costsFull) avgCostFull += c;
        // for(auto s : stepsFull) avgStepsFull += s;
        // avgCostFull /= n;
        // avgStepsFull /= n;
        
        // double avgCostRandom = 0, avgStepsRandom = 0;
        // for(auto c : costsRandom) avgCostRandom += c;
        // for(auto s : stepsRandom) avgStepsRandom += s;
        // avgCostRandom /= n;
        // avgStepsRandom /= n;
    
        // cout << "Full Local Search:\n";
        // cout << "  Avg Cost: " << avgCostFull << "\n";
        // cout << "  Avg Steps: " << avgStepsFull << "\n";
        // cout << "  Best Cost: " << bestCostFull << "\n";
        // cout << "Random Local Search:\n";
        // cout << "  Avg Cost: " << avgCostRandom << "\n";
        // cout << "  Avg Steps: " << avgStepsRandom << "\n";
        // cout << "  Best Cost: " << bestCostRandom << "\n\n";
    }
    return 0;
}