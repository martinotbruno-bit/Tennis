# Pixel Open — Manuel du joueur

Pixel Open est un jeu de tennis 8 bits original pour NES et Famicom. Il se joue seul contre l'ordinateur ou à deux, en simple comme en double, en match, en compétition ou à l'entraînement.

Ce manuel est aussi intégré au jeu : à l'écran titre, choisissez **MODE D'EMPLOI** (8 pages ; GAUCHE/DROITE pour changer de page, B pour revenir).

![Aperçu](img/apercu.png)

## 1. Écran titre

**HAUT/BAS** choisit une entrée et **A** la valide. **START** ouvre directement la configuration.

| Entrée | Rôle |
|---|---|
| JOUER | Configuration de la partie |
| MODE D'EMPLOI | Les commandes et les règles, dans le jeu |
| PALMARÈS | Statistiques cumulées, titres, historique, records |
| CODE DE REPRISE | Reprendre une compétition avec son code |

Sans action pendant 20 secondes, une démonstration démarre (alternativement en simple et en double). **A** ou **START** rend la main.

## 2. Configuration

**HAUT/BAS** choisit une ligne, **GAUCHE/DROITE** change la valeur. **START** lance la partie, **B** revient au titre. Les lignes affichées dépendent du mode. Tout le contenu est disponible dès le départ.

| Ligne | Choix |
|---|---|
| MODE | 1 joueur contre CPU · 2 joueurs adversaires · 2 joueurs en équipe · Compétition · Entraînement · Démo |
| JEU | Simple · Double |
| JOUEUR 1 / JOUEUR 2 | 16 personnages (8 hommes, 8 femmes), chacun avec son style de jeu. En compétition de double, JOUEUR 2 devient PARTENAIRE |
| NIVEAU CPU | Facile · Normal · Expert |
| SURFACE | Terre battue · Gazon · Dur · Moquette · Plage · Glace (les deux dernières sont des terrains fantaisie) |
| LIEU | Stade (tribunes pleines) · Club (extérieur, arbres) · Salle (intérieur) · Club nudiste (bonus à débloquer, voir § 9 bis) |
| CONDITIONS | Jour · Nuit · Pluie. Il ne pleut pas en salle : la pluie y devient « jour » |
| FORMAT | 1 set · 3 sets · 5 sets · Rapide (sets de 4 jeux) |
| RALENTIS | Oui/Non : ralenti automatique après les plus beaux points |

Selon le mode, d'autres lignes apparaissent :
- **Compétition** : type de compétition et, pour le tournoi personnalisé, le nombre de joueurs, la progression des adversaires et la variété des surfaces ;
- **Entraînement** : l'exercice et, pour l'exercice personnalisé, le côté, la vitesse, l'effet et le nombre de balles.

La ligne **STYLE** indique le style de jeu de l'adversaire choisi.

| Personnage | Style |
|---|---|
| Alex, Ines | Équilibré |
| Maya, Luca | Fond de court |
| Leon, Ivan | Puissance |
| Nina, Lina | Filet (service-volée) |
| Omar, Jade | Défensif |
| Zoé, Tom | Offensif |
| Hugo, Noah | Tactique (lobs, amorties) |
| Emma, Sara | Placement (précision) |

Avant chaque match, un **écran de présentation** rappelle l'épreuve, les joueurs, leur niveau et leur style, la surface, le lieu, les conditions et le format. **A** lance le match.

## 3. Commandes

### Pendant l'échange

| Commande | Action |
|---|---|
| Croix | Se déplacer sur toute sa moitié de court |
| **A** (maintenir puis relâcher) | Frappe liftée. Plus vous armez longtemps avant l'impact, plus la frappe est forte (et risquée) |
| **B** | Coup coupé (slice) : balle basse et lente |
| **A** + HAUT | Frappe à plat, longue et rapide |
| **A** + BAS | Croisé court, lifté |
| **B** + HAUT | Lob |
| **B** + BAS | Amortie |
| GAUCHE / DROITE à l'impact | Viser le long d'une ligne ; sans direction, le coup part croisé |
| **START** | Pause : CONTINUER ou QUITTER (HAUT/BAS puis A). SELECT quitte directement |

La frappe part toute seule quand la balle entre dans votre zone de frappe pendant que vous armez, ou juste après avoir relâché le bouton.

Le type de coup dépend aussi de la situation :
- balle prise avant le rebond : **volée** ;
- balle haute avant le rebond : **smash** ;
- balle loin du corps : frappe en extension, moins précise.

### Au service

1. Placez-vous derrière la ligne de fond avec GAUCHE/DROITE.
2. **A** ou **B** lance la balle.
3. Au sommet du lancer, **A** frappe un service puissant, **B** un service coupé plus sûr.
   - Plus la frappe est proche du sommet, plus le service est précis.
   - GAUCHE/DROITE à l'impact vise l'extérieur ou le T.
   - Si vous ne frappez pas, la balle retombe et le lancer est à refaire, sans pénalité.

Le lancer reste visible quelle que soit votre position, y compris quand vous servez du haut de l'écran.

## 4. Règles

- Points 0, 15, 30, 40, égalité, avantage ; il faut deux points d'écart pour gagner le jeu.
- Sets de 6 jeux avec deux jeux d'écart ; **jeu décisif** en 7 points à 6-6 (à 4-4 en format rapide).
- Le service alterne à chaque jeu, du côté droit puis gauche.
- **Fautes** : deux fautes de service font une **double faute**. Un service qui touche la bande du filet puis tombe dans le bon carré est un **let** : il est à remettre, sans faute.
- Une balle qui touche la bande pendant l'échange et passe reste en jeu.
- **Changement de côté** après chaque jeu impair, tous les 6 points du jeu décisif, et en fin de set si le total de jeux est impair.
- **En double** :
  - les couloirs sont en jeu, sauf au service ;
  - les partenaires servent à tour de rôle ;
  - chacun reçoit de son côté (égalité ou avantage).

## 5. Affichage

- Les deux **panneaux de score** occupent les coins supérieurs, au-dessus des tribunes ; les joueurs ne peuvent pas les masquer.
  - Coin gauche : l'équipe qui joue en bas de l'écran ; coin droit : celle du haut.
  - Chaque panneau affiche le nom, les sets et jeux (« 1-3 »), puis les points. `*` désigne le serveur.
- Les **annonces** de l'arbitre s'affichent au centre du bandeau : faute, let, égalité, avantage, balle de set ou de match, jeu décisif, changement de côté, ace, coup gagnant…
- Le **public** se lève et applaudit après les beaux points. Après un ace, un coup gagnant ou un long échange, un **ralenti** rejoue le point (A pour le passer).
- **En fin de match** :
  - le vainqueur célèbre et le perdant baisse la tête ;
  - l'écran de résultats donne les statistiques : points gagnés, aces, doubles fautes, coups gagnants, fautes, pourcentage de premières balles, plus long échange ;
  - **A** relance une revanche, **B** revient au menu.

## 6. Surfaces, lieux et conditions

| Surface | Comportement |
|---|---|
| Terre battue | Rebond haut, balle ralentie : échanges longs |
| Gazon | Rebond bas, balle qui fuse : avantage au service-volée |
| Dur | Compromis équilibré |
| Moquette | Rapide et assez bas |
| Plage (fantaisie) | Sable : balle très lente, rebond très haut |
| Glace (fantaisie) | Balle qui file au ras du sol |

L'effet modifie aussi le rebond : le lifté rebondit plus haut, le coupé reste bas.

Conditions :
- **Nuit** : décor assombri.
- **Pluie** : gouttes à l'écran, rebonds plus bas et balle qui glisse.

## 7. Modes

- **Match** : simple ou double, contre l'ordinateur ou à deux.
  - **2 joueurs en équipe** : en double, les manettes 1 et 2 jouent ensemble contre deux joueurs CPU.
  - **2 joueurs adversaires** : en double, chacun a un partenaire CPU.
- **Démo** : l'ordinateur joue seul.

### Compétitions

| Compétition | Déroulement |
|---|---|
| Tournoi | 8 joueurs, élimination directe : quart, demi, finale |
| Poules | 2 poules de 4 (3 journées). Les 2 premiers de chaque poule vont en demi-finales croisées, puis en finale |
| Championnat | 6 joueurs, toutes rondes (5 journées). Classement aux victoires, départage par confrontation directe |
| Saison | 4 tournois de 8 joueurs sur terre, gazon, dur puis moquette. Points : 45 (quart), 90 (demi), 150 (finale), 250 (victoire). Classement général à la fin |
| Personnalisé | 4 ou 8 joueurs ; adversaires de niveau fixe ou progressif ; même surface ou surface différente à chaque tour |

Toutes les compétitions se jouent **en simple ou en double** (ligne JEU) :
- en double, vous choisissez votre partenaire (ligne PARTENAIRE), puis qui le joue (ligne JOUE PAR) : l'ordinateur ou la **manette 2**. À deux joueurs humains, vous formez une seule équipe contre les équipes CPU ;
- les autres équipes sont tirées au sort parmi les 16 personnages, qui sont tous différents.

Quand vous gagnez une compétition, votre joueur (et votre partenaire) **brandit la coupe** sous une pluie de confettis.

L'écran d'étape montre le prochain adversaire (niveau, surface), le tableau ou le classement, et le **code de reprise**. Les matchs entre joueurs CPU sont simulés selon leur niveau.

### Entraînement

| Exercice | Contenu |
|---|---|
| Frappes / cibles | 20 balles ; 1 point par balle dans le court, 2 dans la zone de l'étoile |
| Déplacements | Balles alternées dans les deux coins |
| Régularité | Série sans faute la plus longue (jusqu'à 50) |
| Précision | Seule la zone désignée (fond ou court, gauche ou droite) rapporte des points |
| Volée | Vous êtes au filet ; balles tendues à volleyer |
| Service | 20 services ; bonus si le service tombe à l'extérieur ou au T |
| Joueur fixe | Échanges contre un adversaire immobile qui renvoie les balles au centre |
| Personnalisé | Côté (coup droit, revers, alterné, aléatoire), vitesse, effet (lifté, coupé, lob) et nombre de balles (10, 20, 30) |
| Tutoriel | Leçon guidée : frapper, amortie, lob, viser, servir |

Votre meilleur score de chaque exercice est conservé dans le palmarès.

## 8. Codes de reprise

Dans une compétition, l'écran d'étape affiche un **code de 13 signes** (lettres et chiffres). Il contient la compétition, l'étape, vos résultats, votre personnage (et, en double, votre partenaire et qui le joue), les réglages et le nombre de trophées.

Pour reprendre plus tard :
1. À l'écran titre, choisissez **CODE DE REPRISE** (ou appuyez sur **SELECT**).
2. HAUT/BAS change le signe, GAUCHE/DROITE change de position.
3. **START** valide.

Une somme de contrôle de 12 bits détecte les erreurs de recopie.

## 9. Palmarès

Le palmarès comporte 3 pages (GAUCHE/DROITE) :
- **Statistiques** : matchs joués, gagnés et perdus en simple et en double ; points, aces, doubles fautes, coups gagnants, fautes, pourcentage de premières balles ; titres et compétitions disputées ; records (plus long échange, aces en un match).
- **Résultats** : meilleur résultat par type de compétition et historique des 8 dernières.
- **Records d'entraînement** : meilleur score par exercice.

Seuls les matchs où la manette 1 joue sont comptés ; en double, ce sont les statistiques de son équipe (donc aussi celles de la manette 2 si elle joue le partenaire). Pour tout effacer, appuyez deux fois sur **SELECT**.

Le palmarès est conservé tant que la console reste allumée. Sur une cartouche équipée d'une pile, il est conservé même éteinte.

## 9 bis. Bonus : le club nudiste

Gagnez une compétition (tournoi, poules, championnat, saison ou personnalisé, en simple ou en double) pour débloquer un nouveau lieu : le **club nudiste « Les Bronzés »**.
- Les joueurs y jouent au naturel. Les silhouettes sont couleur peau, sans aucun détail ; une serviette en écharpe aux couleurs de l'équipe permet de distinguer les camps.
- Pas de gradins : le public, lui aussi au naturel, est assis sur ses serviettes dans l'herbe et se lève pour applaudir.
- L'arbitre adapte ses annonces au lieu : « FAUTE — PAS DE POCHE ! », « ÉGALITÉ — NU À NU », « COUP GAGNANT — BRONZAGE TOTAL », « CHANGEMENT DE CÔTÉ — ET DE CRÈME ! »… En pause, il rappelle la crème solaire.

Le déblocage est gardé dans le palmarès (définitivement si la cartouche a une pile). Un code de reprise contenant au moins un trophée débloque aussi le club sur une autre console.

## 10. Conseils

- Placez-vous **à côté** de la trajectoire, pas dessus : la balle doit passer à portée de raquette.
- Armez tôt sur les balles lentes ; sur les balles rapides, une frappe courte suffit.
- Viser les lignes avec beaucoup de puissance augmente le risque de faute.
- Contre un volleyeur (Nina, Lina) : le **lob**. Contre un défenseur au fond (Omar, Maya) : l'**amortie**.
- L'ordinateur s'adapte : s'il voit que vous visez toujours le même côté, il s'y place à l'avance. Si vous montez souvent au filet, il lobe davantage. Variez votre jeu.
