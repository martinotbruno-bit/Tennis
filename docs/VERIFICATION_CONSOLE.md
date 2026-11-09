# Royal Martinot — Liste de vérification sur vraie console

Le jeu a été testé uniquement sur émulateur. Cette liste sert à le vérifier pas à pas sur une vraie NES ou Famicom. Cochez chaque case et notez ce qui ne va pas, si possible avec une photo de l'écran. Comptez environ une heure pour l'ensemble.

## 0. Matériel

- [ ] Une console : NES (Europe = PAL, États-Unis = NTSC), Famicom (Japon, NTSC), ou un clone Dendy (PAL).
- [ ] Une cartouche flash compatible **MMC1** (mapper 1), par exemple de la gamme EverDrive N8 ou PowerPak. Elle doit gérer :
  - **256 Ko** de programme (mapper 1, SNROM) ;
  - 8 Ko de **CHR-RAM** (pas de CHR-ROM) ;
  - 8 Ko de RAM de sauvegarde.
- [ ] Le fichier `dist/royal_martinot.nes`, copié sur la carte mémoire de la cartouche.
- [ ] Si possible, deux manettes (le mode à deux joueurs utilise la manette 2).
- [ ] Notez le modèle de console, la cartouche et sa version de logiciel, et le type d'écran (télévision cathodique, écran plat, convertisseur HDMI).

> Sauvegarde : avec un EverDrive, activez la sauvegarde de la RAM de cartouche (fichier `.srm` ou `.sav`) dans le menu de la cartouche. Sans cela, le palmarès et les réglages sont perdus à l'extinction, ce qui est normal. Les codes de reprise restent la sauvegarde principale.

## 1. Démarrage

| # | Vérification | Attendu | OK ? |
|---|---|---|---|
| 1.1 | Allumer la console | Le générique démarre (« BRUNO MARTINOT PRESENTS », fanfare), barre à 117 %, mentions légales, lettre N tombée puis remise par le stagiaire, puis l'écran titre « ROYAL MARTINOT » après une trentaine de secondes ; sans image brouillée | ☐ |
| 1.1 b | Allumer, puis appuyer deux fois sur un bouton | Au 1er appui : « ARE YOU SURE? THIS IS VERY PRESTIGIOUS. » ; au 2e : « FINE. » puis l'écran titre | ☐ |
| 1.2 | Musique du titre | Polka jouée en boucle, sans trou, grésillement ni note bloquée ; elle continue sans repartir au début en passant au menu | ☐ |
| 1.3 | HAUT/BAS dans le titre | L'étoile passe d'une ligne à l'autre avec un petit « bip » | ☐ |
| 1.4 | Attendre sans toucher | La démo CPU contre CPU démarre au bout d'environ **20 secondes**. Chronométrez-la : c'est un bon test de vitesse, voir § 8 | ☐ |
| 1.5 | Bouton RESET pendant la démo | Retour propre à l'écran titre | ☐ |
| 1.6 | Bords de l'écran | Aucun texte coupé à gauche ou à droite (« MODE D'EMPLOI », « 1/8 »…). L'apostrophe de « D'EMPLOI » est visible | ☐ |

## 2. Menus et mode d'emploi

| # | Vérification | Attendu | OK ? |
|---|---|---|---|
| 2.0 | Changements d'écran | Fondu au noir puis retour progressif, sans image brouillée ; la musique ne s'interrompt pas | ☐ |
| 2.1 | Ouvrir le mode d'emploi, parcourir les 8 pages | Aucune ligne coupée ; GAUCHE/DROITE change de page | ☐ |
| 2.2 | JOUER > configuration | Lignes lisibles ; le personnage animé est à droite du titre et ne masque aucun texte | ☐ |
| 2.3 | Mode COMPETITION, type PERSONNALISE, jeu DOUBLE | 19 lignes, puis STYLE et l'aide en bas, sans chevauchement | ☐ |
| 2.4 | JOUEUR 1 : faire défiler jusqu'à MARGAUX | MARGAUX apparaît après INES : cheveux longs, blonds et bouclés, tenue rose clair | ☐ |
| 2.5 | LIEU : COURT MARGAUX | La valse « boîte à musique » remplace le thème du titre | ☐ |
| 2.6 | SON : BRUITAGES SEULS, puis AUCUN, puis MUSIQUE + SONS | La musique s'arrête, puis tout se tait, puis la musique repart | ☐ |

## 3. Match en simple (1 joueur contre le CPU)

Réglages : FORMAT RAPIDE, NIVEAU FACILE, VITESSE JEU NORMAL.

| # | Vérification | Attendu | OK ? |
|---|---|---|---|
| 3.1 | Écran de présentation, puis A | Le court s'affiche d'un coup, sans lignes parasites. **Aucune musique pendant le match** | ☐ |
| 3.2 | Service : A pour lancer, A au sommet | La balle monte et redescend ; le service part. Faites-le aussi en étant le joueur du haut (après un changement de côté) | ☐ |
| 3.2 b | Service canon : A pile au sommet du lancer, environ une demi-seconde après | Bandeau « SERVICE CANON », « boum », traînée derrière la balle. Notez combien vous en réussissez sur 20 services A (palmarès : SERVICES CANON) | ☐ |
| 3.2 c | Frappe parfaite : se placer à côté de la trajectoire et relâcher A juste avant l'arrivée de la balle | « Tchak » sec, éclat blanc près de la raquette, balle plus rapide. Palmarès : COUPS PARFAITS (nombre et %) | ☐ |
| 3.3 | Échanges | La balle grossit en hauteur ; son ombre reste au sol ; les joueurs ont une ombre sous les pieds | ☐ |
| 3.4 | Se déplacer latéralement | Le joueur court de profil, avec des bruits de pas discrets | ☐ |
| 3.5 | Coup gagnant | Bandeau « COUP GAGNANT ! », « PASSING ! », « AMORTIE ! »… et un « ting » bref | ☐ |
| 3.6 | Lob court sur soi, près du filet : A | Smash sauté : le joueur s'élève puis retombe, son ombre reste au sol | ☐ |
| 3.7 | Ralenti après un beau point | Le point est rejoué à mi-vitesse ; A le passe | ☐ |
| 3.8 | START pendant le jeu | Menu CONTINUER / QUITTER ; le jeu reprend sans saut | ☐ |
| 3.9 | Panneaux de score | « J n » pour les jeux, `*` pour le serveur, un rond plein par set gagné | ☐ |
| 3.9 b | Après chaque jeu | Le bandeau affiche le score set par set (« ALEX 6 3 / MAYA 4 1 ») | ☐ |
| 3.9 c | Fin d'un set (format 3 sets) | Écran « FIN DU 1ER SET » : tableau, statistiques, joueur au service, petite musique ; un bouton reprend aussitôt le match sur le même court, musique coupée | ☐ |
| 3.10 | Fin de match (gagnée) | Le vainqueur sautille raquette levée sous une pluie d'étoiles et de confettis, perdant déçu, puis écran de statistiques ; pas de fête si le CPU gagne | ☐ |

## 4. Lieux, surfaces et conditions (couleurs)

Pour chaque ligne, lancez un match et regardez l'écran pendant une dizaine de secondes. **Les joueurs, leurs jambes et la balle doivent toujours se détacher nettement du court.** Notez toute couleur qui se confond, et le type d'écran utilisé : les couleurs d'un écran plat ou d'un convertisseur HDMI peuvent différer de celles d'un téléviseur cathodique.

| # | Lieu / surface / conditions | Points à regarder | OK ? |
|---|---|---|---|
| 4.1 | Stade / terre battue / jour | Poussière au rebond, arbitre sur sa chaise à gauche, qui tourne la tête vers la balle | ☐ |
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

## 10. Ce qu'il faut renvoyer

Pour chaque case non cochée :
1. le numéro de la ligne (par exemple 4.6) ;
2. ce que vous voyez ou entendez, avec une photo ou une courte vidéo si possible ;
3. la console, la cartouche et l'écran utilisés ;
4. si possible, comment reproduire le problème : les réglages du menu, et le moment où il survient.

Ces informations permettent de corriger le problème précisément, même sans avoir la console sous la main.
