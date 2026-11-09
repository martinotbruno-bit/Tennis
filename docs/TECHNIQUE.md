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
| 7 (fixe, $C000) | Démarrage, NMI, primitives (sprites, VRAM, manettes, trampolines `bank_call` / `far_call`, enregistrement des ralentis), boucle principale, règles, codes, palettes, 17 personnages, noms des compétitions, échange des tuiles de personnages, son, bibliothèque cc65 | 13 552 |
| 0 | Moteur de match : physique, frappes, service, arbitrage, IA | 15 973 |
| 1 | Écrans : titre, configuration, présentation, résultats, compétitions, levée de coupe, saisie de code, palmarès | 14 353 |
| 2 | Motifs graphiques (8 Ko), copiés en CHR-RAM au démarrage | 8 192 |
| 3 | Annexes du match : score et annonces du club nudiste, courts (4 lieux), public animé, pluie, ralentis, exercices, tutoriel | 9 899 |
| 4 | Mode d'emploi | 3 581 |
| 5 | Compétitions (tirages, simulations, classements, équipes de double) et palmarès | 7 451 |
| 6 | Tuiles des personnages « au naturel » (club nudiste), copiées en CHR-RAM à la demande | 3 456 |

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

Margaux occupe l'index 16 : tuiles féminines communes, palette propre (peau $27, tenue rose clair $35, cheveux châtains $07), style FANTAISIE (9e style : amorties, lobs, montées au filet). Son nom fait 7 lettres :
- les écrans à colonnes étroites passent par `team_label` (« MARG » sur 4 lettres, « MX » sur 2 lettres pour ne pas se confondre avec MAYA « MA ») ;
- en double, la présentation met le partenaire sur la ligne suivante.

Couleurs : trois tenues ont changé pour que le rose clair soit unique. IVAN passe du lilas au gris ($10), TOM du lavande au crème ($37), INES du pervenche au chocolat ($07). `tools/contrast.py` vérifie que la tenue de Margaux reste à ΔE ≥ 25 de toutes les autres.

**Codes, version 3** : personnage et partenaire passent sur 5 bits. Pour tenir dans les 52 bits de données, le type de compétition et les conditions sont regroupés (type × 3 + conditions, 15 valeurs sur 4 bits), de même que la surface et le lieu (surface × 5 + lieu, 30 valeurs sur 5 bits). Les codes restent à 13 signes. Les anciens codes (version 2) sont toujours lus. Vérifié par `password_test` : 200 000 allers-retours, plus un ancien code et Margaux en joueuse et en partenaire.

### Fatigue (option FATIGUE)

`tired` (0 à 10) vaut `rally_hits - 10` au-delà de 10 frappes dans l'échange, plafonné à 10 ; il est recalculé à chaque trame. La vitesse humaine perd `3 × tired` (140 → 110 FP par trame, -21 %), celle de l'IA `2 × tired` (-18 à -27 % selon le niveau) : la fatigue est la même pour tous. Une goutte de sueur (tuile 14) clignote au-dessus des joueurs dès le niveau 4 ; en double, seulement au-dessus des humains, pour la limite de sprites. L'option est désactivée à l'entraînement.

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
| Logique médiane par trame | 14 300 cycles | 19 700 cycles |
| Trames d'échange en dépassement (> 27 000 cycles) | 1 / 5 957 | 90 / 5 926 (1,5 %) |
| Toutes trames en dépassement | 45 / 5 957 | 151 / 5 926 (2,5 %) |

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
| `tests/emu_test.mjs` | 153 vérifications : démarrage MMC1 et CHR-RAM, mode d'emploi, menus adaptatifs, mode en équipe (toujours en double, partenaire CPU ou manette 2), présentation, menu de pause, lancer du serveur du haut, balle ratée qui sort de l'écran, let, matchs complets (6 surfaces × 3 lieux × 3 conditions, double), ralentis, revanche, palmarès, 9 exercices (joueur robot via la manette), 5 compétitions (dont éliminations), double en compétition (dont à 2 joueurs humains), levée de coupe, court Margaux, déblocage et club nudiste (tenues échangées en CHR-RAM puis restaurées), reprise par code, vitesse du jeu (lent/normal/rapide, aucun appui perdu), coups gagnants nommés, smash sauté (réservé aux humains et au CPU Expert), musiques hors match (Margaux, finale) et silence en match, smash sauté du CPU Expert et dans les ralentis, ombres, poussière, arbitre, repère d'atterrissage, réglage SON, réglages mémorisés après « rallumage », Margaux (menu, présentation, abréviation MX, code de reprise), fatigue avec et sans l'option, 50 Hz |
| `tools/contrast.py 25` | Contraste (ΔE ≥ 25) de chaque personnage avec chaque court, de jour, de nuit et sous la pluie ; balle ; tenue de Margaux face à toutes les autres tenues. Lancé par `make test` |
| `tools/check_texts.py` | Relecture des textes : caractères présents dans la police, colonnes, lignes du mode d'emploi, annonces de 14 caractères au plus |

Les résultats détaillés sont dans le [rapport](RAPPORT.md).
