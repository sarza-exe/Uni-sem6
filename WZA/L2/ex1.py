import itertools

# a) Struktura i operacje pomocnicze
def clean_poly(p):
    """Usuwa jednomiany o zerowych wspolczynnikach."""
    return {k: v for k, v in p.items() if abs(v) > 1e-9}

def poly_add(p1, p2):
    res = dict(p1)
    for k, v in p2.items():
        res[k] = res.get(k, 0) + v
    return clean_poly(res)

def term_mult(coeff, exp, p):
    """Mnozy wielomian p przez jednomian (coeff * X^exp)."""
    res = {}
    for k, v in p.items():
        new_exp = tuple(e1 + e2 for e1, e2 in zip(k, exp))
        res[new_exp] = v * coeff
    return res

# b) Generatory kluczy porzadkujacych
def order_lex(permutation):
    """
    Zwraca funkcje klucza dla porzadku Lex. 
    permutation to krotka indeksow, np. (1, 0, 2) oznacza y > x > z.
    """
    def key_func(exp):
        return tuple(exp[i] for i in permutation)
    return key_func

def order_grlex(permutation):
    """Zwraca funkcje klucza dla porzadku GrLex."""
    def key_func(exp):
        lex_part = tuple(exp[i] for i in permutation)
        return (sum(exp), lex_part)
    return key_func

def get_lt(p, order_key):
    """Zwraca wiodacy jednomian: (potegi, wspolczynnik)."""
    if not p:
        return None, None
    best_exp = max(p.keys(), key=order_key)
    return best_exp, p[best_exp]

def divides(exp1, exp2):
    """Sprawdza czy jednomian X^exp1 dzieli X^exp2."""
    return all(e2 >= e1 for e1, e2 in zip(exp1, exp2))

# c) Algorytm dzielenia wielomianow
def polynomial_reduce(f, G, order_key):
    alphas = [{} for _ in G]
    r = {}
    p = dict(f)

    while p:
        lt_p_exp, lt_p_coeff = get_lt(p, order_key)
        division_occurred = False

        for i, g in enumerate(G):
            lt_g_exp, lt_g_coeff = get_lt(g, order_key)
            if not lt_g_exp:
                continue
            
            # Jesli LT(g_i) dzieli LT(p)
            if divides(lt_g_exp, lt_p_exp):
                mult_exp = tuple(e1 - e2 for e1, e2 in zip(lt_p_exp, lt_g_exp))
                mult_coeff = lt_p_coeff / lt_g_coeff
                
                # alpha_i = alpha_i + (mult_coeff * X^mult_exp)
                alphas[i][mult_exp] = alphas[i].get(mult_exp, 0) + mult_coeff
                alphas[i] = clean_poly(alphas[i])
                
                # p = p - (mult_coeff * X^mult_exp) * g_i
                term_times_g = term_mult(mult_coeff, mult_exp, g)
                neg_term = {k: -v for k, v in term_times_g.items()}
                p = poly_add(p, neg_term)
                
                division_occurred = True
                break
                
        if not division_occurred:
            # Przenosimy LT(p) do reszty
            r[lt_p_exp] = r.get(lt_p_exp, 0) + lt_p_coeff
            r = clean_poly(r)
            del p[lt_p_exp]

    return alphas, r

if __name__ == "__main__":

# d)
    # Definicja porządku GradedLex (x > y > z) -> indeksy (0, 1, 2)
    order_k = order_grlex((0, 1, 2))

    # f = x^3 - x^2y - x^2z
    f = {
        (3, 0, 0): 1.0, 
        (2, 1, 0): -1.0, 
        (2, 0, 1): -1.0
    }

    # g1 = x^2y - z
    g1 = {
        (2, 1, 0): 1.0, 
        (0, 0, 1): -1.0
    }

    # g2 = xy - 1
    g2 = {
        (1, 1, 0): 1.0, 
        (0, 0, 0): -1.0
    }

    print("--- Ćwiczenie 37 ---")
    
    # 1. Redukcja przez (g1, g2)
    alphas1, r1 = polynomial_reduce(f, [g1, g2], order_k)
    print(f"1. Reszta r1 dla (g1, g2): {r1}")

    # 2. Redukcja przez (g2, g1)
    alphas2, r2 = polynomial_reduce(f, [g2, g1], order_k)
    print(f"2. Reszta r2 dla (g2, g1): {r2}")





# e)

    # Indeks: 279686 -> a=2, b=7, c=9, d=6, e=8, f=6
    # h(x,y,z) = x^2 y^7 - y^9 z^6 + x^8 z^6
    h = {
        (2, 7, 0): 1.0,
        (0, 9, 6): -1.0,
        (8, 0, 6): 1.0
    }
    
    # Wymyslony ciag G, ktory ladnie "zjada" czesci h w rozny sposob
    g1 = {(2, 0, 0): 1.0, (0, 0, 1): -1.0}  # x^2 - z
    g2 = {(0, 7, 0): 1.0, (1, 0, 0): -1.0}  # y^7 - x
    G = [g1, g2]

    # Sprawdzamy wszystkie permutacje zmiennych (x, y, z) dla porzadku Lex
    permutations = list(itertools.permutations([0, 1, 2]))
    unique_remainders = []

    print("Testowanie wielomianu h dla G = [x^2 - z, y^7 - x]:\n")
    for perm in permutations:
        order_k = order_lex(perm)
        alphas, r = polynomial_reduce(h, G, order_k)
        
        # Formatowanie permutacji do czytelnego napisu, np (x>y>z)
        var_names = {0: 'x', 1: 'y', 2: 'z'}
        perm_str = " > ".join([var_names[i] for i in perm])
        
        if r not in unique_remainders:
            unique_remainders.append(r)
            print(f"Nowa reszta dla porzadku {perm_str}:")
            print(f"Reszta (r): {r}\n")