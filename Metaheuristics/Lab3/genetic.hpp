#pragma once

#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <cmath>
#include <climits>
#include <omp.h>

#include "utils.hpp"
#include "localSearch.hpp"

using namespace std;

// struktura pojedynczego wspinacza (osobnika) - trzyma jego trase i jej dlugosc (koszt)
struct Individual {
    vector<int> tour;
    long long cost;
};

// typ wyliczeniowy by w mainie latwo wybierac miedzy ox a pmx
enum CrossoverType { OX, PMX };

// selekcja turniejowa - losuje kilku wspinaczy i wygrywa ten z najkrotsza trasa (zostaje rodzicem)
int tournamentSelection(const vector<Individual>& population, int tournamentSize, mt19937& rng) {
    uniform_int_distribution<int> dist(0, population.size() - 1);
    int bestIdx = dist(rng);
    
    for (int i = 1; i < tournamentSize; ++i) {
        int idx = dist(rng);
        if (population[idx].cost < population[bestIdx].cost) {
            bestIdx = idx;
        }
    }
    return bestIdx;
}

// krzyzowanie ox - kopiuje srodek z pierwszego rodzica, a reszte uzupelnia po kolei z drugiego
// liczy się które miasto z którym jest połączone
Individual crossoverOX(const Individual& p1, const Individual& p2, mt19937& rng) {
    int n = p1.tour.size();
    uniform_int_distribution<int> dist(0, n - 1);
    int idx1 = dist(rng);
    int idx2 = dist(rng);
    if (idx1 > idx2) swap(idx1, idx2);

    Individual child;
    child.tour.assign(n, -1);
    vector<bool> inside(n, false);

    for (int i = idx1; i <= idx2; ++i) {
        child.tour[i] = p1.tour[i];
        inside[p1.tour[i]] = true;
    }

    int childIdx = (idx2 + 1) % n;
    int parentIdx = (idx2 + 1) % n;

    while (child.tour[childIdx] == -1) {
        if (!inside[p2.tour[parentIdx]]) {
            child.tour[childIdx] = p2.tour[parentIdx];
            childIdx = (childIdx + 1) % n;
        }
        parentIdx = (parentIdx + 1) % n;
    }
    return child;
}

// krzyzowanie pmx - bardziej skomplikowane, kopiuje srodek od pierwszego i uzywa mapowania by wstawic geny drugiego
// liczy się które miasto ma jaki indeks 
Individual crossoverPMX(const Individual& p1, const Individual& p2, mt19937& rng) {
    int n = p1.tour.size();
    uniform_int_distribution<int> dist(0, n - 1);
    int idx1 = dist(rng);
    int idx2 = dist(rng);
    if (idx1 > idx2) swap(idx1, idx2);

    Individual child;
    child.tour.assign(n, -1);
    
    vector<int> posInP2(n);
    for (int i = 0; i < n; ++i) {
        posInP2[p2.tour[i]] = i;
    }

    for (int i = idx1; i <= idx2; ++i) {
        child.tour[i] = p1.tour[i];
    }

    for (int i = idx1; i <= idx2; ++i) {
        int candidate = p2.tour[i];
        bool alreadyPresent = false;
        for (int k = idx1; k <= idx2; ++k) {
            if (child.tour[k] == candidate) {
                alreadyPresent = true;
                break;
            }
        }
        
        if (!alreadyPresent) {
            int currIdx = i;
            int mappedValue = p1.tour[currIdx];
            int targetIdx = posInP2[mappedValue];
            
            while (targetIdx >= idx1 && targetIdx <= idx2) {
                currIdx = targetIdx;
                mappedValue = p1.tour[currIdx];
                targetIdx = posInP2[mappedValue];
            }
            child.tour[targetIdx] = candidate;
        }
    }

    for (int i = 0; i < n; ++i) {
        if (child.tour[i] == -1) {
            child.tour[i] = p2.tour[i];
        }
    }
    return child;
}

// mutacja - 5% szans ze u dziecka odwrocimy kawalek trasy (to samo co jeden ruch 2-opt)
void mutateInvert(Individual& ind, double mutationRate, mt19937& rng) {
    uniform_real_distribution<double> rdist(0.0, 1.0);
    if (rdist(rng) < mutationRate) {
        int n = ind.tour.size();
        uniform_int_distribution<int> idist(0, n - 1);
        int i = idist(rng);
        int j = idist(rng);
        if (i > j) swap(i, j);
        if (i != j) {
            reverse(ind.tour.begin() + i, ind.tour.begin() + j + 1);
        }
    }
}

// migracja - wyspy wymieniaja sie wiedza, 5 najlepszych wspinaczy plynnie na nastepna wyspe
void executeMigration(vector<vector<Individual>>& islands) {
    int numIslands = islands.size();
    vector<vector<Individual>> migrants(numIslands);

    // najpierw zbieramy po 5 najlepszych z kazdej wyspy
    for (int i = 0; i < numIslands; ++i) {
        sort(islands[i].begin(), islands[i].end(), [](const Individual& a, const Individual& b) {
            return a.cost < b.cost;
        });
        for (int k = 0; k < 5; ++k) {
            migrants[i].push_back(islands[i][k]);
        }
    }

    // potem wrzucamy ich na sasiadujaca wyspe na miejsce 5 najslabszych (bo posortowalismy malejaco)
    for (int i = 0; i < numIslands; ++i) {
        int nextIsland = (i + 1) % numIslands;
        
        sort(islands[nextIsland].begin(), islands[nextIsland].end(), [](const Individual& a, const Individual& b) {
            return a.cost > b.cost;
        });
        
        for (int k = 0; k < 5; ++k) {
            islands[nextIsland][k] = migrants[i][k];
        }
    }
}

// glowna petla algorytmu ewolucyjnego
Individual runIslandGeneticAlgorithm(const vector<int>& distMatrix, int n, CrossoverType crossType) {
    const int NUM_ISLANDS = 8;
    const int POP_SIZE = 20;
    const int TOURNAMENT_SIZE = 5;
    const double MUTATION_RATE = 0.05; 
    
    // tu ustawiamy szanse na wejscie local searcha i limit czasu dzialania
    const double LS_PROBABILITY = 0.10;
    const int STAGNATION_LIMIT = 10; 
    const int NO_EPOCHS = 50; // liczba pokolen na wyspie zanim nastapi migracja
    
    vector<vector<Individual>> islands(NUM_ISLANDS, vector<Individual>(POP_SIZE));
    
    // kazda wyspa ma wlasny generator losowy zeby watki nie wchodzily sobie w droge
    vector<mt19937> rngs(NUM_ISLANDS);
    random_device rd;
    for (int i = 0; i < NUM_ISLANDS; ++i) rngs[i].seed(rd() + i);

    // tworzenie 100 wspinaczy na kazdej z 8 wysp i wyslanie ich od razu na trening (local search)
    for (int i = 0; i < NUM_ISLANDS; ++i) {
        for (int j = 0; j < POP_SIZE; ++j) {
            islands[i][j].tour = randomPermutation(n);
            islands[i][j].cost = localSearch(distMatrix, islands[i][j].tour, 10);
        }
    }

    long long globalBestCost = LLONG_MAX;
    Individual globalBestIndividual;
    int generationsWithoutImprovement = 0;
    
    uniform_real_distribution<double> lsidist(0.0, 1.0);

    // petla kreci sie dopoki wspinacze bija rekordy
    while (generationsWithoutImprovement < STAGNATION_LIMIT) {
        bool localImprovementFound = false;

        // od teraz kazda z 8 wysp liczy sie na osobnym rdzeniu procesora
        #pragma omp parallel for num_threads(NUM_ISLANDS) shared(islands, rngs)
        for (int islandId = 0; islandId < NUM_ISLANDS; ++islandId) {
            auto& rng = rngs[islandId];
            
            // ewolucja toczy sie przez 50 pokolen zanim wyspy sie ze soba skontaktuja
            for (int epoch = 0; epoch < NO_EPOCHS; ++epoch) {
                vector<Individual> nextGeneration;
                nextGeneration.reserve(POP_SIZE);
                
                // elitaryzm - przepisywanie najlepszego starca do nowej populacji by nie stracic rekordu
                auto bestOnIsland = min_element(islands[islandId].begin(), islands[islandId].end(), 
                    [](const Individual& a, const Individual& b) { return a.cost < b.cost; });
                nextGeneration.push_back(*bestOnIsland);

                // wypelnianie wyspy nowymi dziecmi
                while (nextGeneration.size() < POP_SIZE) {
                    // wybieramy matke i ojca
                    int p1Idx = tournamentSelection(islands[islandId], TOURNAMENT_SIZE, rng);
                    int p2Idx = tournamentSelection(islands[islandId], TOURNAMENT_SIZE, rng);

                    // pilnujemy by wspinacz nie skrzyzowal sie sam ze soba
                    while( p1Idx == p2Idx){
                        p2Idx = tournamentSelection(islands[islandId], TOURNAMENT_SIZE, rng);
                    }
                    
                    // krzyzowanie zalezne od wybranego parametru
                    Individual child;
                    if (crossType == OX) {
                        child = crossoverOX(islands[islandId][p1Idx], islands[islandId][p2Idx], rng);
                    } else {
                        child = crossoverPMX(islands[islandId][p1Idx], islands[islandId][p2Idx], rng);
                    }

                    // drobna mutacja
                    mutateInvert(child, MUTATION_RATE, rng);

                    // algorytm memetyczny - co dziesiate dziecko uczy sie (idzie na local search)
                    if (lsidist(rng) < LS_PROBABILITY) {
                        child.cost = localSearch(distMatrix, child.tour, 10);
                    } else {
                        child.cost = calculateCost(distMatrix, child.tour); // reszta dzieci ma tylko liczony koszt
                    }

                    nextGeneration.push_back(child);
                }
                islands[islandId] = move(nextGeneration);
            }
        }

        // po 50 epokach szukamy czy na jakiejs wyspie padl nowy rekord swiata
        for (int i = 0; i < NUM_ISLANDS; ++i) {
            for (int j = 0; j < POP_SIZE; ++j) {
                
                // jesli ktos jest blisko rekordu a nie mial treningu to dajemy mu szanse
                if (islands[i][j].cost < globalBestCost) {
                    localSearch(distMatrix, islands[i][j].tour, 20);
                    islands[i][j].cost = calculateCost(distMatrix, islands[i][j].tour);
                    
                    if (islands[i][j].cost < globalBestCost) {
                        globalBestCost = islands[i][j].cost;
                        globalBestIndividual = islands[i][j];
                        localImprovementFound = true;
                    }
                }
            }
        }

        // aktualizacja licznika stagnacji by wiedziec kiedy przerwac program
        if (localImprovementFound) {
            generationsWithoutImprovement = 0;
        } else {
            generationsWithoutImprovement += 50;
        }

        // jesli jeszcze nie konczymy to odpalamy migracje by wyspy wymieszaly geny
        if (generationsWithoutImprovement < STAGNATION_LIMIT) {
            executeMigration(islands);
        }
    }

    return globalBestIndividual;
}