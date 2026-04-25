import itertools

a, b, c, d, e, f_val = 2, 7, 9, 6, 8, 6

def le_product(u, v):
    """Częściowy porządek produktowy <= na N^n."""
    return all(ui <= vi for ui, vi in zip(u, v))

def find_minimal_elements(A):
    """
    Zwraca elementy <=-minimalne w skończonym zbiorze A.
    Element x jest minimalny, jeśli w zbiorze A nie ma takiego y (y != x), że y <= x.
    """
    minimal = []
    for x in A:
        is_minimal = True
        for y in A:
            if x == y:
                continue
            if le_product(y, x):  # jeśli istnieje mniejsze y, x nie jest minimalne
                is_minimal = False
                break
        if is_minimal and x not in minimal:
            minimal.append(x)
    return minimal

print("Porównanie par (a)")
p1, p2, p3 = (a, b), (c, d), (e, f_val)
pairs = [p1, p2, p3]
for u, v in itertools.permutations(pairs, 2):
    print(f"{u} <= {v} {le_product(u, v)}")

print("\nPorównanie trójek (b)")
t1 = (a, c, e)
t2 = (b, d, f_val)
print(f"{t1} <= {t2}: {le_product(t1, t2)}")
print(f"{t2} <= {t1}: {le_product(t2, t1)}")

print("\nElementy minimalne w zbiorze A")
# A: (x-a)^2 + (y-b)^2 < 5
# koło o środku (a,b)=(2,7) i promieniu sqrt(5).
# Przeszukujemy x i y w otoczeniu ktore na pewno zawiera cale kolko
A = []
for x in range(10):
    for y in range(15):
        if (x - a)**2 + (y - b)**2 < 5:
            A.append((x, y))

min_A = find_minimal_elements(A)
print(f"Elementy minimalne w A to: {min_A}")




print("\nElementy minimalne w zbiorze B")

def f(x1, x2, x3, x4):
    return (x1 - c)**2 + (x2 - d)**2 + (x3 - e)**2 + (x4 - f_val)**2

# Znajdowanie górnych ograniczeń (maksymalnych wartości dla pojedynczych osi)
bounds = []
centers = [c, d, e, f_val]
for i in range(4):
    k = 0
    while True:
        test_pt = [0, 0, 0, 0]
        test_pt[i] = k
        if f(*test_pt) > 224:
            bounds.append(k)
            break
        k += 1

print(f"Znalezione górne granice dla osi: {bounds}")

# Generowanie przestrzeni poszukiwan
ranges = [
    list(range(0, bounds[0] + 1)),
    list(range(0, bounds[1] + 1)),
    list(range(0, bounds[2] + 1)),
    list(range(0, bounds[3] + 1))
]

candidates = list(itertools.product(*ranges))
print(f"Liczba punktów do sprawdzenia po optymalizacji: {len(candidates)}")

# Filtrowanie tylko tych punktów, które należą do zbioru B
B_subset = [x for x in candidates if f(*x) > 224]

# Znajdowanie elementów minimalnych
def le_product(u, v):
    return all(ui <= vi for ui, vi in zip(u, v))

minimal_B = []
for x in B_subset:
    is_minimal = True
    for y in B_subset:
        if x != y and le_product(y, x):
            is_minimal = False
            break
    if is_minimal:
        minimal_B.append(x)

print(f"\nElementy minimalne zbioru B ({len(minimal_B)} sztuk):")
for el in minimal_B:
    print(el)