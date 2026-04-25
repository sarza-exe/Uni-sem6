class Polynomial:
    def __init__(self, terms=None):
        """
        Inicjalizacja słownikiem w formacie {wykładnik: współczynnik}.
        Np. {2: 3.0, 0: 1.0} to 3x^2 + 1.
        """
        self.terms = {}
        if terms:
            for exp, coeff in terms.items():
                if abs(coeff) > 1e-10:  # Odfiltrowanie zer, inaczej nie działa bo ON
                    self.terms[exp] = float(coeff)
        if not self.terms:
            self.terms[0] = 0.0 # Wielomian zerowy

    def norm(self):
        """Norma euklidesowa w K[x] to stopień wielomianu."""
        if 0 in self.terms and self.terms[0] == 0.0 and len(self.terms) == 1:
            return -1  # Konwencja dla wielomianu zerowego
        return max(self.terms.keys())

    def lc(self):
        """Leading Coefficient - wiodący współczynnik."""
        deg = self.norm()
        if deg == -1: return 0.0
        return self.terms[deg]

    def __add__(self, other):
        res = self.terms.copy()
        for exp, coeff in other.terms.items():
            res[exp] = res.get(exp, 0.0) + coeff
        return Polynomial(res)

    def __sub__(self, other):
        res = self.terms.copy()
        for exp, coeff in other.terms.items():
            res[exp] = res.get(exp, 0.0) - coeff
        return Polynomial(res)

    def __mul__(self, other):
        res = {}
        for e1, c1 in self.terms.items():
            for e2, c2 in other.terms.items():
                res[e1 + e2] = res.get(e1 + e2, 0.0) + c1 * c2
        return Polynomial(res)
        
    def __str__(self):
        if self.norm() == -1: return "0"
        return " + ".join(f"{c:.3f}x^{e}" for e, c in sorted(self.terms.items(), reverse=True))

def div_rem(A, B):
    """Dzielenie z resztą wielomianów A = Q*B + R"""
    Q = Polynomial()
    R = Polynomial(A.terms.copy())
    degB = B.norm()
    lcB = B.lc()
    
    if degB == -1:
        raise ZeroDivisionError("Dzielenie przez wielomian zerowy")
        
    while R.norm() >= degB and R.norm() != -1:
        deg_diff = R.norm() - degB
        coeff_ratio = R.lc() / lcB
        
        term = Polynomial({deg_diff: coeff_ratio})
        Q = Q + term
        R = R - (term * B)
        
    return Q, R

def nwd(a, b):
    """Algorytm Euklidesa"""
    while b.norm() != -1:
        _, r = div_rem(a, b)
        a = b
        b = r
    lc = a.lc()
    return Polynomial({e: c / lc for e, c in a.terms.items()}) if lc != 0 else a

def nww(a, b):
    """NWW(a, b) = (a * b) / NWD(a, b)"""
    prod = a * b
    gcd = nwd(a, b)
    q, _ = div_rem(prod, gcd)
    
    lc = q.lc()
    return Polynomial({e: c / lc for e, c in q.terms.items()}) if lc != 0 else q

def ext_nwd(a, b):
    """Rozszerzony algorytm Euklidesa aX + bY = d"""
    if b.norm() == -1:
        return a, Polynomial({0: 1.0}), Polynomial()
    
    q, r = div_rem(a, b)
    d, X, Y = ext_nwd(b, r)
    
    return d, Y, X - (q * Y)

# {2: 3.0, 0: 1.0} to 3x^2 + 1.

# 279686
# abcdef
# cx^a + b
cab = Polynomial({2: 9.0, 0: 7.0})
xone = Polynomial({1: 1.0, 0: 1.0})

print(f"N({cab}) = {cab.norm()}")
q, r = div_rem(cab, xone) # A = Q*B + R
print(f"{cab} = ({q})*({xone}) + ({r})")

v = Polynomial({3: 2.0, 2: 7.0, 1: 9.0, 0: 6.0})
w = Polynomial({3: 6.0, 2: 8.0, 1: 6.0})

print()

d, X, Y = ext_nwd(v,w)
print(f"v = {v}")
print(f"w = {w}")
print(f"NWD(v, w) = {nwd(v,w)}")
print("d = vX + wY")
print(f"{d} = ({v})({X}) + ({w})({Y})")
print()

w += Polynomial({0: 28.0})
d, X, Y = ext_nwd(v,w)
print("d = vX + (w + 28)Y")
print(f"{d} = ({v})({X}) + ({w})({Y})")
print(f"NWD(v, w + 28) = {nwd(v,w)}")
print(f"NWW(v, w + 28) = {nww(v,w)}")

