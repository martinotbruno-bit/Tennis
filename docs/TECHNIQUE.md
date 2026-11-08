# Pixel Open — Documentation technique

## 1. Vue d'ensemble

| Élément | Choix |
|---|---|
| Plateforme | NES / Famicom, NTSC et PAL (Dendy traité comme PAL) |
| Cartouche | NROM-256 (mapper 0) : 32 Ko PRG + 8 Ko CHR, sans extension |
| Langages | C (cc65 2.19) pour la logique ; assembleur 6502 (ca65) pour le démarrage, la NMI et les routines critiques |
| ROM | `dist/pixel_open.nes` (40 976 octets, iNES) |
| Occupation | PRG environ 28,4 Ko / 32 Ko (87 %), RAM 622 / 768 octets, page zéro 45 / 240 octets |

## 2. Choix du mapper

| Option | Avantages | Inconvénients | Décision |
|---|---|---|---|
| **NROM-256 (mapper 0)** | Aucune logique de banques ; compatible avec toutes les consoles et cartouches de reproduction ; le plus simple à fabriquer | 32 Ko PRG, 8 Ko CHR fixes | **Retenu** pour la version 1.0 |
| UNROM (mapper 2) | Jusqu'à 256 Ko PRG, CHR-RAM | CHR en RAM à remplir par logiciel | Option d'évolution n° 1 |
| MMC1 (mapper 1) | PRG/CHR en banques, SRAM de sauvegarde possible | Écritures série plus lentes | Option si l'on veut une sauvegarde par pile |
| MMC3 (mapper 4) | IRQ de scanline (bandeau de score fixe, effets), banques fines | Puce plus complexe et plus chère | Pour une future version « deluxe » |
| Extensions audio (VRC6, MMC5, N163, FDS) | Canaux supplémentaires | Indisponibles sur NES occidentale, ou matériel spécifique | Non utilisées (CDC 11.4 : jamais indispensables) |

Avec 13 % de PRG libre, tout le contenu actuel tient sans banques. Le passage à UNROM ou MMC1 est prévu dès que l'espace manquera (voir le rapport final). Le code est déjà découpé en modules indépendants pour faciliter ce passage.

## 3. Organisation mémoire

```
CPU
$0000-$000F  libre (réservé)
$0010-$00FF  page zéro : variables NMI, manettes, sprites, pile C (sp), registres cc65
$0100-$01FF  pile matérielle 6502
$0200-$02FF  tampon OAM (copié par DMA à chaque trame)
$0300-$037F  tampon de mises à jour VRAM : [adresse hi, lo, longueur, données]... $FF
$0400-$06FF  données C (DATA + BSS)
$0700-$07FF  pile de paramètres C (cc65)
$8000-$FFF9  code et données (PRG)
$FFFA-$FFFF  vecteurs NMI / RESET / IRQ

PPU
$0000-$0FFF  table de motifs du fond : police ASCII ($20-$5F), court, logo, balle géante
$1000-$1FFF  table de motifs des sprites : balle, ombre, étoile, goutte, 4 x 7 poses de personnage
$2000-$23FF  écran (le court est stocké compressé en RLE : 657 octets au lieu de 1 024)
```

Les locaux C sont statiques (`--static-locals`) : la pile C ne sert qu'au passage d'arguments. Le test automatique vérifie qu'elle revient à `$0800` à chaque trame.

## 4. Architecture logicielle

| Module | Rôle |
|---|---|
| `src/crt0.s` | En-tête iNES, RESET, détection de région, NMI (DMA OAM, VRAM, palette), lecture des manettes, sprites rapides, RLE, générateur aléatoire |
| `src/main.c` | Machine d'états : titre, configuration, match, résultats, tournoi, saisie de code. Boucle principale et compensation 50 Hz |
| `src/game.c` | Moteur de match : joueurs (2 ou 4), physique de balle, frappes, service, arbitrage, IA, affichage du score, sprites |
| `src/rules.c` | Règles et score du tennis (C portable, testé sur PC) |
| `src/password.c` | Codes de sauvegarde (C portable, testé sur PC) |
| `src/gfx.c` | Palettes (surfaces, conditions, personnages), texte, sprites de personnage |
| `src/audio.c` | Bruitages, musiques et jingles |
| `tools/gen_assets.py` | Génère toutes les ressources graphiques (police, court, logo, sprites) de manière déterministe |

### Boucle de trame

1. `wait_frame()` publie la trame. La NMI suivante envoie l'OAM, les mises à jour VRAM et la palette, puis rend la main.
2. `pad_poll()` lit les deux manettes et calcule les fronts d'appui.
3. `logic_tick()` met à jour l'audio puis l'écran courant. Pendant un match : `match_tick()` (entrées, IA, balle, arbitrage, score), puis `match_draw()` (OAM).

La NMI n'envoie rien tant que le thread principal n'a pas fini sa trame. Un dépassement occasionnel retarde donc l'image d'une trame, sans jamais l'abîmer.

### Physique de la balle

- Positions et vitesses en virgule fixe 10.6 (1/64 de pixel) ; hauteur `z` séparée, dessinée comme un décalage vertical au-dessus de l'ombre.
- Gravité par type de coup : plat 5, lifté 7, coupé 4, lob 6, amortie 9, smash 10 (en 1/64 px/trame²).
- `launch(tx, ty, T, g)` calcule les vitesses pour atteindre la cible en T trames. La hauteur au filet est évaluée sans division, par la formule de la parabole `z(t) = z0·(1 - t/T) + g·t·(T - t)/2`. Si la balle ne passe pas, T est allongé.
- Au rebond : coefficient de restitution et d'adhérence par surface, modifiés par l'effet (lifté +, coupé -) et par la pluie.
- Le filet est détecté au franchissement de sa ligne avec `z < NET_H` : la balle repart faiblement vers le frappeur.

### Arbitrage

- 1er rebond : il doit avoir lieu dans la moitié adverse, dans les limites du simple ou du double (couloirs), ou dans le bon carré pour un service.
- 2e rebond : point au frappeur (coup gagnant ; ace si le service n'a pas été touché).
- Sortie de l'écran sans rebond : faute ; après un rebond valide : point au frappeur.
- `rules.c` gère le score, le jeu décisif, l'alternance des services et les changements de côté. En double, le joueur au service alterne au sein de l'équipe chaque fois qu'elle récupère le service : cela vaut aussi pendant le jeu décisif.

### Intelligence artificielle

| Paramètre | Facile | Normal | Expert |
|---|---|---|---|
| Vitesse (px/trame) | 1,22 | 1,53 | 1,81 |
| Délai de réaction (trames) | 16 | 9 | 3 |
| Dispersion de visée (px) | 30 | 18 | 11 |
| Marge par rapport aux lignes (px) | 26 | 15 | 9 |
| Puissance | faible | moyenne | forte |

Comportements de l'IA :
- elle prédit le point de frappe : après le rebond annoncé, ou par interception de volée près du filet ;
- elle se place à côté de la balle et vise loin de l'adversaire ;
- elle varie ses coups : lob contre un volleyeur, amortie contre un joueur au fond (expert), coupé ;
- elle monte au filet après une balle courte ;
- au niveau facile, elle commet des fautes directes aléatoires.

Les niveaux diffèrent par leurs décisions et leur précision, pas seulement par leur vitesse.

En double, chaque équipe désigne un **responsable** pour chaque balle : le partenaire le plus proche du point de frappe prévu. L'autre couvre sa moitié du court, ce qui évite que les deux partenaires jouent la même balle. Au service, le partenaire du serveur se place au filet.

## 5. Graphismes

- **Palettes de fond** : 0 = court (fond universel, surface, lignes, filet) ; 1 = score et texte ; 2 = public ; 3 = réserve. La surface et les conditions (nuit, pluie) ne changent que les palettes : un seul écran de court sert à toutes les combinaisons.
- **Palettes de sprites** : 0 = équipe 1, 1 = équipe 2, 2 = balle et ombre, 3 = curseur, cible, gouttes.
- **Personnages** : 16 x 24 pixels (2 x 3 tuiles), 7 poses (attente, 2 de course, armé, accompagnement, service, joie), vus de dos (bas du court) ou de face (haut), en version homme et femme. Le revers est un miroir horizontal du coup droit.
- **Limite de 8 sprites par ligne** : au pire 2 joueurs côte à côte (4 sprites) + balle + ombre. Les gouttes de pluie s'affichent une trame sur deux, en alternance.
- Le **score** occupe les coins supérieurs, au-dessus des tribunes. Les joueurs sont limités à x = 48..208, donc aucun sprite n'y passe ; la colonne 0, rognée par de nombreux téléviseurs, n'est pas utilisée.

## 6. Audio

Le jeu utilise uniquement l'APU natif (2 canaux carrés, triangle, bruit) :
- **carré 1 + bruit** : bruitages (impact selon la puissance, rebond, filet, faute, sifflet, public, menus, lancer de balle) ;
- **carré 2 + triangle** : musique de titre en boucle et jingles (point, jeu, victoire, défaite).

Aucune musique ne joue pendant les échanges (CDC 11.2). Toutes les mélodies sont originales.

## 7. NTSC / PAL

- **Détection** : au démarrage, une boucle de 16 cycles est comptée entre deux VBlank (NTSC ≈ 1 861, PAL ≈ 2 078, Dendy ≈ 2 216). Au-delà de 1 970, la console est considérée comme 50 Hz.
- **Compensation** : à 50 Hz, une trame logique supplémentaire est exécutée toutes les 5 trames. Le jeu, l'audio et les animations gardent ainsi la vitesse du 60 Hz, sans imposer 60 images/s à l'affichage PAL (CDC 2.4).

## 8. Optimisations (mesurées)

Le profileur `tests/profile.mjs` compte les cycles 6502 par fonction dans l'émulateur.

| Mesure | Avant | Après |
|---|---|---|
| Double : trames d'échange en dépassement (> 27 000 cycles) | 11 % | 0,05 % (4 sur 8 000) |
| Double : logique médiane par trame | 22 100 cycles | 16 300 cycles |
| Simple : trames d'échange en dépassement | 1,2 % | 0 |
| Simple : logique médiane par trame | 11 800 cycles | 12 000 cycles (avec plus de fonctions) |

Optimisations appliquées :
- sprites (`spr1`, `spr_meta`, `spr_end`) en assembleur : environ 10 fois plus rapides qu'en C ;
- suppression des calculs 32 bits (multiplications et divisions `long`) dans la physique et l'IA ;
- hauteur au filet calculée sans division ;
- prédictions de l'IA en cache (recalcul une trame sur quatre) ;
- en double, chaque IA est mise à jour une trame sur deux, avec un pas doublé ;
- sur une trame de frappe, les sprites de la trame précédente sont conservés et les IA restantes passent leur tour.

Les seuls dépassements restants ont lieu pendant les transitions entre les points (balle morte : score, annonces). Ils n'ont aucune incidence sur les commandes.

**Point d'attention cc65** : la division d'un `int` signé par un `unsigned char` est faite en non signé. Toutes les divisions de vitesse passent donc par un diviseur `int` explicite (`launch()`).

## 9. Compilation

Outils nécessaires : `cc65` (2.19 : ca65, cc65, ld65), `python3` (Pillow facultatif, pour les aperçus), `make`, et pour les tests `gcc` et `node` ≥ 18.

```sh
make            # génère les ressources, compile -> build/pixel_open.nes
make test       # tests unitaires (règles, codes) + 62 vérifications sur émulateur
make clean
```

La compilation est **reproductible** : deux compilations successives produisent le même fichier, empreinte SHA-256 `cbd42b12d22be7183dbba1b9bdc54565892bad8844aac1f2efc1306cc2479f11`.

Outils de mise au point :
- `node tests/profile.mjs build/pixel_open.nes [double] [trames]` : profil CPU ;
- `node tests/sim.mjs build/pixel_open.nes niveau surface double` : match CPU complet avec statistiques ;
- `node tests/human.mjs build/pixel_open.nes trames niveau` : joueur robot utilisant uniquement la manette ;
- `node tests/shots.mjs build/pixel_open.nes dossier` : captures d'écran.

## 10. Tests

Voir le [rapport final](RAPPORT.md) pour les résultats détaillés.

| Suite | Contenu |
|---|---|
| `tests/rules_test.c` | 42 vérifications : points, égalité prolongée, sets, 7-5, jeu décisif (service, côtés, fin), matchs en 3 sets, format court |
| `tests/password_test.c` | Aller-retour de tous les codes valides (13 824) ; 1 244 160 codes altérés d'une lettre, refusés à 99,98 % |
| `tests/emu_test.mjs` | Démarrage, détection de région, NMI, menu, service humain, pause/abandon, 5 matchs complets (4 surfaces, 3 niveaux, jour/nuit/pluie, double), revanche enchaînée, pile C, entraînements, tournoi et code de reprise, compensation 50 Hz |

## 11. Limites connues

Voir la section « Limitations » du [rapport final](RAPPORT.md).
