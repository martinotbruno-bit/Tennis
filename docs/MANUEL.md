# Pixel Open — Manuel du joueur

Pixel Open est un jeu de tennis 8 bits original pour NES et Famicom. Il se joue seul contre l'ordinateur ou à deux, en simple comme en double.

![Aperçu](img/apercu.png)

## 1. Démarrer

1. Lancez la ROM `dist/pixel_open.nes` sur une console (via une cartouche flash) ou dans un émulateur.
2. À l'écran titre :
   - **START** ouvre l'écran de configuration ;
   - **SELECT** ouvre la saisie d'un code de tournoi.

Sans action pendant 20 secondes, une démonstration démarre (CPU contre CPU). Appuyez sur **START** ou **A** pour revenir au titre.

## 2. Configuration de la partie

**HAUT/BAS** choisit une ligne, **GAUCHE/DROITE** modifie la valeur. **A** sur « COMMENCER », ou **START** n'importe où, lance la partie. **B** revient au titre.

Tous les modes, personnages et terrains sont disponibles dès le départ. Les réglages sont conservés entre deux parties.

| Ligne | Choix |
|---|---|
| MODE | 1 joueur contre CPU · 2 joueurs adversaires · 2 joueurs en équipe · Tournoi · Entraînement · Service · Démo |
| JEU | Simple · Double |
| JOUEUR 1 / JOUEUR 2 | Alex, Maya, Leon, Nina, Omar, Zoé (3 hommes, 3 femmes) |
| NIVEAU CPU | Facile · Normal · Expert |
| SURFACE | Terre battue · Gazon · Dur · Moquette |
| CONDITIONS | Jour · Nuit · Pluie |
| FORMAT | 1 set · 3 sets (2 gagnants) · 5 sets (3 gagnants) · Rapide (1 set de 4 jeux) |

En double, les partenaires sont choisis automatiquement parmi les personnages restants. Ils portent la tenue de leur équipe.

## 3. Commandes

### Pendant l'échange

| Commande | Action |
|---|---|
| Croix | Se déplacer sur toute sa moitié de court |
| **A** (maintenir puis relâcher) | Frappe liftée. Plus vous maintenez **A** avant l'impact, plus la frappe est puissante (et risquée) |
| **B** | Coup coupé (slice) : balle basse et lente |
| **A** + HAUT | Frappe à plat, longue et rapide |
| **A** + BAS | Croisé court, lifté |
| **B** + HAUT | Lob |
| **B** + BAS | Amortie |
| GAUCHE / DROITE au moment de la frappe | Viser le long de la ligne gauche ou droite (sans direction : croisé naturel) |
| **START** | Pause. **SELECT** pendant la pause quitte la partie |

La frappe part automatiquement dès que la balle entre dans votre zone de frappe pendant que vous armez (bouton maintenu) ou pendant le geste (juste après le relâchement).

Le type de coup dépend aussi de la situation :
- balle prise avant le rebond : **volée** ;
- balle haute avant le rebond : **smash** ;
- balle loin du corps : frappe en extension, moins précise.

### Au service

1. Déplacez-vous latéralement derrière la ligne de fond (GAUCHE/DROITE).
2. **A** ou **B** lance la balle.
3. **A** frappe un service puissant, **B** un service coupé plus sûr.
   - Frappez au sommet du lancer pour être précis.
   - GAUCHE/DROITE pendant la frappe vise l'extérieur ou le T du carré.
   - Si vous ne frappez pas, la balle retombe et le lancer est rejoué sans pénalité.

## 4. Règles

Les règles modernes du tennis s'appliquent :
- points 0, 15, 30, 40, égalité, avantage ;
- jeux, sets avec deux jeux d'écart ;
- **jeu décisif** à 6-6 (4-4 en format rapide), en 7 points avec deux points d'écart ;
- service alterné à chaque jeu, du côté droit puis gauche ;
- deux fautes de service = **double faute** ; une balle de service qui touche le filet et ne passe pas est une faute ;
- **changement de côté** après chaque jeu impair, tous les 6 points du jeu décisif, et en fin de set si le total de jeux est impair ;
- en double : les **couloirs** sont en jeu (sauf au service), l'ordre des serveurs alterne entre les partenaires, et le receveur dépend du côté (égalité ou avantage).

Le jeu annonce à l'écran « ÉGALITÉ », « AVANTAGE », « BALLE DE SET », « BALLE DE MATCH », « JEU DÉCISIF », « CHANGEMENT DE CÔTÉ », « FAUTE », « DOUBLE FAUTE », « ACE », etc.

## 5. Affichage

- **En haut à gauche** : l'équipe qui joue en bas de l'écran. **En haut à droite** : celle du haut.
- Chaque panneau affiche le nom, les sets et jeux (« 1-3 ») puis les points. L'étoile `*` indique le serveur.
- **Au centre du bandeau** : les annonces de l'arbitre.

Les statistiques détaillées apparaissent à la fin du match : points gagnés, aces, doubles fautes, coups gagnants, fautes, pourcentage de premières balles et échange le plus long. Écran de fin : **A** = revanche immédiate, **B** = menu.

## 6. Terrains et conditions

| Surface | Comportement |
|---|---|
| Terre battue | Rebond haut, balle ralentie : échanges longs |
| Gazon | Rebond bas, balle qui fuse : jeu rapide, avantage au service-volée |
| Dur | Compromis équilibré |
| Moquette | Rapide et assez bas |

L'effet de la balle modifie le rebond : le lifté rebondit plus haut, le coupé reste bas.

- **Nuit** : décor assombri, court éclairé.
- **Pluie** : gouttes à l'écran, rebonds plus bas et balle qui glisse.

## 7. Modes

- **Match** : simple ou double, contre le CPU ou à deux.
- **2 joueurs en équipe** : en double, les manettes 1 et 2 jouent ensemble contre deux joueurs CPU.
- **Tournoi** : quart de finale, demi-finale puis finale contre des adversaires de plus en plus forts. Une défaite élimine.
- **Entraînement (lanceur de balles)** : 20 balles. Une balle renvoyée dans le court rapporte 1 point ; une balle dans la zone cible (étoile, fond de court) en rapporte 2.
- **Service** : 20 services pour travailler la précision ; les côtés alternent.
- **Démo** : deux équipes CPU s'affrontent.

## 8. Codes de tournoi

Avant chaque manche, l'écran du tournoi affiche un **code de 6 lettres**. Il contient la manche, le personnage, la surface, le format, les conditions et le nombre de trophées gagnés.

Pour reprendre plus tard :
1. À l'écran titre, appuyez sur **SELECT**.
2. HAUT/BAS change la lettre, GAUCHE/DROITE change de position.
3. **START** valide.

Un code mal saisi est refusé : une somme de contrôle détecte les erreurs de saisie.

## 9. Conseils

- Placez-vous **à côté** de la trajectoire, pas dessus : la balle doit passer à portée de raquette.
- Armez tôt (maintenez **A**) sur les balles lentes pour frapper fort ; sur les balles rapides, une frappe courte suffit.
- Viser les lignes avec beaucoup de puissance augmente le risque de faute.
- Contre un joueur au filet, le **lob** (B + HAUT) est redoutable. Contre un joueur au fond, essayez l'**amortie** (B + BAS).
- Sur gazon, montez au filet ; sur terre battue, construisez le point depuis le fond.
