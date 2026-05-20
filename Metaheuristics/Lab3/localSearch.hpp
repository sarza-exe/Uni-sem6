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

#include "utils.hpp"

using namespace std;

/**
 * runs local search with invert neighborhood on the given solution and distance matrix
 * returns the cost of the final solution and the number of improvement steps taken
 * modifies the solution vector in place to represent the final tour
 */
long long localSearch(const vector<int>& distMatrix, vector<int>& solution, int noSteps){
    //1. Wyznacz wartość funkcji celu wszystkich sąsiadów rozwiązania aktualnego (dla otoczenia invert)
    //Jako kandydata do poprawy wybierz najlepszego z ocenionych sąsiadów.
    //jeśli kandydat nie jest lepszy od aktualnego rozwiązania, to zakończ algorytm
    //Zastąp aktualne rozwiązanie kandydatem i przejdź do kroku 1
    long long currCost = calculateCost(distMatrix, solution);
    const int n = solution.size();

    for(int step = 0; step < noSteps; step++){
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
        std::reverse(solution.begin() + newI, solution.begin() + newJ + 1);
    }
    return currCost;
}
