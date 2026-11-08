# Pixel Open — Documentation technique

## 1. Vue d'ensemble

| Élément | Choix |
|---|---|
| Plateforme | NES / Famicom, NTSC et PAL (Dendy traité comme PAL) |
| Cartouche | **MMC1 (SNROM, mapper 1)** : 128 Ko de PRG en 8 banques de 16 Ko, 8 Ko de CHR-RAM, 8 Ko de PRG-RAM (avec pile facultative) |
| Langages | C (cc65 2.19) pour la logique ; assembleur 6502 (ca65) pour le démarrage, la NMI, les sprites, les trampolines de banque et l'enregistrement des ralentis |
| ROM | `dist/pixel_open.nes` (131 088 octets, iNES) |

## 2. Choix de la cartouche

La version 1.0 tenait en NROM-256 (32 Ko, plein à 87 %). Il a fallu plus de place pour le contenu manquant du CDC :
- mode d'emploi intégré ;
- compétitions ;
- trois environnements ;
- ralentis ;
- palmarès ;
- exercices d'entraînement.

| Option | Avantages | Inconvénients | Décision |
|---|---|---|---|
| NROM-256 | Universelle, sans banques | 32 Ko, plein | Abandonnée |
| UNROM | 256 Ko, simple | Pas de PRG-RAM : ni ralentis ni palmarès persistant | Écartée |
| **MMC1 SNROM** | 128 à 256 Ko, banque fixe en $C000, **8 Ko de PRG-RAM** (ralentis, palmarès, sauvegarde par pile possible), CHR-RAM (public animé), très répandue (Zelda, Metroid…) | Écritures série (5 écritures par registre) | **Retenue** |
| MMC3 | IRQ de ligne, banques fines | Plus complexe et plus chère | Évolution possible |
| Extensions audio | Canaux supplémentaires | Absentes des consoles occidentales | Non utilisées (CDC 11.4) |

**Mise sous tension** : l'état initial du MMC1 n'est pas garanti. Chaque banque se termine donc par un petit programme (`STUBn`) qui réinitialise le mapper (ce qui fixe la banque 7 en $C000) puis saute au vrai démarrage.

## 3. Organisation mémoire

### Banques de PRG (octets utilisés / 16 368)

| Banque | Contenu | Occupation |
|---|---|---|
| 7 (fixe, $C000) | Démarrage, NMI, primitives (sprites, VRAM, manettes, trampolines `bank_call` / `far_call`, enregistrement des ralentis), boucle principale, règles, codes, palettes, son, bibliothèque cc65 | 8 816 |
| 0 | Moteur de match : physique, frappes, service, arbitrage, IA | 16 314 |
| 1 | Écrans : titre, configuration, présentation, résultats, compétitions, saisie de code, palmarès | 12 172 |
| 2 | Motifs graphiques (8 Ko), copiés en CHR-RAM au démarrage | 8 192 |
| 3 | Annexes du match : score, courts (3 lieux), public animé, pluie, ralentis, exercices, tutoriel | 6 996 |
| 4 | Mode d'emploi | 3 470 |
| 5 | Compétitions (tirages, simulations, classements) et palmarès | 7 324 |
| 6 | Libre | 0 |

### RAM

```
$0010-$00FF  page zéro : NMI, manettes, sprites, banque courante, pile C (sp), cc65
$0100-$01FF  pile 6502
$0200-$02FF  tampon OAM
$0300-$037F  tampon VRAM (96 octets envoyés au plus par trame : budget VBlank)
$0400-$06FF  variables du moteur (661 / 768 octets)
$0700-$07FF  pile de paramètres C
$6000-$77FF  RAM de cartouche : écrans, compétitions, tampon des ralentis (2 945 / 6 144)
$7800-$7FFF  palmarès (sauvegardé si la cartouche a une pile ; signature + somme de contrôle)
```

### Règles de banque

- La boucle principale (banque fixe) sélectionne la banque de l'écran courant : écrans, match ou mode d'emploi.
- Les changements d'écran entre banques passent par `go_req`, traité par la boucle principale.
- Une banque commutable n'appelle jamais directement une autre banque commutable. Elle passe par :
  - `bank_call(banque, fn)` : fonction sans argument ;
  - `far_call(banque, fn, arg)` : un argument et un résultat (registres A/X).
  
  Ces deux trampolines de la banque fixe restaurent ensuite la banque d'origine.
- Dans chaque banque, le **code et les données constantes sont dans des segments distincts** (`BANKn` / `RODATAn`). Sinon, cc65 place les tableaux `static const` locaux à l'adresse même de la fonction. Ce défaut a été trouvé et corrigé pendant les tests.

## 4. Architecture logicielle

| Module | Banque | Rôle |
|---|---|---|
| `src/crt0.s` | fixe | En-tête iNES, démarrage MMC1, copie CHR-RAM, détection de région, NMI, sprites, RLE, trampolines, `rp_record` |
| `src/main.c` | fixe | Boucle principale, transitions entre banques, compensation 50 Hz |
| `src/rules.c` | fixe | Score du tennis (portable, testé sur PC) |
| `src/password.c` | fixe | Codes de reprise sur 12 caractères (portable, testé sur PC) |
| `src/gfx.c` | fixe | Personnages, styles, surfaces, lieux, palettes, texte |
| `src/audio.c` | fixe | Bruitages, musiques, jingles |
| `src/game.c` | 0 | Moteur de match |
| `src/screens.c` | 1 | Écrans hors match |
| `src/extras.c` | 3 | Score, courts, public, pluie, ralentis, exercices, tutoriel |
| `src/manual.c` | 4 | Mode d'emploi (8 pages) |
| `src/comp.c`, `src/stats.c` | 5 | Compétitions, palmarès |
| `tools/gen_assets.py` | — | Génère tous les graphismes : police, logo, 3 courts, public assis et debout, 10 personnages × 8 poses |

### Physique et arbitrage

- Positions en virgule fixe 10.6 (1/64 de pixel), hauteur `z` séparée.
- Gravité selon le coup ; restitution et adhérence selon la surface, l'effet et la pluie.
- `launch()` atteint la cible en T trames et contrôle sans division le passage au-dessus du filet.
- **Filet** :
  - balle sous la bande : renvoyée vers le frappeur ;
  - balle qui frôle la bande (2 unités sous son sommet) : elle passe, ralentie. Au service, si elle tombe ensuite dans le carré, c'est un **let**.
- `rules.c` gère les points, jeux, sets, jeu décisif, services et changements de côté. En double, le serveur change au sein de l'équipe chaque fois qu'elle récupère le service.

### Intelligence artificielle

- **Trois niveaux** (vitesse, réaction, dispersion, marge par rapport aux lignes, puissance).
- **Huit styles** (CDC 10.2), propres à chaque personnage. Ils modifient :
  - la puissance, la marge et la dispersion ;
  - la fréquence de montée au filet, de lobs, d'amorties et de coupés ;
  - la profondeur du replacement.
- **Adaptation** (CDC 10.4) : pendant le match, l'IA compte, pour chaque équipe humaine, les balles jouées à gauche ou à droite et les frappes au filet.
  - Dès 6 frappes observées, aux niveaux normal et expert, elle décale son replacement vers le côté préféré du joueur (au plus 16 px).
  - Si le joueur monte souvent au filet, elle lobe davantage.
  
  Les compteurs repartent de zéro à chaque match ; le comportement reste borné et déterministe.
- **Double** : un responsable par balle (le partenaire le plus proche du point de frappe prévu). L'autre couvre sa moitié du court.

### Compétitions (`comp.c`)

- **Déterminisme** : une graine de 8 bits fixe le tirage des adversaires, leur niveau (0 à 2) et l'issue de tous les matchs entre joueurs CPU. La probabilité de victoire vaut 50 % ± 20 % par écart de niveau.
- **Reprise** : le code n'a besoin que de la graine, de l'étape et des 8 bits de résultats du joueur. Tout le reste est recalculé à la reprise.
- **Formats** :
  - **tournoi** : tableau à 8 ;
  - **poules** : calendriers 4 × 3 journées, puis demi-finales croisées ;
  - **championnat** : méthode du cercle à 6 joueurs ;
  - **saison** : 4 tableaux différents, un par surface, et barème de points.

### Codes de reprise (`password.c`)

45 bits de données + 3 bits de version + 12 bits de somme de contrôle = 60 bits, soit 12 symboles de 5 bits (alphabet sans I, O, 0, 1). Les symboles sont brouillés en chaîne pour qu'une erreur en modifie plusieurs.

### Ralentis

- **Enregistrement** : à chaque trame d'échange, `rp_record` (assembleur, ~450 cycles) enregistre la balle et les 4 joueurs (position et pose) dans un tampon circulaire de 160 trames en RAM de cartouche.
- **Déclenchement** : après un ace, un coup gagnant ou un échange d'au moins 12 frappes, les 140 dernières trames sont rejouées à demi-vitesse. A, B ou START les passent.

### Public animé

Grâce à la CHR-RAM, les 6 motifs de spectateurs sont remplacés par leur version debout, bras levés, puis restaurés. Le remplacement se fait à raison d'une tuile (16 octets) par trame, pour rester dans le budget VBlank.

## 5. Graphismes

- **Fond** : 221 tuiles sur 256. Trois écrans de court compressés en RLE (stade 657 octets, club 648, salle 647). La surface et les conditions ne changent que les palettes.
- **Sprites** : 208 tuiles sur 256.
  - 2 vues (dos et face) × 2 silhouettes (homme et femme) × 8 poses : attente, course ×2, frappe ×2, service, joie, déception.
  - 10 palettes de personnages.
  - Partenaires de double : couleurs de leur équipe (limite de 4 palettes de sprites).
- **Score** : dans les coins supérieurs. Les joueurs sont limités à x = 48..208 et la colonne 0 n'est pas utilisée (zone rognée par les téléviseurs).
- **Service** : la hauteur du lancer est affichée à moitié, pour que la balle reste visible quand le serveur est en haut de l'écran.

## 6. Audio

Le jeu n'utilise que l'APU natif :
- **carré 1 + bruit** : bruitages ;
- **carré 2 + triangle** : musiques et jingles.

Aucune musique pendant les échanges. Toutes les mélodies sont originales.

## 7. NTSC / PAL

- **Détection** : au démarrage, une boucle de 16 cycles est comptée entre deux VBlank (NTSC ≈ 1 861 ; PAL > 1 970).
- **Compensation** : à 50 Hz, une trame logique supplémentaire est exécutée toutes les 5 trames, ce qui garde la vitesse du 60 Hz.

## 8. Performances (mesurées avec `tests/profile.mjs`)

| Mesure (CPU contre CPU, ralentis activés) | Simple | Double |
|---|---|---|
| Logique médiane par trame | 13 600 cycles | 12 400 cycles |
| Trames d'échange en dépassement (> 27 000 cycles) | 1 / 5 975 | 11 / 5 944 (0,2 %) |

Le budget est d'environ 27 000 cycles par trame NTSC, hors NMI. Une trame en dépassement retarde l'image d'une trame, sans corrompre l'affichage : la NMI n'envoie rien tant que la trame n'est pas prête.

Les quelques dépassements restants ont lieu sur les trames de frappe, pendant les transitions entre points et au début des ralentis.

Optimisations appliquées :
- sprites, enregistrement des ralentis et trampolines en assembleur ;
- calculs 16 bits uniquement ;
- hauteur au filet sans division ;
- prédictions de l'IA en cache ;
- en double, IA mise à jour une trame sur deux (aussi pendant les balles mortes) ;
- sur une trame de frappe, sprites non redessinés et IA restantes reportées.

## 9. Compilation

Outils : `cc65` 2.19, `python3` (Pillow facultatif), `make` ; pour les tests, `gcc` et `node` ≥ 18.

```sh
make          # génère les graphismes, compile -> build/pixel_open.nes (+ occupation par banque)
make test     # règles, codes, puis suite complète sur émulateur (~10 min)
make clean
```

La compilation est reproductible : deux compilations propres donnent le même fichier (empreinte dans le rapport).

Outils de mise au point :
- `node tests/profile.mjs build/pixel_open.nes [double] [trames]` : cycles par fonction, banques comprises ;
- `node tests/shots.mjs build/pixel_open.nes dossier` : captures d'écran de référence.

## 10. Tests

| Suite | Contenu |
|---|---|
| `tests/rules_test.c` | 42 vérifications des règles |
| `tests/password_test.c` | 200 000 codes aléatoires en aller-retour ; 7,44 millions de codes altérés d'un signe, refusés à 99,99 % |
| `tests/emu_test.mjs` | 86 vérifications : démarrage MMC1 et CHR-RAM, mode d'emploi, menus adaptatifs, présentation, menu de pause, lancer du serveur du haut, let, matchs complets (6 surfaces × 3 lieux × 3 conditions, double), ralentis, revanche, palmarès, 9 exercices (joueur robot via la manette), 5 compétitions (dont éliminations), reprise par code, 50 Hz |

Les résultats détaillés sont dans le [rapport](RAPPORT.md).
