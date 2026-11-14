#!/usr/bin/env python3
"""Royal Martinot — textes d'aide des réglages (version 2.58) : deux lignes
de 28 caractères au plus, en bas de l'écran RÉGLAGES, pour la ligne choisie
(et sa valeur). Sortie : src/assets_help.c (banque 14)."""
import os
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# option -> liste de paires (une par valeur) ou une seule paire (toutes valeurs)
H = {
 'START': [("LANCE LA PARTIE AVEC CES", "REGLAGES (OU START).")],
 'MODE': [("VOUS CONTRE L'ORDINATEUR,", "EN SIMPLE."),
          ("DEUX JOUEURS FACE A FACE ;", "EN DOUBLE : PARTENAIRES CPU."),
          ("EN DOUBLE CONTRE 2 CPU, AVEC", "UN PARTENAIRE (CPU OU J2)."),
          ("TOURNOI, POULES, SAISON ET", "GRANDS CHELEMS MARTINOT."),
          ("EXERCICES POUR PROGRESSER :", "FRAPPES, SERVICE, VOLEE..."),
          ("L'ORDINATEUR JOUE SEUL.", "A OU START REND LA MAIN.")],
 'TYPE': [("UN JOUEUR DE CHAQUE COTE.", ""),
          ("DEUX CONTRE DEUX : LES", "COULOIRS SONT EN JEU.")],
 'P': None,      # joueurs : style (calculé)
 'LEVEL': [("ADVERSAIRE LENT, QUI FAIT", "DES FAUTES : POUR DEBUTER."),
           ("ADVERSAIRE SOLIDE ET", "EQUILIBRE."),
           ("ADVERSAIRE RAPIDE ET PRECIS,", "TRES PEU DE FAUTES.")],
 'SURF': [("REBOND HAUT, BALLE LENTE :", "ON GLISSE SUR LA TERRE."),
          ("REBOND BAS, BALLE QUI FUSE :", "AVANTAGE AU SERVICE-VOLEE."),
          ("LE COMPROMIS : NI LENT,", "NI RAPIDE."),
          ("RAPIDE ET ASSEZ BAS.", ""),
          ("SABLE : BALLE TRES LENTE,", "REBOND TRES HAUT."),
          ("BALLE AU RAS DU SOL ; LES", "JOUEURS GLISSENT !")],
 'ENV': [("TRIBUNES PLEINES, GRANDE", "AMBIANCE."),
         ("COURT EN PLEIN AIR, ENTOURE", "D'ARBRES."),
         ("COURT COUVERT : JAMAIS", "DE PLUIE."),
         ("LE CLUB NUDISTE : LE BONUS", "DEBLOQUE."),
         ("COURT ROSE, COEURS, FLEURS", "ET ETOILES."),
         ("MURS DE PIERRE, BANNIERES", "ROYALES ET TORCHES."),
         ("ARBRES AUX COULEURS DE", "L'AUTOMNE.")],
 'COND': [("EN PLEIN JOUR.", ""),
          ("MATCH EN NOCTURNE,", "DECOR ASSOMBRI."),
          ("GOUTTES, REBONDS PLUS BAS,", "BALLE QUI GLISSE."),
          ("LE SOLEIL SE COUCHE AU FIL", "DES JEUX, PUIS PROJECTEURS.")],
 'FMT': [("UN SET GAGNANT :", "UN MATCH COURT."),
         ("DEUX SETS GAGNANTS.", ""),
         ("TROIS SETS GAGNANTS :", "LE GRAND FORMAT."),
         ("SETS DE 4 JEUX :", "UN MATCH EXPRESS.")],
 'REPLAY': [("PAS DE RALENTI.", ""),
            ("RALENTI TELE : GROS PLAN DU", "COUP, ZOOM SUR LE REBOND."),
            ("RALENTI SIMPLE, SANS GROS", "PLAN NI ZOOM (PLUS COURT).")],
 'COMP': [("", "")],     # (explication en 4 lignes, banque 5)
 'CSIZE': [("TOURNOI A 4 JOUEURS :", "DEMI-FINALE ET FINALE."),
           ("TOURNOI A 8 JOUEURS :", "QUART, DEMI ET FINALE.")],
 'CPROG': [("TOUS LES ADVERSAIRES ONT", "LE NIVEAU CHOISI."),
           ("ADVERSAIRES DE PLUS EN PLUS", "FORTS A CHAQUE TOUR.")],
 'CVAR': [("LA MEME SURFACE A CHAQUE", "TOUR."),
          ("UNE SURFACE DIFFERENTE A", "CHAQUE TOUR.")],
 'EX': [("20 BALLES : 1 POINT DANS LE", "COURT, 2 DANS L'ETOILE."),
        ("BALLES ALTERNEES DANS LES", "DEUX COINS : COUREZ !"),
        ("LA PLUS LONGUE SERIE SANS", "FAUTE (JUSQU'A 50)."),
        ("SEULE LA ZONE DESIGNEE", "RAPPORTE DES POINTS."),
        ("VOUS ETES AU FILET : BALLES", "TENDUES A VOLLEYER."),
        ("20 SERVICES : BONUS A", "L'EXTERIEUR OU AU T."),
        ("ECHANGES CONTRE UN JOUEUR", "IMMOBILE, RENVOI AU CENTRE."),
        ("CHOISISSEZ COTE, VITESSE,", "EFFET ET NOMBRE DE BALLES."),
        ("LECON GUIDEE : FRAPPER,", "AMORTIE, LOB, VISER, SERVIR.")],
 'SIDE': [("LES BALLES ARRIVENT SUR", "VOTRE COUP DROIT."),
          ("LES BALLES ARRIVENT SUR", "VOTRE REVERS."),
          ("UNE BALLE EN COUP DROIT,", "UNE EN REVERS."),
          ("COUP DROIT OU REVERS,", "AU HASARD.")],
 'SPEED': [("VITESSE DES BALLES", "ENVOYEES.")],
 'SPIN': [("EFFET DES BALLES ENVOYEES :", "LIFTE, COUPE OU LOB.")],
 'COUNT': [("NOMBRE DE BALLES DE", "L'EXERCICE.")],
 'CPART': [("VOTRE PARTENAIRE EST JOUE", "PAR L'ORDINATEUR."),
           ("UN AMI JOUE VOTRE", "PARTENAIRE (MANETTE 2).")],
 'GSPEED': [("75 % : POUR DEBUTER ; UNE", "CROIX MONTRE LE REBOND."),
            ("VITESSE NORMALE.", ""),
            ("125 % : POUR LES EXPERTS.", "")],
 'SOUND': [("MUSIQUE DES MENUS ET", "BRUITAGES."),
           ("BRUITAGES SEULS, SANS", "MUSIQUE."),
           ("SILENCE COMPLET.", "")],
 'FATIG': [("LES JOUEURS NE SE FATIGUENT", "JAMAIS."),
           ("APRES 10 FRAPPES, LES", "JOUEURS RALENTISSENT.")],
 'VIEW': [("LA VUE D'ORIGINE, VUE", "DU DESSUS."),
          ("LE COURT SE RESSERRE AU", "FOND, VU DES GRADINS.")],
}
ORDER = ['START', 'MODE', 'TYPE', 'P', 'P', 'LEVEL', 'SURF', 'ENV', 'COND', 'FMT', 'REPLAY', 'COMP',
         'CSIZE', 'CPROG', 'CVAR', 'EX', 'SIDE', 'SPEED', 'SPIN', 'COUNT', 'CPART', 'GSPEED', 'SOUND', 'FATIG', 'VIEW']
STYLE = [("BON PARTOUT, SANS FAIBLESSE.",), ("LONGS ECHANGES DU FOND.",), ("FRAPPES FORTES ET LOURDES.",),
         ("SERVICE-VOLEE : MONTE VITE.",), ("DEFEND, ATTEND LA FAUTE.",), ("PREND LA BALLE TOT, ATTAQUE.",),
         ("LOBS ET AMORTIES.",), ("VISE LES LIGNES, PRECIS.",), ("VARIE TOUT : AMORTIES, LOBS.",)]
OK = set(" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,:;!?'-/()%+<>")
strs, idx = [], []
def add(t):
    assert len(t) <= 28 and set(t) <= OK, t
    strs.append(t)
first = []      # indice de la 1re paire de chaque option
nval = []
for o in ORDER:
    if o == 'P':
        first.append(255); nval.append(0); continue
    first.append(len(strs) // 2); nval.append(len(H[o]))
    for a, b in H[o]:
        add(a); add(b)
style0 = len(strs)
for (a,) in STYLE:
    add(a)
L = ['/* Fichier généré par tools/gen_help.py — ne pas modifier. */',
     '/* Aide des réglages : deux lignes pour la ligne choisie (banque 14). */', '',
     '#include "runtime.h"', '#include "gfx.h"', '#include "comp.h"', '',
     '#pragma bss-name ("WBSS")      /* (RAM de cartouche, variables locales comprises : la RAM interne est pleine) */', 'u8 help_o, help_v;           /* ligne (option) et valeur à expliquer */', '',
     'static const char * const help_s[%d] = {' % len(strs)]
L += ['    "%s",' % t for t in strs] + ['};']
L += ['static const u8 help_first[%d] = { %s };' % (len(first), ', '.join(map(str, first))),
      'static const u8 help_n[%d] = { %s };' % (len(nval), ', '.join(map(str, nval))), '',
      '/* remplit comp_l1 (28 caractères) avec la ligne r (0, 1) de l\'aide */',
      'u8 __fastcall__ menu_help(u8 r)',
      '{',
      '    register const char *t, *u;     /* (variables en page zéro : aucune RAM fixe) */',
      '    register u8 i, k;',
      '    u = ""; i = 0; k = help_first[help_o];',
      '    if (k == 255) {             /* joueur : son style */',
      '        if (r == 0) { t = "STYLE : "; u = style_name[char_style[help_v]]; }',
      '        else t = help_s[%d + char_style[help_v]];' % style0,
      '    } else {',
      '        if (help_n[help_o] > 1) k += help_v;',
      '        t = help_s[k * 2 + r];',
      '    }',
      '    while (*t) comp_l1[i++] = *t++;',
      '    while (*u && i < 28) comp_l1[i++] = *u++;',
      '    while (i && comp_l1[i - 1] == \' \') --i;     /* (noms de surface complétés d\'espaces) */',
      '    while (i < 28) comp_l1[i++] = \' \';',
      '    comp_l1[28] = 0;',
      '    return 0;',
      '}', '']
open(os.path.join(ROOT, 'src', 'assets_help.c'), 'w').write('\n'.join(L))
print('aide : %d lignes, %d octets de texte' % (len(strs), sum(len(t) + 1 for t in strs)))
