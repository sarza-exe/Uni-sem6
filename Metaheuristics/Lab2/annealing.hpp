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
 * @brief runs simulated annealing on the given solution and distance matrix
 * @param distMatrix The flat (row-major) distance matrix of the TSP instance
 * @param solution The initial tour (will be modified in place)
 * @param initialTemp The initial temperature for the annealing process
 * @param coolingRate The rate at which the temperature decreases
 * @param epochs The number of epochs to run
 * @param stepsPerEpoch The number of steps to take per epoch
 * @return The cost of the final solution
 */
long long annealing(const vector<int>& distMatrix, vector<int>& solution, double initialTemp = 100.0, double coolingRate = 0.95, int epochs = 1000, int stepsPerEpoch = 100) {
    // 1. Wylosuj rozwiązanie początkowe X.
    // 2. Wybierz losowe rozwiązanie X′ znajdujące się w sąsiedztwie X.
    // 3. Jeśli X′ jest lepsze to je przyjmij (X := X′). W przeciwnym razie, wyznacz prawdopo-
    // dobieństwo przyjęcia nowego rozwiązania używając wzoru e(f (X)−f (X′))/T i z tym praw-
    // dopodobieństwem X := X′.
    // 4. Jeśli nie wykonano jeszcze odpowiedniej liczby prób w obrębie danej epoki, wróć do
    // punktu 2.
    // 5. Zmniejsz temperaturę.
    // 6. Jeśli nie osiągnięto jeszcze warunku stopu, wróć do punktu 2
    long long currCost = calculateCost(distMatrix, solution);
    const int n = solution.size();

    mt19937 mt{};
    std::uniform_int_distribution randN{0, n-1};
    std::uniform_real_distribution<double> randProb(0.0, 1.0);  

    double temp = initialTemp;

    for(int epoch = 0; epoch < epochs; epoch++){
        for(int trial = 0; trial < stepsPerEpoch; trial++){
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

            if (delta < 0 || exp(-delta / temp) > randProb(mt)) {
                currCost += delta;
                std::reverse(solution.begin() + i, solution.begin() + j + 1);
            }
        }

        temp *= coolingRate;
    }
    return calculateCost(distMatrix, solution);
}


/**
 * runs local search with invert n random neighbors on the given solution and distance matrix
 * returns the cost of the final solution and the number of improvement steps taken
 * modifies the solution vector in place to represent the final tour
 */
pair<long long, int> localSearchFast(const vector<int>& distMatrix, vector<int>& solution){
    //1. Wyznacz wartość funkcji celu wszystkich sąsiadów rozwiązania aktualnego (dla otoczenia invert)
    //Jako kandydata do poprawy wybierz najlepszego z ocenionych sąsiadów.
    //jeśli kandydat nie jest lepszy od aktualnego rozwiązania, to zakończ algorytm
    //Zastąp aktualne rozwiązanie kandydatem i przejdź do kroku 1
    int noSteps = 0;
    long long currCost = calculateCost(distMatrix, solution);
    const int n = solution.size();

    mt19937 mt{};
    uniform_int_distribution randN{0, n-1};

    for(;; noSteps++){
        long long bestDelta = 0;
        int newI = -1; int newJ = -1;

        for (int k = 0; k < n; k++) {
            int i = randN(mt);
            int j = randN(mt);
            if(i == j || (i == 0 && j == n - 1)) continue;
            if(i > j) swap(i,j);

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

            if (delta < bestDelta) {
                bestDelta = delta;
                newI = i; newJ = j;
            }
        }

        if(bestDelta >= 0) break;
        currCost += bestDelta;
        std::reverse(solution.begin() + newI, solution.begin() + newJ + 1);
    }
    return {currCost,noSteps};
}