#Script to find GCD of N1 and N2 when both modulus have common P
#Input: N1; : : : ;Nm RSA moduli
#1: Compute P = Pi Ni using a product tree.
#2: Compute zi = (P mod Ni^2 ) for all i using a remainder tree.
#3: Output: gcd(Ni; zi/Ni) for all i.

import math

N1 = 152623106102998859355071417176551223629547132342475055921113655030598105103295962158619774832211755537121217985142194043382936112115788261691421191777978074316019608144843975141961739666317964113156874048882931598523622294034736041234900051121641913634966224247780499751205718318247665396814452113278578744791

N2 = 116960410376538796436586976720948821611672454495183638944999105642425335107298642427290299095547711533650561189299925421861921679870758372620886759643453123314517678884015959296081753004079171849136211214393988944287474900393176025413873079837842577551328430047515246344177615184104529106722233164211788610929

P = N1*N2
print(P)
Z1 = (P % N1^2)
Z2 = (P % N2^2)
print(Z1)
print(Z2)
#G1 = math.gcd(N1, Z1/N1)
#G2 = math.gcd(N2, Z2/N2)
#print(G1)
#print(G2)

G3 = math.gcd(N1, N2)
print(G3)
