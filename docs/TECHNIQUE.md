# Royal Martinot — Documentation technique

## 1. Vue d'ensemble

| Élément | Choix |
|---|---|
| Plateforme | NES / Famicom, NTSC et PAL (Dendy traité comme PAL) |
| Cartouche | **MMC1 (SNROM, mapper 1)** : 256 Ko de PRG en 16 banques de 16 Ko (128 Ko jusqu'à la version 2.9), 8 Ko de CHR-RAM, 8 Ko de PRG-RAM (avec pile facultative) |
| Langages | C (cc65 2.19) pour la logique ; assembleur 6502 (ca65) pour le démarrage, la NMI, les sprites, les trampolines de banque et l'enregistrement des ralentis |
| ROM | `dist/royal_martinot.nes` (131 088 octets, iNES) |

## 2. Choix de la cartouche

> Depuis la version 2.62, la cartouche est une **MMC3** (mapper 4, carte TGROM) ; ce chapitre décrit le choix initial du MMC1.

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

**Mise sous tension** : l'état initial du MMC1 n'est pas garanti. Chaque banque se termine donc par un petit programme (`STUBn`) qui réinitialise le mapper (ce qui fixe la dernière banque, la 15, en $C000) puis saute au vrai démarrage.

## 3. Organisation mémoire

### Banques de PRG (octets utilisés / 16 368)

| Banque | Contenu | Occupation |
|---|---|---|
| 15 (fixe, $C000) | Démarrage, NMI, primitives (sprites, VRAM, manettes, trampolines `bank_call` / `far_call`, enregistrement des ralentis, affichage des personnages et de la balle), boucle principale, règles, palettes, 17 personnages, noms des compétitions, échange des tuiles de personnages, effets du court, son, échantillons DPCM (voix de l'arbitre, « pok »), bibliothèque cc65 | 15 051 (échantillons DPCM compris) |
| 0 | Moteur de match : physique, frappes, service, arbitrage, IA, fête de victoire | 15 766 |
| 1 | Écrans : titre, configuration, présentation, résultats, fin de set, compétitions, levée de coupe, saisie de code, palmarès | 16 315 |
| 2 | Motifs graphiques (8 Ko), copiés en CHR-RAM au démarrage | 8 192 |
| 3 | Annexes du match : poses des joueurs et arbitre (`poses.s`), score et annonces du club nudiste, chargement des courts, effets du court (`match_fx`), panneau du court Margaux, public (regard, ola, réactions), pluie, ralentis, exercices, tutoriel, IA tactique, adresses des courts en perspective | 16 006 |
| 4 | Mode d'emploi | 3 661 |
| 5 | Compétitions (tirages, simulations, classements, équipes de double), palmarès et codes de reprise (`password.c`, appelé par `far_call`) | 14 726 |
| 6 | Tuiles des personnages « au naturel » (club nudiste, réserve commune de 180 tuiles), copiées en CHR-RAM à la demande ; images 4 à 6 des écrans d'annonce (Martinworld Tour : tuiles, tables de noms, attributs, `assets_gm2.c`) | 10 240 |
| 7 | Générique de lancement | 4 742 |
| 8 | Podium d'une compétition gagnée | 4 600 |
| 9 | Générique de fin | 2 258 |
| 10 | Musiques (`music_data.c`, généré par `tools/gen_music.py`), copiées en RAM de cartouche par `music_play` | 3 960 |
| 11 | Vue en perspective : décors des 7 courts (RLE), tuiles nouvelles par lieu et leurs places, tables de projection (`assets_persp.c`) | 15 066 |
| 12 | Décors des 7 courts à plat (RLE, `assets_courts12.c`) | 3 948 |
| 13 | Jeux de tuiles par phase (célébrations, revers : `assets_swap.c`), coups (`shots.c`), copie rapide `swap_put` | 2 966 |
| 14 | Écrans d'annonce du Grand Martinot et du Martinworld Tour : images 0 à 3, palettes et animations des 7 images, aiguille et flocon (`assets_gm.c`), `gmintro.c` | 12 007 |

### RAM

```
$0010-$00FF  page zéro : NMI, manettes, sprites, banque courante, pile C (sp), cc65
$0100-$01FF  pile 6502
$0200-$02FF  tampon OAM
$0300-$037F  tampon VRAM (96 octets envoyés au plus par trame : budget VBlank)
$0400-$06FF  variables du moteur (742 / 768 octets)
$0700-$07FF  pile de paramètres C
$6000-$77FF  RAM de cartouche : écrans, compétitions, tampon des ralentis, aide des menus, fond des menus (5 771 / 6 144)
$7800-$7FFF  palmarès (sauvegardé si la cartouche a une pile ; signature + somme de contrôle)
```

### Réglages mémorisés

Le bloc `so` (signature « PO », 23 octets d'options, somme de contrôle) suit le palmarès `sv` en RAM de cartouche. Il est écrit à chaque lancement de partie et relu une seule fois, au premier écran titre. Les valeurs sont bornées, et le club nudiste n'est restauré que s'il a été débloqué. Le palmarès garde sa disposition, donc les sauvegardes existantes restent valides.

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
| `src/sprp.s` | fixe | Affichage d'un personnage 16 × 24 (assembleur) |
| `src/gamefix.c` | fixe | Calcul de trajectoire et utilitaires du moteur (déplacés pour libérer la banque 0) |
| `src/screens.c` | 1 | Écrans hors match |
| `src/extras.c` | 3 | Score, courts, public, pluie, ralentis, exercices, tutoriel |
| `src/poses.s` | 3 | Poses des joueurs à chaque trame (assembleur) |
| `src/manual.c` | 4 | Mode d'emploi (8 pages) |
| `src/comp.c`, `src/stats.c` | 5 | Compétitions, palmarès |
| `tools/gen_assets.py` | — | Génère tous les graphismes : police, logo, 3 courts, public assis et debout, personnages × 11 poses (rangées de tuiles partagées) |

### Effets du court (`match_fx`, banque fixe)

Appelé à la fin de `match_draw`, après les joueurs (les effets passent donc derrière eux) :
- **ombres** : une tuile ellipse sous les pieds. Toujours en simple ; en double, seulement pendant un saut, pour rester sous 8 sprites par ligne quand quatre joueurs sont alignés ;
- **poussière** (terre, plage) et **éclaboussure** (pluie) : 12 trames au point de rebond, en deux tailles. Le premier et le deuxième rebond sont détectés par le changement de `bounces` ;
- **chaise d'arbitre** (tous les lieux) : petite chaise de profil, dessinée dans le décor de chaque court, à gauche du filet et centrée sur lui, au bord du court contre le poteau (16 × 32 px, x 32 à 47, y 112 à 143 ; avant la version 2.36, elle était dans les tribunes, x 16 à 31). Pour que ses 2 blocs d'attributs ne contiennent que les abords, la surface de jeu dépasse de 6 px les lignes de côté (au lieu de 10) et les poteaux sont aux bouts du filet. Ses 2 blocs d'attributs sont en palette 3 ($0F noir, $37 beige, $30 blanc ; le beige est aussi la peau des spectateurs du club, de la salle, du château et du parc, en jaune $28 avant la version 2.41). Ni la nuit ni la pluie ne la changent. Le vert classique était impossible : aucune palette de sprites ni du décor n'a de place pour lui ;
- **arbitre de chaise** : 2 sprites de profil, tournés vers le court (palette 2), soit environ 60 % de la taille de la première version (28 px de haut avec la chaise). Tête 11 (regard vers le bas du court) ou 12 (vers le haut), selon la moitié du court où se trouve la balle ; buste 13. Trois tuiles sont chargées par `load_court` à la place de la coupe, inutile pendant un match : buste bras tendu, main, bras levé. `chr_borrow_restore`, appelé par `screen_begin`, les rétablit hors du match. Gestes : bras tendu pendant les premières trames d'une faute de service ou d'un point perdu sur une faute (dehors, filet ; `ump_arm`, positionné par `serve_fault` et `point_over`) ; bras levé qui s'agite sur un let ; bras levé qui salue à la fin du match. `ump_draw` (assembleur, `src/sprp.s`) écrit 2 ou 3 sprites. Il est appelé par `match_fx` et aussi par `rp_play` : l'arbitre ne disparaît plus pendant les ralentis. **Court Margaux** : l'arbitre est une princesse. `load_court` charge un second jeu de 7 tuiles (`ump_chr`, 2 × 112 octets en banque 3) : tuiles 11 à 13 (têtes à couronne et cheveux longs, buste en robe) et 4 tuiles à la place de la coupe (bras tendu, main tenant la baguette à étoile, bras levé avec la baguette, baguette tenue au repos). `ump_princess` demande à `ump_draw` un 4e sprite au repos : la baguette. Mêmes couleurs que l'arbitre (palette de sprites 2), sauf le blanc, remplacé par du rose ($34) sur ce court : robe rose, baguette et étoile dorées (jaune pâle). `chr_borrow_restore` rétablit aussi les tuiles 11 à 13 hors du match. Une première version en C, dessinée à chaque trame dans tous les lieux, faisait déborder des trames déjà chargées. La version en assembleur coûte moins que l'ancien arbitre du stade (médiane de la logique : 7 371 cycles par trame, contre 7 562) ;
- **repère d'atterrissage** (vitesse Lent) : une croix sur `land_x/land_y` pendant 30 trames après chaque frappe destinée à une équipe où joue un humain.

Les tuiles de sprites 5 à 13 (auparavant libres) contiennent l'ombre, la poussière, les éclaboussures, la croix et l'arbitre.

### Margaux (17e personnage) et codes de reprise

Margaux occupe l'index 16, avec le style FANTAISIE (9e style : amorties, lobs, montées au filet). Elle a les cheveux longs, blonds et bouclés :
- **Tuiles** : seule la rangée de la tête (2 tuiles) lui est propre. Elle est générée pour chaque vue et chaque pose (les bras levés passent dans cette rangée), les têtes identiques étant partagées (`marg_map[face × 11 + pose]`), plus 1 tête de profil commune aux deux images de course. Le corps reste celui des joueuses. `spr_person` remplace les deux premières entrées OAM après `spr_meta`, en échangeant gauche et droite en miroir. Les ralentis et tous les écrans en profitent.
- **Couleurs** : peau $27, tenue rose clair $35, cheveux blonds $28 (ΔE ≥ 42 sur tous les courts). Sur le sable jaune ($28, $18 sous la pluie), `pal_set_player` passe les cheveux en blond clair $38 (ΔE 30). Comme la raquette, les chaussures et le liseré de jupe utilisent la même couleur, ils sont dorés. En double, la palette est celle de l'équipe : en partenaire, Margaux garde sa coiffure mais prend les couleurs du joueur 1 de l'équipe.
- **Club nudiste** : Margaux n'y apparaît jamais. Le lieu est sauté au menu quand elle est choisie (et inversement), il est ignoré dans les réglages mémorisés, et un code qui l'associerait au club est refusé.
- **Adversaires tirés au sort** : uniquement parmi les 16 personnages d'origine. Les anciens codes redonnent donc les mêmes tableaux. Son nom fait 7 lettres :
- les écrans à colonnes étroites passent par `team_label` (« MARG » sur 4 lettres, « MX » sur 2 lettres pour ne pas se confondre avec MAYA « MA ») ;
- en double, la présentation met le partenaire sur la ligne suivante.

Couleurs : trois tenues ont changé pour que le rose clair soit unique. IVAN passe du lilas au gris ($10), TOM du lavande au crème ($37), INES du pervenche au chocolat ($07). `tools/contrast.py` vérifie que la tenue de Margaux reste à ΔE ≥ 25 de toutes les autres.

**Codes, version 3** : personnage et partenaire passent sur 5 bits. Pour tenir dans les 52 bits de données, le type de compétition et les conditions sont regroupés (type × 3 + conditions, 15 valeurs sur 4 bits), de même que la surface et le lieu (surface × 5 + lieu, 30 valeurs sur 5 bits). Les codes restent à 13 signes. Les anciens codes (version 2) sont toujours lus. Vérifié par `password_test` : 200 000 allers-retours, plus un ancien code et Margaux en joueuse et en partenaire.

### Fatigue (option FATIGUE)

`tired` (0 à 10) vaut `rally_hits - 10` au-delà de 10 frappes dans l'échange, plafonné à 10 ; il est recalculé à chaque trame. La vitesse humaine perd `3 × tired` (140 → 110 FP par trame, -21 %), celle de l'IA `2 × tired` (-18 à -27 % selon le niveau) : la fatigue est la même pour tous. Une goutte de sueur (tuile 14) clignote au-dessus des joueurs dès le niveau 4 ; en double, seulement au-dessus des humains, pour la limite de sprites. L'option est désactivée à l'entraînement.

### Service canon

Le lancer part toujours avec `bvz = 172` et une gravité de 6 : la vitesse verticale passe par 4, puis -2, aux trames 28 et 29 après le lancer. Un service frappé avec A pendant ces deux trames (`quality == 0`, soit |vz| < 8) est un service canon :
- vitesse 96 au lieu de 64 (balle environ 50 % plus rapide, mesure 352 contre 237 FP par trame), gravité 12. Sur 36 services canon tirés en essai (3 directions, premières et deuxièmes balles), 36 sont tombés dans le carré ;
- dispersion 1 en première balle et 6 en deuxième balle, au lieu de 7 plus l'imprécision du timing ;
- `sfx_canon` (bruit sourd et claquement descendant), bandeau « SERVICE CANON » ;
- traînée de deux points derrière la balle (`match_fx`) tant que le service n'a pas rebondi.

Le CPU Expert le réussit une fois sur deux quand son tirage le fait frapper au sommet (environ 12 % de ses premières balles) ; Facile et Normal jamais. Compteurs : `st_canon` (réussis) et `st_ctry` (services A des humains) par match. Au palmarès, ils sont cumulés dans un bloc séparé de la RAM de cartouche (`sx`, signature « PC ») : la disposition du palmarès ne change pas, et SELECT × 2 les efface aussi.

Taux de réussite attendu pour un adulte (estimation, pas une mesure), avec une régularité de geste de 40 à 60 ms en début de pratique et d'environ 25 ms entraîné : 25 à 30 % au début, 35 à 40 % après une heure ou deux, environ 50 % pour un joueur assidu.

### Frappe parfaite (`perfect_hit`, banque fixe)

Pour les coups lifté, à plat, coupé et croisé court joués après le rebond :
- **Placement** : écart latéral entre la balle et le joueur de 5 à 14 px. La frappe part dès que la balle entre dans la zone ; la profondeur n'est donc pas un critère, car la balle est toujours prise au bord avant de la zone.
- **Timing (humain)** : la frappe doit tomber dans les 3 premières trames du geste (`pl_timer` ≥ 12), c'est-à-dire bouton relâché 0 à 2 trames avant l'entrée de la balle dans la zone (`swing_early`). Si le bouton est encore enfoncé à l'arrivée, la frappe part pendant l'armement : frappe normale.
- **CPU** : tirage 40/256 en Expert et 10/256 en Normal, sous la même condition de placement ; jamais en Facile.
- **Effet** : vitesse +37,5 % (mesuré +41 % face à une frappe normale presque identique), dispersion 1, puissance d'armement ignorée ; `sfx_perfect` ; éclat (étoile blanche, palette 2) au bout de la raquette pendant 12 trames, placé hors du sprite du joueur (sinon il passe derrière lui), à droite ou à gauche selon le côté de la balle ; « COUP PARFAIT ! » si le coup est gagnant.
- **Compteurs** : `st_perf` et `st_ptry` (coups lifté, à plat, coupé des humains) par match ; au palmarès, `sx.pok` et `sx.ptry` (bloc `sx`, signature « PD » ; les compteurs du service canon ont été remis à zéro une fois lors de ce changement).

`can_hit` a été déplacée du moteur (banque 0) vers la banque fixe pour faire la place (606 octets libérés).

### Générique de lancement (`src/intro.c`, banque 7)

Nouvel état de l'application `AS_INTRO`, ouvert à l'allumage (`GO_INTRO`). Sept scènes (160, 160, 160, 230, 240, 180 et 690 trames), chacune construite écran coupé, avec les fondus habituels :
- scène 4, chargement : barre de `#` tracée par le tampon VRAM (20 cases = 100 %), qui marque une pause à 99 % puis déborde du cadre à 117 % (`sfx_canon`, « OVERLOAD ! ») ;
- scène 6, mentions légales absurdes ;
- scène 7, logo : une lettre toutes les 8 trames (`sfx_canon`), or scintillant (couleurs 2 et 3 de la palette 2 du fond), étoiles et confettis en sprites, sous-titres tapés ; ovation ratée (`sfx_clap`, un seul bruit blanc court, et « *CLAP* »). À la trame 320 une balle frappe le N : ses 4 tuiles de fond sont effacées et recopiées dans les tuiles de la coupe des sprites (`letter_to_sprites`, tuiles d'origine sauvegardées par `cup_save` et rétablies par `cup_restore`), la lettre tombe avec rebond (`sfx_net`). Le stagiaire (personnage 0, `spr_person`) entre, s'excuse, la lettre remonte, le fond est réécrit, puis il repart.

Passage : le premier appui affiche « ARE YOU SURE? THIS IS VERY PRESTIGIOUS. » (`sfx_fault`), le second « FINE. » puis l'écran titre 10 trames plus tard. Fanfare `MUS_INTRO` « qui déraille » : 8 mesures majestueuses, puis dièses faux, accélération, arpèges de 4 trames, glissando chromatique de 2 trames par demi-ton, silence de 92 trames et « pouet » final (environ 1 165 trames, carré 50 % et triangle). Le réglage SON mémorisé est relu dès `intro_begin`. Taille : environ 4,7 Ko, dans une banque qui était libre.

### Filet

`draw_net` (outil de génération) : bande blanche de 2 px, mailles aérées (brins verticaux tous les 4 px et horizontaux toutes les 2 lignes, on voit le court à travers), câble du bas, sangle blanche au centre, ombre portée tramée sur le court juste en dessous, poteaux de 3 px avec reflet et tête claire. Le motif des mailles se répète tous les 8 px, donc une seule tuile par rangée en dehors des lignes du court.

**Filet qui tremble** : quand la balle touche le filet (bande ou mailles), `net_shake` passe à 14. `net_tick` (banque 3, après le score car le tampon VRAM est partagé) alterne les deux tuiles de mailles entre leur dessin au repos et un dessin « secoué » (brins décalés, bande affaissée d'un pixel), en CHR-RAM : 2 × 16 octets par changement, deux changements visibles en 14 trames. `load_court` remet le dessin au repos.

### Commandes relatives au côté

`rel_pad` (banque fixe) est appliqué aux directions d'un joueur humain au moment de la frappe : en haut du court (`side_of` = 1), HAUT et BAS sont échangés quand une seule des deux est enfoncée. HAUT veut ainsi toujours dire « vers l'adversaire » (frappe à plat, lob) et BAS « vers soi » (croisé court, amortie). Le CPU choisit ses coups directement et n'est pas concerné. Les déplacements ne changent pas.

### Fête de victoire

Pendant `MS_OVER` (160 trames, « JEU, SET ET MATCH ! »), `win_party` (banque 0) est vraie sauf si le CPU bat un humain (même règle que le choix entre `MUS_VICTORY` et `MUS_DEFEAT`). Dans `match_draw`, chaque joueur de l'équipe gagnante bondit toutes les 16 trames : hauteur `t·(16−t)/8` (8 px au plus) avec `t = ms_timer & 15`, en phase avec l'alternance des poses de joie. La hauteur vient d'une table (`hop_h`), sans multiplication. `win_fx` (`src/sprp.s`, banque fixe, en assembleur) dessine 10 sprites qui tombent à 3 vitesses en se balançant : 5 étoiles (tuile de la frappe parfaite, blanches ou jaunes, visibles sur tous les courts) et 5 confettis. Les positions avancent par simple addition. Une première version en C (multiplication et modulo sur 16 bits) faisait perdre 13 trames sur 120 en double : la version en assembleur n'en perd aucune (mesuré par les tests).

Fanfare `MUS_VICTORY` : 160 trames exactement (mélodie et basse), la durée de `MS_OVER` ; elle s'achève à l'ouverture de l'écran des résultats (à vitesse normale). Feux d'artifice (`gamefix.c`, banque fixe) : `fw_sound`, appelé par la logique (donc jamais sauté en vitesse rapide), lance `sfx_fw_launch` (carré 1, balayage montant) 15 trames avant chacun des 3 éclatements (`ms_timer` = 115, 75, 35), puis `sfx_fw_pop` (bruit court et note grave) ; `fw_draw` dessine la gerbe, 4 étoiles qui s'écartent d'un pixel par trame pendant 14 trames. En double, la fête coûte au plus une trame logique sur 120 (saccade de 1/60 s, invisible), la fin de match en double étant déjà presque au maximum du processeur.

### Panneau de fond de court (court Margaux) et annonces centrées

- **Panneau** : petit panneau de 32 × 16 px (4 × 2 tuiles) dans le bandeau du haut, juste à droite du score de gauche (colonnes 5 à 8, rangées 2 et 3), à l'écart des annonces et du court. Les 3 blocs d'attributs qu'il couvre utilisent la palette des abords du court Margaux (rose clair, magenta, blanc) : face magenta, lettres « MARGAUX » blanches en petite police 3 × 5, liseré clair sur le dessus, rebord et deux pieds pour l'effet de volume, fond du bandeau autour. La table des décors étant pleine, ses 8 tuiles sont chargées en CHR-RAM par `load_court` à la place de 8 tuiles distinctes inutilisées sur ce court. L'outil de génération les choisit (`sign_idx`), et `load_court` rétablit leur dessin d'origine (`sign_orig`) sur tout autre court. Effet de bord accepté : dans ces blocs, le dernier caractère du score de gauche (`*`, rond de set) et la première colonne des annonces ont une ombre rose clair au lieu de noire.
- **Panneau du club nudiste** : même principe, même emplacement, « BRONZES » (club « Les Bronzés ») en lettres noires sur une face jaune soleil, liseré blanc, rebord et pieds noirs (palette 3 du décor : noir, jaune, blanc ; les ombres du score voisin restent noires). Ses 8 tuiles (`sign2_idx`) sont prises parmi le décor des autres courts (public du stade, arbres, murs), hors panneau Margaux, et rétablies (`sign2_orig`) ailleurs. Le texte « CLUB NUDISTE » du bandeau est supprimé.
- Toutes les annonces du bandeau sont centrées sur leurs 14 caractères (`center14`, banque fixe), y compris celles de l'entraînement, du tutoriel et du club nudiste. Les deux lignes du récapitulatif du score ont la même longueur et restent alignées.

### Bandeau devant le joueur du haut

Au service, le joueur du haut a les pieds 2 px derrière sa ligne de fond (y = 46) : sa tête (y 22 à 29) entre dans le bandeau du score et des annonces (lignes 0 à 31). `spr_person` donne alors aux 2 sprites de sa rangée de tête la priorité « derrière le fond » (bit 5 de l'attribut) quand cette rangée est entièrement dans le bandeau. Le fond du bandeau est transparent : les lettres des annonces passent devant et restent lisibles, et la tête reste visible entre elles. Le reste du sprite, sur le court, garde la priorité normale. Le terrain et la position de service ne changent pas.

### Récapitulatif du score

- **Après chaque jeu** (et au début d'un jeu décisif) : `recap_pend` est armé dans `after_point`. Quand l'annonce « JEU » s'efface, `score_recap` (banque fixe) écrit une ligne par équipe dans le bandeau pendant 120 trames : nom sur 4 lettres, jeux de chaque set terminé, puis jeux du set en cours (`score_line`, 14 caractères pour 5 sets).
- **Après chaque set** (hors démonstration) : `match_tick` renvoie 2, et la boucle principale ouvre l'écran `SCR_SETBREAK` (banque des écrans). Il affiche le tableau des sets, le vainqueur du set, les statistiques et le joueur au service, sur l'interlude `MUS_BREAK` (8 mesures en boucle, carré 25 %). N'importe quel bouton des deux manettes, ou 600 trames, demande `GO_RESUME` sans fondu (`fade_skip`) : retour au court en 4 trames. `match_resume` coupe la musique, recharge le court, les palettes et les tuiles (y compris au naturel), puis affiche le récapitulatif dans le bandeau. L'état du match n'est pas touché.
- **En fin de match** : le même tableau des sets (`set_table`) ouvre l'écran des résultats.

### Fondus entre les écrans

La NMI et `ppu_on` envoient la palette assombrie de `fade_lvl` (0, 16, 32, 48 ou 64, soit un cran de luminosité NES par 16 ; en dessous du plus sombre, noir). `ppu_off` fait un fondu au noir en 3 crans de 2 trames, puis coupe l'écran. Le nouvel écran s'allume noir, et `logic_tick` le rallume d'un cran toutes les 2 trames. Quand le même écran est reconstruit (pages du mode d'emploi ou du palmarès), `fade_skip` supprime le fondu : la navigation reste instantanée et aucun appui n'est perdu. Au menu, un changement de MODE (ou de compétition, d'exercice) ne coupe plus l'écran : `menu_soft` recalcule la liste, et `menu_soft_tick` réécrit les lignes par le tampon VRAM, environ deux par trame. Les lignes devenues inutiles sont effacées. Il n'y a plus d'éclair noir.

**Budget du VBlank** (NTSC : environ 2 270 cycles). Il comprend le DMA des sprites (513), le tampon VRAM (jusqu'à environ 1 450 pour 96 octets) et la palette (environ 740 avec fondu, 480 sans). Les trois ensemble débordaient sur le haut de l'image, ce qui donnait des traits horizontaux pendant les fondus et quand on change de ligne au menu. Désormais, si le tampon dépasse 32 octets, la palette attend la trame suivante (`pal_dirty` reste à 1). Hors fondu, la palette est copiée sans calcul. **Palette et rendu coupé.** Quand le rendu est coupé, la NES affiche la couleur pointée par l'adresse VRAM, si elle est dans la palette. `ppu_on` envoyait la palette du nouvel écran immédiatement, donc pendant que l'image se dessinait : chaque allumage d'écran (changements de scène du générique, transitions) montrait une image striée de traits de couleur. jsnes ne reproduit pas cet effet, mais la console et les émulateurs précis si. `ppu_on` attend maintenant le début du VBlank (`wait_frame`, rendu encore coupé) pour écrire la palette. Un test intercepte les écritures en $2006 et $2007 : avant la correction, 160 écritures de palette pendant l'affichage au cours du générique, 0 après. Un autre test d'émulation (`tests/emu.mjs`, interception des écritures en $2005 à $2007) relève la position de la dernière écriture de chaque NMI. Après correction, la marge mesurée est d'au moins 600 cycles partout : générique, titre, menus, transitions et match.

### Rotation de l'ordre des sprites (seulement si nécessaire)

Le PPU n'affiche que les 8 premiers sprites d'une ligne (dans l'ordre de l'OAM). En double, quand au moins 3 joueurs sont dans une même bande de 24 px (les quatre au filet, par exemple), une ligne peut porter jusqu'à 12 sprites, ombres comprises. `match_draw` fait alors commencer l'écriture des sprites à `spr_rot = frame_cnt × 36`, soit 9 sprites plus loin à chaque trame : ce n'est plus toujours le même sprite qui est écarté, il clignote au lieu de disparaître. Sinon `spr_rot = 0` et l'ordre reste fixe. `spr_meta` écrit ses deux sprites l'un après l'autre pour que l'index reboucle sur 256 octets (sinon, un personnage à cheval sur la fin de la table débordait sur le tampon VRAM et bloquait le jeu ; trouvé par le test). Les ombres sont donc affichées aussi en double.

### Affichage des personnages en assembleur (`src/sprp.s`)

`spr_person` était la fonction la plus coûteuse de chaque trame (environ 570 cycles par personnage, passage des 7 arguments compris). Sa version assembleur fait la même chose : rognage, tuiles, course de profil, bruit de pas, tête de Margaux. Mesure en double, 6 000 trames : trames d'échange en dépassement passées de 90 à 27, aucune en simple.

### Retour sur le timing (entraînement)

`serve_fb` (service A : écart au sommet en images, d'après la vitesse verticale, qui perd 6 par trame) et `perfect_hit` (placement, puis `swing_early` = valeur de `pl_timer` à l'impact, 0 si le bouton est encore enfoncé) écrivent l'annonce « TROP TOT DE n IMAGES », « TROP TARD DE n IMAGES », « RELACHEZ LE BOUTON AVANT », « BALLE TROP PRES/LOIN DU CORPS », « COUP PARFAIT ! » ou « SERVICE CANON ». Uniquement en mode entraînement.

### Smash sauté

Une balle entre 40 et 62 px de haut, avant le rebond, est jouable si le joueur est à moins de 70 px du filet. C'est réservé aux humains et au CPU Expert, pour ne pas enlever le lob aux joueurs moyens. La frappe est plus rapide (96 au lieu de 80) et plus tendue (gravité 12). Le sprite saute de 12 px (j²/16), au plus haut à l'impact, puis retombe en 14 trames (`pl_jump`). j/4 est rangé dans les bits 6 et 7 de `pose_cache` (la pose occupe les bits 0 à 3 depuis qu'il y a 11 poses), que `rp_record` enregistre déjà : le saut apparaît dans les ralentis sans agrandir le tampon (16 octets par trame).

Le CPU Expert sait l'utiliser. Sur un lob adverse peu profond, il vise le point situé environ 11 trames avant la chute, où la balle passe vers 50 px de haut. Il ne le fait pas toujours : la décision suit l'erreur d'anticipation tirée à chaque frappe, soit environ 7 fois sur 10. Mesure : 17 lobs sur 30 sont smashés en sautant ; contre Normal, aucun.

### Physique et arbitrage

- Positions en virgule fixe 10.6 (1/64 de pixel), hauteur `z` séparée.
- Gravité selon le coup ; restitution et adhérence selon la surface, l'effet et la pluie.
- Amortie : vitesse 40, gravité 5 (trajectoire basse juste au-dessus du filet) et effet propre `SPIN_DROP` : au rebond, restitution et adhérence divisées par 2, la balle « meurt » près du filet. Risque selon la distance d au filet : profondeur visée 22 − d/8 px et dispersion + d/8 px. Jouée de loin, elle peut rester dans le filet ou être rattrapée.
- `launch()` atteint la cible en T trames et contrôle sans division le passage au-dessus du filet.
- **Filet** :
  - balle sous la bande : renvoyée vers le frappeur ;
  - balle qui frôle la bande (2 unités sous son sommet) : elle passe, ralentie. Au service, si elle tombe ensuite dans le carré, c'est un **let**.
- `rules.c` gère les points, jeux, sets, jeu décisif, services et changements de côté. En double, le serveur change au sein de l'équipe chaque fois qu'elle récupère le service.

### Intelligence artificielle

- **Trois niveaux** (vitesse, réaction, dispersion, marge par rapport aux lignes, puissance).

  | Paramètre | Facile | Normal | Expert |
  |---|---|---|---|
  | Vitesse (px/trame) | 1,16 | 1,34 | 1,72 |
  | Délai de réaction (trames) | 22 | 16 | 6 |
  | Dispersion de visée (px) | 34 | 26 | 16 |
  | Marge par rapport aux lignes (px) | 28 | 22 | 11 |
  | Puissance | nulle | très faible | forte |
  | Imprécision d'anticipation (px, tirée à chaque frappe adverse) | ±18 | ±12 | ±5 |
  | Faute directe | 1 frappe sur 4 | 1 sur 8 | 1 sur 16 |

  Zone de frappe (tous les joueurs) : ±24 px en largeur, de -15 à +10 px en profondeur, jusqu'à 40 px de hauteur.

  **Calibrage** (versions 2.3 puis 2.4, après deux retours de test : « Normal un peu trop difficile », puis « encore très difficile ») : `tests/calib.mjs` fait jouer un robot « humain » contre l'IA. Ce robot réagit en 9 à 16 trames, se place avec une erreur qui se corrige à l'approche de la balle et frappe dans des directions variées. Sur 4 parties par niveau (v2.4), il gagne environ 94 % des points contre Normal, avec des échanges bien plus courts qu'avant, et environ 53 % contre Expert. Le robot étant plus régulier qu'un humain, Normal doit maintenant être abordable et Expert rester exigeant.

- **Huit styles** (CDC 10.2), propres à chaque personnage. Ils modifient :
  - la puissance, la marge et la dispersion ;
  - la fréquence de montée au filet, de lobs, d'amorties et de coupés ;
  - la profondeur du replacement.
- **Adaptation** (CDC 10.4) : pendant le match, l'IA compte, pour chaque équipe humaine, les balles jouées à gauche ou à droite et les frappes au filet.
  - Dès 6 frappes observées, aux niveaux normal et expert, elle décale son replacement vers le côté préféré du joueur (au plus 16 px).
  - Si le joueur monte souvent au filet, elle lobe davantage.
  
  Les compteurs repartent de zéro à chaque match ; le comportement reste borné et déterministe.
- **Tactique selon le score** (version 2.39) : `ai_tactic` (banque 3) est appelée une fois par point, quand les joueurs se placent pour servir. Pour chaque équipe, elle fixe des corrections lues par l'IA de frappe :

  | Situation | Puissance | Marge | Fautes directes | Montées au filet | Service |
  |---|---|---|---|---|---|
  | L'adversaire a une balle de set ou de match | −2 | +6 | deux fois moins | — | — |
  | Balle de set ou de match pour soi | +4 | — | — | +60/256 | — |
  | Mené de 2 jeux ou plus (un set compte pour 3 jeux) | +5 | −3 | — | +50/256 | — |
  | Devant de 2 jeux ou plus | — | +3 | deux fois moins | — | — |
  | Balle de break contre soi, au service | — | +4 | deux fois moins | — | première balle coupée (assurée) |
  | Balle de break pour soi, en retour | +3 | — | — | +30/256 | — |

  Les fautes directes passent de 1 sur 4/8/16 à 1 sur 8/16/32 (masque `ai_fault | tac_err`).
- **Double** : un responsable par balle (le partenaire le plus proche du point de frappe prévu). L'autre couvre sa moitié du court.

### Compétitions (`comp.c`)

- **Déterminisme** : une graine de 8 bits fixe le tirage des adversaires, leur niveau (0 à 2) et l'issue de tous les matchs entre joueurs CPU. La probabilité de victoire vaut 50 % ± 20 % par écart de niveau.
- **Reprise** : le code n'a besoin que de la graine, de l'étape et des 8 bits de résultats du joueur. Tout le reste est recalculé à la reprise.
- **Formats** :
  - **tournoi** : tableau à 8 ;
  - **poules** : calendriers 4 × 3 journées, puis demi-finales croisées ;
  - **championnat** : méthode du cercle à 6 joueurs ;
  - **saison** : 4 tableaux différents, un par surface, et barème de points.

### Le Grand Martinot (`CT_GRAND`, version 2.44)

`comp.c` (banque 5) : 16 inscrits en simple (`c_n` = 16, niveaux `rt16` : 4 forts, 7 moyens, 4 faibles), 8 équipes en double. Les tableaux des inscrits (`c_pl`, `c_rt`, `c_pl2`, `ko_cur`) passent à 16 entrées, en RAM de cartouche ; `reach`, `order` et `pts` restent en RAM interne (16 entrées). Étape = tournoi × tours + tour (`ev_of`, `rd_of`, calculés sans division : les routines de division de cc65 iraient dans la banque fixe, pleine). Tours gagnés par le joueur : 3 bits par tournoi dans `c_gres`. Surfaces `grand_surf` = dur, terre, gazon, dur ; noms `grand_name`. Points `grand_pts` = 180, 360, 720, 1 200, 2 000 (en double, à partir des quarts). `c_slam` : les 4 tournois gagnés ; le podium écrit « GRAND CHELEM ! », le générique « GRAND SLAM CHAMPION ». Les noms des compétitions (`comp_cname`) et leurs explications (`comp_desc`, 4 lignes de 28 signes) sont en banque 5 ; le menu (`menu_info_tick`) les écrit une ligne par trame quand le curseur est sur la ligne COMPETITION.

**Écrans d'annonce** (version 2.44) : `tools/gen_gm.py` dessine les 4 scènes en couleurs NES (224 x 128 px), choisit pour chaque bloc de 16 x 16 px la meilleure des 4 palettes de la scène, partage les tuiles identiques (101, 86, 158 et 126 tuiles ; 154, 133 et 115 pour les 3 images du Martinworld Tour : sous la limite de 160, sinon les plus proches seraient fusionnées) et écrit `src/assets_gm.c` (banque 14, avec `assets/preview_gm.png`). `gmintro.c` (banque 14) : `gm_intro_build` copie les tuiles en $60-$FF de la table du décor (`bg_dirty` : rétablie par `chr_borrow_restore`), la table de noms (rangées 6 à 21), les attributs et les palettes, écrit les textes et joue la fanfare (`MUS_GM1` + tournoi, 11 à 14, générées par `tools/gen_music.py`) ; `gm_intro_tick` fait tourner des couleurs (eau de Sydney, lumières de la tour Eiffel, fenêtres de New York) et dessine la grande aiguille de Big Ben, un sprite qui prend 8 directions avec 3 tuiles retournées, posées à la place de la coupe. `comp_gm_intro` (banque 5) dit s'il faut annoncer un tournoi (premier tour, pas encore annoncé à cette étape) ; `comp_next` (écrans) mémorise le tournoi à annoncer (`gm_ev`, $FF : aucun) ; depuis la version 2.56, l'image (`SS_GMI`) s'affiche après la présentation du match (A sur `SS_PRESENT`), juste avant son début : A recharge les motifs d'origine et lance le match. Le Grand Martinot se joue toujours au stade, de jour.

### Le Martinworld Tour (`CT_WORLD`, version 2.47)

Type 6 : 7 tournois de 8 inscrits (8 équipes en double), un par surface, toujours au stade, de jour : Open d'Augustralie (dur), Margaux-les-Neiges (glace), Debogarros (terre battue), Martinbledon (gazon), Rio de Bruneiro (plage), US Opaul (dur), Deborcy Masters (moquette). Les 7 tournois partagent leurs tables avec le Grand Martinot : `ev_surf` et `ev_name` (par image, 0 à 6), `world_ev` (ordre du Martinworld Tour), `ev_img` (tournoi → image). `nev()` vaut 4 ou 7 ; `c_gres` garde 2 bits par tournoi (0 à 3 tours gagnés, 14 bits ; `gres_w`, décalages par boucle). Format (version 2.53) : `comp_surface` impose aussi `c_fmt` (1 : 3 sets, 2 : 5 sets en finale, `rd_of() == nrounds() - 1`). Points `grand_pts` à partir des quarts : 360, 720, 1 200, 2 000, jusqu'à 14 000 ; le classement s'écrit sur 5 chiffres (`num5`). `c_slam` : les 7 tournois gagnés. L'écran de fin lit le classement avec `c_stage` = 99 (le tableau s'arrête au dernier tournoi).

Écrans d'annonce : `comp_gm_intro` rend l'image (0 à 6) ; `gmintro.c` en déduit le rang (`gmi_rank`) et la surface, écrit « TOURNOI 2/7 : GLACE », joue la fanfare `MUS_GM1` + image (15 à 17 : tyrolienne, samba, entrée des champions). Les images 4 à 6 sont en banque 6 (`gm_bank`) : la table de noms et les attributs sont copiés avec `chr_copy` depuis leur banque. Margaux-les-Neiges : la neige tombe (12 sprites, tuile flocon posée à la 4e place de la coupe, couleur blanche). Rio de Bruneiro : la mer scintille (palette 3 : à égalité, un bloc prend la première palette, et les nuages ne doivent pas prendre celle qui tourne). Deborcy Masters : 3 faisceaux de projecteurs de 3 couleurs qui tournent.

Palmarès : `SaveData.comp_best` n'a que 5 places (types 0 à 4). Jusqu'à la version 2.46, le rang du Grand Martinot s'écrivait dans l'octet suivant, `nude` (une 3e place débloquait le club nudiste). Les meilleurs rangs des types 5 et 6 sont maintenant dans un bloc à part, `sw_best` ('B', type 5, type 6, contrôle), remis à zéro avec le palmarès.

### Séquences animées du match (`src/cer.c`, banque 13, version 2.52)

`cer` (phase) et `cer_t` (trames) : 1 entrée (marche depuis le tunnel x 216, y 236, par le couloir de droite puis vers la place, sorties échelonnées de 16 trames), 2 salut (raquette levée, `P_SERVE` / `P_IDLE`), 3 pièce (tuile de la balle, grande puis écrasée, en arc depuis la main de l'arbitre ; « PILE ! » / « FACE ! » et « XXXX SERT ») ; `CER_CHAIRS` ($FD, version 2.54 : chaises affichées dès la fin de l'échange qui amène un changement de côté, `cer_watch` et `change_next`, sans remplacer le jeu), 4 vers les chaises (x 46, y 100 / 158, 88 / 170 en double, du côté où se trouve le joueur), 5 assis (`POSE_SIT` : spr_person le dessine de profil, image 4 de `prof_map`, selon le miroir ; chaise de profil `chair_tile13`, tuile permanente ; serviette en écharpe et gourde devant), 6 vers la nouvelle place (couloir x 46, à côté du poteau) ; 7 fin du match (0-59 fête habituelle, 60-129 `POSE_VICTORY`, 130-199 `POSE_SUPINE` et raquette au sol), 8 vers le filet (bas à gauche du centre, haut à droite, paires décalées de 26 px en double), 9 poignée de main (`POSE_SHAKE`, en miroir pour le joueur du haut), 10 salut à l'arbitre (tournés vers la gauche). Appels : `cer_entry` (match_begin), `cer_save` / `cer_change` (after_point, changement de côté sans fin de set), `cer_end` (balle de match), `cer_tick` (match_tick : remplace le jeu tant que `cer` ≠ 0), `cer_front` et `cer_draw` (match_draw : accessoires devant et derrière les joueurs). Les poses sont imposées par `pose_force` (bit 7 : miroir), lu en tête de `choose` (poses.s). A ou START passe une séquence ; `cer_skip`, mis à 1 à chaque trame par le banc d'essai (`Emu.cerSkip`), les passe toutes. Variables dans la RAM de cartouche libre du bloc SAVEDATA (non sauvegardées). Jeu de tuiles F (`SET_F`, poses 28 à 31 et accessoires `prop_tile`) : chargé par `swap_tick` pendant les séquences (en fin de match, après la fête habituelle, qui garde le jeu C). Les poses 27, 29 et 31 sont dessinées 4 px plus bas (`POSE_LOW`).

### Bustes « bras caché » de la vue de dos (version 2.52)

Le dessin des personnages est commun aux vues de face et de dos (rangées de tuiles partagées). Pour les 9 poses où un bras passe devant le buste (`XP`, calculé par gen_assets.py : 8, 10, 11, 13, 22 à 26), le générateur produit la rangée du buste « bras caché » (`HIDE_ARM`) pour chaque silhouette : `xrow1` (32 octets par pose et silhouette) et `xpose_idx` (assets_swap.c, banque 13). `slot_tick` (cer.c), appelé par match_draw après le calcul des poses et par rp_play : pour chaque joueur vu de dos dans une de ces poses, si le jeu R est complet (`swap_on` = 2 : la fin de la zone partagée, `slot_tile13`, 2 x 2 tuiles, est libre) et sans tenue au naturel, les 2 tuiles sont mises dans le tampon VRAM (`swap_put`) si la case ne les contient pas déjà (`slot_have`), et `row1_ovr[p]` reçoit la tuile de la case ; `spr_person` (sprp.s) prend alors cette tuile pour la rangée du buste. Tuiles et sprites arrivent dans la même image (même NMI). `swap_restore` remet `row1_ovr` à zéro pour les écrans.

### Habillage des menus (`src/menuart.c`, banque 8, version 2.57)

`tools/gen_menu.py` produit la police surlignée (le texte, couleur 3, passe en couleur 2 ; tuiles $60-$9F), les ornements dorés ($A0-$A7 : filet en relief, losange, extrémités, étoile), les lettres doublées des titres (4 tuiles par caractère, `src/assets_menu12.c`, banque 12) et les tables d'attributs des écrans. Ces tuiles occupent pendant les menus celles des courts : `menu_deco_begin` (appelée par `screen_begin`) les charge et met `bg_dirty` à 2 ; `chr_borrow_restore` ne rétablit alors que le logo ($CE-$FB), et `load_court` (extras.c) recharge toute la table du décor avant le match. `txt_off` (gfx.c) ajoute $40 aux caractères écrits par `txt_at` et `vbuf_text` : texte surligné (ligne choisie des réglages, joueur et en-têtes des tableaux, nom du tournoi). Palettes des menus (`pal_set_menu`) : fond vert $0A, ombre noire, couleur 2 surlignée, couleur 3 : blanc, bleu pâle (libellés), jaune (valeurs), or. `deco` (screens.c) copie le titre dans `deco_str` puis appelle `deco_title` (lettres doublées aux rangées 1-2, filet rangée 3) et `deco_frame` (attributs, filet du bas). Réglages : la ligne choisie est réécrite surlignée par le tampon VRAM (`menu_hl_tick`, en attente si le tampon est plein), flèches autour de la valeur. Écran de compétition : `comp_path` (banque 5) écrit le parcours, `ent_at` surligne le joueur, médailles en sprites. Le palmarès (`palmares_build`) est passé en banque 8.

**Affiche VS** (`vs_build`) : bustes générés par `tools/gen_teaser.py` (pose bras croisés, version en miroir pour la droite, homme et femme ; `src/assets_vs.c`, banque 10), 2 palettes par buste (A : sombre, peau, tenue ; B : tenue ombrée, tenue, blanc, où s'écrit le texte blanc). Contours et yeux en couleur sombre (la couleur 0 est le fond). Variantes : `vs_g*` (Margaux, fillette) et `vs_p*` (Papi) aux cheveux clairs, avec leurs palettes A (noir, peau, cheveux) et B (noir, peau, tenue) ; leur texte prend la palette blanche de l'autre côté. Les images de la bande-annonce ont une 3e variante (fillette, `tz_art[TZ_N][3]`) et occupent aussi les banques 6 et 7. Tuiles : gauche $60-$9F puis $01-$1F (`tz_unpack` reprend où il s'était arrêté, `tz_src`), droite $A0-$FF. Fond : couleur de la surface.

**Écran titre** : reflet (couleur 2 de la palette du logo) toutes les 128 trames, scintillement de l'édition. **Ouverture des menus** : pendant le fondu d'ouverture, `scroll_y` = `fade_lvl` / 4 (main.c) : l'écran glisse vers sa place.

### Menus, version 2.58

- **Tuiles des menus** : $A8-$AB traits du tableau des tournois, $AC-$AF accolades des réglages, $B0-$FB lettres doublées des titres (19 lettres distinctes au plus). Les données (police surlignée, ornements, traits, accolades, attributs, court de l'écran titre) sont en banque 13 (`assets_menu.c`) ; `menu_bigchars` et la taille du court restent en banque 8 (`assets_menu8.c`), lus directement.
- **Aide des réglages** (`tools/gen_help.py` → `assets_help.c`, banque 14) : `menu_help` remplit `comp_l1` avec la ligne 0 ou 1 de l'aide de `help_o` / `help_v` ; `menu_info_tick` (screens.c) redessine les rangées 24 à 27 par le tampon VRAM quand la ligne ou sa valeur change. Variables en RAM de cartouche (la RAM interne est pleine ; `menu_help` utilise des variables `register`).
- **Accolades** : `grp_tile` (banque 8) donne la tuile de la ligne i selon le groupe de sa ligne et de ses voisines.
- **Portraits** (`tools/gen_port.py` → `assets_port.c`, banque 13) : 4 dessins de 4 x 3 sprites (homme, femme, Margaux, Papi) et 7 vignettes de lieu (2 sprites), copiés par `port_load` à la place des tuiles des personnages ($10-$4D) ; `people_dirty` les fait rétablir (992 octets) au prochain `people_tiles`. `spr_port` (banque fixe) affiche un portrait, retourné à droite ; `port_pal` règle sa palette ; `menu_marks` affiche la pastille de surface et la vignette du lieu (palette 3, couleur 2 propre au lieu).
- **Affiche VS** : `vs_anim(t)` (banque 8), appelée à chaque trame après le fondu (`brk_t`, 48 trames) : palettes de chaque côté au fond, en blanc puis en noir avant les couleurs (`vs_pals`) ; « CONTRE » écrit à t = 32 avec un flash du fond.
- **Tableau à élimination** (`comp_draw_table`, banque 5) : pour chaque tour b, `ko_run(0, 3, b)` donne les inscrits ; noms en colonne 2 + 6b, rangée 1 + i·2^b + 2^b − 1 ; coin, verticales et jonction en colonne + 5.
- **Palmarès** : `palmares_sprites` (banque 8) met une médaille (sprite de la balle, palettes or, argent, bronze) devant les rangs 1 à 3.
- **Écran titre** : `title_court` copie les tuiles du court ($60, à la place de la police surlignée) et sa carte (rangées 8 à 13), attributs des rangées 12-13 en palette 2.
- **Sons** : `sfx_menu_val(droite)` (glissando) et `sfx_menu_back`.
- **Tests** : `seedHook` (emu_test.mjs) fixe `rand_seed` la trame qui suit l'appui lançant un match (A sur la présentation, START sur les réglages) : les matchs des tests sont reproductibles malgré la durée variable des menus.

### Version 2.59 : lob, trames en retard, fond des menus

- **Lob** (`shot_select`, banque 13) : profondeur 60 + min(armé, 22) ; vitesse 19 si le frappeur est à 80 px ou plus du filet (lob défensif, plus haut), 31 à moins de 60 px (lob d'attaque), 24 sinon ; gravité 6, lift. `perfect_hit` accepte le lob : lob parfait = profondeur + 4, écart 1, sans gain de vitesse. IA (`predict`, banque 0) : un joueur au filet fait demi-tour sur un lob s'il est Expert et `ai_err & 1` (une fois sur deux) ; la prédiction ordinaire (point de chute + 12 trames) le mène derrière le rebond.
- **Trames en retard** : `dive_try` (banque 13) sort tout de suite si la balle est à plus de 56 px (devant) ou 60 px (derrière) en profondeur, ne décide pour l'ordinateur qu'une trame sur deux (sur quatre en double), et sa boucle utilise des variables `register` (page zéro). L'ola (`crowd_tick`, banque 3) écrit une rangée par trame. `move_toward` est en assembleur (`src/fastmove.s`, banque fixe). Hauteur du saut : table `jump_h` (j² / 16). `can_hit` teste la profondeur avant la largeur. `score_line` et `score_recap` sont passés en banque 0. Mesure : `PC` hors de `wait_frame` à la fin d'une trame ; profil par fonction (cycles des trames en retard, routines de la bibliothèque C imputées à leur appelant).
- **Fond des menus** : `menu_bk` (RAM de cartouche, dernier octet libre) tiré par `menu_bk_pick` au premier écran titre parmi $07, $0A, $02, $05 ; `pal_set_menu` l'utilise pour les couleurs 0 ; ombre des lettres dorées en noir sur $07.
- **Chaises** : `cer_watch` (banque 13) n'affiche pas les chaises tant que `rp_pending` (ralenti prévu ou en cours).
- **Aide des réglages** : touches « HAUT/BAS: LIGNE  <>: VALEUR » et « START : JOUER   B : RETOUR » (`comp_help`, banque 5).

### Version 2.60 : coups de A, réglages centrés

- **Lifté** (`shot_select`, banque 13) : profondeur 62 si armé < 4, sinon 70 ; gravité 7 + armé / 8. **Croisé court** : armé borné à 24 (T) ; vitesse 34 + T / 4 ; `hit_margin` = 14 − T / 3 (distance à la ligne de côté, lue par `do_hit`) ; armé transmis divisé par 2 (imprécision). Smash dans l'angle : `hit_margin` = 8. `hit_margin` n'est plus statique (game.c).
- **IA** (`ai_update`, banque 0) : à partir de Normal, après le lob et l'amortie, croisé court avec une probabilité 90/256 si l'adversaire est au filet, 20/256 sinon.
- **Réglages** : première rangée `MY0` = 4 + (18 − `nvis`) / 2 (screens.c ; même calcul dans `menu_marks`) ; `menu_soft` réécrit les 18 rangées ; portraits en y = `MY0` × 8 − 30, entre 7 et 16. Tests : `menuY(e, i)`.

### Version 2.72 : carrière, fiche, reprise, cérémonie

- **Banque 19** (`career.c`) : textes de l'affiche VS (déplacés de screens.c, banque 1 pleine) et fiche (`vs_texts`, `fiche_tick` : une rangée par trame quand le tampon VRAM a 32 octets libres ; tables taille, gaucher, classement), carrière (`car_rank`, `car_comp_end`, `car_match_setup`, `car_end_text`, `car_pal_text`), sauvegarde du match (`msave_*`). Tampon de texte : `comp_l1`. Variables en WBSS.
- **Sauvegardes** (stats.c, SAVEDATA) : `sw_h2h[23]` ('H', un octet par personnage : victoires × 16 + défaites, contrôle), `sw_car[12]` ('C', points, compétitions, titres, meilleur classement, derniers points, option, contrôle). Ils sont définis après `sw_spd` et ne sont déclarés dans aucun en-tête : cc65 range les variables dans l'ordre des déclarations `extern` puis des définitions, et un `extern` dans stats.h aurait déplacé `sw_best`, `so`, `sx`, `sw_view` et `sw_spd` (palmarès perdu sur les cartouches existantes). Test : adresses.
- **Carrière** : gain = base(rang) × 4 × type × niveau / 16 (base 100, 60, 35, 35, 18… ; type 4 à 12 ; niveau 2, 4, 6) ; classement = 1 + 59 400 / (600 + points) (16 bits) ; qualités : n = points / 400 (15 au plus), service (n + 2) / 3, vitesse (n + 1) / 3, précision n / 3, 5 au plus. `car_srv` (vitesse du service, `serve_hit`), `car_spd` (`human_update`), `car_acc` (dispersion, `shot_select`), posés par `court_post` pour le joueur 1 humain d'une compétition en carrière. Option : `opt_val[25]` (`O_CAREER`), gardée dans `sw_car[8]` par `so_save` / `so_load`.
- **Reprise** : `pod_buf[144]` (podium.c : ses `src`, `out`, `at` ; libre hors du podium) reçoit à chaque fin de point (`hl_point_ev` → `msave_write`) 'M', le mode du menu, `cfg`, le score (`sc_*`) et les statistiques (`st_*`, `srv_slot`), puis un contrôle. Titre : `msave_ok` ; B : `msave_load_cfg` (cfg, `mresume`), affiche VS, A ; `cer_entry` → `msave_restore` (si cfg identique) puis `ms = MS_DEAD`, `ms_timer = 1`, `end_reason = END_LET` : `after_point` replace les joueurs. Effacée par `after_match` et par `cer_entry` d'un autre match.
- **Changement de côté** : `band_pg` (0 score, 1-4 statistiques), page suivante toutes les 50 trames tant que `cer == CER_SEAT` ; assis 250 trames en match (cer.c).
- **Plongeon sur canon** : `dive_try` (shots.c) : `cn = srv_canon && b_serve` ; humain 54 px, ordinateur portée + 8 et probabilité 250/256 ; `dive_canon` = joueur + 1. `hl_hit` : coup 4 (plongeon) de ce joueur → `hl_cdv[t]`, message ; `hl_select` : si le point est gagné par ce plongeon → « PLONGEON / GAGNANT ! », `rp_pending = 1`.
- **Podium** : `pod_extra_build` (tuiles $B1-$B2 du décor : spectateurs, colonnes 0-1 et 30-31 ; tuiles de sprites $3C-$3D : plaque) et `pod_extra_tick` (cup_t ≥ 24 : public debout, bras qui alternent toutes les 16 trames ; plaque, OAM 62-63, palette des médailles), banque 15 ; confettis : palettes 0 (vainqueur) et 2 (or). `palmares_sprites` passe en banque 15 (banque 8 pleine).
- **Mode d'emploi** : séquences (pose, durée, balle) par page, `spr_person` à (220, 40) ; `people_tiles(0)` au dessin de la page.
- **Correctif 2.71** : `court_post` (chargement du court, rendu coupé) appelle `band_init` puis `sun_apply` ; `sun_apply` seule à la fin des jeux (avant : `band_init` écrivait dans la VRAM écran allumé).

### Version 2.71 : bandeau du score, crépuscule, voix, record de vitesse

- **Bandeau du score** (highlight.c, banque 15) : `band_init` (au chargement du court, par `sun_apply`) écrit la tuile $5F (filet : trait blanc, ombre noire ; « _ » n'est jamais écrit en match et reste hors de `PERSP_NOTXT`), puis les rangées 24 à 29 de l'écran 1 ($2400 : filets en 25 et 28, texte en 26 et 27) et leurs attributs (palette 1). `band_draw` (fin de `match_draw`, chaque trame) : visible si match, pas de pause ni de séquence du ralenti, et `ms == MS_DEAD` (hors séquence, ou chaises qui apparaissent) ou joueurs assis (`cer == CER_SEAT`). `band_vis` : 1 texte (une rangée de 28 octets par trame, `vbuf_raw`, quand le score change), 2 sprites cachés (y de 192 à 239 mis à $FF dans `oam_buf`), 3 affiché : `band_on(199)`. crt0.s : `split_on` 2, `split_l` (valeur de recharge du compteur, 31 pour l'affiche VS) ; l'interruption écrit PPUCTRL avec l'écran 1 (le bit horizontal passe dans l'adresse du PPU en fin de ligne) ; la NMI réécrit PPUCTRL dès qu'une interruption est active (trames en retard) ; `ppu_off` arrête l'interruption.
- **Crépuscule** (`COND_SUNSET` 3) : `sun_apply` (court_load et fin de chaque jeu, `hl_point_ev`) : g = jeux joués, L = sets gagnants × jeux par set × 6 / 5, palier = min(3, 4 g / L) ; 0 jour, 1 accentuation rouge ($20, PAL $40), 2 rouge et bleu, 3 palette de nuit sans accentuation. `ppu_emph` (page zéro) est ajouté à PPUMASK par la NMI et remis à 0 par `ppu_off`. En salle, à l'entraînement et en compétition : jour.
- **Voix** : `gen_voice.py` ajoute deuce (4), game (5), adv (6, 7 046 Hz, en dernier : pas de bourrage) ; `sfx_umpire(k)`. `hl_point_ev(ev)` (far_call depuis game.c à chaque fin de point) pose `voice_pend` (jeu : game ; 3-3 et plus hors jeu décisif : deuce ou adv), dit 24 trames plus tard quand le DMC est libre ($4015 bit 4) et que `ms` vaut MS_DEAD ou MS_SERVE (`voice_tick`). Place : `rules.c` en banque 18 (`RULES_BANKED` : fonctions rb_*), `rules_far.c` dans la banque fixe (changement de banque autour de chaque appel) ; 48 octets libres restent avant BOOT.
- **Performances** : `sc_crit` (rules_far.c ; bits 0-1 balle de set ou de match de l'équipe 0 ou 1, bit 2 balle de match), calculé par `hl_point_ev` à chaque point et remis à 0 par `rules_init`, remplace les appels à `rules_is_*_point` de l'IA (`ai_tactics`) et du public (extras.c). `rules_ad_side` reste dans la banque fixe (une addition). Les tests qui posent un score à la main posent aussi `sc_crit`.
- **Record de vitesse** : `sw_spd[4]` (SAVEDATA, bloc à part : 'S', km/h − 80, personnage, contrôle) mis à jour par `sv_after_match` (équipe 1 humaine) d'après `st_vmax[0]` et `st_vmaxp[0]` (joueur qui a servi) ; `sp_new` → « KM/H : RECORD ! » (`hl_vmax_row`) ; palmarès : `hl_rec_row` (banque 15, appelée depuis menuart.c), rangée 22.

### Version 2.70 : vitesse du service

- **Vitesse** : `serve_hit` (game.c) pose `hl_spd` = vitesse du service (44 coupé, 64 à plat, 96 canon) et `hl_srv` = équipe, puis `bank_call(BANK_HL, hl_serve)` : `hl_spd` devient km/h − 80 = v + v/4 + v/8 + (0 à 7, au hasard), et `st_vmax[équipe]` garde le maximum (remis à 0 au début du match). Sur un ace, `hl_ace_msg` affiche « ACE ! » / « xxx KM/H ».
- **Statistiques** : `hl_vmax_row` (banque 15, appelée par `far_call` depuis screens.c, rangée 20) écrit SERVICE MAX KM/H, ou « --- ». `stat_row` écrit jusqu'à 3 chiffres (colonnes 19 à 21 et 25 à 27). Écran de fin de set : « AU SERVICE » descend en rangée 22.

### Version 2.69 : portrait du vainqueur, plongeon, écran des résultats

- **Second émulateur** (`tools/fceux_check.py`) : FCEUX 2.6 (nouveau moteur PPU, `--newppu 1`, au point près), piloté par un programme Lua sans écran (`xvfb-run`), vérifie ce que jsnes rend mal : la glissade de l'affiche VS (buste gauche seulement sur les lignes paires, droit seulement sur les impaires) et, à plat et en perspective, un ralenti avec son gros plan puis son zoom, et le retour au court (images dans `build/fceux`). Résultat en 2.69 : conforme.

- **Portrait du vainqueur** (`final_tick`, highlight.c, `hl_mode` 3) : à la fin d'un match (mode match, hors démonstration et club nudiste), quand `match_tick` rendrait 1 (`match_end`, game.c, une fois par match : `hl_fin`), la séquence prend la main : flash, `TZ_VICTORY` (en pied, bras levés) du vainqueur en travelling éclair, confettis, ovation, « NOM GAGNE ! » (double : « VICTOIRE ! ») ; puis `TZ_VICTORY_Z` (gros plan ; en double, le partenaire) qui tremble ; 160 trames ; A ou START passe (après 20 trames). Fin : fondu, `chr_reload`, puis l'écran des résultats. `match_draw` ne dessine rien tant que `hl_mode` vaut 1 à 3. Les tests qui forcent la fin d'un match posent `hl_fin` (comme `cer_skip`).
- **Plongeon** : une frappe faite pendant un plongeon (`dive_t`) est notée 4 ; le gros plan du ralenti montre alors l'image de glissade (`TZ_SLIDE`), la balle arrive au ras du sol.
- **Trace du rebond (zoom)** : ellipse de 6 × 4 px centrée sur le point de rebond (le centre de la balle, celui que juge l'arbitre), intérieur couleur de la balle, contour sombre (palette 3, couleur 2 mise à $0F pendant le zoom) ; seuls les joueurs (palettes 0 et 1) sont agrandis : ni ombres ni poussière (8 sprites par ligne au plus, la trace passait à la trappe), ni la balle (plus large que le point jugé, elle chevauchait la ligne d'une balle dehors).
- **Réglage RALENTIS** : 3 valeurs (`opt_max` 3) : 0 NON, 1 OUI, TELE (gros plan et zoom ; la valeur 1 des réglages déjà sauvegardés), 2 OUI, SIMPLE (`hl_check` n'arme aucun repère) ; descriptions dans `gen_help.py`.
- **Écran de fin de set** : titre doré et cadre comme les autres écrans (`deco("FIN DE SET")`), « 1ER SET POUR ALEX » en or rangée 4.
- **Écran des résultats** : le tableau des sets passait sous les lettres du titre (rangées 2 à 4, défaut ancien) : « VICTOIRE DE » rangée 4, tableau rangées 6 à 8 ; les joueurs (joie, déception) descendent sous les statistiques (y 188).

### Version 2.68 : filet qui tremble en perspective ; gros plan du service, du smash et du revers

- **Image `smash`** (`gen_teaser.py`, pose `smash` : raquette au-dessus de la tête et en avant, bras libre replié, buste cambré ; échelle 0,86, en pied ; homme, femme, fillette ; `TZ_SMASH`, repères `TZ_SMASH_RACKET_X/Y`). Images rangées par le générateur dans les banques libres, dont la nouvelle banque 17 (`assets_tz17.c`).
- **Image `backhand`** : revers à deux mains à l'impact (miroir du coup droit, bras droit fléchi vers le bas et avant-bras relevé, bras gauche presque tendu : les deux mains sur le manche), échelle 1,4 ; `TZ_BACKHAND`.
- **Gros plan** : `hl_sk` (1 smash, 2 service, 3 revers : balle à gauche du joueur à la frappe, comme les poses du jeu) choisit `TZ_SMASH`, `TZ_BACKHAND` ou `TZ_IMPACT` ; sur le revers, la balle arrive de la gauche et repart vers la droite ; sur le smash et le service, la balle tombe d'en haut sur le tamis (cachée tant qu'elle est au-dessus du cadre) et repart vers le bas. Titre en haut de l'écran (y 12 et 22), sous la marge rognée par les téléviseurs.


- **Générateur** (`persp_net(w, shake)`, `persp_build`) : le filet en perspective est redessiné secoué (mailles décalées de 2 px, bande affaissée d'un pixel, comme `draw_net` à plat). Toute tuile de l'écran dont le dessin change est retenue si toutes ses places changent de la même façon (sinon elle reste immobile) : 15 tuiles par lieu (bande, mailles, croisements des lignes, sangle, bords). `src/assets_pnet.c` (banque 16) : par lieu, le nombre, puis pour chaque tuile son numéro, son dessin au repos et son dessin secoué (33 octets) ; `persp_net[7]` (le parc reprend le club).
- **Jeu** : `net_tick` (banque 3) passe la main à `pnet_tick` (banque 15) en perspective : le filet est affaissé tant que `net_shake` > 6, puis revient ; les tuiles sont écrites par le tampon VRAM (`vbuf_raw`, lues dans la banque 16 par `bank_read`), sur une ou plusieurs trames selon le budget ; `net_shake` ne s'achève qu'une fois le filet revenu au repos.
- **Tests** (« Filet qui tremble ») : dans 5 lieux en perspective, toutes les tuiles listées prennent leur dessin secoué puis reviennent au repos.

### Version 2.67 : mise en valeur du point pendant le ralenti ; cartouche de 512 Ko

- **Cartouche** : 32 banques de 16 Ko (512 Ko, `cfg/mmc3.cfg`, en-tête 32) ; `set_bank` masque sur 5 bits ; la banque fixe reste la dernière. Banque 15 : `highlight.c` (mise en valeur, récapitulatif du score `score_line` / `score_recap`, sortis de la banque 0) ; banques 16 à 30 libres.
- **Repères** (`highlight.c`, appelés depuis game.c par `far_call` / `bank_call`) : `hl_hit(p | coup << 8)` à chaque frappe (`do_hit`, `serve_hit` ; bit 15 : service, début du point) note l'indice de la trame (`rp_head`), le joueur, le coup et `rp_seq` (compteur 16 bits des trames enregistrées, incrémenté par `rp_record`, crt0.s) ; `ball_bounce` note le premier rebond de chaque balle (indice, `rp_seq`, position à plat, dedans). `hl_select` (fin de point, titre recopié depuis `msg1`, en RAM) retient la dernière frappe de l'équipe gagnante et le dernier rebond. `hl_check` (au début du ralenti, `rp_start`) : un repère n'est retenu que si son âge en trames enregistrées (`rp_seq` − repère) est au plus `rp_left` (tampon circulaire : un indice ancien est réutilisé) ; une frappe plus ancienne ouvre le ralenti (son image ne dépend pas de la trame). Club nudiste : aucun repère.
- **Déclenchement** : `rp_play` (banque 3), à la deuxième présentation de la trame repérée, met `hl_mode` à 1 (gros plan) ou 2 (zoom) ; `match_tick` appelle alors `hl_tick` (banque 15) à chaque trame. A, B ou START : retour au court et fin du ralenti.
- **Gros plan** (`closeup_tick`, environ 2 s) : 11 trames figées (flash à la 9e) ; `cut_build` (comme la bande-annonce : image haute définition `TZ_IMPACT`, homme, femme ou fillette, palettes tirées de `pal_set_player`) ; travelling éclair puis dérive ; balle qui arrive sur le tamis, flash d'impact, traînée ; titre en sprites (lettres de la police copiées dans la zone des jeux de tuiles, une ou deux lignes de 8 lettres au plus) ; puis `TZ_VICTORY_Z` (gros plan, bras levés), tremblement amorti, confettis.
- **Zoom** (`zoom_tick`, environ 1,6 s) : réticule (4 coins, emplacements de sprites 60 à 63) qui se resserre sur le rebond (projeté par `proj2` en perspective) ; `zoom_build`, rendu coupé (écran blanc, 3 à 5 trames) : fenêtre de 16 × 15 tuiles alignée autour du point (dans les rangées 4 à 29) ; tuiles de l'écran et sprites de la trame figée lus dans la mémoire du PPU (`vread` : lecture différée forcée dans une variable, sinon le compilateur la supprime) ; chaque tuile source agrandie en 4 quarts (table `dbl` : 4 bits → 8 bits), les quarts unis remplacés par 4 tuiles unies, au plus 80 tuiles sources (tampons dans la zone libre de SAVEDATA) ; tuiles du fond écrites en $1000 et lues par le PPU grâce à `ctrl_swap` = $18 (NMI : `CTRL_VAL ^ ctrl_swap`, fond en $1000, sprites en $0000) pendant que les sources restent lisibles en $0000 ; attributs : un quart de bloc par tuile source ; sprites agrandis (12 au plus, miroirs respectés, quarts vides omis) en $0000 ; trace de la balle et BONNE ! / DEHORS ! (sprites 52 à 63) sur une partie unie du court.
- **Retour** (`back_to_court`) : `chr_reload` (8 Ko), `court_reload` (banque 0 : `court_load`, bandeau), puis la trame du ralenti.
- **Tests** (section « Ralenti : gros plan et zoom ») : à plat et en perspective, gros plan puis zoom, écran du court identique après chaque séquence (tables de noms et tuiles du court), tables de tuiles rétablies ; A pendant le gros plan ; club nudiste. Les tests de blocage des matchs complets tiennent compte de la durée des séquences.

### Version 2.66 : ralentissements

Mesure (`lag`, 15 000 trames, 4 joueurs de l'ordinateur ; une trame est en retard quand le programme n'attend pas déjà la suivante) :

| Configuration | 2.63 | 2.66 |
|---|---|---|
| Simple à plat | 83 | 83 |
| Simple en perspective | 220 | 81 |
| Double à plat | 1 802 | 698 |
| Double en perspective | 2 942 | 1 056 |

- **`spr_person_v`** (sprp.s) : entrée de `spr_person` sans arguments sur la pile C ; `match_draw` pose `pv_x`, `pv_y`, `pv_c`, `pv_front`, `pv_pose`, `pv_flip`, `pj_sg` (variables de `spr_person`, exportées) ; `pv_mode` fait finir par `rts` au lieu de `incsp8`.
- **`draw_order`** (fastmove.s) : positions à l'écran (`dr_xs`, `dr_ys` : (v << 2) >> 8 en octets) et ordre de dessin `dr_ord` (tri par insertion stable, du plus bas au plus haut) ; `match_fx` réutilise `dr_xs` / `dr_ys` pour les ombres.
- **`vbuf_raw`** (fastmove.s) : en assembleur (environ 14 cycles par octet au lieu de 60) ; même contrat (0 si le budget `VBUF_MAX` serait dépassé).
- **`dive_try`** : sortie immédiate si la balle s'éloigne du joueur.
- **IA** : le décalage vers le côté préféré de l'adversaire (multiplication et division 16 bits) n'est recalculé que quand `ad_n` change (`ai_bias`, `ai_bias_n`).
- **Test** (section « Vue en perspective ») : double en perspective, 3 000 trames : moins de 11 % de trames en retard (2.66 : environ 9 % ; 2.63 : 24 %, refusé).

### Version 2.65 : glissade stable, symétrie, lignes au filet

- **Glissade VS** : `vs_slide` ne contient que des multiples de 8. Le décalage fin X (registre x du PPU) est mis à jour aussitôt par l'écriture de $2005, au milieu de la ligne ; seuls la colonne de départ (t) et le bit d'écran sont copiés à la colonne 257. Avec un décalage fin toujours nul, les écritures de l'interruption (vers la colonne 150 de chaque ligne) ne changent rien à la ligne en cours. Test : décalages relevés pendant l'entrée, tous multiples de 8.
- **Lignes au filet** : `PERSP_NETFIX = False` (gen_assets.py) : plus d'alignement forcé des lignes de côté de part et d'autre du filet (marche d'un pixel derrière le filet, à sa place exacte).
- **Symétrie** (axe : colonne 128) : rambarde droite en x 225 (à plat, `draw_court`, la colonne 224 devient abords ; en perspective, `persp_rail` projette 225 ; `persp_sides` efface les colonnes 0 et 1 de la dernière tuile quand le décor glisse) ; sangle du filet à plat de 3 px (127-129), reflet des poteaux côté extérieur.
- **Tuiles libres en perspective** : `PERSP_NOTXT` = `" $ & [ \ ] ^ # % ( ) ; < =` ; `_persp_check_texts` arrête la génération si un texte d'un module du match (hors écrans de menu) en contient un. Blocs pleins 1 à 3 libres s'ils ne sont pas dans le décor du court.

### Version 2.64 : lignes fines en perspective

- **Échelle par ligne de pixels** (`persp_inv`, `persp_lines`, gen_assets.py) : l'échelle horizontale est calculée pour chaque ligne de l'image, et non plus une fois par rangée de tuiles. Les lignes obliques du court avancent d'un pixel dès que la projection le demande (toutes les 2 à 3 lignes de pixels pour les lignes de côté), au lieu de marches de 8 lignes. Il faut 112 à 146 tuiles nouvelles par lieu (70 à 104 avant).
- **Place** : pour les lieux avec du public (stade, club, salle, château ; le parc reprend le club), le générateur ne garde que les tuiles réellement affichées pendant le match : tuiles 0 à 3, police sauf les 7 signes absents de tous les textes (`" $ & [ \ ] ^`, `PERSP_NOTXT`), filet, décor du court en perspective et « bras levés » de l'ola (tuile + 4) ; la tuile 4 (ancienne flèche des menus) ne sert plus. Les panneaux et les spectateurs de la vue à plat absents du court en perspective sont libérés. `persp_flags[7]` (banque 3) : bit 0 panneaux 1, bit 1 panneaux 2, bit 2 spectateurs du stade (`crowd_idx`), bit 3 spectateurs du club et de la salle (`crowdb_idx`). `load_court` le copie dans `persp_free` (WBSS) ; `load_court`, `crowd_load` et `crowd_tick` ne chargent plus ces tuiles quand le bit est mis. Le club nudiste et le court Margaux gardent l'ancienne règle (ils ont assez de place). Rambarde des tribunes au pixel près partout.
- **Compression** : les tuiles du court en perspective (banque 11) sont compressées au format de `tz_unpack` (`tz_pack` : masque de 16 bits des octets répétés, puis les octets nouveaux), décompressées une à une à leur place par `load_court`. Sans cela, la banque 11 débordait de 2 160 octets.
- **Banque 3** : `crowd_cheer` (96 octets, jamais lu) n'est plus généré.
- **Test** (section « Vue en perspective ») : dans 5 lieux, après 900 trames de match entre ordinateurs avec 6 olas, les tuiles du court en perspective sont comparées aux données de la ROM (décompressées par le test) : aucune n'est réécrite, hors des 24 places des petits spectateurs (animés). Un programme volontairement fautif (public de la vue à plat rechargé) est bien détecté.

### Version 2.63 : interruption par ligne (affiche VS)

- **Interruption** (`irq`, crt0.s) : la NMI arme le compteur de la MMC3 à chaque trame tant que `split_on` vaut 1 et que le rendu est actif (même si le thread principal n'a pas fini sa trame) : $C000 et $C001 = 31, $E000 puis $E001. L'interruption tombe à la fin de la ligne 30 (même résultat avec jsnes, où $C000 est le compteur et $C001 la recharge). Une écriture de défilement faite pendant la ligne L vaut pour la ligne L + 1 (copie horizontale à la colonne 257) : le gestionnaire écrit, ligne par ligne de 31 à 142, `PPUCTRL` (bit d'écran) et le défilement X pour les lignes 32 à 143 (rangées 4 à 17, celles des bustes), puis rétablit `scroll_x` / `scroll_nt`. Boucle calée à 227 cycles par paire de lignes (2 × 113,67) : chaque écriture a une fenêtre d'une ligne entière ; la dérive est d'environ 19 cycles sur 112 lignes. `bit PPUSTATUS` avant chaque écriture (double accès de $2005) et à la sortie. Durée : environ 12 700 cycles par trame, pendant l'entrée seulement.
- **Glissade** (`vs_anim`, menuart.c) : le buste gauche est dans l'écran 0 ($2000), le droit dans l'écran 1 ($2400, miroir vertical ; attributs recopiés en $27C0). Lignes paires : écran 0 décalé de +o ; impaires : défilement 256 − o (fin de l'écran 0, vide à droite, puis l'écran 1). Un seul défilement par ligne ne peut pas écarter deux objets posés sur les mêmes lignes : d'où l'entrelacement. o suit `vs_slide` (96 → 0 en 24 trames, freinage). Ensuite, le buste droit est recopié dans l'écran 0 (6 rangées de 15 octets par appel, tampon VRAM), `split_stop` (aussi appelé par `ppu_off`) coupe l'interruption, éclair blanc de 2 trames.
- `split_start(o)` / `split_stop()` : `cli` / `sei`, l'interruption n'est autorisée que pendant l'entrée de l'affiche. Variables `split_on`, `split_o`, `split_c`, `split_n` en `WBSS`.

### Version 2.62 : cartouche MMC3

- **Carte** : mapper 4 (`cfg/mmc3.cfg`), 256 Ko de programme, CHR-RAM de 8 Ko, PRG-RAM de 8 Ko sauvegardée. Mode PRG 0 : $8000 (R6) et $A000 (R7) commutables, $C000-$FFFF fixes (blocs 30 et 31 = ancienne banque 15).
- **`set_bank(n)`** (crt0.s) : R6 = 2n, R7 = 2n + 1 ; ne touche ni X ni Y ; rien n'est écrit si `cur_bank` vaut déjà n (`cur_bank` est mis à 0 par l'amorçage).
- **Amorçage** (segment `BOOT`, dans les 8 derniers Ko, seuls fixes à l'allumage) : mode 0, banque 0, puis `reset` ; registres CHR R0-R5 à l'identité, miroir vertical, PRG-RAM activée, IRQ coupée. Les programmes STUBn des banques (MMC1) sont inertes.
- **Miroir** : `mirror_h(h)` écrit $A000 (0 vertical, 1 horizontal).
- **Tests** : en-tête MMC3 et banques de 16 Ko en deux blocs de 8 Ko (section « Démarrage et cartouche MMC3 »).
- **Sprites** (`swap_tick`, extras.c) : un jeu de tuiles (célébrations / rebond avant le service : 7, 15-18 ; revers : 22-26) reste chargé tant qu'un joueur en montre une pose.
- **Perspective** (`gen_assets.py`) : projection et lignes centrées sur 128,5 ; sangle de 3 px ; reflet des poteaux côté extérieur.

### Version 2.61 : mémoire, volée croisée, bande-annonce, coups de B

- **Mémoire** : `intro.c`, `podium.c` et `credits.c` (modules liés après `stats.c`) rangent leurs variables dans `SAVEDATA`, après les blocs sauvegardés, qui gardent leurs adresses (`sv` $7800, `sw_best` $7851, `so` $7855, `sx` $7870, `sw_view` $787B). Ces variables ne sont pas remises à zéro au démarrage : elles sont toutes initialisées avant usage. Pluie (`rain_x`, `rain_y`) en `WBSS`.
- **Volée** (`shot_select`) : croisé court en volée, profondeur − 2 (au lieu de − 10), vitesse plafonnée à 48.
- **Bande-annonce** (`gen_teaser.py`, `render`) : classe 0 (contours, yeux) des personnages → classe 5 (sombre), sauf fillette ; si l'image dépasse 246 tuiles, rendu d'origine. `teaser.c` : D = $00 si $10 (Papi). Budget de la banque 12 : 11 500.
- **Coups de B** : coupé, gravité 5 ; IA : amortie seulement si le frappeur est à moins de 70 px du filet.

### Bande-annonce (`src/teaser.c`, banque 7, version 2.56)

`start_comp` (screens.c) lance `teaser_build` pour le Grand Martinot et le Martinworld Tour (`SS_TEASER`) ; `teaser_tick` à chaque trame ; à la fin (`teaser_done`), `chr_reload` puis `comp_next` (annonce du 1er tournoi).

**Images en haute définition** (`tools/gen_teaser.py`) : chaque pose est décrite par un squelette (`skeleton` : position du bassin, angle du buste, angles des membres ; longueurs de segments du canon de dessin de figure, en unités de tête `HU` = 22 px : femme 7,5 têtes, homme 7,8 têtes aux membres plus longs, épaules et hanches selon le sexe ; pieds ramenés au sol) ; le corps est dessiné en formes vectorielles (membres effilés, tête, cheveux, chaussures, raquette cordée ; homme : polo porté sur un short ample ; femme : épaules plus étroites, taille marquée, hanches plus larges, membres plus fins, jambes galbées, robe de tennis sans manches dont la jupe plissée évasée est l'enveloppe convexe des hanches et des cuisses (pas de fente), frange, queue de cheval, cils, lèvres), avec une classe par pixel (contour = couleur du fond, peau, peau ombrée, tenue, tenue ombrée, sombre, blanc), une ombre sur le bord opposé à la lumière et un contour là où une pièce passe devant une autre. Le même dessin, rendu à une autre échelle, donne les plans rapprochés (zoom). Pour chaque bloc 16 x 16, la palette la moins coûteuse est choisie parmi P1 (peau, peau ombrée, sombre), P2 (tenue, tenue ombrée, sombre), P3 (peau, tenue, blanc ; en zoom : peau, son ombre, tenue) et, en zoom, P0 (peau, sombre, blanc : yeux, dents) ; les couleurs viennent de `char_pal` (ombres : un cran plus sombre). Tuiles dédoublonnées (au plus 246, $01-$F6), compressées (par tuile : masque 16 bits des octets répétés, puis les octets nouveaux), rangées dans les banques libres 2, 4, 9, 10 et 12 (`src/assets_tz*.c`), descripteurs et repères (tamis, pied, visage) dans `src/assets_tz.h`.

**Chargement** (rendu coupé, environ 5 trames : noir, ou blanc pour un zoom) : `tz_unpack` (crt0.s) décompresse les tuiles vers le PPU, `tz_copy` copie la table de noms (rectangle utile) et les attributs depuis leur banque. `chr_copy` (gfx.c) utilise aussi `tz_copy` par blocs de 256 octets : la recopie complète des motifs (`chr_reload`, 8 Ko) passe d'environ une seconde à une dizaine de trames, ce qui accélère tous les changements d'écran. Les deux écrans ($2000, $2400) reçoivent le fond (uni : des bandes feraient une démarcation autour des blocs 16 x 16 du personnage, qui ont ses palettes) et le sol.

**Caméra** : `scroll_x` (nouveau, écrit par la NMI) et le bit 0 de `scroll_nt` font défiler horizontalement sur les deux écrans : entrée en travelling éclair (`whip_in`), dérive lente, sortie éclair (`whip_out`) quand le plan suivant arrive aussi en travelling ; tremblements par `scroll_y`. Les sprites (balle, poussière, confettis, éclats) suivent la caméra.

Titre : la police est rechargée ($20-$5F), lettres doublées (4 tuiles chacune) écrites une par une par le tampon VRAM. Musique `MUS_ROCK` (19). Le banc d'essai la passe (`cer_skip`). Variables : bloc SAVEDATA.

### Cérémonies de fin de compétition (`src/podium.c`, banque 8, version 2.52)

Compétitions classiques : `podium_build` masque les palettes 1 à 3 du décor (joueurs agrandis) et prépare les médailles (`medal_at` : position, cou, couleur ; tuiles `S_MED`, $38 à $3B de la table des sprites : ruban, or, argent, bronze, palette de sprites 3 = or $28, argent $10, bronze $17). `medal_tick` (trames `pod_t`) : 3e à 20, 2e à 60, 1er à 100 (palettes rétablies, flash, `MUS_MEDAL`), médailles qui descendent de 3 px par trame (bronze 120, argent 145, or 170, entrées 54 à 61 de l'OAM), flashs (fond blanc 2 trames) jusqu'à 420 ; la coupe et les feux d'artifice d'avant partent à `MED_T0` = 210. Grand Martinot et Martinworld Tour : `gm_build` (tapis : tuiles $60 et $61, table $62 et $63) et `gm_tick` : finaliste (40-266 : montée, plaque, se range), vainqueur (266-390 montée, 390-420 coupe dans les mains, 420-480 baiser, puis coupe levée, confettis, feux d'artifice), ramasseurs et officiel (palette de sprites 2) ; A saute à la fin (pas avant 60 trames, pour ne pas prendre l'appui qui a mené à l'écran), puis `gm_press_build` (conférence : toile $64, table, micros, flashs ; `gm_q` : 4 questions et réponses, 3 montrées, tirées au sort). `pod_busy` retient A dans screens.c jusqu'à la dernière question. Variables dans la RAM de cartouche libre (bloc SAVEDATA).

### Codes de reprise (`password.c`)

52 bits de données (dont double, partenaire, « partenaire joué par la manette 2 » et lieu) + 2 bits de version + 11 bits de somme de contrôle = 65 bits, écrits en 13 symboles de 5 bits (alphabet sans I, O, 0, 1). Les symboles sont brouillés en chaîne pour qu'une erreur en modifie plusieurs.

Version 0 (version 2.44) : le Grand Martinot (type 5). Champs propres : conditions, étape, graine, personnage, lieu, format, niveau, résultats sur 12 bits (`gres` : 0 à 4 tours gagnés par tournoi, 3 en double, vérifiés au décodage), trophées, double, partenaire, manette 2, puis 3 bits à zéro ; pas de surface (imposée) ni d'options. Le Martinworld Tour (type 6, version 2.47) utilise aussi la version 0, avec la marque 1 dans les bits 49 à 51 (0 pour le Grand Martinot) : étape sur 5 bits (0 à 20), graine, personnage, format, niveau, résultats sur 14 bits (2 par tournoi), trophées, double, partenaire, manette 2 ; ni surface, ni lieu, ni conditions. Les versions 1 à 3 ne changent pas.

Formats (version sur 2 bits) : 2, ancien format à 16 personnages ; 3, format courant (surface × 5 + lieu sur 5 bits, niveau sur 2 bits) ; 1 (version 2.39), pour le château et le parc (lieux 5 et 6) : (surface × 7 + lieu) × 3 + niveau sur 7 bits. Les codes des 5 premiers lieux restent identiques. Personnage et partenaire sur 5 bits : jusqu'à 32 personnages (21 aujourd'hui).

### Personnages (version 2.39 : 21)

16 personnages d'origine, Margaux (16), puis MAX (17, puissance, tenue orange), LOLA (18, offensive, anthracite), PAPI (19, défensif, cheveux gris) et TITOU (20, fantaisie, canard foncé). Les tenues passent `tools/contrast.py` (ΔE ≥ 25 sur tous les courts, de jour, de nuit et sous la pluie). Comme Margaux, les nouveaux personnages ne sont jamais tirés au sort comme adversaires en compétition : les anciens codes redonnent les mêmes tableaux.

### Lieux (version 2.39 : 7)

- **Château** (5) : murs de pierre (une seule tuile de briques décalées, répétée), bannières royales, torches, deux balcons de spectateurs (palette 3, comme au club). Palette des abords : pierre $10, joints $00, bannières et flammes $16. 6 tuiles de décor nouvelles (254 sur 256 utilisées).
- **Parc** (6) : le décor du club (même table de noms, aucune tuile ni octet de plus) aux couleurs de l'automne : herbe $19, feuilles orange $27, troncs $07.

### Compétitions en double

- **Tirage** : avec 17 personnages, les 8 équipes d'un tableau sont toutes différentes. Le joueur et le partenaire choisi forment l'équipe 1 ; 14 des 15 autres personnages sont mélangés par la graine et appariés deux à deux.
- **Simulation** : les mêmes formules qu'en simple, avec le niveau de l'équipe.
- **Deux joueurs** : option « JOUE PAR : CPU / MANETTE 2 ». Avec la manette 2, `cfg.ctrl[2]` passe à la manette 2 pour tous les matchs de la compétition ; le réglage est mémorisé dans le code de reprise.
- **Affichage** : les noms sont abrégés en « AL/MA ».

### Générique de fin (`src/credits.c`, banque 9)

Nouvel état `AS_CREDITS` (`GO_CREDITS`). Il s'ouvre sur A depuis le podium d'une saison gagnée (`c_type == CT_SEASON`), après `chr_reload`.
- **Défilement vertical** : une ligne de générique occupe 2 rangées (16 px). Un écran n'a que 30 rangées, et une ligne écrite dans la partie visible apparaîtrait aussitôt. `mirror_h(1)` passe donc le MMC1 en miroir horizontal : $2000 et $2800 sont empilés, soit 60 rangées et 30 emplacements de ligne, dont environ 15 visibles. Chaque fois que le défilement avance de 16 px, l'emplacement situé 16 lignes plus bas que le haut de l'écran (caché) est réécrit par le tampon VRAM : texte, interligne et 8 octets d'attributs, soit 81 octets.
- **NMI** : le registre de défilement vertical reçoit `scroll_y` (0 à 239), et PPUCTRL reçoit `CTRL_VAL | scroll_nt` (écran du haut ou du bas). Hors générique, les deux valent 0.
- **Couleurs** : texte blanc (palette 0) pour les postes, or (palette 1, $28) pour les noms. Les attributs sont tenus dans une copie en RAM de chaque écran.
- **Rythme et fin** : 1 px toutes les 2 trames (2 px par trame avec A ou START maintenu). Arrêt quand la dernière ligne arrive au milieu de l'écran, `sfx_clap` une seconde après, puis A : rendu coupé, défilement remis à 0, miroir vertical rétabli, écran titre.
- **Musique** : `MUS_CREDITS`, 8 mesures de 96 trames en boucle. Carré 25 % à volume constant (le « kazoo »), notes liées, basse en blanches.

### Podium (`src/podium.c`, banque 8)

Écran de fin d'une compétition gagnée. Les 3 premiers montent sur un podium centré, posé sur le court de la compétition, en **taille double** (32 × 48 px).
- **Classement** : `comp_podium` (banque 5) remplit `c_pod[3]`. Pour un tournoi : le finaliste, et le demi-finaliste battu par le joueur. Pour les poules : le vainqueur de l'autre demi-finale, et l'adversaire de la demi-finale du joueur. Pour le championnat et la saison : le classement final.
- **Agrandissement** : la NES ne sait pas agrandir les sprites. À la construction de l'écran (rendu coupé), `dbl_tile` lit chaque tuile du personnage en CHR-RAM (vu de face, tête propre de Margaux comprise) et en écrit 4, chaque pixel devenant 2 × 2 (table `dbl` de 16 quartets). Un personnage fait 24 tuiles.
- **Où vont les tuiles** : les joueurs vont dans le **décor** (tuiles $60 à $A7, à la place de tuiles inutiles sur cet écran), avec une palette du décor chacun : 1er au centre, 2e à gauche, 3e à droite. La coupe agrandie (16 sprites) va dans la table des **sprites**, à la place des personnages vus de dos.
- **Double** : il faudrait 6 palettes. Les 2 vainqueurs et le capitaine finaliste sont dans le décor (palettes 1 à 3). Le capitaine classé 3e est en sprites agrandis (24 sprites, palette de sprites 0). Le nom de chaque équipe complète est écrit sous sa marche.
- **Décor** : couleur de la surface en fond universel, lignes de fond et de côté, marches grises numérotées. Ces tuiles sont fabriquées au vol : chiffres tirés de la police, en blanc sur la marche. Sur la plage et la glace, le texte est noir. Les joueurs commencent sur des rangées paires, pour que chaque bloc d'attributs de 16 × 16 px ne contienne qu'un seul élément.
- **Animation** : la coupe monte des mains au-dessus de la tête puis oscille. 5 feux d'artifice (`fw_sound` / `fw_draw` de la banque fixe, compte à rebours en paramètre) partent de `cup_t` = 24 à 233. Les confettis tombent. Pour tenir dans une image, seuls les éléments qui bougent sont réécrits, directement dans la copie de l'OAM. Une première version qui réécrivait tous les sprites à chaque trame tournait à mi-vitesse.
- **Sortie** : A ou START appelle `chr_reload` (toute la CHR-RAM recopiée depuis la banque 2), puis l'écran titre. Les tests vérifient que les 8 Ko sont redevenus identiques à la ROM.
- **Lecture de la CHR-RAM** : la lecture « à vide » du tampon du PPU s'écrit `x = PPU_DATA;`. cc65 supprime un simple `(void)PPU_DATA;`, ce qui décalait chaque tuile lue d'un octet. Le générique avait le même défaut, corrigé aussi.

### Court Margaux

- **Lieu** : 5ᵉ lieu (`ENV_MARGAUX`), disponible d'emblée. Sa palette remplace celle de la surface (rose $25, abords prune $14), mais la surface garde son comportement physique.
- **Décor** : écran de court dédié, avec 3 motifs (cœur, fleur, étoile) sur fond rose pâle et le panneau « COURT MARGAUX » en ligne 3. Leurs versions scintillantes sont échangées en CHR-RAM après les beaux points, avec le même mécanisme que le public.
- **Contrôle** : le rose est vérifié par `tools/contrast.py` comme les autres surfaces. Pour lui, trois tenues ont changé : Alex en blanc, Hugo en bleu canard, Noah en cyan.
- **Code de reprise** : le lieu y est codé sur 3 bits, et la somme de contrôle passe à 11 bits.

### Club nudiste (bonus)

- **Déblocage** : par la première victoire en compétition. Il est stocké dans le palmarès (`sv.nude`) et dans la RAM (`nude_unlocked`) ; un code de reprise contenant au moins un trophée débloque aussi le club.
- **Tenues** : `people_tiles(1)` (banque fixe) copie en CHR-RAM, rendu coupé, les 192 tuiles des personnages au naturel depuis la banque 6 ; `people_tiles(0)` restaure les tuiles normales depuis la banque 2. Les silhouettes n'ont aucun détail anatomique ; une serviette aux couleurs de l'équipe distingue les camps.
- **Court** : un 4ᵉ écran de court, sans gradins. 9 spectateurs debout, en pied (16 × 24, 3 modèles de 6 tuiles), placés au centre des bandes latérales, hors de la zone rognée par les téléviseurs ; leur version bras levés est échangée en CHR-RAM quand le public applaudit.
- **Annonces** : un complément humoristique est ajouté par `hud_update` (banque 3), sans toucher au moteur.
- **Écart au CDC** : contrairement au § 5.3 (tout accessible dès le départ), ce lieu est à débloquer. C'est un bonus demandé explicitement, en dehors du contenu de base.

### Ralentis

- **Enregistrement** : à chaque trame d'échange, `rp_record` (assembleur, ~450 cycles) enregistre la balle et les 4 joueurs (position et pose) dans un tampon circulaire de 160 trames en RAM de cartouche.
- **Déclenchement** : après un ace, un coup gagnant ou un échange d'au moins 12 frappes, les 140 dernières trames sont rejouées à demi-vitesse. A, B ou START les passent.

### Public animé

`crowd_tick` (banque 3) est appelé à chaque trame par `match_tick`. Il gère :
- **les spectateurs** : un par tuile, assis en rangées, tête de 4 × 4. Deux jeux de 6 motifs : le jeu A (stade, palette des tribunes : gradins rouges, peau orange, cheveux clairs) et le jeu B (club et salle, palette 3 : peau jaune, cheveux noirs, maillots blancs ou noirs). Une première version, à deux spectateurs par tuile, faisait une masse illisible ; au club et en salle, elle ressemblait à des rayures ;
- **le regard** : chaque motif existe de face (visage avec deux yeux, regard vers le bas du court) et de dos (chevelure, `crowd_up` / `crowdb_up`, regard vers le haut). Quand la balle change de moitié du court, les 6 tuiles du jeu du lieu sont réécrites en CHR-RAM, une par trame (16 octets) : toute la tribune tourne la tête ;
- **l'ola** (stade, club, salle), relancée chaque fois que le jeu ranime le public (`crowd_timer`) : une bande de 3 rangées descend les deux tribunes, une rangée toutes les 2 trames. Chaque cellule de spectateur y prend sa version bras levés (6 tuiles propres par jeu, `cheer_idx` et `cheerb_idx`), puis reprend son dessin d'origine. Ce dessin est copié de l'écran au chargement du court (`crowd_load` : rangées 4 à 29, colonnes 0-3 et 28-31, 208 octets en RAM de cartouche). Au plus 28 octets de tampon VRAM par trame ;
- **les réactions** sur le canal de bruit : « OOOH » (`sfx_ooh`) quand un point se perd sur une balle sortie de moins de 5 px, soupir (`sfx_groan`) sur une double faute, applaudissements rythmés (`sfx_clap` toutes les 18 trames) pendant l'attente d'un service de balle de match, silence (`sfx_hush`) au lancer ;
- **le court Margaux et le club nudiste**, qui gardent l'ancienne animation : motifs échangés en CHR-RAM.

Au club, 4 bandes de spectateurs de 2 rangées. En salle, des gradins sur les 16 px extérieurs de chaque côté, avec les panneaux publicitaires côté court. Les deux sont en palette 3 (noir, jaune, blanc), pour être bien visibles. `court_attr` prend maintenant le lieu en paramètre.

### Vue en perspective (option VUE, version 2.42)

Chaise d'arbitre (version 2.55) : dans `persp_build`, un point du court à plat qui tombe sous la chaise prend la valeur de la rangée juste au-dessus de la chaise (les abords) : la chaise est redessinée à sa place ensuite, ses pixels échantillonnés ne laissent plus de bande à côté d'elle. Alignement au filet (version 2.51) : `persp_lines` (gen_assets.py) donne à chaque ligne de côté la même colonne dans la rangée de tuiles visible au-dessus du filet et dans celle au-dessous ; au-dessus, l'escalier de la ligne est décalé d'une rangée en remontant jusqu'à rejoindre sa place (marches d'un pixel), la ligne échantillonnée du court à plat est effacée là où elle change de colonne.

Le jeu calcule tout à plat, comme avant ; seul l'affichage est projeté. Projection retenue (15 %) : avec `a = 1/0,85 − 1` et `d = (216 − Y) / 168` (0 sur la ligne de fond du bas, 1 sur celle du haut), l'échelle vaut `s = 1 / (1 + a·d)`, puis `y' = 216 − 168·(1 + a)·d·s` et `x' = 128 + (X − 128)·s`. La ligne de fond du haut est donc 15 % plus étroite que celle du bas, et les distances se tassent vers le fond.

- **Courts** : `tools/gen_assets.py` redessine pour chaque lieu le court projeté (surface, lignes, couloirs, filet à mailles régulières, poteaux, chaise d'arbitre) avec une échelle horizontale constante par rangée de tuiles (par ligne de pixels depuis la version 2.64 ; lignes verticales redessinées en perspective par `persp_lines` : l'échantillonnage du court à plat sautait parfois leur colonne d'un pixel au fond, d'où des morceaux de ligne manquants avant la version 2.43), ce qui limite les tuiles nouvelles à 64 à 67 par lieu. Elles sont placées dans des tuiles du décor inutilisées par ce lieu (jamais dans les 96 premières ni dans les tuiles du public animé). `load_court` (banque 3) copie ces tuiles en CHR-RAM, décompresse le décor projeté de la banque 11 et met `bg_dirty` à 1 ; `chr_borrow_restore` recopie alors les 4 Ko de tuiles du décor au retour aux écrans. Les décors à plat sont passés en banque 12 pour faire la place.
- **Tables** (`proj_ram`, RAM de cartouche, 1 152 octets copiés depuis la banque 11 au chargement du court) : `y'` pour chaque Y ; un octet d'échelle tassé (signe, `|m|` = échelle × 128 − 128, au plus 19) ; une table de produits `round(v·|m|/128)` pour v par pas de 8. `_proj` (assembleur, `src/sprp.s`) donne `x'`, `y'` et l'échelle d'un point ; `_pscale` met une longueur à l'échelle (hauteur de la balle) par une seule lecture de table (erreur au plus 1 px).
- **Objets projetés** : cible de l'entraînement, joueurs (`spr_person` garde la position projetée de chaque joueur dans `psh_x/psh_y` pour son ombre et sa goutte de sueur, sans second calcul), balle et ombre (`spr_ball`, hauteur réduite selon la distance), balle du dribble et poussière des glissades (`poses.s`), poussière, éclaboussures, repère d'atterrissage, traînée du service canon et éclat de la frappe parfaite (`match_fx`), ralentis. Les joueurs et la balle gardent leur taille dans cette étape.
- **Coût** : une première version (multiplication par décalages, ombres projetées en C) doublait les trames en dépassement en double ; avec la table de produits et la réutilisation de la projection des joueurs, la mesure (double, 4 CPU, 6 000 trames) donne 508 trames d'échange en dépassement contre 477 à plat. En simple, aucune différence.
- **Tribunes** : `persp_sides` (`tools/gen_assets.py`) place la rambarde au point X = 31 (ou 224) projeté avec l'échelle de la rangée de tuiles, au pixel près (pas de 2 px en secours si un lieu manquait de tuiles). Les tribunes s'avancent jusqu'à elle. Rangées avec public : colonnes ajoutées de spectateurs (stade, bancs du club et du parc, balcons du château), puis une colonne « bord » (fond, rambarde, abords). Rangées sans public : le décor à plat entier glisse vers le court (arbres, bannières, torches et spectateurs du club nudiste restent entiers), du fond est ajouté côté écran. Salle : les gradins restent dans le bloc de palette 3 (colonnes 0-1), le mur s'élargit avant les panneaux. Court Margaux : les cœurs continuent leur motif. Devant la chaise d'arbitre (rangées 14 à 17, à gauche), la tribune s'arrête à x 31, comme à plat. Les blocs d'attributs des colonnes 4-5 et 26-27 prennent la palette de la tribune quand elle y entre (le générateur vérifie qu'aucun pixel du court n'y est).
- **Spectateurs** (`spectator`) : 4 modèles (cheveux courts, longs, casquette, cheveux clairs), même style pour le jeu A (stade, palette 2 : $37 peau, $16 gradins, $0F cheveux, marche du gradin en couleur 0) et le jeu B (club, salle, château, parc, palette 3). Les 6 places de `crowd_idx` utilisent les 4 dessins (tuiles 5-8 et 13-16, bras levés à + 4 : vérifié par le générateur). En perspective, la taille suit l'échelle de la rangée : normale (≥ 0,965), `med` (environ 90 %), `small` (80 %), `tiny` (4 petites têtes par tuile, rangées du fond). Les 3 tailles réduites occupent 24 places consécutives par lieu (`persp_ps` → `ps_idx[0]`) : pour chaque taille, 4 dessins puis leurs 4 « bras levés » ; `ola_row` lève une tuile en lui ajoutant 4 (trois comparaisons d'intervalle, sans boucle). `crowd_tick` tourne les têtes de 18 tuiles (6 normales et 12 réduites, une par trame) et s'arrête de vérifier quand toutes regardent du bon côté (`cr_all`). Pour faire la place (77 à 111 tuiles nouvelles par lieu), les tuiles du court à plat absentes du décor en perspective sont réutilisées pendant le match ; restent réservées la police, le public animé, les panneaux et le filet.
- **Tremblement du filet** : en perspective depuis la version 2.68 (voir plus haut).

### Choix du coup (`shot_select`, banque 3, version 2.42)

`do_hit` (banque 0) passe le joueur, les directions, le bouton, la charge, l'écart et la position de la balle (`sh_*`) à `shot_select` (`src/extras.c`, appelé par `bank_call`), qui rend profondeur, vitesse, gravité, écart, effet, type de coup et frappe parfaite. **Smash** : balle prise avant le rebond à plus de 24 px de haut (sauté au-delà de 40 px). A : à plat (profondeur 62, vitesse 80 ; sauté 56 et 96) ; A + HAUT : profondeur + 12 ; A + BAS : profondeur 34 et visée de l'angle comme le croisé court (`sh_short`) ; B : smash coupé (52, 58, effet coupé) ; B + BAS : smash amorti (24, 44, effet amortie). Au smash à plat, la charge ajoute `charge / 2` à la vitesse et `charge / 4` à l'écart. **Volée** (avant le rebond, plus bas) : le coup choisi est gardé, profondeur − 10 (amortie − 6), vitesse plafonnée à 42, ou à 52 pour une volée à plat chargée à 16 ou plus. Le déplacement a libéré 560 octets dans la banque 0.

### Jeu de tuiles par phase et revers (version 2.43)

Les tuiles de sprites sont pleines (252 sur 256). Les rangées de tuiles propres aux poses de célébration (déception 7, rebond avant le service 15, poing serré 16, colère 17, fatigue 18) et celles propres au revers (préparation 22, impact à deux mains 23, fin à deux mains 24, revers coupé à une main 25, volée de revers 26, qui reprend l'impact) occupent la même zone de 34 tuiles (`swap_base`), y compris les têtes de Margaux de ces poses. `tools/gen_assets.py` classe chaque rangée (permanente, célébrations, revers), range les permanentes à la suite, puis les deux jeux au même endroit ; les tuiles de chaque jeu, normales et au naturel, sont en banque 13 (`assets_swap.c`). La CHR d'origine contient le jeu des célébrations (écrans de résultats).

`swap_tick` (`extras.c`, appelé par `crowd_tick` et par `rp_play`) choisit le jeu voulu : revers pendant le lancer, l'échange et le ralenti ; célébrations sinon. Il le charge jusqu'à 3 tuiles par trame par `swap_put` (`crt0.s`, segment de la banque 13, appelée par `far_call` : 16 octets copiés directement dans le tampon VRAM), en 10 à 12 trames, sans ralentir le jeu (une première version en C, `bank_read` puis `vbuf_raw`, faisait tourner la boucle à mi-vitesse pendant le chargement). `swap_on` ne désigne le jeu qu'une fois entièrement chargé ; d'ici là, `poses_all` (`fallback`, `src/poses.s`) et `rp_play` (`pose_ok`) montrent la pose de secours (`pose_fb` : celle d'avant la version 2.43, par exemple l'impact du coup droit vu en miroir), jamais une pose aux tuiles incomplètes. `load_court` charge les célébrations (normales ou au naturel) d'un coup, rendu coupé ; `chr_borrow_restore` rétablit les célébrations normales en quittant le match (`swap_restore`). Revers : préparation quand la balle arrive côté revers ; armé à deux mains, ou revers coupé à une main si le joueur humain arme avec B ; impact et fin à deux mains, sauf le coupé (gestes à une main du coup droit en miroir) ; volée de revers à deux mains. `runfr` (`sprp.s`) a une entrée par pose (27).

### Dérapages selon la surface (`src/slip.s`, banque 3, version 2.42)

`slip_tick` est appelé par `match_tick` (`bank_call`) après les déplacements de tous les joueurs. Le déplacement voulu de la trame est la différence entre la position et celle de la trame précédente (`sp_x`, `sp_y`) ; il passe par la surface, avec une vitesse par joueur (`sv_x`, `sv_y`, 1/64 px par trame) :

- **glace** : `V += (D − V) / 4` (inertie) ; sans poussée, la vitesse décroît jusqu'à moins de 16/64 px puis s'annule ;
- **terre battue** : `V = D` en mouvement ; à l'arrêt et au demi-tour lancé (vitesse d'au moins 48/64 px dans l'autre sens), `V -= V / 4` à chaque trame (environ 6 px de glissade ; 2 px avant la version 2.43) ;
- **plage** : `V` rejoint `D` de 50/64 px par trame au plus (départ lourd) ; arrêt net ;
- gazon, dur et moquette : inchangés.

Seulement pendant l'échange et le point joué. Un saut de plus de 5 px (mise en place au service) remet la vitesse à zéro. En double, une IA ne bouge qu'une trame sur deux (au double de sa vitesse) : ses trames de repos sont ignorées et son accélération dans le sable est doublée. Les bornes (x 48 à 208, côté du filet) sont réappliquées. Un arrêt brusque ou un demi-tour (vitesse d'au moins 48/64 px) arme `slide_t` (8 trames) et note le départ (`sl_ox`, `sl_oy`) : `pose_fx` dessine aux pieds la poussière (terre, tuiles 6-7), des grains de sable ou des éclats de glace (tuile 15, blanc), et sur terre et glace une trace au point de départ. Calibrage (`tests/calib.mjs`, variable `SURF`) : le départ dans le sable a été adouci (36 → 50/64 px) pour que la plage ne pénalise pas davantage le joueur humain que le CPU.

## 5. Graphismes

### Animation des joueurs

`poses_all` (banque 3, en assembleur dans `src/poses.s`, appelée par `match_draw` via `bank_call` avant de dessiner les joueurs) choisit la pose et le miroir de chaque joueur (`pose_now`, `flip_cache`) :
- **attente** : rebond d'1 px toutes les 16 trames, décalé de 8 trames d'un joueur à l'autre (drapeau `POSE_LIFT`, bit 5 ; `POSE_DOWN`, bit 6, abaisse d'1 px). La position mémorisée pour la course reste celle du sol ;
- **split-step** : quand l'adversaire frappe, un joueur à l'arrêt fait un petit saut (`pl_jump` = 7), puis se réceptionne appuis écartés ;
- **préparation** : quand la balle vient vers lui à moins de 40 px, le joueur arme son coup droit, ou son revers à deux mains si la balle est à sa gauche ;
- **frappe**, en 3 temps : armé (coup droit, revers à deux mains, extension au-delà de 12 px, **volée** si la balle n'a pas rebondi, smash armé comme le service si elle est à plus de 24 px avant le rebond), puis **impact** (5 trames : bras tendu, ou geste propre à la volée, au smash ou à l'amortie), puis accompagnement propre au coup (le lob finit raquette vers le ciel, l'amortie reste retenue) ;
- **plongeon** (version 2.50 ; `dive_t`, `dive_f`, `dive_x`, `dive_y`, `dive_step`, `src/poses.s` ; décision `dive_try`, `src/shots.c`, banque 13) : il part **avant** la frappe. `dive_try` suit la balle trame par trame jusqu'à ce qu'elle soit frappable en profondeur (comme `can_hit`) : écart d (px) et trames n (12 au plus). Joueur humain : au lâcher du bouton (`ST_CHARGE` relâché), sans direction à tenir ; il plonge si 24 + n/2 < |d| < 46 (bien placé : frappe normale). Ordinateur : jamais sur une balle qui va sortir (`land_x`, `land_y`) ; il plonge si |d| dépasse ce qu'il atteindrait en courant (22 + 3/4 de sa vitesse × n) sans dépasser la portée du plongeon (24 + 3 (n - 2), 42 au plus), avec une probabilité de 40, 120 ou 220 / 256 par trame (Facile, Normal, Expert). Le moteur règle `dive_t` = 42 ; `dive_step` : 41 position retenue, 41-40 élan (fente), 39-34 vol de 3 px par trame (18 px), en arc (`dive_j` : 4 à 9 px de haut), 33 réception (poussière, sable ou éclats), 33-18 au sol, 17-10 relevé un genou à terre (pose 27, dessinée 4 px plus bas par `match_draw` et `rp_play`), 9-8 en garde. Jusqu'à 8, le joueur ne peut pas bouger. Pendant l'élan, le vol et 3 trames au sol (`dive_t` ≥ 30), le moteur frappe dès que `can_hit` le permet : `dive_hit`, lob de sauvetage (B + HAUT, sans puissance, écart 24). Avant la version 2.50, le plongeon partait après la frappe, comme une simple animation ;
- **course en 4 images** (foulée A, passage, foulée B, passage, relevés d'1 px), de face, de dos et de profil (4 images de profil) ; **pas chassés** (appuis serrés et écartés) quand la balle arrive tout près ; **glissade** avec poussière sur terre battue et sur glace en extension ou en plongeon ;
- **avant le service** : le serveur fait rebondir la balle 2 fois (balle dessinée par `poses_all`, petit bruit de rebond), puis la garde en main ; le receveur, genoux fléchis, se balance d'un pied sur l'autre ;
- **après le point** (match) : le gagnant fête à sa façon selon le personnage (poing serré, bras levés, ou poing serré en sautillant ; Margaux fait une pirouette, son miroir alterne) ; le perdant met les mains sur les genoux après un long échange (fatigue), tape le sol avec sa raquette après une faute (dehors, filet), ou baisse la tête sinon.

Performances : la première version en C coûtait environ 2 000 cycles par trame en double. En assembleur, et en recalculant chaque joueur une trame sur deux en double (les compteurs avancent à chaque trame), la charge reste celle d'avant : 257 trames d'échange au-delà de 27 000 cycles sur 11 416 en double (244 avant), 0 en simple. Le code des poses a quitté la banque fixe, pleine.

Ralentis : l'octet de pose d'un joueur garde la pose (bits 0-4), le miroir (bit 5) et le saut (bits 6-7). Le côté n'est plus enregistré : `sc_swap` ne change qu'après le ralenti.

- **Fond** : 247 tuiles sur 256. Cinq écrans de court compressés en RLE (stade 657 octets, club 648, salle 647, club nudiste 648, court Margaux 675). La surface et les conditions ne changent que les palettes.
- **Sprites** : 253 tuiles sur 256 (dont la coupe, les confettis et 4 tailles de balle avec la couture qui tourne).
  - Course : de profil (4 images, miroir selon le sens) dès que le déplacement est surtout latéral, de face ou de dos sinon ; la direction est mémorisée par personnage dans `spr_person` (banque fixe) pour ne pas clignoter.
  - Raquettes : tamis ovale de 5 × 7 px (5 × 5 au-dessus de la tête), cadre sombre et cordage en damier, manche de 2 à 3 px.
  - Balle : 4 × 4 près du sol, 5 × 5 à partir de 12 px de hauteur, 6 × 6 à partir de 20 px, 7 × 7 à partir de 40 px (vrais lobs), aussi dans les ralentis. À l'échelle du court (environ 13 px par mètre), une vraie balle ferait moins d'un pixel : 4 px est le minimum lisible en mouvement. La taille de 6 px (version 2.35) évite le saut de 5 à 7 px, et la grande balle, plus large qu'une tête de joueur, est réservée aux lobs. L'ombre reste au sol et **rétrécit** quand la balle monte : grande (4 × 2) sous 12 px, moyenne, puis petite (2 × 1) à partir de 40 px. `spr_ball` est en assembleur (`src/sprp.s`, banque fixe) depuis la version 2.34.
  - **Couture qui tourne** : chaque taille a 4 dessins, couture blanche plus ou moins haute. Lifté : un quart de tour toutes les 3 trames, la couture va dans le sens du déplacement à l'écran ; coupé : en sens inverse ; amortie : en sens inverse, toutes les 2 trames ; à plat : immobile. On voit ainsi l'effet de la balle adverse.
  - **Rebond écrasé** : quand la balle touche le sol (hauteur sous 2 px après avoir été au-dessus), elle est dessinée aplatie (6 × 3) pendant 3 trames. Une balle qui roule ne l'est pas.
  - **Derrière le filet** : dans le camp du haut, quand le centre de la balle est dans la bande du filet (y 124 à 134, entre les poteaux), elle est dessinée en damier : on la voit à travers les mailles. Dans le camp du bas, elle passe devant.
  - **Couleur** : la balle utilise la palette des effets (couleur 1 : balle, 2 : bleu de la pluie et de la sueur, 3 : blanc de la couture et des étoiles). L'arbitre garde ainsi sa peau dans la palette 2. Couleur par court (`ball_col`, `gfx.c`) : $28, le jaune NES le plus proche d'une vraie balle de tennis, partout où il se voit bien ; sur le sable jaune de la plage, couleur approchante la plus lisible, vert-jaune vif $2A. Les gouttes et éclaboussures passent en couleur 2 (bleu, bleu clair sous la pluie).
  - 2 vues (dos et face) × 2 silhouettes (homme et femme) × 23 poses : attente, course ×4 (2 foulées, 2 passages), coup droit armé, accompagnement, service (lancer), joie / appuis écartés, déception, revers à deux mains, extension, impact du service, préparation, impact, volée, amortie, rebond de balle avant le service, poing serré, colère (raquette au sol), fatigue (mains sur les genoux), plongeon, un genou à terre (relevé du plongeon : seule la rangée des jambes est nouvelle, 2 tuiles). Le smash armé reprend la pose du service et la fin du lob celle de l'impact du service : mêmes gestes, aucune tuile de plus.
  - **Réserve commune de tuiles** : beaucoup de rangées de 2 tuiles (tête, buste, jambes) se répètent d'une pose à l'autre. `gen_assets.py` ne range chaque rangée qu'une fois, à partir de la tuile $10. Une rangée n'est partagée que si elle est identique dans la version habillée **et** dans la version au naturel, si bien que les deux réserves ont les mêmes numéros et la même taille (180 tuiles pour 22 poses, `people_bytes` = 2 880 octets ; il en faudrait 576 sans partage). Pour partager davantage, la queue de cheval vue de dos s'arrête au cou : les bustes sont communs aux deux vues. `person_map_m` et `person_map_f` (une table par silhouette, 132 octets chacune) donnent, pour chaque (silhouette, vue, pose), la tuile gauche de chacune des 3 rangées (la droite suit) ; `prof_map` fait de même pour la course de profil. `spr_meta` lit ces 3 tuiles (`_meta_tile`, `_meta_t1`, `_meta_t2`) au lieu de 6 tuiles consécutives.
  - 16 palettes de personnages.
  - Partenaires de double : couleurs de leur équipe (limite de 4 palettes de sprites).
- **Score** : dans les coins supérieurs, un panneau de 4 caractères par équipe : nom ; « J » + jeux du set + `*` (serveur) ; points + un rond par set gagné (glyphe `@` de la police). Les joueurs sont limités à x = 48..208 et la colonne 0 n'est pas utilisée (zone rognée par les téléviseurs).
- **Service** : le lancer monte à environ 56 px. Il est affiché à sa hauteur réelle pour le serveur du bas, et réduit (× 9/16) pour celui du haut afin que la balle reste à l'écran. Les joueurs humains se déplacent à 2,19 px/trame.

### Lisibilité des couleurs

`tools/contrast.py` mesure, sur la palette réelle de la NES, l'écart de couleur perçu (ΔE CIE76) entre chaque couleur de chaque personnage (peau, tenue, cheveux/raquette) et le court et ses abords des 6 surfaces, de jour, de nuit et sous la pluie.

Les surfaces, les 16 tenues et la règle d'assombrissement (nuit, pluie : un cran plus sombre, jamais jusqu'au noir des cheveux et des raquettes) ont été choisies pour que **toutes les paires dépassent ΔE 25**, sans liseré. Avant ce réglage, plusieurs couleurs étaient strictement identiques : la peau sur la terre battue, des tenues de la couleur exacte d'un court. Le minimum est maintenant de 26.

L'outil contrôle aussi la balle sur chaque court, contre la surface et les abords, de jour, de nuit et sous la pluie : jaune de balle de tennis ($28) partout, vert-jaune vif ($2A) sur le sable jaune de la plage. L'écart est d'au moins ΔE 40 partout (le jaune $28 serait invisible sur le sable : ΔE 0).

```sh
python3 tools/contrast.py 25    # liste les paires sous le seuil (aucune actuellement)
```

## 6. Audio

Le jeu n'utilise que l'APU natif :
- **priorités du bruit** : chaque bruitage du canal de bruit a une priorité et une durée (`noise(env, période, priorité, durée)`). Tant qu'un son plus important n'est pas fini, un son moins important est ignoré. Ordre : geste (0), rebond (1), frappe, ovation, applaudissement (2), filet, frappe parfaite, feu d'artifice (3), service canon, « OOOH », soupir (4). Les pas ne jouent que sur un canal libre ;
- **pendant le match**, la musique est muette, donc le carré 2 et le triangle sont libres. La frappe ajoute au « pok » une note grave et brève au triangle (compteur linéaire d'environ 40 ms, plus grave quand la frappe est forte). Un service faux (dehors ou dans le filet) déclenche le même « bip » de juge électronique qu'une balle dehors (`sfx_line`, appelé par `serve_fault`), à la place de l'ancien bourdonnement. Le crissement des chaussures (demi-tours brusques sur dur) a été supprimé en version 2.32, à la demande ;
- **bip du juge électronique** (`sfx_line`, carré 1, 1/12 s) quand un point se perd sur une balle dehors ; souffle bref (`sfx_swing`) au geste d'un joueur humain ;
- **carré 1 + bruit** : bruitages, dont des bruits de pas très discrets à chaque foulée (bruit sourd au volume 1, jamais par-dessus un rebond, une frappe ou le public) ;
- **musiques à 4 voix** (version 2.39) : carré 2 (mélodie), triangle (basse), carré 1 (harmonie) et bruit (percussions). L'harmonie et les percussions cèdent la place aux bruitages : un bruitage sur le carré 1 réserve le canal pendant 12 trames (`p1_hold`), les percussions attendent la fin de `noise_hold` ;
- **DMC** (version 2.39) : échantillons DPCM de la voix de l'arbitre et du « pok » de la balle.

**Musiques à 4 voix.** Les partitions (mélodie et basse) sont dans `tools/music_src.c`. `tools/gen_music.py` ajoute à chaque morceau une harmonie, calculée sur l'accord dont la basse donne la fondamentale (tierce et quinte diatoniques en do majeur ; une note qui frotterait d'un demi-ton ou d'un ton contre la mélodie est remplacée par une autre note de l'accord) : « oum-pa » en contretemps pour la polka et la valse, accords tenus pour la marche, l'hymne, la victoire et l'interlude. Il ajoute aussi des percussions (grosse caisse, caisse claire, charleston au bruit, volume constant et durée courte). Chaque voix a la durée exacte de la mélodie : les boucles restent calées. Les jingles du match (point, jeu, défaite) gardent une seule voix. Le résultat, `src/music_data.c` (2,2 Ko), est dans la banque 10 : `music_play` copie le morceau demandé (au plus 510 octets) en RAM de cartouche, où la NMI le lit quelle que soit la banque en place. La banque fixe y gagne la place des échantillons.

**Voix de l'arbitre et « pok ».** `tools/gen_voice.py` synthétise « Out! », « Fault! » et « Let! » avec espeak-ng (voix anglaise), coupe les silences, rééchantillonne à 7 919 Hz et encode en DPCM (1 bit par pas, ±2 sur un compteur de 0 à 127). Chaque échantillon part de 0 et y revient par des rampes douces : entre deux sons, $4011 reste à 0 et le triangle et le bruit gardent leur volume. Depuis la version 2.40, la voix est normalisée puis saturée en douceur (tanh) et occupe toute la plage du compteur (64 ± 60) ; à 36 ± 32, elle était couverte par le bip et le public. Pendant une annonce, le bip passe au volume 3 et le canal de bruit est réservé 30 trames (le murmure du public est coupé). Le « pok » est calculé (sinus de 900 Hz amorti et pointe de bruit, 30 ms, 21 307 Hz). `src/dpcm.s` (versionné ; la compilation n'a pas besoin d'espeak-ng) place les 4 échantillons (1 280 octets) dans le segment DPCM de la banque fixe, aligné sur 64 octets (le DMC ne lit que $C000-$FFFF). Annonces : « Out! » avec le bip d'une balle dehors, « Fault! » avec celui d'un service faux, « Let! » avec le sifflet ; « pok » à chaque frappe, sans jamais couper une annonce. Au court Margaux, la princesse annonce 2 crans de fréquence plus haut (voix plus aiguë).

**Manettes et DMC.** Une lecture du DMC pendant la lecture de la manette peut lui faire perdre un bit (défaut connu de la NES). Pendant un échantillon ($4015 bit 4), `read_pads` lit les manettes deux fois, jusqu'à deux lectures identiques.

**Lecteur dans la NMI.** `audio_tick` (assembleur, `src/sprp.s`) est appelé par la NMI à chaque trame, même quand le rendu est coupé ; en 50 Hz, il l'est une fois de plus toutes les 5 trames, pour garder le tempo du 60 Hz. Auparavant, la boucle principale appelait `audio_update`. Pendant la construction d'un écran (rendu coupé, 2 à 3 trames), la note en cours s'allongeait donc et tout le reste se décalait : 12 trames de retard cumulées sur le générique, avec un premier raté dès la 2e scène. Mesuré après correction : les 118 notes de la mélodie et les 41 de la basse tombent à la trame près. `music_play` et `snd_set` positionnent `audio_lock` pendant qu'ils changent l'état, pour que la NMI n'en lise pas un à moitié écrit. Les bruitages (carré 1, bruit) restent déclenchés par le jeu. Effet secondaire : en vitesse « rapide », les jingles ne sont plus accélérés.

Aucune musique pendant les matchs. Toutes les mélodies sont originales :
- **polka de l'écran titre et du menu** (`MUS_TITLE`) : petite polka de cirque ridicule, en 2 temps, 16 mesures de 48 trames (12,8 s) en boucle. Mélodie liée en croches de 12 trames (carré 12,5 % « kazoo » à volume constant : une enveloppe décroissante faisait s'éteindre les noires, ce qui s'entendait comme des trous), broderies chromatiques (Ré♯–Mi, Fa♯–Sol) et une chute d'une octave en fin de phrase ; « oum-pa » du triangle qui saute du grave à l'octave à chaque croche. Aucun silence dans aucune des deux voix ; la dernière mesure remonte vers le Do de la première, si bien que la boucle ne s'entend pas. Elle continue sans repartir du début quand on passe du titre au menu (et retour) ;
- **valse du court Margaux** : boîte à musique en 3/4 (16 mesures, carré 25 % à volume constant). Elle joue dans le menu dès que ce lieu est choisi, puis sur l'écran de présentation ;
- **interlude de fin de set** : air calme de 8 mesures, en boucle sur l'écran récapitulatif, coupé dès la reprise ;
- **thème de la finale** : marche de 8 mesures, sur l'écran d'étape et la présentation de la finale d'une compétition (finale du tournoi, des poules, ou de chaque tournoi de la saison) ;
- jingles de point, de jeu, de victoire et de défaite.

**Notes tenues (version 2.29)** : la fanfare du générique, la valse de Margaux, l'interlude, le thème de la finale et la fanfare de victoire utilisaient une enveloppe décroissante qui éteignait chaque note en 0,3 à 0,5 s : les notes longues se coupaient avant la suivante et la musique semblait hachée (mesuré dans l'émulateur : mélodie presque muette 134 trames sur 470 dans la fanfare, 69 sur 480 dans la valse ; après correction, 2 et 18, uniquement les silences écrits). Tous ces morceaux sont maintenant à volume constant ; les notes répétées sont séparées par un silence d'1 trame écrit dans la partition. `audio_tick` écrit $4007 à chaque note : toutes les notes sont relancées de la même façon. En version 2.29, il ne l'écrivait que si l'octet changeait, pour éviter la remise à zéro de la forme d'onde ; mais alors seules certaines notes (celles qui changent d'octet de période, par exemple sol 4 ↔ la 4) avaient ce petit clic, ce qui s'entendait comme des ratés (version 2.31). Les jingles courts gardent l'enveloppe décroissante (une note qui s'éteint).

`music_cur` mémorise le morceau en cours, pour ne pas relancer un thème déjà joué.

**Unité de balayage** : sur la vraie NES, un canal carré réglé « balayage montant, décalage 0 » ($4001/$4005 = $00) calcule en permanence une période cible double et coupe le son au-delà de $7FF, même quand le balayage est désactivé : toute note de période supérieure à $3FF serait muette. Tous les sons et la musique écrivent donc $08 (sens descendant) quand ils n'utilisent pas le balayage. jsnes ne simulant pas cette coupure, `emu_test` la vérifie lui-même sur toutes les notes jouées.

**Réglage SON** (`snd_set`) : 0 = musique et bruitages ; 1 = bruitages seuls (`music_play` ne fait que noter le morceau demandé) ; 2 = aucun son (canaux de l'APU coupés par $4015).

**Rebond selon la surface** : la période du bruit et la durée de l'enveloppe dépendent de `cfg.surface` (terre sourde, gazon doux, dur sec, moquette mate, sable feutré, glace claquante).

Coup gagnant : un « ting » montant bref sur le carré 1 (`sfx_winner`), en plus de la réaction du public déjà présente. Le nom du coup (`win_label`, banque fixe) dépend du dernier coup du gagnant (`last_shot`), d'un adversaire au filet au moment de la frappe (`hit_pass`) et du saut (`hit_jump`).

## 7. NTSC / PAL

- **Détection** : au démarrage, une boucle de 16 cycles est comptée entre deux VBlank (NTSC ≈ 1 861 ; PAL > 1 970).
- **Compensation** : à 50 Hz, une trame logique supplémentaire est exécutée toutes les 5 trames, ce qui garde la vitesse du 60 Hz.

### Vitesse du jeu (option VITESSE JEU)

Elle s'applique pendant le match seulement (`cfg.gspeed`, boucle principale de `main.c`) :
- **lent** : une trame logique sur 4 est sautée (75 %). Les appuis de cette trame sont gardés pour la suivante : un appui bref n'est jamais perdu. Le son continue normalement ;
- **rapide** : une trame logique sans dessin est ajoutée toutes les 4 trames (125 %). Sans le dessin, le coût reste dans le budget : en simple, 2,2 % de trames en retard (contre 0,5 % en normal). En double, c'est 10 % contre 4 %, donc de légers à-coups possibles.

Elle se combine avec la compensation 50 Hz.

## 8. Performances (mesurées avec `tests/profile.mjs`)

| Mesure (CPU contre CPU, ralentis activés) | Simple | Double |
|---|---|---|
| Logique médiane par trame | 15 400 cycles | 18 900 cycles |
| Trames d'échange en dépassement (> 27 000 cycles) | 0 / 5 973 | 27 / 5 950 (0,45 %) |
| Toutes trames en dépassement | 27 / 5 973 | 74 / 5 950 (1,2 %) |

En double, avant l'optimisation de la version 2.4, 664 trames sur 5 749 (11 %) dépassaient : les deux IA d'une même équipe décidaient dans la même trame.

Le budget est d'environ 27 000 cycles par trame NTSC, hors NMI. Une trame en dépassement retarde l'image d'une trame, sans corrompre l'affichage : la NMI n'envoie rien tant que la trame n'est pas prête.

Les quelques dépassements restants ont lieu sur les trames de frappe, pendant les transitions entre points et au début des ralentis.

Optimisations appliquées :
- sprites, enregistrement des ralentis et trampolines en assembleur ;
- calculs 16 bits uniquement ;
- hauteur au filet sans division ;
- prédictions de l'IA en cache ;
- en double, IA mise à jour une trame sur deux (aussi pendant les balles mortes). Les deux IA d'une équipe décident en alternance (une seule décision par trame), et le partenaire réagit 2 trames plus tard ;
- tri des sprites sur des clés 8 bits ; multiplications par constantes remplacées par des décalages ;
- affichage des personnages en assembleur (`src/sprp.s`) ;
- sur une trame de frappe, sprites non redessinés et IA restantes reportées.

## 9. Compilation

Outils : `cc65` 2.19, `python3` (Pillow facultatif), `make` ; pour les tests, `gcc` et `node` ≥ 18.

```sh
make          # génère les graphismes, compile -> build/royal_martinot.nes (+ occupation par banque)
make test     # règles, codes, puis suite complète sur émulateur (~10 min)
make clean
```

La compilation est reproductible : deux compilations propres donnent le même fichier (empreinte dans le rapport).

Outils de mise au point :
- `node tests/profile.mjs build/royal_martinot.nes [double] [trames]` : cycles par fonction, banques comprises ;
- `node tests/shots.mjs build/royal_martinot.nes dossier` : captures d'écran de référence.

## 10. Tests

| Suite | Contenu |
|---|---|
| `tests/rules_test.c` | 42 vérifications des règles |
| `tests/password_test.c` | 200 000 codes aléatoires en aller-retour ; 8,06 millions de codes altérés d'un signe, refusés à 99,98 % |
| `tests/emu_test.mjs` | 250 vérifications : générique de lancement (scènes, fanfare à la trame près et notes tenues sans coupure, palette jamais écrite pendant l'affichage, barre à 117 %, mentions, « clap », lettre tombée puis remise, passage en deux appuis), démarrage MMC1 et CHR-RAM, mode d'emploi, menus adaptatifs, mode en équipe (toujours en double, partenaire CPU ou manette 2), présentation, menu de pause, lancer du serveur du haut, balle ratée qui sort de l'écran, let, matchs complets (6 surfaces × 3 lieux × 3 conditions, double), ralentis, revanche, palmarès, 9 exercices (joueur robot via la manette), 5 compétitions (dont éliminations), générique de fin après une saison gagnée (défilement sur 2 écrans, textes, retour au titre), double en compétition (dont à 2 joueurs humains), podium (classement, joueurs agrandis au pixel près, marches, noms, double avec le 3e en sprites, Margaux, coupe agrandie, 5 feux d'artifice à pleine vitesse puis le calme, CHR-RAM rétablie), court Margaux, déblocage et club nudiste (tenues échangées en CHR-RAM puis restaurées), reprise par code, vitesse du jeu (lent/normal/rapide, aucun appui perdu), coups gagnants nommés, à plat plus rapide que le lifté et croisé court en angle, HAUT/BAS relatifs au côté du joueur (avec A et avec B), bruitages (souffle, « pok » au triangle, bip dehors, bip du service faux, priorités du bruit, pas de crissement), smash sauté (réservé aux humains et au CPU Expert), musiques hors match (même polka au titre et au menu, sans reprise, en boucle ; Margaux, finale) et silence en match, smash sauté du CPU Expert et dans les ralentis, ombres, poussière, chaise d'arbitre (identique dans tous les lieux et conditions), tête qui suit la balle, gestes de l'arbitre et arbitre visible pendant les ralentis, princesse arbitre au court Margaux, coupe rétablie hors du match, repère d'atterrissage, réglage SON, réglages mémorisés après « rallumage », Margaux (menu, présentation, abréviation MX, code de reprise, chevelure propre, jamais au club nudiste), service canon (fenêtre de 2 trames mesurée en frappant à chaque trame du lancer, vitesse, compteurs, jamais pour le CPU Normal), frappe parfaite (fenêtre de 3 trames mesurée en relâchant à chaque trame, placement, compteurs, +30 % de vitesse au moins), retour sur le timing à l'entraînement, rotation des sprites en double (aucun sprite masqué en permanence), fondus, récapitulatif du score (bandeau, écran de fin de set avec interlude, fermeture par n'importe quel bouton, reprise), bandeau devant la tête du joueur du haut, panneaux Margaux et du club nudiste à côté du score (tuiles empruntées rétablies au court suivant), filet qui tremble, fatigue avec et sans l'option (déplacements plus lents, frappes plus rapides), public (regard qui suit la balle, ola rétablie, gradins de la salle et bancs du club, réactions sonores), menu sans ligne JEU contre le CPU et redessiné sans couper l'écran, fête de victoire (étoiles, confettis, bonds, fanfare de 160 trames, 3 feux d'artifice ; rien si le CPU gagne), balle (couleur par court, 4 tailles selon la hauteur, ombre qui rétrécit, rebond écrasé, couture qui tourne selon l'effet, derrière le filet), aucune note coupée par l'unité de balayage (comportement de la vraie console simulé), IA tactique selon le score (attaque menée, première balle assurée sur balle de break), 7 lieux et 21 personnages au menu, animation des joueurs (réserve de tuiles, revers à deux mains, service en 3 temps, préparation, impact, volée ou amortie, joie, colère, fatigue, rebonds de balle avant le service, receveur genoux fléchis, split-step, rebond d'attente, course en 4 images, poses des ralentis sur 5 bits), 50 Hz |
| `src/main.c` (hasard) | `rand8` (LFSR 16 bits, crt0.s) part d'une graine fixe ; hors du match, la boucle principale l'appelle à chaque trame et mêle `frame_cnt` à `rand_seed` à chaque appui : la durée passée dans les menus rend le tirage au sort, le premier serveur et la suite des échanges différents d'une partie à l'autre |
| `tests/run_tests.mjs` | Lanceur de la suite : `make test` (ou `node run_tests.mjs rom`) répartit les 44 sections sur un processus par cœur, en équilibrant les durées mesurées au passage précédent (`tests/section_times.json`), et affiche les résultats dans l'ordre ; `make test-only T=bande-annonce,plongeon` (ou `--only=`) ne lance que les sections dont le nom contient l'un des mots : contrôle en quelques secondes d'une modification localisée. `emu_test.mjs` accepte aussi `--only=`, `--sections=0,4,7` et `--list`. Règle suivie : tests ciblés pour une retouche localisée ; suite complète pour toute modification du moteur commun (minutage, chargements, règles, aléatoire) et avant chaque livraison |
| `tools/contrast.py 25` | Contraste (ΔE ≥ 25) de chaque personnage avec chaque court, de jour, de nuit et sous la pluie ; balle ; tenue de Margaux face à toutes les autres tenues. Lancé par `make test` |
| `tools/check_texts.py` | Relecture des textes : caractères présents dans la police, colonnes, lignes du mode d'emploi, annonces de 14 caractères au plus |

Les résultats détaillés sont dans le [rapport](RAPPORT.md).
