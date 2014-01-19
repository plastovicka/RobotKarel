pý¡kaz DEMO

 æACHOVNICE
 Ÿekej 1000

 B·H
 Ÿekej 500

 DLA¦DICE
 Ÿekej 2000

 POKRYJ(n hoda(32))
 Ÿekej 200

 STüED
 opakuj 4
   OBRAZ
   vlevo
 konec
 opakuj 60
   O_POLÖ¬KO_BLÖ¦
   vlevo
   Ÿekej 40
 konec
 VYBER
 Ÿekej 1000

 opakuj 50
  Hanojsk‚_vØ§e
  Ÿekej 30
 konec
 Ÿekej 1000

 BLUDIæT·
 DOMY
 STüED;ZPµTKY
 PYRAMIDA 5
konec

pý¡kaz ¬ELEM_VZAD 
 vlevo 
 vlevo
konec

pý¡kaz KE_ZDI
 dokud nen¡ zeÔ
  krok
 konec
konec

pý¡kaz ZPµTKY
 ¬ELEM_VZAD
 KE_ZDI
 ¬ELEM_VZAD
konec

pý¡kaz ZµDY_KE_ZDI
 KE_ZDI
 ¬ELEM_VZAD
konec

pý¡kaz OBRAT
 kdy§ je zeÔ
  ¬ELEM_VZAD
 konec
konec

pý¡kaz ZATO¬
 dokud je zeÔ
  vlevo
 konec
konec

podm¡nka VLEVO_ZEÒ
 rychle
 vlevo
 VLEVO_ZEÒ=zeÔ
 vpravo
 pomalu
konec

podm¡nka VPRAVO_ZEÒ
 rychle
 vpravo
 VPRAVO_ZEÒ=zeÔ
 vlevo
 pomalu
konec

podm¡nka VZADU_ZEÒ
 rychle
 ¬ELEM_VZAD
 VZADU_ZEÒ=zeÔ
 ¬ELEM_VZAD
 pomalu
konec

pý¡kaz DOKOLA(KOLIK)
 opakuj KOLIK
  vlevo
 konec
konec

pý¡kaz NA_SEVER
 kdy§ z pad
  vpravo
 jinak
  dokud nen¡ sever
   vlevo
  konec
 konec
konec

pý¡kaz NA_JIH
 kdy§ vìchod
  vpravo
 jinak
  dokud nen¡ jih
   vlevo
  konec
 konec
konec

pý¡kaz NA_ZµPAD
 kdy§ jih
  vpravo
 jinak
  dokud nen¡ z pad
   vlevo
  konec
 konec
konec

pý¡kaz NA_VíCHOD
 kdy§ sever
  vpravo
 jinak
  dokud nen¡ vìchod
   vlevo
  konec
 konec
konec

pý¡kaz NAHORU
 kdy§ sever
  krok
 jinak
  vlevo
  NAHORU
  vpravo
 konec
konec

pý¡kaz DOLÞ
 kdy§ jih
  krok
 jinak
  vlevo
  DOLÞ
  vpravo
 konec
konec

pý¡kaz DOLÞ_KE_ZDI
 kdy§ jih
  KE_ZDI
 jinak
  vlevo
  DOLÞ_KE_ZDI
  vpravo
 konec
konec

pý¡kaz ZP·T
 rychle
 VZAD 1
 pomalu
konec

pý¡kaz P_éKROK
 rychle
 vpravo
 ?KROK
 vlevo
 pomalu
konec

pý¡kaz P_æIKMO
 kdy§ je zeÔ
  P_éKROK
  ?KROK
 jinak
  P_éKROK
  krok
 konec
konec

pý¡kaz L_éKROK
 rychle
 vlevo
 ?KROK
 vpravo
 pomalu
konec

pý¡kaz L_æIKMO
 kdy§ je zeÔ
  L_éKROK
  ?KROK
 jinak
  krok
  L_éKROK
 konec
konec

pý¡kaz ?KROK
 kdy§ nen¡ zeÔ
  krok
 konec
konec

pý¡kaz ?ZVEDNI
 kdy§ je znaŸka
  zvedni
 konec
konec

pý¡kaz ?POLO¦
 kdy§ je m¡sto
  polo§
 konec
konec

pý¡kaz VYBER
 dokud je znaŸka
  zvedni
 konec
konec

pý¡kaz VYPLÕ
 dokud je m¡sto
  polo§
 konec
konec

pý¡kaz POLO¦n(n)
 opakuj n
  polo§
 konec
konec

pý¡kaz OZNA¬(n)
 VYBER
 POLO¦n(n)
konec

pý¡kaz NA_ZNA¬KU
 dokud nen¡ znaŸka a nen¡ zeÔ
  krok
 konec
konec

pý¡kaz NA_ZNAK
 dokud nen¡ znak a nen¡ zeÔ
  krok
 konec
konec

// poŸet znaŸek na aktu ln¡m pol¡Ÿku
funkce ZNA¬KY
 Ÿ¡slo I=0
 dokud znaŸka
  zvedni
  I=I+1
 konec
 opakuj I
  polo§
 konec
 ZNA¬KY=I
konec

// udØl  k krok…
pý¡kaz KROKY(K)
 kdy§ K<0
  VZAD(-K)
 jinak
  opakuj K
   ?KROK
  konec
 konec
konec

// udØl  k krok… dozadu
pý¡kaz VZAD(K)
 kdy§ K<0
  KROKY(-K)
 jinak
  ¬ELEM_VZAD
  KROKY K
  ¬ELEM_VZAD
 konec
konec


// projde bludiçtØ a vysb¡r  vçechny znaŸky
// na vytvoýen¡ bludiçtØ pou§ij Shift+F5
pý¡kaz BLUDIæT·
 opakuj 4;  bludiçtØ1('.'); konec
 opakuj 4;  bludiçtØ1(0);   konec
 maluj 0
konec
pý¡kaz bludiçtØ1(Z)
 kdy§ zeÔ
  vpravo
 jinak
  krok
  vlevo
  kdy§ znak=Z
   vlevo
  jinak
   maluj Z
   opakuj 3
    bludiçtØ1(Z)
   konec
  konec
  krok
  vlevo
 konec
konec

//namaluje çachovnici
pý¡kaz æACHOVNICE
 dom…
 opakuj
  maluj ' '
  DALæÖ
  kdy§ nen¡ sever
   maluj '±'
   DALæÖ
  konec
 a§ sever
 dom…
konec

//celou plochu pokryje znakem z
pý¡kaz POKRYJ (z)
 dom…
 opakuj
  maluj z
  DALæÖ
 a§ je sever
 dom…
konec

//vyŸist¡ plochu, kter  neobsahuje zdi
pý¡kaz CLS
 POKRYJ 0
konec


//pýesune se doprostýed plochy
pý¡kaz STüED
 dom…
 DO_POLOVINY
 vpravo
 DO_POLOVINY
konec

pý¡kaz DO_POLOVINY
 kdy§ je zeÔ
  ¬ELEM_VZAD
 jinak
  KROKY 2
  DO_POLOVINY
  krok
 konec
konec

pý¡kaz PüIDEJ_ZNA¬KU
 kdy§ je zeÔ
  ?POLO¦
  ¬ELEM_VZAD
 jinak
  krok
  kdy§ nen¡ znaŸka
   ?POLO¦
   ¬ELEM_VZAD
   krok
  jinak
   PüIDEJ_ZNA¬KU
   krok
  konec
 konec
konec

//pýem¡st¡ znaŸky na dalç¡ pol¡Ÿko
pý¡kaz ST·HUJ
 kdy§ je znaŸka
  zvedni
  ST·HUJ
  ?POLO¦
 jinak
  kdy§ je zeÔ
   ZPµTKY
  jinak
   krok
  konec
 konec
konec

pý¡kaz ¬µRA
 ZPµTKY
 dokud nen¡ zeÔ
  ?POLO¦
  krok
 konec
 ?POLO¦
konec

pý¡kaz DALæÖ
 kdy§ nen¡ zeÔ
  krok
 jinak
  kdy§ z pad
    vpravo
    kdy§ nen¡ zeÔ
     krok
     vpravo
    konec
  jinak
    vlevo
    kdy§ nen¡ zeÔ
     krok
     vlevo
    konec
  konec
 konec
konec

// pýed zd¡ polo§¡ znaŸku a vr t¡ se zpØt
pý¡kaz OBRAZ
 kdy§ je zeÔ
  ¬ELEM_VZAD
  polo§
 jinak
  krok
  OBRAZ
  krok
 konec
konec

// bØ§¡ çikmo a odr §¡ se od stØn
pý¡kaz MÖ¬
 dokud nen¡ kl vesa
   kdy§ je zeÔ
    vlevo
   jinak
    kdy§ VLEVO_ZEÒ
     vpravo
    konec
   konec
   L_æIKMO
   Ÿekej 40
 konec
konec

// dojde ke zdi a zpØt
pý¡kaz BALàNEK
 kdy§ je zeÔ
  ¬ELEM_VZAD
 jinak
  krok
  BALàNEK
  krok
 konec
konec

// chod¡ ke zdi a zp tky
pý¡kaz DRIBLUJ
 opakuj
  BALàNEK
  ¬ELEM_VZAD
 a§ je kl vesa
konec

pý¡kaz éHLOPüÖ¬KY
 dom…
 éHLOPüÖ¬KA
 ¬ELEM_VZAD
 KE_ZDI
 vlevo
 éHLOPüÖ¬KA
 dom…
konec
pý¡kaz éHLOPüÖ¬KA
 opakuj
  maluj 'X'
  L_æIKMO
 a§ je zeÔ nebo VLEVO_ZEÒ
 maluj 'X'
konec

// dojde ke znaŸce, pýem¡st¡ ji o pol¡Ÿko bl¡§
// a vr t¡ se zpØt na p…vodn¡ m¡sto
pý¡kaz O_POLÖ¬KO_BLÖ¦
 kdy§ je zeÔ
  ¬ELEM_VZAD
 jinak
  krok
  kdy§ je znaŸka
   ¬ELEM_VZAD
   ST·HUJ
  jinak
   O_POLÖ¬KO_BLÖ¦
   krok
  konec
 konec
konec

pý¡kaz POSTAV_ZEÒ
 opakuj 5
  maluj '²'
  ?KROK
 konec
 vpravo
konec

// namaluje d…m
pý¡kaz DÞM
//ZDI
 KROKY 2;  vlevo
 POSTAV_ZEÒ;  P_æIKMO
 POSTAV_ZEÒ;  POSTAV_ZEÒ
 KROKY 2;  vpravo

//DVEüE
 maluj 'º';  krok
 maluj '»';  vlevo;  krok
 maluj 'Í';  krok;  vlevo
 maluj 'É';  krok
 maluj 'º';  ¬ELEM_VZAD

//STüECHA
 KROKY 3
 L_æIKMO;  L_æIKMO
 opakuj 3
  maluj '/';  P_æIKMO
 konec
 maluj '^';  vpravo
 opakuj 3
  P_æIKMO;  maluj '\'
 konec
 VZAD 3
 maluj '';  KROKY 3;  vpravo
 KROKY 5
 vlevo;  KROKY 2
konec

pý¡kaz DOMY
 dom…
 opakuj
  DÞM
  KROKY 8
  kdy§ nen¡ zeÔ
   VZAD 8
  konec
 a§ je zeÔ
 ZPµTKY
konec

// nakresl¡ Ÿtverec
pý¡kaz ¬TVEREC
 NA_VíCHOD
 maluj 'À';  krok
 opakuj 3
  maluj 'Ä';  krok
 konec
 maluj 'Ù';  vlevo;  krok
 maluj '³';  krok
 maluj '¿';  vlevo;  krok
 opakuj 3
  maluj 'Ä';  krok
 konec
 maluj 'Ú';  vlevo;  krok
 maluj '³';  krok;  vlevo
konec

// celou plochu vypln¡ Ÿtverci
pý¡kaz DLA¦DICE
 dom…
 opakuj
  opakuj
   ¬TVEREC
   KROKY 10
   kdy§ nen¡ zeÔ
    VZAD 5
   konec
  a§ je zeÔ
  ZPµTKY
  vlevo
  KROKY 6
  kdy§ nen¡ zeÔ
   VZAD 3
  konec
 a§ je zeÔ
 dom…
konec

pý¡kaz SRDCE
 dom…
 DO_POLOVINY
 ¬ELEM_VZAD
 opakuj 4
   L_æIKMO;  krok;  maluj ''
 konec
 vlevo
 opakuj 3
   krok;  maluj ''
 konec
 opakuj 2
  krok;  vlevo
  opakuj 3
    KROKY 2;  maluj ''
  konec
  L_æIKMO;  krok;  maluj ''
  vpravo
 konec
 ¬ELEM_VZAD
 opakuj 3
   krok;   maluj ''
 konec
 opakuj 4
   L_æIKMO;  L_éKROK;  maluj ''
 konec
 dom…
konec

// sma§e plochu tak, §e chod¡ n hodnØ
pý¡kaz B·H
 opakuj 8000
  kdy§ n hoda(10)<5
   vlevo
  konec
  kdy§ n hoda(10)<5
   vpravo
  konec
  ZATO¬
  krok
  maluj 0
  kdy§ n hoda(4)=0
   Ÿekej 1
  konec
 konec
konec

// ovl d n¡ robota kl vesnic¡
pý¡kaz KAREL
 opakuj
  Ÿekej -1
  zvolit kl vesa
   pý¡pad -75  //vlevo
    vlevo
   pý¡pad -77  //vpravo
    vpravo
   pý¡pad -72  //NAHORU
    ?KROK
   pý¡pad -80  //DOLU
    ZP·T
   pý¡pad -115 //Ctrl+vlevo
    L_éKROK
   pý¡pad -116 //Ctrl+vpravo
    P_éKROK
   pý¡pad -141 //Ctrl+nahoru
    KE_ZDI
   pý¡pad -145 //Ctrl+dolu
    ZPµTKY
   pý¡pad 13   //Enter
    n vrat
   pý¡pad '-'
    ?ZVEDNI
   pý¡pad '+'
    ?POLO¦
   pý¡pad 32
    maluj 0
  konec
 konec
konec

// vytvoý¡ Pascal…v troj£heln¡k
pý¡kaz PYRAMIDA(vìçka)
 kdy§ VPRAVO_ZEÒ
  STüED
 konec
 ?POLO¦
 vìçka=vìçka-1
 kdy§ vìçka>0
  L_æIKMO
  PYRAMIDA vìçka
  vpravo
  KROKY 2
  vlevo
  PYRAMIDA vìçka
  ¬ELEM_VZAD
  P_æIKMO
  ¬ELEM_VZAD
 konec
konec

pý¡kaz Hanojsk‚_vØ§e
 DOLÞ_KE_ZDI
 kdy§ jih nebo sever
  vlevo
 konec
 OBRAT
 krok
 kdy§ zeÔ
  ¬ELEM_VZAD
  krok
 jinak
  ZP·T
 konec
 kdy§ nen¡ znaŸka
  POSTAV_V·¦(9)
 konec
 HANOJ(1,2)
konec

pý¡kaz POSTAV_V·¦(vìçka)
 kdy§ vìçka>0
  VYBER
  POLO¦n(vìçka)
  NAHORU
  POSTAV_V·¦(vìçka-1)
  DOLÞ
 konec
konec

//pýesune Hanojskou vØ§ na tyŸ k
//pou§ije pomocnou tyŸ p
pý¡kaz HANOJ(k,p)
 kdy§ k<0
  ¬ELEM_VZAD
  HANOJ(-k,-p)
  ¬ELEM_VZAD
 jinak
  //zjisti velikost disku
  Ÿ¡slo n=ZNA¬KY
  //pýesuå sloupec na pomocnou tyŸ
  kdy§ n>1
   NAHORU
   HANOJ(p,k)
   VZAD(p)
   dokud znaŸka
    NAHORU
   konec
   DOLÞ
  konec
  //pýesuå jeden disk
  VYBER
  DOLÞ_KE_ZDI
  KROKY(k)
  dokud znaŸka
   NAHORU
  konec
  POLO¦n(n)
  DOLÞ_KE_ZDI
  //pýesuå sloupec na c¡lovou tyŸ
  kdy§ n>1
   VZAD(k-p)
   dokud ZNA¬KY>=n
    NAHORU
   konec
   HANOJ(k-p,-p)
  konec
 konec
konec


// vyn sob¡ matice m,n, vìsledek d  do z
// Matice musej¡ m¡t spr vn‚ rozmØry !
pý¡kaz #nasob(z[][], m[][], n[][])
 Ÿ¡slo i,j,k,Suma,r[]
 i=0
 opakuj d‚lka(m)
  r=z[i]
  j=0
  opakuj d‚lka(n[0])
   Suma=0
   k=0
   opakuj d‚lka(n)
    Suma=Suma+m[i][k]*n[k][j]
    k=k+1
   konec
   z[i][j]=Suma
   j=j+1
  konec
  i=i+1
 konec
konec

// seŸte matice m,n, vìsledek d  do z
// Matice musej¡ m¡t stejn‚ rozmØry !
pý¡kaz #secti(z[][], m[][], n[][])
 Ÿ¡slo i,j,r[]
 i=0
 opakuj d‚lka(m)
  r=z[i]
  j=0
  opakuj d‚lka(m[0])
   r[j]=m[i][j]+n[i][j]
   j=j+1
  konec
  i=i+1
 konec
konec

// pýeŸte Ÿ¡slo, kter‚ je vpravo od Karla
funkce read
 Ÿ¡slo x=0, minus=0
 NA_VíCHOD
 NA_ZNAK
 kdy§ znak='-'
  minus=1
  krok
 konec
 dokud znak>='0' a znak<='9'
  x= 10*x + (znak-'0')
  ?KROK
 a§ je zeÔ
 kdy§ je minus
  x=-x
 konec
 read=x
konec

// nap¡çe Ÿ¡slo vpravo od robota
pý¡kaz write(x)
 NA_VíCHOD
 kdy§ x<0
  maluj '-'
  x=-x
 konec
 kdy§ x>9
  write(x/10)
 jinak
  krok
 konec
 maluj x%10+'0'
 krok
konec

// pýeŸte matici
// mus¡ st t v lev‚m doln¡m rohu matice
pý¡kaz #precti(pole[][])
 Ÿ¡slo i,j
 i=d‚lka(pole)-1
 opakuj d‚lka(pole)
  j=0
  opakuj d‚lka(pole[0])
    pole[i][j]=read
    j=j+1
  konec
  i=i-1
  ZPµTKY
  L_éKROK
 konec
konec

//vyp¡çe matici
pý¡kaz #vypis(pole[][])
 Ÿ¡slo i,j
 i=d‚lka(pole)-1
 opakuj d‚lka(pole)
  j=0
  opakuj d‚lka(pole[0])
    write pole[i][j]
    j=j+1
    krok
  konec
  i=i-1
  ZPµTKY
  L_éKROK
 konec
konec

// seŸte a vyn sob¡ dvØ Ÿtvercov‚ matice
// Karel je v lev‚m doln¡m rohu prvn¡ matice,
// druh  matice je naps na nad prvn¡ matic¡,
// oddØlen‚ jsou pr zdnìm ý dkem.
pý¡kaz matice
 Ÿ¡slo dim=0
 NA_ZNAK
 dokud nen¡ zeÔ a je znak
   dokud je znak
    krok
   konec
   NA_ZNAK
   dim=dim+1
 konec
 ZPµTKY

 Ÿ¡slo B[dim][dim],C[dim][dim]
 #precti(B)
 L_éKROK
 #precti(C)
 L_éKROK

 Ÿ¡slo Z[dim][dim]
 #secti(Z,B,C)
 #vypis(Z)
 L_éKROK
 #nasob(Z,B,C)
 #vypis(Z)
 dom…
konec


