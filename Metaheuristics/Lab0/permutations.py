import random
import math
import matplotlib.pyplot as plt
import glob

# the cost of travel between cities is specified by the Euclidean distance rounded to the nearest whole number
def EUC_2Dnorm(a, b):
    return int(math.sqrt((b[0]-a[0])**2 + (b[1]-a[1])**2)+0.5)

def hamiltonianLength(candidates):
    length = EUC_2Dnorm(candidates[len(candidates)-1], candidates[0])
    for i in range(len(candidates)-1):
        length += EUC_2Dnorm(candidates[i], candidates[i+1])
    return length

def readFile(filename):
    coords = []
    with open(filename) as f:
        # print(f.read())
        for line in f:
            line_split = line.split()
            if(line_split[0].isdigit()):
                #print(line, end="")
                coords.append((float(line_split[1]), float(line_split[2])))

    return coords

def randomHamilton(coords, filename):
    if(len(coords) == 0):
        return
    
    candidates = []
    min_total = float('inf')
    best_path = []
    for i in range(1000):
        current_perm = list(coords)
        random.shuffle(current_perm)
        curr_len = hamiltonianLength(current_perm)
        candidates.append(curr_len)
        if(curr_len < min_total):
            min_total = curr_len
            best_path = current_perm


    # (a) średnią z minimum dla każdych 10 kolejnych losowań (100 grup po 10 losowań),
    # (b) średnią z minimum dla każdych 50 kolejnych losowań (20 grup po 50 losowań),
    # (c) i minimalną wartość dla tych 1000 losowań.

    minH10_list = []
    for i in range(0, 1000, 10): # Skaczemy co 10
        grupa = candidates[i : i+10] # Wycinamy 10 elementów 
        minH10_list.append(min(grupa))
    srednia_a = sum(minH10_list) / len(minH10_list)

    minH50_list = []
    for i in range(0, 1000, 50): # Skaczemy co 50
        grupa = candidates[i : i+50] # Wycinamy 50 elementów 
        minH50_list.append(min(grupa))
    srednia_b = sum(minH50_list) / len(minH50_list)

    print(f"Średnia z minimów (grupy 10): {srednia_a}")
    print(f"Średnia z minimów (grupy 50): {srednia_b}")
    print(f"Najmniejszy wynik ogółem: {min_total}")

    plot_path(best_path, min_total, filename.split('.')[0]+"minRandom")


def prim_MST(coords, filename):
    n = len(coords)
    tree = {i: [] for i in range(n)} # np. tree[0] = [1, 5] means city 0 is connected with cities 1 and 5
    in_tree = [False] * n
    
    min_dist = [float('inf')] * n # min distance from city i to the tree
    parent = [-1] * n

    min_dist[0] = 0
    weight = 0
    
    for _ in range(n):
        # find vertix with min distance from the tree
        best_dist = float('inf')
        u = -1
        for i in range(n):
            if not in_tree[i] and min_dist[i] < best_dist:
                best_dist = min_dist[i]
                u = i
                
        in_tree[u] = True # add u to the tree
        
        # if not first, add edge to the tree
        if parent[u] != -1:
            tree[u].append(parent[u])
            tree[parent[u]].append(u)
            weight += best_dist
            
        # update min_dist for all vertices not in the tree
        for v in range(n):
            if not in_tree[v]:
                dist = EUC_2Dnorm(coords[u], coords[v])
                if dist < min_dist[v]:
                    min_dist[v] = dist
                    parent[v] = u

    path = dfs_path(tree, coords)
    hamiltLen = hamiltonianLength(path)
    plot_path(path, hamiltLen, filename.split('.')[0]+"MST")

    return weight, hamiltLen

def dfs_path(tree, coords):
    n = len(coords)
    length = 0
    mark = [False]*n
    path = []
    stack = [0]
    while(len(stack) != 0):
        v = stack.pop()
        mark[v] = True
        path.append(coords[v])
        for u in tree[v]:
            if not mark[u]:
                stack.append(u)
    return path

def plot_path(path, min, filename):
    x = [p[0] for p in path]
    y = [p[1] for p in path]

    x.append(path[0][0])
    y.append(path[0][1])

    plt.figure(figsize=(10,6))
    plt.plot(x, y, 'ro-')
    plt.title(f"Znaleziony cykl ma długość {min}")
    plt.xlabel("X")
    plt.ylabel("Y")
    plt.savefig("plots/" + filename + ".png")

tsp_list = ["dj38.tsp", "qa194.tsp", "uy734.tsp", "wi29.tsp", "zi929.tsp"]
tsp = "dj38.tsp"
for tsp in tsp_list:
    print(tsp)
    coords = readFile(tsp)
    randomHamilton(coords, tsp)
    weight, hamWeight = prim_MST(coords,tsp)
    print("Waga MST wynosi ", weight)
    print("Waga cyklu hamiltona na podstawie MST wynosi ", hamWeight)
    print()

for tsp_file in glob.glob('./plots/*'):
    print(tsp_file.split("/")[2])