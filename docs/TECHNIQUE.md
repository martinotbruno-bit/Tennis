# Pixel Open — Documentation technique

## 1. Vue d'ensemble

| Élément | Choix |
|---|---|
| Plateforme | NES / Famicom, NTSC et PAL (Dendy traité comme PAL) |
| Cartouche | **MMC1 (SNROM, mapper 1)** : 256 Ko de PRG en 16 banques de 16 Ko (128 Ko jusqu'à la version 2.9), 8 Ko de CHR-RAM, 8 Ko de PRG-RAM (avec pile facultative) |
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

**Mise sous tension** : l'état initial du MMC1 n'est pas garanti. Chaque banque se termine donc par un petit programme (`STUBn`) qui réinitialise le mapper (ce qui fixe la dernière banque, la 15, en $C000) puis saute au vrai démarrage.

## 3. Organisation mémoire

### Banques de PRG (octets utilisés / 16 368)

| Banque | Contenu | Occupation |
|---|---|---|
| 15 (fixe, $C000) | Démarrage, NMI, primitives (sprites, VRAM, manettes, trampolines `bank_call` / `far_call`, enregistrement des ralentis, affichage des personnages), boucle principale, règles, palettes, 17 personnages, noms des compétitions, échange des tuiles de personnages, effets du court, son, bibliothèque cc65 | 13 819 |
| 0 | Moteur de match : physique, frappes, service, arbitrage, IA | 16 054 |
| 1 | Écrans : titre, configuration, présentation, résultats, fin de set, compétitions, levée de coupe, saisie de code, palmarès | 15 899 |
| 2 | Motifs graphiques (8 Ko), copiés en CHR-RAM au démarrage | 8 192 |
| 3 | Annexes du match : score et annonces du club nudiste, courts (5 lieux), panneau du court Margaux, public animé, pluie, ralentis, exercices, tutoriel | 10 012 |
| 4 | Mode d'emploi | 3 636 |
| 5 | Compétitions (tirages, simulations, classements, équipes de double), palmarès et codes de reprise (`password.c`, appelé par `far_call`) | 9 773 |
| 6 | Tuiles des personnages « au naturel » (club nudiste), copiées en CHR-RAM à la demande | 3 456 |
| 7 à 14 | Libres (environ 128 Ko) pour les évolutions futures | 0 |

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
| `src/manual.c` | 4 | Mode d'emploi (8 pages) |
| `src/comp.c`, `src/stats.c` | 5 | Compétitions, palmarès |
| `tools/gen_assets.py` | — | Génère tous les graphismes : police, logo, 3 courts, public assis et debout, 10 personnages × 8 poses |

### Effets du court (`match_fx`, banque fixe)

Appelé à la fin de `match_draw`, après les joueurs (les effets passent donc derrière eux) :
- **ombres** : une tuile ellipse sous les pieds. Toujours en simple ; en double, seulement pendant un saut, pour rester sous 8 sprites par ligne quand quatre joueurs sont alignés ;
- **poussière** (terre, plage) et **éclaboussure** (pluie) : 12 trames au point de rebond, en deux tailles. Le premier et le deuxième rebond sont détectés par le changement de `bounces` ;
- **arbitre de chaise** (stade) : deux sprites sur la chaise du décor. La tête change selon la moitié du court où se trouve la balle ;
- **repère d'atterrissage** (vitesse Lent) : une croix sur `land_x/land_y` pendant 30 trames après chaque frappe destinée à une équipe où joue un humain.

Les tuiles de sprites 5 à 13 (auparavant libres) contiennent l'ombre, la poussière, les éclaboussures, la croix et l'arbitre.

### Margaux (17e personnage) et codes de reprise

Margaux occupe l'index 16, avec le style FANTAISIE (9e style : amorties, lobs, montées au filet). Elle a les cheveux longs, blonds et bouclés :
- **Tuiles** : la table de sprites n'avait que 18 tuiles libres ($0F, $EF à $FF). Seule la rangée de la tête (2 tuiles) lui est propre : 4 variantes de bras (attente, frappe armée, accompagnement, service) × 2 vues, plus 1 tête de profil commune aux deux images de course, soit 18 tuiles. Le corps reste celui des joueuses. `spr_person` remplace les deux premières entrées OAM après `spr_meta`, en échangeant gauche et droite en miroir. Les ralentis et tous les écrans en profitent.
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
- vitesse 84 au lieu de 64 (balle environ 30 % plus rapide, mesure 312 contre 237 FP par trame), gravité 11 ;
- dispersion 1 en première balle et 6 en deuxième balle, au lieu de 7 plus l'imprécision du timing ;
- `sfx_canon` (bruit sourd et claquement descendant), bandeau « SERVICE CANON » ;
- traînée de deux points derrière la balle (`match_fx`) tant que le service n'a pas rebondi.

Le CPU Expert le réussit une fois sur deux quand son tirage le fait frapper au sommet (environ 12 % de ses premières balles) ; Facile et Normal jamais. Compteurs : `st_canon` (réussis) et `st_ctry` (services A des humains) par match. Au palmarès, ils sont cumulés dans un bloc séparé de la RAM de cartouche (`sx`, signature « PC ») : la disposition du palmarès ne change pas, et SELECT × 2 les efface aussi.

Taux de réussite attendu pour un adulte (estimation, pas une mesure), avec une régularité de geste de 40 à 60 ms en début de pratique et d'environ 25 ms entraîné : 25 à 30 % au début, 35 à 40 % après une heure ou deux, environ 50 % pour un joueur assidu.

### Frappe parfaite (`perfect_hit`, banque fixe)

Pour les coups lifté, à plat et coupé joués après le rebond :
- **Placement** : écart latéral entre la balle et le joueur de 5 à 14 px. La frappe part dès que la balle entre dans la zone ; la profondeur n'est donc pas un critère, car la balle est toujours prise au bord avant de la zone.
- **Timing (humain)** : la frappe doit tomber dans les 3 premières trames du geste (`pl_timer` ≥ 12), c'est-à-dire bouton relâché 0 à 2 trames avant l'entrée de la balle dans la zone (`swing_early`). Si le bouton est encore enfoncé à l'arrivée, la frappe part pendant l'armement : frappe normale.
- **CPU** : tirage 40/256 en Expert et 10/256 en Normal, sous la même condition de placement ; jamais en Facile.
- **Effet** : vitesse +25 %, dispersion 1, puissance d'armement ignorée ; `sfx_perfect` ; éclat (étoile blanche, palette 2) près de la raquette pendant 8 trames ; « COUP PARFAIT ! » si le coup est gagnant.
- **Compteurs** : `st_perf` et `st_ptry` (coups lifté, à plat, coupé des humains) par match ; au palmarès, `sx.pok` et `sx.ptry` (bloc `sx`, signature « PD » ; les compteurs du service canon ont été remis à zéro une fois lors de ce changement).

`can_hit` a été déplacée du moteur (banque 0) vers la banque fixe pour faire la place (606 octets libérés).

### Panneau de fond de court (court Margaux) et annonces centrées

- **Panneau** : petit panneau de 32 × 16 px (4 × 2 tuiles) au fond du court, derrière la ligne de fond du haut, côté gauche (colonnes 6 à 9, rangées 4 et 5), à l'écart du bandeau et du joueur au service. Il utilise les couleurs du court : face prune, lettres « MARGAUX » blanches en petite police 3 × 5, liseré clair sur le dessus, rebord sombre, deux pieds et une ombre portée tramée, pour l'effet de volume. Le sol rose autour est la couleur du court, donc le raccord est invisible. La table des décors étant pleine, ses 8 tuiles sont chargées en CHR-RAM par `load_court` à la place des 8 premières tuiles du public du club nudiste, inutilisées sur ce court. Elles sont rétablies (`crowd_norm3`) au chargement de tout autre court.
- Toutes les annonces du bandeau sont centrées sur leurs 14 caractères (`center14`, banque fixe), y compris celles de l'entraînement, du tutoriel et du club nudiste. Les deux lignes du récapitulatif du score ont la même longueur et restent alignées.

### Bandeau devant le joueur du haut

Au service, le joueur du haut a les pieds 2 px derrière sa ligne de fond (y = 46) : sa tête (y 22 à 29) entre dans le bandeau du score et des annonces (lignes 0 à 31). `spr_person` donne alors aux 2 sprites de sa rangée de tête la priorité « derrière le fond » (bit 5 de l'attribut) quand cette rangée est entièrement dans le bandeau. Le fond du bandeau est transparent : les lettres des annonces passent devant et restent lisibles, et la tête reste visible entre elles. Le reste du sprite, sur le court, garde la priorité normale. Le terrain et la position de service ne changent pas.

### Récapitulatif du score

- **Après chaque jeu** (et au début d'un jeu décisif) : `recap_pend` est armé dans `after_point`. Quand l'annonce « JEU » s'efface, `score_recap` (banque fixe) écrit une ligne par équipe dans le bandeau pendant 120 trames : nom sur 4 lettres, jeux de chaque set terminé, puis jeux du set en cours (`score_line`, 14 caractères pour 5 sets).
- **Après chaque set** (hors démonstration) : `match_tick` renvoie 2, et la boucle principale ouvre l'écran `SCR_SETBREAK` (banque des écrans). Il affiche le tableau des sets, le vainqueur du set, les statistiques et le joueur au service, sur l'interlude `MUS_BREAK` (8 mesures en boucle, carré 25 %). N'importe quel bouton des deux manettes, ou 600 trames, demande `GO_RESUME` sans fondu (`fade_skip`) : retour au court en 4 trames. `match_resume` coupe la musique, recharge le court, les palettes et les tuiles (y compris au naturel), puis affiche le récapitulatif dans le bandeau. L'état du match n'est pas touché.
- **En fin de match** : le même tableau des sets (`set_table`) ouvre l'écran des résultats.

### Fondus entre les écrans

La NMI et `ppu_on` envoient la palette assombrie de `fade_lvl` (0, 16, 32, 48 ou 64, soit un cran de luminosité NES par 16 ; en dessous du plus sombre, noir). `ppu_off` fait un fondu au noir en 3 crans de 2 trames, en continuant d'appeler `audio_update`, puis coupe l'écran. Le nouvel écran s'allume noir, et `logic_tick` le rallume d'un cran toutes les 2 trames. Quand le même écran est reconstruit (changement de MODE au menu, pages du mode d'emploi ou du palmarès), `fade_skip` supprime le fondu : la navigation reste instantanée et aucun appui n'est perdu.

### Rotation de l'ordre des sprites (seulement si nécessaire)

Le PPU n'affiche que les 8 premiers sprites d'une ligne (dans l'ordre de l'OAM). En double, quand au moins 3 joueurs sont dans une même bande de 24 px (les quatre au filet, par exemple), une ligne peut porter jusqu'à 12 sprites, ombres comprises. `match_draw` fait alors commencer l'écriture des sprites à `spr_rot = frame_cnt × 36`, soit 9 sprites plus loin à chaque trame : ce n'est plus toujours le même sprite qui est écarté, il clignote au lieu de disparaître. Sinon `spr_rot = 0` et l'ordre reste fixe. `spr_meta` écrit ses deux sprites l'un après l'autre pour que l'index reboucle sur 256 octets (sinon, un personnage à cheval sur la fin de la table débordait sur le tampon VRAM et bloquait le jeu ; trouvé par le test). Les ombres sont donc affichées aussi en double.

### Affichage des personnages en assembleur (`src/sprp.s`)

`spr_person` était la fonction la plus coûteuse de chaque trame (environ 570 cycles par personnage, passage des 7 arguments compris). Sa version assembleur fait la même chose : rognage, tuiles, course de profil, bruit de pas, tête de Margaux. Mesure en double, 6 000 trames : trames d'échange en dépassement passées de 90 à 27, aucune en simple.

### Retour sur le timing (entraînement)

`serve_fb` (service A : écart au sommet en images, d'après la vitesse verticale, qui perd 6 par trame) et `perfect_hit` (placement, puis `swing_early` = valeur de `pl_timer` à l'impact, 0 si le bouton est encore enfoncé) écrivent l'annonce « TROP TOT DE n IMAGES », « TROP TARD DE n IMAGES », « RELACHEZ LE BOUTON AVANT », « BALLE TROP PRES/LOIN DU CORPS », « COUP PARFAIT ! » ou « SERVICE CANON ». Uniquement en mode entraînement.

### Smash sauté

Une balle entre 40 et 62 px de haut, avant le rebond, est jouable si le joueur est à moins de 70 px du filet. C'est réservé aux humains et au CPU Expert, pour ne pas enlever le lob aux joueurs moyens. La frappe est plus rapide (96 au lieu de 80) et plus tendue (gravité 12). Le sprite saute de 12 px (j²/16), au plus haut à l'impact, puis retombe en 14 trames (`pl_jump`). j/2 est rangé dans les bits 5 à 7 de `pose_cache`, que `rp_record` enregistre déjà : le saut apparaît dans les ralentis sans agrandir le tampon (16 octets par trame).

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

52 bits de données (dont double, partenaire, « partenaire joué par la manette 2 » et lieu sur 3 bits) + 2 bits de version + 11 bits de somme de contrôle = 65 bits, écrits en 13 symboles de 5 bits (alphabet sans I, O, 0, 1). Les symboles sont brouillés en chaîne pour qu'une erreur en modifie plusieurs.

### Compétitions en double

- **Tirage** : avec 17 personnages, les 8 équipes d'un tableau sont toutes différentes. Le joueur et le partenaire choisi forment l'équipe 1 ; 14 des 15 autres personnages sont mélangés par la graine et appariés deux à deux.
- **Simulation** : les mêmes formules qu'en simple, avec le niveau de l'équipe.
- **Deux joueurs** : option « JOUE PAR : CPU / MANETTE 2 ». Avec la manette 2, `cfg.ctrl[2]` passe à la manette 2 pour tous les matchs de la compétition ; le réglage est mémorisé dans le code de reprise.
- **Affichage** : les noms sont abrégés en « AL/MA ».

### Levée de coupe

Écran de fin de compétition en cas de victoire, en sprites :
- le joueur (et son partenaire) en pose bras levés ;
- la coupe (2 × 2 tuiles) monte des mains au-dessus de la tête puis oscille ;
- 10 confettis tombent, multiplexés sur les 4 palettes.

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

Grâce à la CHR-RAM, les 6 motifs de spectateurs sont remplacés par leur version debout, bras levés, puis restaurés. Le remplacement se fait à raison d'une tuile (16 octets) par trame, pour rester dans le budget VBlank.

## 5. Graphismes

- **Fond** : 247 tuiles sur 256. Cinq écrans de court compressés en RLE (stade 657 octets, club 648, salle 647, club nudiste 648, court Margaux 675). La surface et les conditions ne changent que les palettes.
- **Sprites** : 215 tuiles sur 256 (dont la coupe, les confettis et 3 tailles de balle).
  - Course : de profil (2 images de foulée, miroir selon le sens) dès que le déplacement est surtout latéral, de face ou de dos sinon ; la direction est mémorisée par personnage dans `spr_person` (banque fixe) pour ne pas clignoter.
  - Raquettes : tamis ovale de 5 × 7 px (5 × 5 au-dessus de la tête), cadre sombre et cordage en damier, manche de 2 à 3 px.
  - Balle : petite (4 × 4) près du sol, moyenne (5 × 5) à partir de 12 px de hauteur, grande (7 × 7) à partir de 30 px (`spr_ball`, banque fixe), aussi dans les ralentis. L'ombre reste au sol.
  - 2 vues (dos et face) × 2 silhouettes (homme et femme) × 8 poses : attente, course ×2, frappe ×2, service, joie, déception.
  - 16 palettes de personnages.
  - Partenaires de double : couleurs de leur équipe (limite de 4 palettes de sprites).
- **Score** : dans les coins supérieurs, un panneau de 4 caractères par équipe : nom ; « J » + jeux du set + `*` (serveur) ; points + un rond par set gagné (glyphe `@` de la police). Les joueurs sont limités à x = 48..208 et la colonne 0 n'est pas utilisée (zone rognée par les téléviseurs).
- **Service** : le lancer monte à environ 56 px. Il est affiché à sa hauteur réelle pour le serveur du bas, et réduit (× 9/16) pour celui du haut afin que la balle reste à l'écran. Les joueurs humains se déplacent à 2,19 px/trame.

### Lisibilité des couleurs

`tools/contrast.py` mesure, sur la palette réelle de la NES, l'écart de couleur perçu (ΔE CIE76) entre chaque couleur de chaque personnage (peau, tenue, cheveux/raquette) et le court et ses abords des 6 surfaces, de jour, de nuit et sous la pluie.

Les surfaces, les 16 tenues et la règle d'assombrissement (nuit, pluie : un cran plus sombre, jamais jusqu'au noir des cheveux et des raquettes) ont été choisies pour que **toutes les paires dépassent ΔE 25**, sans liseré. Avant ce réglage, plusieurs couleurs étaient strictement identiques : la peau sur la terre battue, des tenues de la couleur exacte d'un court. Le minimum est maintenant de 26.

L'outil contrôle aussi la balle sur chaque court : jaune pâle ($38) partout, sauf sur le sable jaune de la plage où elle est blanche ($30). L'écart est ainsi d'au moins ΔE 51 sur toutes les surfaces.

```sh
python3 tools/contrast.py 25    # liste les paires sous le seuil (aucune actuellement)
```

## 6. Audio

Le jeu n'utilise que l'APU natif :
- **carré 1 + bruit** : bruitages, dont des bruits de pas très discrets à chaque foulée (bruit sourd au volume 1, jamais par-dessus un rebond, une frappe ou le public) ;
- **carré 2 + triangle** : musiques et jingles.

Aucune musique pendant les matchs. Toutes les mélodies sont originales :
- **titre** (menus) ;
- **valse du court Margaux** : boîte à musique en 3/4 (16 mesures, carré 25 %, enveloppe courte). Elle joue dans le menu dès que ce lieu est choisi, puis sur l'écran de présentation ;
- **interlude de fin de set** : air calme de 8 mesures, en boucle sur l'écran récapitulatif, coupé dès la reprise ;
- **thème de la finale** : marche de 8 mesures, sur l'écran d'étape et la présentation de la finale d'une compétition (finale du tournoi, des poules, ou de chaque tournoi de la saison) ;
- jingles de point, de jeu, de victoire et de défaite.

`music_cur` mémorise le morceau en cours, pour ne pas relancer un thème déjà joué.

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
| `tests/password_test.c` | 200 000 codes aléatoires en aller-retour ; 8,06 millions de codes altérés d'un signe, refusés à 99,98 % |
| `tests/emu_test.mjs` | 173 vérifications : démarrage MMC1 et CHR-RAM, mode d'emploi, menus adaptatifs, mode en équipe (toujours en double, partenaire CPU ou manette 2), présentation, menu de pause, lancer du serveur du haut, balle ratée qui sort de l'écran, let, matchs complets (6 surfaces × 3 lieux × 3 conditions, double), ralentis, revanche, palmarès, 9 exercices (joueur robot via la manette), 5 compétitions (dont éliminations), double en compétition (dont à 2 joueurs humains), levée de coupe, court Margaux, déblocage et club nudiste (tenues échangées en CHR-RAM puis restaurées), reprise par code, vitesse du jeu (lent/normal/rapide, aucun appui perdu), coups gagnants nommés, smash sauté (réservé aux humains et au CPU Expert), musiques hors match (Margaux, finale) et silence en match, smash sauté du CPU Expert et dans les ralentis, ombres, poussière, arbitre, repère d'atterrissage, réglage SON, réglages mémorisés après « rallumage », Margaux (menu, présentation, abréviation MX, code de reprise, chevelure propre, jamais au club nudiste), service canon (fenêtre de 2 trames mesurée en frappant à chaque trame du lancer, vitesse, compteurs, jamais pour le CPU Normal), frappe parfaite (fenêtre de 3 trames mesurée en relâchant à chaque trame, placement, compteurs), retour sur le timing à l'entraînement, rotation des sprites en double (aucun sprite masqué en permanence), fondus, récapitulatif du score (bandeau, écran de fin de set avec interlude, fermeture par n'importe quel bouton, reprise), bandeau devant la tête du joueur du haut, fatigue avec et sans l'option, 50 Hz |
| `tools/contrast.py 25` | Contraste (ΔE ≥ 25) de chaque personnage avec chaque court, de jour, de nuit et sous la pluie ; balle ; tenue de Margaux face à toutes les autres tenues. Lancé par `make test` |
| `tools/check_texts.py` | Relecture des textes : caractères présents dans la police, colonnes, lignes du mode d'emploi, annonces de 14 caractères au plus |

Les résultats détaillés sont dans le [rapport](RAPPORT.md).
