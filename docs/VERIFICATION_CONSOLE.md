# Royal Martinot — Liste de vérification sur vraie console

Le jeu a été testé uniquement sur émulateur. Cette liste sert à le vérifier pas à pas sur une vraie NES ou Famicom. Cochez chaque case et notez ce qui ne va pas, si possible avec une photo de l'écran. Comptez environ une heure et quart pour l'ensemble. Version du guide : 2.76.

## 0. Matériel

- [ ] Une console : NES (Europe = PAL, États-Unis = NTSC), Famicom (Japon, NTSC), ou un clone Dendy (PAL).
- [ ] Une cartouche flash compatible **MMC3** (mapper 4, 512 Ko de programme depuis la version 2.67), par exemple de la gamme EverDrive N8 ou PowerPak (depuis la version 2.62 ; MMC1 avant). Elle doit gérer :
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
| 10.16 | Affiche VS, avant un match (version 2.63, 2.65) | Les deux bustes glissent depuis les bords, l'un une ligne sur deux, l'autre sur les lignes restantes, se rejoignent, puis un éclair blanc ; aucun des deux ne tremble ni ne se dédouble horizontalement ; aucune ligne déchirée ni décalée au-dessus du haut des bustes (titre) ni sous leur bas (noms) ; la musique et les touches répondent pendant l'entrée. Appuyer sur B ou A pendant la glissade : l'écran suivant est normal | ☐ |
| 10.17 | Vue en perspective, dans chaque lieu (version 2.64, 2.65) | Lignes de côté obliques régulières, sans marche ni trou, y compris de part et d'autre du filet ; court, filet et rambardes symétriques (aussi à plat) ; panneaux à côté du score présents là où ils l'étaient ; pendant une ola et quand la balle change de moitié du court, aucune ligne du court ne se transforme en morceau de spectateur ou de panneau | ☐ |
| 10.18 | Ralenti après un beau point (version 2.67 ; cartouche de 512 Ko) | Gros plan du joueur en pleine frappe, puis bras levés ; de retour au ralenti, viseur sur le rebond puis image agrandie deux fois, nette (lignes, joueurs, balle), avec BONNE ! ou DEHORS ! ; retour au court sans tuiles abîmées (public, lignes, panneaux, score) ; A passe la séquence ; aucun écran figé plus d'une demi-seconde | ☐ |
| 10.19 | Vue en perspective, balle dans le filet (version 2.68) | Le filet tremble (bande qui s'affaisse d'un pixel, mailles qui bougent) sur toute sa largeur, sans marche à l'endroit des lignes, puis revient exactement comme avant | ☐ |
| 10.20 | Fin d'un match (version 2.69) | Après la poignée de main, portrait du vainqueur en grand (nom, confettis), puis gros plan ; écran des résultats ensuite, lisible (titre, « VICTOIRE DE », tableau des sets sans chevauchement, joueurs sous les statistiques) | ☐ |
| 10.21 | Ace et statistiques (version 2.70) | Sur un ace, « ACE ! » puis la vitesse (« 174 KM/H ») lisible dans le bandeau ; aux écrans de fin de set et de résultats, ligne SERVICE MAX KM/H alignée avec les chiffres au-dessus, 100 % affiché « 100 » | ☐ |
| 10.22 | Bandeau du score (version 2.71) | Après un point : bandeau en bas (filets, noms, jeux, points), net, sans ligne qui saute ni image brouillée ; aucun joueur dessiné par-dessus ; il disparaît avant le service | ☐ |
| 10.23 | Crépuscule (version 2.71) | CONDITIONS : CREPUSCULE, match en 1 set : teinte chaude, puis rose-violette, puis projecteurs au fil des jeux (sur une console PAL, la teinte doit rester chaude, pas verte) | ☐ |
| 10.24 | Voix (version 2.71) | « Deuce! », « Advantage! », « Game! » audibles et compréhensibles, après « Out! » sans le couper | ☐ |
| 10.25 | Reprise (version 2.72, cartouche à pile) | Éteindre en plein match amical, rallumer : « B : REPRENDRE LE MATCH » ; B puis A : même score, au service | ☐ |
| 10.26 | Fiche et carrière (version 2.72) | Affiche VS : la fiche (taille, main, classement) alterne avec les textes, sans ligne abîmée ; compétition avec CARRIERE : OUI, points et classement à la fin | ☐ |
| 10.27 | Podium (version 2.72) | Public debout de chaque côté quand la coupe est brandie, plaque du finaliste, confettis | ☐ |
| 10.28 | Ralenti (version 2.74) | Ralenti en plein écran, sans bandes noires ; puis le bandeau du score, qui reste jusqu'à un appui (l'ordinateur attend pour servir ; l'appui ne lance pas la balle) | ☐ |
| 10.29 | Survol (version 2.73) | Avant une finale : ciel, tribunes et court glissent à des vitesses différentes ; limites des bandes nettes (au pire une ligne qui tremble) | ☐ |
| 10.30 | Perspective (version 2.73) | Plus de traits horizontaux entre le public et le court | ☐ |
| 10.31 | Plans télé (version 2.73) | Ramasseuse au changement de côté, poignée de main en fin de match, public après un long échange : images nettes, retour au court propre (score, bandeau, joueurs) | ☐ |
| 10.32 | Pause et balle de match (version 2.75) | START : le score apparaît dans un bandeau au milieu de l'écran, sans clignotement ; à la reprise, le court est intact. Avant une balle de match : bandeau « BALLE DE MATCH / POUR … » au milieu, qui disparaît seul ; bords du bandeau nets (au pire une ligne qui tremble) | ☐ |
| 10.33 | Joueur personnalisé (version 2.75) | Compétition : écran TON JOUEUR, changer nom, sexe, peau, cheveux, tenue ; le joueur apparaît ainsi au tableau et en match ; éteindre la console, rallumer : le profil revient à la compétition suivante | ☐ |
| 10.34 | Changement de côté (version 2.76) | Le bandeau reste affiché de la fin du point aux statistiques ; les chaises apparaissent quand il est monté ; START assis relance le jeu sans pause | ☐ |
| 10.35 | Défaite (version 2.76) | Perdre contre l'ordinateur : image du perdant à genoux, « DEFAITE » | ☐ |
| 10.36 | Crépuscule (version 2.76) | Match au crépuscule : au changement de côté qui suit un changement de lumière, plan du soleil couchant net (couleurs, pas de tuiles parasites), retour au court propre | ☐ |
| 10.37 | Pages de choix (version 2.76) | Après les réglages : joueurs (affiche), lieu et surface (aperçu du court), tournoi Martinot ; GAUCHE/DROITE, A, B, START ; pas d'écran figé ni de couleurs fausses au retour au menu | ☐ |
| 10.38 | Fin de set (version 2.76) | Les joueurs s'assoient, bandeau « 1ER SET », écran du set en fondu, musique douce ; au retour, les deux (ou quatre) joueurs repartent sur le court | ☐ |
| 10.39 | Tirage au sort (version 2.76) | Les joueurs face à l'arbitre au filet ; bandeau de choix du gagnant net, manette du gagnant seulement | ☐ |
| 10.40 | Compétition à deux (version 2.76) | JOUEUR 2 : MANETTE 2 en simple : chacun joue ses matchs avec sa manette ; match entre les deux à deux manettes | ☐ |
| 10.41 | Page du court (version 2.77) | Lieu, surface et météo sur le même écran : HAUT/BAS change de ligne (« > »), GAUCHE/DROITE la valeur ; l'aperçu suit (nuit, pluie qui tombe, couchant). Pas de pluie ni de crépuscule en salle | ☐ |
| 10.42 | Pause (version 2.77) | Bandeau central net (aucun public ni sprite au travers), même si START est pressé juste après une frappe ; à la reprise, la ligne RALENTI disparaît | ☐ |
| 10.43 | Victoire et château (version 2.77) | Balle de match gagnée : la fête sans bandeau du score avant ; château au crépuscule : pierre dorée puis brun orangé, pas rose | ☐ |
| 10.44 | Fin de set (version 2.77) | Le bandeau du set reste affiché jusqu'à un appui (A, B ou START), puis l'écran du set | ☐ |
| 10.45 | Compétition (version 2.77) | TON JOUEUR d'abord (joueur 1, puis joueur 2), puis l'affiche VS avec le joueur conçu, puis le court et le tableau | ☐ |
| 10.46 | Profils (version 2.77) | TON JOUEUR : PROFIL fait défiler D'ORIGINE, les profils et NOUVEAU ; une modification crée ou met à jour un profil ; SELECT le supprime ; les profils restent après extinction | ☐ |
| 10.47 | Parcours d'une compétition (version 2.77h) | Réglages : MODE sur COMPETITION, la ligne s'appelle aussitôt JOUEURS ; START : TON JOUEUR, puis choix du tournoi et court, sans page de choix du joueur (A n'y ramène pas) ; tableau à 2 ou 4 joueurs : « A TOI… » sous le titre, le tableau ne touche ni le titre ni l'aide | ☐ |
| 10.48 | Ralentis HD (version 2.77h) | Gros plan de la frappe décisive : fond bleu nuit à tirets noirs (plan moyen), rouge sombre (gros plan) ; aucun carré noir autour du joueur | ☐ |
| 10.49 | Jupe sur la cuisse (version 2.77i) | Glissade d'une joueuse (ralenti) et ramasseuse : la jupe couvre le dessus de la cuisse tendue, aucune bande de peau au-dessus du tissu | ☐ |
| 10.50 | Arborescence (version 2.77i) | Grand Martinot : au premier tour, l'écran s'ouvre sur le tableau des 16 inscrits ; GAUCHE : classement, puis match ; DROITE revient ; saison : tableau des 8 | ☐ |
| 10.51 | Ordinateur en FACILE (version 2.77i) | Un set contre l'ordinateur en FACILE : aucune série de balles très hautes et très rapides qui sortent ou vont dans le filet, même sur balle de set | ☐ |
| 10.52 | Lob de sauvetage (version 2.77i) | Plongeons depuis le fond du court : une partie des lobs de sauvetage sortent (surtout trop longs) ; le lob décidé (B + HAUT) reste aussi sûr qu'avant | ☐ |
| 10.53 | Réglages (version 2.78) | MODE : 1 JOUEUR VS CPU, 2 JOUEURS, COMPETITION, CARRIERE, ENTRAINEMENT, DEMO ; 1 JOUEUR, JEU DOUBLE : partenaire CPU ; 2 JOUEURS, FORMULE CONTRE / ENSEMBLE ; COMPETITION : ligne HUMAINS ; OPTIONS ouvre la page (RALENTIS, VITESSE JEU, FATIGUE, VUE, SON), B y revient ; les réglages sont retrouvés au rallumage | ☐ |
| 10.54 | Lieux (version 2.79) | Château : flammes qui vacillent, fanfare au changement de côté ; parc : feuilles qui tombent sur les côtés, oiseaux ; club : feuillage, applaudissements ; salle : SOIREE (court éclairé), panneaux qui s'allument ; en perspective, public jusqu'à la rambarde et bandeaux lumineux horizontaux, drapeaux dans le public ; Margaux : rose différent selon la surface ; club nudiste : affiche, gros plans et portrait sans vêtements, mosaïque sur le bassin et la poitrine ; aucun son d'ambiance avec SON « aucun » | ☐ |
| 10.55 | Décor animé et bandeau du score (version 2.79c) | Château ou parc : après un point, les flammes / feuilles continuent de bouger tant que le bandeau du score est affiché, même serveur en place ; elles se figent dès que le bandeau est fermé par un appui ou au lancer de balle, et restent figées pendant l'échange | ☐ |
| 10.56 | Petites animations des lieux (version 2.80) | Après un point : stade et salle, flashs blancs dans le public ; court Margaux, cœurs roses qui montent et étoiles qui changent de couleur ; club nudiste, papillons blancs ; château, vol de 3 oiseaux noirs (attendre quelques points) ; parc, écureuil orange qui descend un côté ; rien pendant l'échange ; sous la pluie, gouttes normales et pas de ces animations ; après le match, la pluie du menu des conditions reste des gouttes | ☐ |
| 10.57 | Poignée de main du perdant (version 2.81) | Perdre un match contre l'ordinateur : après le portrait à genoux « DEFAITE », la poignée de main au filet montre le joueur le visage triste (yeux baissés, bouche tombante), sans case abîmée autour du visage ; gagner : sourire comme avant ; idem au club nudiste | ☐ |
| 10.58 | Ralenti passé puis A trop tôt (version 2.82) | Pendant le gros plan d'un ralenti, appuyer sur A, puis de nouveau sur A tout de suite : le bandeau du score monte quand même ; un appui ne le ferme qu'une fois entièrement monté | ☐ |
| 10.59 | Club : vol de petits oiseaux (version 2.83) | Au club, après quelques points : 3 petits oiseaux noirs traversent le haut de l'écran en chantant ; le feuillage ne change plus de couleur, ni au club ni au parc | ☐ |
| 10.60 | Coup gagnant et poignée de main (version 2.84) | Un coup gagnant (ace, passing) : un seul bip bref et grave, bien audible (2.85), puis la clameur, plus de carillon montant ; poignée de main HD de fin de match (homme) : la bande blanche du filet sans ligne couleur peau dessous | ☐ |
| 10.61 | Rebond de la balle (version 2.86) | Sur dur : chaque rebond a un petit « poc » grave sous le souffle ; une amortie rebondit faiblement, une frappe forte claque ; sur terre et gazon le poc est plus sourd, sur glace le rebond tinte, sur le sable un souffle feutré sans poc ; aucun son avec SON « aucun » | ☐ |
| 10.62 | Salle sans double bip (version 2.86) | En salle, après les points : les panneaux s'allument, sans « tudute » | ☐ |
| 10.63 | Superposition des sons (version 2.87) | Pendant un échange : le souffle du rebond n'est plus coupé net par la frappe qui suit ; après un beau point, la clameur du public n'est pas coupée par le service suivant ; sur glace, rebond sec et claquant ; sur le sable, rebond grave et étouffé | ☐ |
| 10.64 | Applaudissements (version 2.88) | Au parc ou au club : après un ace ou un long échange, un crépitement d'applaudissements d'environ une seconde qui s'éteint ; après un point ordinaire, plus de « tac tac tac » | ☐ |
| 10.65 | Geste de raquette (version 2.89) | Smash et coups normaux : au relâcher du bouton, un souffle bref, sourd et discret, qui ne ressemble pas aux applaudissements | ☐ |
| 10.66 | Clameur sur un smash (version 2.90) | Smash gagnant (aussi au stade, et smash sauté) : une seule clameur du public, au point gagné, rien à l'impact | ☐ |
| 10.67 | Revue des sons (version 2.90) | Club, parc, club nudiste : oiseaux environ toutes les 8 secondes ; ralenti : zoom sur la ligne sans bip ; gros plan de joie, portrait du vainqueur et coupe sans carillon ; menus : sons de déplacement et de validation plus doux | ☐ |
| 10.68 | Impact et dribble (version 2.91) | Avant le service : le dribble est un petit souffle discret ; en jeu : frappe à plat et smash claquent (« tac »), lifté plus rond (« ploc »), amortie et lob mats (« poc ») ; un coup armé longtemps sonne plus fort | ☐ |
| 10.69 | Impacts échantillonnés (version 2.93) | Service, frappe à plat, smash : « tac » sec ; lifté, coupé, volée : « ploc » rond ; lob, amortie : « poc » mat ; « Fault! » de l'arbitre toujours compréhensible (un peu plus rapide) ; démarrage normal de la cartouche | ☐ |
| 10.70 | Équilibre impact et rebond (version 2.94) | Sur terre battue surtout : on entend d'abord la frappe, le rebond reste audible mais plus discret | ☐ |
| 10.71 | Impact plus fort (version 2.95) | Pendant l'échange : chaque frappe claque nettement, plus fort que le rebond et que la clameur ; pas de note musicale | ☐ |
| 10.72 | Bandeau et service (version 2.96) | Après un premier service faux : pas de bandeau du score avant le second service ; après une double faute : bandeau comme d'habitude | ☐ |
| 10.73 | Statistiques du double (version 2.96) | Match en double, changement de côté : noms d'équipes complets (« AL/MA ») et pages des statistiques qui défilent | ☐ |
| 10.74 | Menu des réglages (version 2.96) | Plus d'accolades ni de traits dorés ; grande balle tramée en bas à droite, en relief, sur les quatre couleurs de fond ; retour à l'écran titre : logo intact ; écrans suivants (présentation, choix) sans tuiles abîmées | ☐ |
| 10.75 | Changement de côté (version 2.97) | Appuyer sur A dès la fin du dernier échange avant le changement de côté : les joueurs vont quand même s'asseoir, les statistiques défilent à l'appui | ☐ |
| 10.76 | Service de l'ordinateur (version 2.97) | Sur plusieurs points : l'ordinateur se déplace parfois en pas chassés sur la ligne avant de lancer (près du T ou vers le couloir) | ☐ |
| 10.77 | Salle en perspective (version 2.98) | Salle, vue PERSPECTIVE : gradins au bord, mur gris et panneaux verts comme à plat, sans encoche dans les panneaux | ☐ |
| 10.78 | Vue dans la pause (version 2.98) | En match : START, ligne VUE, A : le court passe en perspective (ou à plat) aussitôt ; START : le jeu reprend, lignes de la pause effacées ; le réglage est retenu au match suivant | ☐ |
| 10.79 | Filet en perspective (version 2.98) | Plusieurs balles dans le filet : après chaque tremblement, le filet revient intact (pas de maille « secouée » qui reste) | ☐ |
| 10.80 | Noms et bandeaux (version 2.98) | TITOU et MARGAUX : « TITOU » et « MARGA » en haut et dans le bandeau ; premier service du match : pas de bandeau ; balle de match : bandeau central effacé après 1 s | ☐ |
| 10.81 | Textes (version 2.99) | Pause avec TITOU à droite : « QUITTER » et le nom séparés ; tirage au sort : « TITOU : PILE » ; SAISON : « TOURNOI 1/4 : TERRE BATTUE » ; palmarès : rien de collé après GRAND MARTINOT ; stats du changement de côté : noms au-dessus des chiffres | ☐ |
| 10.82 | Service (version 3.00) | L'ordinateur sert tantôt à plat, tantôt coupé, sur l'extérieur, le corps ou le T, parfois court ; le joueur : BAS à l'impact, service court près du filet ; HAUT, long sur la ligne | ☐ |
| 10.83 | Profondeur du service (version 3.01) | Sans direction : la balle tombe au milieu du carré ; HAUT : sur la ligne de service ; BAS : près du filet ; trois points de chute nettement différents | ☐ |
| 10.84 | Entraînement (version 3.02) | Exercice CIBLES : la page TON JOUEUR ne montre que votre joueur, pas de joueur en face sur le court ; exercice JOUEUR FIXE : page TON ADVERSAIRE après TON JOUEUR | ☐ |
| 10.85 | Coups (version 3.02) | B tenu longtemps : coupé plus long et plus rapide qu'un B lâché aussitôt ; au filet, A armé avec GAUCHE/DROITE : volée vive et en angle ; l'ordinateur joue parfois à plat, au centre, des amorties de volée et des smashs coupés | ☐ |
| 10.86 | Pas vers la balle (version 3.03) | Balle qui passe un peu à côté : le joueur glisse vers elle et la raquette la touche ; plus de frappe avec la balle loin de la raquette | ☐ |
| 10.87 | Salle en perspective (version 3.04) | Public sur toute la tribune jusqu'à la rambarde, plus petit vers le fond ; bandeaux verts horizontaux qui vont jusqu'à la rambarde, sans carré gris au bord | ☐ |
| 10.88 | Marque au sol (version 3.05) | OPTIONS, MARQUE OUI : un point qui finit sur une balle près d'une ligne laisse une marque clignotante à l'impact jusqu'au service suivant ; MARQUE NON : aucune ; réglage retenu après extinction | ☐ |

## 11. Ce qu'il faut renvoyer

Pour chaque case non cochée :
1. le numéro de la ligne (par exemple 4.6) ;
2. ce que vous voyez ou entendez, avec une photo ou une courte vidéo si possible ;
3. la console, la cartouche et l'écran utilisés ;
4. si possible, comment reproduire le problème : les réglages du menu, et le moment où il survient.

Ces informations permettent de corriger le problème précisément, même sans avoir la console sous la main.
