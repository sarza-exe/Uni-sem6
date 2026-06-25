from fractions import Fraction

class Polynomial:
    def __init__(self, terms=None, vars=None):
        if vars is None:
            vars = ['x', 'y', 'z']
        self.terms = {}
        self.vars = vars
        if terms:
            for exps, coeff in terms.items():
                c = Fraction(coeff)
                if c != 0:
                    self.terms[exps] = c
                    
    def leading_term(self, order_vars=None):
        if not self.terms:
            # Dynamiczna generacja krotki zerowej zależnie od liczby zmiennych
            return (0,) * len(self.vars), Fraction(0)
        
        if order_vars is None:
            order_vars = self.vars
        
        indices = [self.vars.index(v) for v in order_vars]
        
        def lex_key(exps):
            return tuple(exps[i] for i in indices)
            
        lt_exps = max(self.terms.keys(), key=lex_key)
        return lt_exps, self.terms[lt_exps]

    def __add__(self, other):
        res_terms = self.terms.copy()
        for exps, coeff in other.terms.items():
            res_terms[exps] = res_terms.get(exps, Fraction(0)) + coeff
        return Polynomial(res_terms, self.vars)

    def __sub__(self, other):
        res_terms = self.terms.copy()
        for exps, coeff in other.terms.items():
            res_terms[exps] = res_terms.get(exps, Fraction(0)) - coeff
        return Polynomial(res_terms, self.vars)

    def __mul__(self, other):
        res_terms = {}
        for e1, c1 in self.terms.items():
            for e2, c2 in other.terms.items():
                e_res = tuple(a + b for a, b in zip(e1, e2))
                res_terms[e_res] = res_terms.get(e_res, Fraction(0)) + c1 * c2
        return Polynomial(res_terms, self.vars)

    def is_zero(self):
        return len(self.terms) == 0

    def __repr__(self):
        if self.is_zero():
            return "0"
        parts = []
        for exps in sorted(self.terms.keys(), reverse=True):
            c = self.terms[exps]
            t_str = ""
            if c < 0:
                sign = " - " if parts else "-"
                c = -c
            else:
                sign = " + " if parts else ""
            
            var_parts = []
            for v, e in zip(self.vars, exps):
                if e == 1:
                    var_parts.append(v)
                elif e > 1:
                    var_parts.append(f"{v}^{e}")
            v_str = "".join(var_parts)
            
            if c == 1 and v_str:
                parts.append(f"{sign}{v_str}")
            else:
                parts.append(f"{sign}{c}{v_str}")
        return "".join(parts)

def lcm_monomials(e1, e2):
    return tuple(max(a, b) for a, b in zip(e1, e2))

def divide_monomials(e1, e2):
    res = []
    for a, b in zip(e1, e2):
        if a < b:
            return None
        res.append(a - b)
    return tuple(res)

def polynomial_reduce(f, G, order_vars=None):
    if order_vars is None:
        order_vars = f.vars
    vars_list = f.vars
    
    r = Polynomial({}, vars_list)
    p = Polynomial(f.terms, vars_list)
    
    while not p.is_zero():
        lt_p_exps, lt_p_coeff = p.leading_term(order_vars)
        reduced = False
        for g in G:
            if g.is_zero():
                continue
            lt_g_exps, lt_g_coeff = g.leading_term(order_vars)
            div = divide_monomials(lt_p_exps, lt_g_exps)
            if div is not None:
                mono_dict = {div: lt_p_coeff / lt_g_coeff}
                monomial_poly = Polynomial(mono_dict, vars_list)
                p = p - monomial_poly * g
                reduced = True
                break
        if not reduced:
            r = r + Polynomial({lt_p_exps: lt_p_coeff}, vars_list)
            p = p - Polynomial({lt_p_exps: lt_p_coeff}, vars_list)
    return r

def syzygy(f, g, order_vars=None):
    if f.is_zero() or g.is_zero():
        return Polynomial({}, f.vars)
    lt_f_exp, lt_f_coeff = f.leading_term(order_vars)
    lt_g_exp, lt_g_coeff = g.leading_term(order_vars)
    
    lcm_exp = lcm_monomials(lt_f_exp, lt_g_exp)
    
    div_f = divide_monomials(lcm_exp, lt_f_exp)
    div_g = divide_monomials(lcm_exp, lt_g_exp)
    
    S_f = Polynomial({div_f: Fraction(1, lt_f_coeff)}, f.vars) * f
    S_g = Polynomial({div_g: Fraction(1, lt_g_coeff)}, g.vars) * g
    return S_f - S_g

def buchberger(G_init, order_vars=None):
    G = [g for g in G_init if not g.is_zero()]
    pairs = [(G[i], G[j]) for i in range(len(G)) for j in range(i+1, len(G))]
    
    while pairs:
        g1, g2 = pairs.pop(0)
        S = syzygy(g1, g2, order_vars)
        rem = polynomial_reduce(S, G, order_vars)
        if not rem.is_zero():
            for g in G:
                pairs.append((g, rem))
            G.append(rem)
            
    cleaned_G = []
    for g in G:
        lt_exp, lt_coeff = g.leading_term(order_vars)
        # Dynamiczna generacja krotki zerowej dla wymuszenia unormowania
        zero_tuple = (0,) * len(g.vars)
        cleaned_G.append(g * Polynomial({zero_tuple: Fraction(1, lt_coeff)}, g.vars))
        
    reduced_G = []
    for i in range(len(cleaned_G)):
        other_G = cleaned_G[:i] + cleaned_G[i+1:]
        rem = polynomial_reduce(cleaned_G[i], other_G, order_vars)
        if not rem.is_zero():
            lt_exp, lt_coeff = rem.leading_term(order_vars)
            zero_tuple = (0,) * len(rem.vars)
            rem = rem * Polynomial({zero_tuple: Fraction(1, lt_coeff)}, rem.vars)
            if rem.terms not in [r.terms for r in reduced_G]:
                reduced_G.append(rem)
    return reduced_G

# Let's verify for part c:
# Index variables: a=2, b=7, c=9, d=6, e=8, f=6
# System: x^2 + y^2 - z^2 = 0
# order_vars = ['z', 'x', 'y'] for eliminating z? Wait, to eliminate z, we put z highest: z > x > y
order_elim_z = ['z', 'x', 'y']

print("\n\tPODPUNKT C")

cone = Polynomial({(2,0,0): 1, (0,2,0): 1, (0,0,2): -1}) # x^2 + y^2 - z^2

# f1 = (c+1)z = 10z
f1 = Polynomial({(0,0,1): 10})
gb1 = buchberger([cone, f1], order_elim_z)
print("GB 1:", gb1)

# f2 = z + d + 1 = z + 7
f2 = Polynomial({(0,0,1): 1, (0,0,0): 7})
gb2 = buchberger([cone, f2], order_elim_z)
print("GB 2:", gb2)

# f3 = x + z - e - 1 = x + z - 9
f3 = Polynomial({(1,0,0): 1, (0,0,1): -1, (0,0,0): -9})
gb3 = buchberger([cone, f3], order_elim_z)
print("GB 3:", gb3)

# f4 = x + y + z + f + 1 = x + y + z + 7
f4 = Polynomial({(1,0,0): 1, (0,1,0): 1, (0,0,1): 1, (0,0,0): 7})
gb4 = buchberger([cone, f4], order_elim_z)
print("GB 4:", gb4)

# f5 = y/a + z + 1 = 0.5*y + z + 1
f5 = Polynomial({(0,1,0): 0.5, (0,0,1): 1, (0,0,0): 1})
gb5 = buchberger([cone, f5], order_elim_z)
print("GB 5:", gb5)






print("\n\tPODPUNKT D")
# Układ: (x^2 + y^2 - 2x)^2 - z^2(x^2 + y^2) = 0 oraz x + 2y + 3z = 0
# Kolejność zmiennych: ['x', 'y', 'z']
vars_d = ['x', 'y', 'z']

# Ręczne rozwinięcie: (x^2 + y^2 - 2x)^2 - z^2(x^2 + y^2)
# = x^4 + y^4 + 4x^2 + 2x^2y^2 - 4x^3 - 4xy^2 - x^2z^2 - y^2z^2
eq1_d = Polynomial({
    (4, 0, 0): 1,   # x^4
    (0, 4, 0): 1,   # y^4
    (2, 0, 0): 4,   # 4x^2
    (2, 2, 0): 2,   # 2x^2 y^2
    (3, 0, 0): -4,  # -4x^3
    (1, 2, 0): -4,  # -4xy^2
    (2, 0, 2): -1,  # -x^2 z^2
    (0, 2, 2): -1   # -y^2 z^2
}, vars=vars_d)

# x + 2y + 3z = 0
eq2_d = Polynomial({
    (1, 0, 0): 1,   # x
    (0, 1, 0): 2,   # 2y
    (0, 0, 1): 3    # 3z
}, vars=vars_d)

order_elim_x = ['x', 'y', 'z']
gb_d = buchberger([eq1_d, eq2_d], order_elim_x)

print("Baza Groebnera dla podpunktu d):")
for g in gb_d:
    # Jeśli najwyższy potęga przy 'x' to 0, mamy wyeliminowane x
    if g.leading_term(order_elim_x)[0][0] == 0:
        print("--> Wielomian wyeliminowany (bez x):", g)


print("\n\tPODPUNKT E")
# Wprowadzamy zmienne pomocnicze dla układu biegunowego, aby obejść trygonometrię.
# Zmienne: ['r', 'c', 's', 'x', 'y']
# r - promień, c - cos(theta), s - sin(theta)
vars_e = ['r', 'c', 's', 'x', 'y']

# 1. Definicja x: x - r*c = 0
p1 = Polynomial({
    (0, 0, 0, 1, 0): 1,   # x
    (1, 1, 0, 0, 0): -1   # -r*c
}, vars=vars_e)

# 2. Definicja y: y - r*s = 0
p2 = Polynomial({
    (0, 0, 0, 0, 1): 1,   # y
    (1, 0, 1, 0, 0): -1   # -r*s
}, vars=vars_e)

# 3. Jedynka trygonometryczna: c^2 + s^2 - 1 = 0
p3 = Polynomial({
    (0, 2, 0, 0, 0): 1,   # c^2
    (0, 0, 2, 0, 0): 1,   # s^2
    (0, 0, 0, 0, 0): -1   # -1
}, vars=vars_e)

# 4. Równanie krzywej (po wymnożeniu przez c): r*c - 28*c^2 - 7 = 0
p4 = Polynomial({
    (1, 1, 0, 0, 0): 1,   # r*c
    (0, 2, 0, 0, 0): -28, # -28c^2
    (0, 0, 0, 0, 0): -7   # -7
}, vars=vars_e)

# Porządek leksykograficzny, który eliminuje r, c, s
order_elim_rcs = ['r', 'c', 's', 'x', 'y']

# UWAGA: Generowanie bazy dla 5 zmiennych może zająć chwilę 
# w zależności od optymalizacji Twojej funkcji buchberger.
gb_e = buchberger([p1, p2, p3, p4], order_elim_rcs)

print("Baza Groebnera dla podpunktu e):")
for g in gb_e:
    # Szukamy wielomianu, który nie ma r, c, ani s 
    # (czyli pierwsze 3 wykładniki w wiodącym jednomianie to 0)
    lt_exps = g.leading_term(order_elim_rcs)[0]
    if lt_exps[0] == 0 and lt_exps[1] == 0 and lt_exps[2] == 0:
        print("--> Wzór trysektrysy (tylko x, y):", g)