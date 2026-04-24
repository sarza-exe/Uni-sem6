from functools import reduce

class GaussInt:
    def __init__(self, re, im):
        self.re = int(re)
        self.im = int(im)

    def norm(self):
        return self.re**2 + self.im**2

    def __mul__(self, other):
        return GaussInt(self.re * other.re - self.im * other.im, 
                        self.re * other.im + self.im * other.re)

    def __sub__(self, other):
        return GaussInt(self.re - other.re, self.im - other.im)

def div_rem(a, b):
    """Zwraca (q, r) takie, że a = q*b + r oraz N(r) < N(b)"""
    if b.re == 0 and b.im == 0:
        raise ValueError("Dzielenie przez zero")
    
    # Obliczenie dokładnego ilorazu w ciele liczb zespolonych
    denominator = b.norm()
    exact_re = (a.re * b.re + a.im * b.im) / denominator
    exact_im = (a.im * b.re - a.re * b.im) / denominator
    
    # Zaokrąglenie do najbliższej liczby całkowitej
    q_re = round(exact_re)
    q_im = round(exact_im)
    
    q = GaussInt(q_re, q_im)
    r = a - (q * b)
    
    return q, r

def nwd(a, b):
    """Oblicza NWD za pomocą algorytmu Euklidesa"""
    while not (b.re == 0 and b.im == 0):
        q, r = div_rem(a, b)
        a = b
        b = r
    return a

def nwd_list(lst):
    if not lst:
        return GaussInt(0, 0)
    return reduce(nwd, lst)

def nww(a, b):
    # nww(a,b) = (a*b)/nwd(a,b) (dzielenie dokładne)
    gcd = nwd(a, b)
    if gcd.re == 0 and gcd.im == 0:
        return GaussInt(0, 0)
    prod = a * b
    q, r = div_rem(prod, gcd)
    return q

def nww_list(lst):
    if not lst:
        return GaussInt(1, 0)
    return reduce(nww, lst)

# 279686
# abcdef
ab = GaussInt(2, 7)
print(f"N(2 + 7i) = {ab.norm()}")
#pc  aq  pd  bqi przez e  f i. 
im2 = GaussInt(2 + 9, 6 + 7)
im3 = GaussInt(8, 6)
q, r = div_rem(im2, im3) # im2 = qim3+r
print(f"(11 + 13i) / (8 + 6i) = {q.re} + {q.im}i")

# a+bi, c+di, e+di
cd = GaussInt(9,6)
ed = GaussInt(8,6)
nwd3 = nwd_list([ab,cd,ed])
print(f"NWD(2 + 7i, 9 + 6i, 8 + 6i) = {nwd3.re} + {nwd3.im}i")
nww3 = nww_list([ab,cd,ed])
print(f"NWW(2 + 7i, 9 + 6i, 8 + 6i) = {nww3.re} + {nww3.im}i")