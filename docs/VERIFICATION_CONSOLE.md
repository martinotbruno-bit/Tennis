# Royal Martinot — Liste de vérification sur vraie console

Le jeu a été testé uniquement sur émulateur. Cette liste sert à le vérifier pas à pas sur une vraie NES ou Famicom. Cochez chaque case et notez ce qui ne va pas, si possible avec une photo de l'écran. Comptez environ une heure et quart pour l'ensemble. Version du guide : 2.63.

## 0. Matériel

- [ ] Une console : NES (Europe = PAL, États-Unis = NTSC), Famicom (Japon, NTSC), ou un clone Dendy (PAL).
- [ ] Une cartouche flash compatible **MMC3** (mapper 4), par exemple de la gamme EverDrive N8 ou PowerPak (depuis la version 2.62 ; MMC1 avant). Elle doit gérer :
  - **256 Ko** de programme (mapper 4, carte TGROM ou équivalent) ;
  - 8 Ko de **CHR-RAM** (pas de CHR-ROM) ;
  - 8 Ko de RAM de sauvegarde.
- [ ] Le fichier `dist/royal_martinot.nes`, copié sur la carte mémoire de la cartouche.
- [ ] Si possible, deux manettes (le mode à deux joueurs utilise la manette 2).
- [ ] Notez le modèle de console, la cartouche et sa version de logiciel, et le type d'écran (télévision cathodique, écran plat, convertisseur HDMI).

> Sauvegarde : avec un EverDrive, activez la sauvegarde de la RAM de cartouche (fichier `.srm` ou `.sav`) dans le menu de la cartouche. Sans cela, le palmarès et les réglages sont perdus à l'extinction, ce qui est normal. Les codes de reprise restent la sauvegarde principale.

## 1. Démarrage

| # | Vérification | Attendu | OK ? |
|---|---|---|---|
| 1.1 | Allumer la console (cartouche réglée en mapper 4 si elle le demande) | Le générique démarre (« BRUNO MARTINOT PRESENTS », fanfare aux notes bien tenues, sans raté aux changements de scène), barre à 117 %, mentions légales, lettre N tombée puis remise par le stagiaire, puis l'écran titre « ROYAL MARTINOT » après une trentaine de secondes ; sans image brouillée | ☐ |
| 1.1 b | Allumer, puis appuyer deux fois sur un bouton | Au 1er appui : « ARE YOU SURE? THIS IS VERY PRESTIGIOUS. » ; au 2e : « FINE. » puis l'écran titre | ☐ |
| 1.2 | Musique du titre | Polka jouée en boucle, sans trou, grésillement ni note bloquée ; elle continue sans repartir au début en passant au menu | ☐ |
| 1.3 | HAUT/BAS dans le titre | L'étoile passe d'une ligne à l'autre avec un petit « bip » | ☐ |
| 1.4 | Attendre sans toucher | La démo CPU contre CPU démarre au bout d'environ **20 secondes**. Chronométrez-la : c'est un bon test de vitesse, voir § 8 | ☐ |
| 1.5 | Bouton RESET pendant la démo | Retour propre à l'écran titre | ☐ |
| 1.6 | Bords de l'écran | Aucun texte coupé à gauche ou à droite (« MODE D'EMPLOI », « 1/8 »…). L'apostrophe de « D'EMPLOI » est visible | ☐ |

## 2. Menus et mode d'emploi

| # | Vérification | Attendu | OK ? |
|---|---|---|---|
| 2.0 | Changements d'écran | Fondu au noir puis retour progressif, sans image brouillée ni trait horizontal ; la musique ne s'interrompt pas et ne prend aucun retard | ☐ |
| 2.0 b | Au menu, changer de MODE plusieurs fois, puis de personnage | Les lignes se réécrivent sans éclair noir ni trait horizontal ; en 1 JOUEUR CONTRE CPU, pas de ligne JEU ; en 2 JOUEURS ADVERSAIRES, ligne JEU (simple ou double) | ☐ |
| 2.1 | Ouvrir le mode d'emploi, parcourir les 8 pages | Aucune ligne coupée ; GAUCHE/DROITE change de page | ☐ |
| 2.2 | JOUER > configuration | Lignes lisibles, liste centrée entre les deux filets dorés ; les portraits des joueurs encadrent le titre, entiers (cheveux non coupés en haut de l'écran) et sans morceau manquant quand la balle curseur rebondit | ☐ |
| 2.3 | Mode COMPETITION, type PERSONNALISE, jeu DOUBLE | Toutes les lignes tiennent entre les filets, puis l'aide en bas (2 lignes d'explication, 2 lignes de touches), sans chevauchement ; le portrait du partenaire s'affiche à droite | ☐ |
| 2.4 | JOUEUR 1 : faire défiler jusqu'à MARGAUX | MARGAUX apparaît après INES : cheveux longs, blonds et bouclés, tenue rose clair | ☐ |
| 2.5 | LIEU : COURT MARGAUX | La valse « boîte à musique » remplace le thème du titre, sans notes hachées | ☐ |
| 2.5 b | Match au court Margaux | L'arbitre est une princesse : couronne, robe rose, baguette dorée à étoile ; elle tend sa baguette sur une faute et la lève sur un let | ☐ |
| 2.5 c | LIEU : CHÂTEAU, puis PARC | Château : murs de pierre, bannières rouges, torches, spectateurs aux balcons ; parc : arbres orange. Aucune tuile parasite, aussi de nuit et sous la pluie | ☐ |
| 2.5 d | Musiques du titre et du menu | Polka à 4 voix : mélodie, basse, « oum-pa » d'harmonie et percussions ; un déplacement dans le menu (bip) ne coupe pas la musique | ☐ |
| 2.6 | SON : BRUITAGES SEULS, puis AUCUN, puis MUSIQUE + SONS | La musique s'arrête, puis tout se tait, puis la musique repart | ☐ |

## 3. Match en simple (1 joueur contre le CPU)

Réglages : FORMAT RAPIDE, NIVEAU FACILE, VITESSE JEU NORMAL.

| # | Vérification | Attendu | OK ? |
|---|---|---|---|
| 3.1 | Écran de présentation, puis A | Le court s'affiche d'un coup, sans lignes parasites. **Aucune musique pendant le match** | ☐ |
| 3.2 | Service : A pour lancer, A au sommet | La balle monte et redescend ; le service part. Faites-le aussi en étant le joueur du haut (après un changement de côté) | ☐ |
| 3.2 b | Service canon : A pile au sommet du lancer, environ une demi-seconde après | Bandeau « SERVICE CANON », « boum », traînée derrière la balle. Notez combien vous en réussissez sur 20 services A (palmarès : SERVICES CANON) | ☐ |
| 3.2 c | Frappe parfaite : se placer à côté de la trajectoire et relâcher A juste avant l'arrivée de la balle | « Tchak » sec, éclat blanc près de la raquette, balle plus rapide. Palmarès : COUPS PARFAITS (nombre et %) | ☐ |
| 3.3 | Échanges | La balle grossit en hauteur, son ombre reste au sol et rétrécit quand la balle monte ; la balle s'aplatit au rebond ; sa couture tourne sur les liftés et les coupés ; elle passe derrière le filet dans le camp du haut ; jaune sur tous les courts, verte sur le sable ; les joueurs ont une ombre sous les pieds | ☐ |
| 3.4 | Se déplacer latéralement | Le joueur court de profil, avec des bruits de pas discrets | ☐ |
| 3.4 b | Animation des joueurs | À l'arrêt, léger rebond sur place ; petit saut (split-step) quand l'adversaire frappe ; préparation pendant que la balle arrive, impact bras tendu, accompagnement ; volée raquette devant ; service en 3 temps, précédé de 2 rebonds de balle ; receveur genoux fléchis qui se balance ; revers à deux mains quand la balle arrive à gauche ; fente ou plongeon sur une balle lointaine (glissade sur terre et glace) ; course en 4 images ; après le point : joie (pirouette pour Margaux), raquette qui tape le sol après une faute, mains sur les genoux après un long échange. Aucun clignotement ni tuile parasite, aussi au club nudiste et avec Margaux | ☐ |
| 3.4 c | Faute de service ; demi-tour brusque sur dur | Bip aigu du juge électronique à chaque service faux ; aucun crissement de chaussures | ☐ |
| 3.4 d | Voix de l'arbitre et « pok » | « Out! » sur une balle dehors, « Fault! » sur un service faux, « Let! » sur un let, compréhensibles ; un « pok » à chaque frappe ; la manette répond normalement pendant les annonces (pas de bouton fantôme) ; au court Margaux, voix plus aiguë | ☐ |
| 3.5 | Coup gagnant | Bandeau « COUP GAGNANT ! », « PASSING ! », « AMORTIE ! »… et un « ting » bref | ☐ |
| 3.6 | Lob court sur soi, près du filet : A | Smash sauté : le joueur s'élève puis retombe, son ombre reste au sol | ☐ |
| 3.7 | Ralenti après un beau point | Le point est rejoué à mi-vitesse ; A le passe | ☐ |
| 3.8 | START pendant le jeu | Menu CONTINUER / QUITTER ; le jeu reprend sans saut | ☐ |
| 3.9 | Panneaux de score | « J n » pour les jeux, `*` pour le serveur, un rond plein par set gagné | ☐ |
| 3.9 b | Après chaque jeu | Le bandeau affiche le score set par set (« ALEX 6 3 / MAYA 4 1 ») | ☐ |
| 3.9 c | Fin d'un set (format 3 sets) | Écran « FIN DU 1ER SET » : tableau, statistiques, joueur au service, petite musique ; un bouton reprend aussitôt le match sur le même court, musique coupée | ☐ |
| 3.10 | Fin de match (gagnée) | Le vainqueur sautille raquette levée sous une pluie d'étoiles et de confettis, fanfare jusqu'à l'écran de statistiques, 3 feux d'artifice (sifflement, « pop », gerbe d'étoiles) ; perdant déçu ; pas de fête si le CPU gagne | ☐ |
| 3.10 b | Compétition gagnée | Podium centré sur le court : les 3 premiers en grand, sans pixels parasites, coupe géante, confettis, fanfare, 5 feux d'artifice, puis le calme ; A : écran titre intact (logo et balle corrects) | ☐ |
| 3.10 c | Saison gagnée, puis A sur le podium | Générique de fin : défilement régulier sans saut ni ligne qui apparaît d'un coup, noms en or, hymne au kazoo ; A maintenu accélère ; à la fin un « clap », puis A : écran titre normal (pas décalé) | ☐ |

## 4. Lieux, surfaces et conditions (couleurs)

Pour chaque ligne, lancez un match et regardez l'écran pendant une dizaine de secondes. **Les joueurs, leurs jambes et la balle doivent toujours se détacher nettement du court.** Notez toute couleur qui se confond, et le type d'écran utilisé : les couleurs d'un écran plat ou d'un convertisseur HDMI peuvent différer de celles d'un téléviseur cathodique.

| # | Lieu / surface / conditions | Points à regarder | OK ? |
|---|---|---|---|
| 4.1 | Stade / terre battue / jour | Poussière au rebond ; petite chaise d'arbitre blanche de profil au bord du court, contre le poteau gauche du filet (aussi au club, en salle, au court Margaux, de nuit et sous la pluie) ; l'arbitre suit la balle du regard, tend le bras sur une faute, agite le bras sur un let, lève le bras à la fin du match, et reste visible pendant les ralentis | ☐ |
| 4.2 | Club / gazon / pluie | Gouttes de pluie, éclaboussures au rebond, couleurs assombries mais lisibles | ☐ |
| 4.3 | Salle / dur / jour | Pas de pluie possible en salle | ☐ |
| 4.4 | Club / plage / jour | Balle **blanche** sur le sable jaune ; poussière au rebond | ☐ |
| 4.5 | Stade / glace / nuit | Court éclairé, décor assombri sans noir complet | ☐ |
| 4.6 | Court Margaux (toutes surfaces) | Court rose, cœurs, fleurs et étoiles, petit panneau magenta « MARGAUX » à côté du score de gauche. Margaux (tenue rose clair, cheveux blonds) reste bien visible sur le rose ; sur la plage, ses cheveux sont blond clair | ☐ |
| 4.6 a | Club nudiste | Petit panneau jaune « BRONZES » à côté du score ; en repassant ensuite au stade, le public du stade est intact | ☐ |
| 4.6 b | Filet (tous les courts) | Bande blanche épaisse, mailles où l'on voit le court, ombre sous le filet ; quand la balle touche le filet, il tremble brièvement | ☐ |
| 4.7 | Personnages proches | MAYA et HUGO, EMMA et NOAH, TOM (crème) et MARGAUX (rose clair) se distinguent en match | ☐ |

## 5. Double et deux manettes

| # | Vérification | Attendu | OK ? |
|---|---|---|---|
| 5.1 | Match de double, 4 CPU (mode DÉMO en double) | Quand les 4 joueurs sont près du filet, un léger clignotement est normal ; aucun joueur, aucune ombre ni la balle ne doit **disparaître** durablement | ☐ |
| 5.2 | Fluidité en double | Pas de ralentissement visible ; quelques à-coups très brefs restent possibles | ☐ |
| 5.3 | Mode EN EQUIPE, JOUE PAR : MANETTE 2 | La manette 2 déplace le partenaire | ☐ |
| 5.4 | Mode 2 J. ADVERSAIRES | La manette 2 contrôle l'adversaire, START (manette 1) met en pause | ☐ |
| 5.5 | Panneaux en double avec Margaux | L'équipe s'affiche « MX.. » (et non « MA », réservé à MAYA) | ☐ |

## 6. Options de confort

| # | Vérification | Attendu | OK ? |
|---|---|---|---|
| 6.1 | VITESSE JEU : LENT | Jeu nettement plus lent ; un appui bref n'est jamais perdu ; une petite croix montre où tombe la balle adverse | ☐ |
| 6.2 | VITESSE JEU : RAPIDE | Jeu plus rapide, image fluide en simple | ☐ |
| 6.3 | FATIGUE : OUI, échange de plus de 10 frappes | Les joueurs ralentissent peu à peu ; une goutte de sueur clignote au-dessus d'eux. Au point suivant, tout redevient normal | ☐ |
| 6.4 | FATIGUE : NON | Aucun ralentissement, pas de goutte | ☐ |
| 6.5 | VUE : PERSPECTIVE, un match dans chaque lieu | Court resserré vers le fond, lignes nettes, filet et chaise d'arbitre sans tuile parasite ; tribunes qui suivent les lignes de côté, spectateurs plus petits au fond, ola complète ; joueurs, balle et ombres posés sur le court | ☐ |
| 6.6 | VUE : PERSPECTIVE, double, échange long | Image fluide, pas de sprite qui disparaît plus qu'à plat | ☐ |
| 6.7 | Après un match en perspective, revenir au menu puis jouer À PLAT | Écrans et court d'origine intacts | ☐ |

## 7. Compétitions, codes et sauvegarde

| # | Vérification | Attendu | OK ? |
|---|---|---|---|
| 7.1 | Compétition TOURNOI avec MARGAUX | Écran d'étape avec tableau et code de 13 signes | ☐ |
| 7.2 | Avant la finale | Thème de la finale (marche) sur l'écran d'étape et la présentation, silence en match | ☐ |
| 7.3 | Gagner une compétition | Levée de coupe, confettis, « MODE NUDISTE DEBLOQUE » | ☐ |
| 7.4 | Recopier un code, éteindre, rallumer, TITRE > CODE DE REPRISE | La compétition reprend à la bonne étape, avec Margaux | ☐ |
| 7.5 | Code mal recopié (une lettre changée) | Refusé | ☐ |
| 7.6 | Changer des réglages, lancer un match, **éteindre puis rallumer** | Avec sauvegarde activée sur la cartouche : réglages et palmarès retrouvés. Sinon : réglages par défaut, ce qui est normal | ☐ |
| 7.7 | Palmarès, SELECT deux fois | Statistiques effacées, réglages conservés | ☐ |

## 8. Région (PAL / NTSC)

| # | Vérification | Attendu | OK ? |
|---|---|---|---|
| 8.1 | Chronométrer le démarrage de la démo (1.4) | Environ 20 s en NTSC comme en PAL : le jeu compense le 50 Hz | ☐ |
| 8.2 | Musique en PAL | Tempo normal ; la hauteur des notes peut être un peu plus basse, c'est propre au matériel PAL | ☐ |
| 8.3 | Famicom | Tout fonctionne ; la manette 2 de la Famicom n'a pas de START, c'est donc la manette 1 qui met en pause | ☐ |

## 9. Endurance

- [ ] Laisser tourner la démo **30 minutes** (elle alterne simple et double) : ni blocage, ni image brouillée, ni son coincé.
- [ ] Jouer un match en **5 sets** jusqu'au bout.

## 10. Nouveautés des versions 2.57 à 2.61

| # | Vérification | Attendu | OK ? |
|---|---|---|---|
| 10.1 | Allumer plusieurs fois | Le fond des menus change d'une fois à l'autre : brun (terre battue), vert (gazon), bleu (dur) ou bordeaux (moquette) ; textes et titres dorés toujours lisibles, aussi sur le brun | ☐ |
| 10.2 | Écran titre | La grosse balle est posée sur un court en perspective (lignes blanches), sans trait jaune horizontal | ☐ |
| 10.3 | Réglages : HAUT/BAS, GAUCHE/DROITE | Sons distincts : bip au déplacement, glissando qui monte ou descend quand la valeur change, note qui descend au retour (B) ; en bas, deux lignes expliquent la ligne choisie | ☐ |
| 10.4 | Réglages : LIEU | Petite vignette du lieu devant sa valeur (stade, arbres, salle, parasol, cœur, château, arbres d'automne), sans clignotement | ☐ |
| 10.5 | Affiche VS avant un match | Après le fondu, le buste gauche jaillit d'un éclair blanc, passe en silhouette noire puis en couleurs, puis le droit ; « CONTRE » s'inscrit sur un flash. Yeux et contours sombres pour tous les personnages, Margaux blonde, Papi avec son visage | ☐ |
| 10.6 | Écran d'un tournoi (après un ou deux matchs) | Tableau à traits dorés, vainqueurs au milieu de leurs deux joueurs, votre joueur en or ; médailles or, argent, bronze bien distinctes dans les classements | ☐ |
| 10.7 | Palmarès, page 2 | Une médaille devant chaque podium | ☐ |
| 10.8 | Lob (B + HAUT) : relâché vite, puis bouton tenu ; depuis le fond, puis depuis l'intérieur du court | Court puis long ; haut et lent depuis le fond, plus bas et plus vif de l'intérieur ; l'ordinateur Expert recule parfois pour le smasher | ☐ |
| 10.9 | Croisé court (A + BAS) : relâché vite, puis bouton tenu | Angle plus prudent, puis plus fermé, près de la ligne ; l'ordinateur joue aussi ce coup | ☐ |
| 10.10 | Changement de côté après un beau point (ralenti) | Les chaises n'apparaissent qu'après le ralenti, jamais pendant ; aucune pose déformée au départ vers les chaises | ☐ |
| 10.11 | Fluidité en simple (à plat et en perspective) | Plus d'à-coups pendant l'échange ; un très bref à-coup peut subsister à la fin d'un point (affichage du score) | ☐ |
| 10.12 | Vue en perspective | Les deux poteaux du filet sont placés de la même façon | ☐ |
| 10.13 | Bande-annonce du Grand Martinot | Yeux et contours sombres sur tous les plans (sauf Margaux, dont les yeux restent de la couleur sombre du fond) | ☐ |
| 10.14 | Sauvegarde, en passant d'une version précédente à la 2.61 (même fichier de sauvegarde) | Palmarès et réglages conservés | ☐ |
| 10.15 | Allumer plusieurs fois de suite, et appuyer sur RESET à divers moments (titre, menu, match) | Démarrage toujours correct (cartouche MMC3 : l'état des banques à l'allumage est quelconque, le jeu le règle lui-même) | ☐ |
| 10.16 | Affiche VS, avant un match (version 2.63) | Les deux bustes glissent depuis les bords, l'un une ligne sur deux, l'autre sur les lignes restantes, se rejoignent, puis un éclair blanc ; aucune ligne déchirée ni décalée au-dessus du haut des bustes (titre) ni sous leur bas (noms) ; la musique et les touches répondent pendant l'entrée. Appuyer sur B ou A pendant la glissade : l'écran suivant est normal | ☐ |

## 11. Ce qu'il faut renvoyer

Pour chaque case non cochée :
1. le numéro de la ligne (par exemple 4.6) ;
2. ce que vous voyez ou entendez, avec une photo ou une courte vidéo si possible ;
3. la console, la cartouche et l'écran utilisés ;
4. si possible, comment reproduire le problème : les réglages du menu, et le moment où il survient.

Ces informations permettent de corriger le problème précisément, même sans avoir la console sous la main.
