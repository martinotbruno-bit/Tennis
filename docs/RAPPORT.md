# Royal Martinot — Rapport de réalisation (version 2.67)

Comme l'exige le CDC (§ 17.5), ce rapport distingue ce qui est **réalisé**, **testé**, **validé sur matériel réel**, **validé uniquement en émulation**, **dépendant d'une extension**, et les **limitations restantes**.

> **Validation matérielle : aucune.** Aucun test n'a été fait sur console réelle (NES, Famicom, PAL, cartouche MMC1). Toutes les validations ont eu lieu en émulation (jsnes) et par des tests unitaires sur PC. Les critères « vérifié sur console physique » (CDC § 2.1, 15.4, 16.A) **ne sont pas remplis**. Ce point ne peut pas être traité dans l'environnement de développement utilisé.

## 1. Évolutions

### Version 2.67

| Demande | Réalisation |
|---|---|
| Un zoom bien étudié au moment du ralenti pour mettre en valeur le point (choix : gros plan et arrêt sur image zoomé) | La NES ne sait pas agrandir l'image : les deux effets sont préparés pendant de courts flashs blancs. **Gros plan** : à la frappe décisive (dernière frappe de l'équipe gagnante), l'image se fige, flash, puis plan moyen en grand du joueur en pleine frappe, à ses couleurs (images haute définition de la bande-annonce : homme, femme, fillette), arrivée de caméra en travelling, balle qui frappe le tamis (flash) et repart en traînée, titre du coup lettre par lettre ; puis gros plan bras levés sous les confettis ; environ 2 s. **Zoom** : au rebond qui a conclu le point, un viseur se resserre sur le point d'impact, puis l'image est agrandie deux fois, calculée pendant le flash (tuiles du court et sprites de la trame figée lus dans la mémoire du PPU et agrandis) : lignes nettes, joueurs proches et balle agrandis, BONNE ! ou DEHORS ! ; environ 1,6 s. Puis retour au court à l'identique et suite du ralenti. A passe la séquence. Frappe décisive plus ancienne que le ralenti : le gros plan l'ouvre. Club nudiste : ralenti simple. Images : `docs/ralenti_267.png` |
| (moyens) | Cartouche portée à 512 Ko (la MMC3 le permet) ; nouvelle banque 15 (mise en valeur, récapitulatif du score sorti de la banque 0, pleine) ; repères des frappes et des rebonds datés par un compteur des trames enregistrées (le tampon du ralenti est circulaire) ; tables de tuiles échangées pendant le zoom |
| (vérification) | Section de tests ajoutée (à plat et en perspective : ordre des séquences, écran du court identique au retour, tables de tuiles rétablies, A, club nudiste) ; tests des matchs complets et de l'IA adaptés à la durée des ralentis ; 350 vérifications, 0 échec. Guide sur console : ligne 10.18 |

### Version 2.66

| Demande | Réalisation |
|---|---|
| Suite des travaux MMC3 | Les effets par bande pendant le match apporteraient peu (le court remplit l'écran et ne défile pas) ; choix : réduire les ralentissements |
| Ralentissements | Mesure sur 15 000 trames, 4 joueurs de l'ordinateur (trames en retard) : double à plat 1 802 → 698 (12 % → 4,7 %), double en perspective 2 942 → 1 056 (20 % → 7 %), simple en perspective 220 → 81 (comme à plat). Moyens : affichage des joueurs appelé sans passer les paramètres par la pile du C, tri par profondeur et positions à l'écran en assembleur, copie vers le tampon d'affichage en assembleur (4 fois plus rapide), ombres réutilisant les positions déjà calculées, plongeon de l'ordinateur ignoré quand la balle s'éloigne, calcul de l'IA adaptative mis en cache. Test ajouté : double en perspective sous 11 % de trames en retard (la 2.63, à 24 %, échoue) |

### Version 2.65

| Demande | Réalisation |
|---|---|
| Pendant la glissade, le portrait de droite est instable | Cause : sur la vraie NES (et les émulateurs précis), le décalage fin (0 à 7 px) écrit par l'interruption s'applique aussitôt, au milieu de la ligne en cours ; comme il différait entre lignes paires et impaires, la moitié droite de chaque ligne prenait le décalage de la ligne suivante : le buste droit tremblait (l'émulateur des tests ne le reproduit pas). La glissade avance maintenant par pas de 8 px (décalage fin toujours nul : rien ne change au milieu d'une ligne, ni sur les bustes ni sur le titre au-dessus), avec freinage en fin de course. Test ajouté |
| Il subsiste des problèmes d'alignement | Au filet, une correction héritée des lignes à marches de 8 lignes forçait la ligne sur une même colonne 24 lignes au-dessus du filet : marche trop courte (8 lignes au lieu de 16). Retirée : les lignes sont à leur place exacte au pixel près, marches régulières de part et d'autre du filet. Les sprites (joueurs, balle) sont projetés ligne par ligne avec la même formule que les lignes du court |
| Terrain parfaitement symétrique ? | Les lignes du court l'étaient. Restaient : la rambarde des tribunes (x 31 à gauche, 224 à droite, soit 97 et 96 px de l'axe du court, colonne 128) dans les deux vues, et à plat, la sangle du filet (2 px) et le reflet du poteau droit (côté intérieur). Rambarde droite en x 225, sangle de 3 px, reflets côté extérieur. Mesure sur les 6 lieux, dans les deux vues : 0 pixel blanc non symétrique, rambardes symétriques ; seuls diffèrent les décors (spectateurs tirés au hasard, textes des panneaux). Image : `docs/symetrie_265.png` |
| (place) | Pour garder la rambarde au pixel près au club et au parc, 7 signes de la police jamais écrits pendant un match (`# % ( ) ; < =`) et les blocs 1 à 3 libèrent leur tuile pendant le match en perspective ; le générateur vérifie à chaque compilation qu'aucun texte du match ne les emploie |

### Version 2.64

| Demande | Réalisation |
|---|---|
| La MMC3 change-t-elle quelque chose à la perspective ? | Non, pas en elle-même : le court en perspective était dessiné comme avant ; seul l'écran VS utilisait l'interruption par ligne |
| Plus de tuiles pour le court en perspective, « le plus fin possible » | Mesure d'abord : la perspective calculait l'échelle une fois par rangée de tuiles, les lignes obliques avançaient par marches de 8 lignes. Au pixel près, il fallait 112 à 146 tuiles nouvelles par lieu ; il en manquait jusqu'à 32 (club). Pas besoin de changer de carte : pendant le match en perspective, les panneaux et les spectateurs de la vue à plat que le court n'affiche pas ne sont plus chargés, ce qui libère leurs tuiles (et 7 signes de la police jamais écrits, l'ancienne flèche des menus). Les 6 lieux (et le parc) ont maintenant des lignes au pixel près, et la rambarde des tribunes aussi. Les tuiles sont compressées pour tenir dans la banque 11. Image avant / après : `docs/perspective_264.png` |
| (vérification) | Test ajouté : dans 5 lieux, après 900 trames de match et 6 olas, les tuiles du court sont identiques aux données de la ROM ; un programme volontairement fautif est détecté. Guide sur console : ligne 10.17 |

### Version 2.63

| Demande | Réalisation |
|---|---|
| MMC3 : glissade des bustes de l'affiche VS | Première utilisation de l'interruption par ligne : les deux bustes glissent depuis les bords (24 trames, avec freinage), se rejoignent, puis éclair blanc et « CONTRE ». Les bustes sont sur les mêmes lignes de l'écran ; un défilement par ligne ne peut pas les écarter : le gauche est dessiné sur les lignes paires (écran 0), le droit sur les impaires (écran 1), l'œil les fond ; une fois réunis, les lignes redeviennent pleines. B ou A pendant la glissade : interruption coupée proprement. Tests ajoutés (lignes paires et impaires, fin de l'entrée, retour par B) |
| Traits manquants sur le court en perspective | Le nettoyage du filet à plat (2.62) effaçait le haut des lignes de côté juste au-dessus du filet : il est limité aux abords des poteaux. Lignes complètes, court toujours symétrique (0 rangée asymétrique dans les 6 lieux) |
| « La perspective serait mieux gérée sur ce type de cartouche » | Précision : l'interruption par ligne ne peut que décaler ou répéter des lignes (défilement différent par bande, compression verticale), pas rétrécir une ligne horizontalement. Le rétrécissement du court vers le fond reste donc dessiné d'avance. Ce que la MMC3 permettra : banques de tuiles commutables (plus de détails dans les tribunes et les décors), effets par bande |
| Validation sur vraie console | Guide mis à jour (version 2.63, ligne 10.16 : glissade) |

### Version 2.62

| Demande | Réalisation |
|---|---|
| Défauts de sprites : en plein échange, le joueur semble s'agiter dans tous les sens | Mesures : pas d'oscillation de position ni de rafale de poses ; en revanche, en double surtout, des poses montrées sans leurs tuiles (rebond de balle avant le service au moment du lancer, fin de revers après le point) : 126 trames sur un match en double en perspective en 2.58, 28 en 2.61. Désormais, un jeu de tuiles reste chargé tant qu'un joueur en montre une pose : 0 dans les 4 configurations (simple, double, à plat, perspective) ; test ajouté (double en perspective) |
| Symétrie du terrain en perspective | La projection resserrait le court autour de x = 128 alors que le court à plat est symétrique autour de 128,5 (ligne médiane sur la colonne 128) : lignes de simple à 47 et 46 px du centre, etc. Projection recentrée, sangle du filet sur 3 px, poteaux en miroir (reflet côté extérieur), restes du filet à plat effacés au-dessus des poteaux : 0 rangée asymétrique dans les 6 lieux |
| Passer en cartouche MMC3 (étape 1 : à l'identique) | Mapper 4 (TGROM : 256 Ko de programme, 8 Ko de CHR-RAM, 8 Ko de PRG-RAM sauvegardée). Organisation inchangée : chaque banque de 16 Ko devient deux blocs de 8 Ko ($8000 R6 = 2n, $A000 R7 = 2n + 1), la banque 15 reste fixe en $C000-$FFFF. Amorçage dans les 8 derniers Ko (seuls sûrs à l'allumage). Le changement de banque n'est pas plus rapide sur MMC3 dans cette organisation (4 écritures, environ 43 cycles contre 33) : il est sauté si la banque est déjà en place ; double : 11,5 % de trames en retard (10,8 % sur MMC1). L'apport de la MMC3 viendra des étapes suivantes (interruption par ligne, banques de tuiles) |

### Version 2.61

| Demande | Réalisation |
|---|---|
| Faire tout : réorganiser la mémoire | Les variables du générique de lancement, du podium et du générique de fin vont dans la zone de sauvegarde libre ($7800-$7FFF, hors des blocs sauvegardés, dont les adresses ne changent pas : palmarès et réglages conservés) ; les gouttes de pluie passent en RAM de cartouche. Libre : RAM interne 28 octets (8 avant), RAM de cartouche 373 (0 avant), zone de sauvegarde 1,2 Ko |
| Croisé court en volée | Il garde sa longueur (28 au lieu de 20) et part plus vite que les autres volées : il ne fait plus double emploi avec l'amortie de volée |
| Yeux de la bande-annonce | Contours et yeux dans la couleur sombre (cheveux) au lieu de celle du fond, sur toutes les images sauf un gros plan qui dépasserait 246 tuiles et les images de Margaux (blonde) ; cheveux de Papi en gris foncé dans la bande-annonce |
| Mesure et ajustement des coups de B | Coupé un peu plus haut au-dessus du filet ; l'ordinateur ne joue plus l'amortie du fond du court. Fautes du coupé : Normal 11 → 6 %, Expert 22 → 15 % ; amortie de l'ordinateur, plus rare : gagnantes / fautes Normal 32 / 48 % → 67 / 17 %, Expert 9 / 51 % → 40 / 24 % |
| Ralentissements restants | Mesure : simple à plat 83 trames en retard sur 15 000, perspective 163, double environ 9 % (22 % au départ). Le reste : dessin du score en une trame à la fin des points (bref, isolé) et, en double, l'affichage des 4 joueurs et l'IA (il faudrait les réécrire en assembleur) |
| (défaut trouvé par les tests) | Point fini pendant un revers : le jeu de tuiles du revers est gardé tant que le geste dure (sinon, une trame avec une pose sans ses tuiles) |
| Validation sur vraie console | Impossible dans l'environnement de développement ; guide `docs/VERIFICATION_CONSOLE.md` mis à jour (version 2.61, section 10 : nouveautés 2.57 à 2.61) |

### Version 2.60

| Demande | Réalisation |
|---|---|
| Avis sur les coups de A et le croisé court ; faire la mesure, puis tous les ajustements | **Mesure** sur des matchs entre ordinateurs (Normal, Expert ; 5 paires de personnages) : points gagnés directement et fautes par type de coup. Le coup à plat ne domine pas (Normal : 5 % gagnants, 50 % fautes ; Expert : 2 %, 20 %) : inchangé. **Lifté** : relâché aussitôt, plus court (62 au lieu de 70) et plus sûr ; armé, plus bombé et plus lifté (gravité 7 à 11). **Croisé court** : l'armé ferme l'angle (14 px de la ligne de côté relâché vite, 6 px bouton tenu) au lieu d'accélérer, et ajoute moitié moins d'imprécision. **L'ordinateur joue le croisé court** (Normal, Expert) : passing contre un volleyeur, parfois pour sortir un adversaire du fond. Smash dans l'angle court : angle propre. Après : croisé court 8 % gagnants, 24 % fautes (Normal), 5 % et 16 % (Expert) ; fautes du lifté 15 → 13 % (Normal), 16 → 14 % (Expert) |
| Centrer le pavé des réglages et réajuster les visages selon la place libre | Liste centrée verticalement entre les filets (rangées 4 à 21, selon le nombre de lignes) ; portraits abaissés dans la place libre, épaules sous le filet du titre ; la balle curseur rebondit de nouveau sur la première ligne quand la liste ne touche plus les portraits |

### Version 2.59

| Demande | Réalisation |
|---|---|
| Améliorer le lob | **Longueur selon l'armé** (relâché vite : lob court, smashable ; bouton tenu : près de la ligne de fond, risque de faute ; l'ordinateur dose selon son niveau). **Trajectoire selon la position** : du fond, lob défensif très haut et lent ; de l'intérieur du court, lob d'attaque plus bas et plus vif. **Lob parfait** (même fenêtre que la frappe parfaite) : précis et plus profond. **L'Expert recule** une fois sur deux sur un lob (smash s'il est court, sinon rattrapage après le rebond) ; Facile et Normal restent au filet. Mesure sur des matchs Hugo (tactique) contre Nina (filet) : lobs gagnants contre Normal 34 % (33 % avant), contre Expert 16 % (32 % avant) |
| En compétition en double, une seule vignette de joueur | Le portrait du partenaire s'affiche à droite du titre des réglages |
| Le jeu ralentit par moments, même en simple ; puis : traiter aussi le double | Mesure des trames en retard (le jeu n'a pas fini avant l'image suivante), défaut déjà présent en 2.57, puis profil des fonctions les plus coûteuses. Décision de plongeon de l'IA : test rapide de la profondeur, une trame sur deux (sur quatre en double), boucle en page zéro. Ola : une rangée par trame. Déplacement des joueurs (`move_toward`) réécrit en assembleur (environ 500 cycles par joueur en C, moins de 150). Hauteur du saut par table (sans multiplication). `can_hit` teste la profondeur d'abord. Trames en retard sur 15 000 : simple à plat 274 → 90 (pendant l'échange : 98 → 6), perspective 576 → 205, double 3 276 → 1 723 (22 % → 11,5 % ; le reste : affichage des 4 joueurs et IA) |
| Au changement de côté, les chaises seulement après le ralenti, pas dessus | Les chaises n'apparaissent plus pendant l'attente du ralenti ni pendant celui-ci (mesure, ralenti forcé à chaque point : 175 trames avec chaises avant, 0 après) |
| Trait jaune horizontal au niveau de la balle (écran titre), qui ressemble au filet | Filet retiré du court de l'écran titre (lignes blanches seules) |
| En perspective, le poteau du filet n'est pas aligné pareil des deux côtés | Poteaux symétriques (le droit était 4 px plus près du court) |
| L'image est abîmée côté droit, à l'épaule de la seconde joueuse, quand la balle curseur rebondit | Sur la première ligne des réglages, la balle croisait le bas des portraits : 9 sprites sur une ligne, la NES en affiche 8. Sur cette ligne, la balle ne monte plus |
| Une autre teinte de fond, discrète ; puis : plusieurs couleurs au hasard au chargement (terre battue, gazon, dur, moquette) | Le fond des menus est tiré au hasard à chaque allumage parmi les couleurs des quatre surfaces (celles de l'affiche VS) ; sur fond terre battue, l'ombre des lettres dorées passe en noir |
| Remettre « HAUT/BAS » dans l'aide des réglages | Touches sur deux lignes : « HAUT/BAS: LIGNE  <>: VALEUR » et « START : JOUER   B : RETOUR » ; l'explication de la ligne choisie reste sur deux lignes |
| (défaut trouvé par les tests) | Au changement de côté qui suit un ralenti, la pose de fête du gagnant pouvait être montrée une trame sans ses tuiles : pose d'attente forcée, jeu de tuiles des séquences chargé dès la 2e trame |
| Le choix du personnage a-t-il une importance à la manette ? | Non : seuls l'apparence et les gestes changent ; le style de jeu ne concerne que l'ordinateur (précisé dans le manuel) |

Mémoire : la RAM de cartouche est pleine (6 144 octets sur 6 144) la banque fixe presque (le récapitulatif du score est passé en banque 0, libérée par `move_toward` en assembleur).

### Version 2.58

| Demande | Réalisation |
|---|---|
| Améliorer encore les menus : faire toutes les propositions | **Sons de navigation** : petit glissando vers l'aigu (droite) ou le grave (gauche) quand une valeur change, note descendante au retour (B). **Aide de la ligne choisie** dans les réglages : deux lignes en bas de l'écran, selon la ligne et sa valeur (147 textes : modes, surfaces, lieux, formats, exercices… ; joueurs : leur style et ce qu'il veut dire), puis les touches ; sur la ligne COMPETITION, l'explication en 4 lignes est conservée. **Groupes** : une accolade dorée en marge réunit les lignes de la partie, des joueurs, du match et des options, sans rangée de plus. **Portraits** des joueurs (tête et épaules, 32 x 24, dessinés au pixel, à leurs couleurs ; Margaux et Papi aux cheveux clairs avec contours noirs) de part et d'autre du titre des réglages. **Vignette du lieu** (stade, club, salle, club nudiste, court Margaux, château, parc) devant sa valeur. **Entrée de l'affiche VS** : après le fondu, chaque buste jaillit d'un éclair blanc, passe en silhouette puis prend ses couleurs (gauche, puis droite), et « CONTRE » s'inscrit sur un flash. **Tableau à élimination** graphique pour les tournois (inscrits, traits dorés, vainqueur de chaque tour centré entre ses deux joueurs, joueur en or). **Palmarès** : médaille or, argent ou bronze devant chaque meilleur résultat et chaque compétition de l'historique terminée sur le podium. **Écran titre** : court en perspective (lignes blanches, filet doré) sous la grosse balle |
| Pas de bouche, puis : je veux une bouche (portraits) | Les portraits ont une bouche bien visible, au-dessus du menton |
| Papi n'est pas centré pareil (affiche VS) | Buste de Papi cadré comme celui des autres hommes |
| Tableau : centrer davantage les noms verticalement | Demi-finalistes et finaliste exactement au milieu de leurs deux joueurs (coin bas et départ latéral ajoutés) ; au premier tour, faute de rangée intermédiaire (8 joueurs sur 8 rangées), le vainqueur reste sur la rangée du second |

La glissade des bustes de l'affiche VS depuis les bords n'est pas possible proprement sur NES (un seul plan de décor, qui défile d'un bloc) : elle est remplacée par l'apparition en éclair et silhouette. Tests : le hasard du jeu dépend des trames ; une trame de plus dans un menu changeait les matchs des tests. La graine est désormais fixée à l'appui qui lance chaque match de test (matchs reproductibles), et `toMenu` attend l'écran des réglages.

### Version 2.57

| Demande | Réalisation |
|---|---|
| Améliorer les menus visuellement : faire l'ensemble des propositions (habillage, lisibilité, aperçus, affiche VS, écran de compétition, écran titre, transitions) | **Habillage** de tous les menus : fond vert gazon, titres en grandes lettres dorées (RÉGLAGES, PALMARÈS, RÉSULTAT, REPRISE, ENTRAÎNEMENT, nom de la compétition) entre des filets dorés à losange et des étoiles ; palettes : texte blanc, libellés bleu pâle, valeurs jaunes, or. **Réglages** : libellés et valeurs en deux couleurs, ligne choisie allumée (libellé en or, valeur en blanc, flèches < > autour de la valeur), balle qui rebondit devant elle, pastille de la couleur de la surface, les deux joueurs choisis de part et d'autre du titre. **Affiche VS** avant chaque match : les deux joueurs en buste, bras croisés, face à face, à leurs couleurs, « CONTRE » au centre, fond de la couleur de la surface, nom, manette ou niveau, style, puis surface, lieu, conditions et format (au club nudiste : présentation d'origine). **Écran de compétition** : nom du tournoi en or, tour, **parcours** (8E > QUART > DEMI > FINALE, tour à jouer allumé), joueur surligné dans le tableau et le classement, en-têtes en or, **médailles** or, argent, bronze devant les 3 premiers. **Écran titre** : reflet qui passe sur le logo et la balle, scintillement de l'édition, filets dorés, balle qui sautille devant l'entrée. **Transitions** : chaque menu glisse vers sa place pendant qu'il s'éclaire. Le palmarès est déplacé dans une autre banque (place) ; les tuiles des menus sont rétablies avant le match. Non fait, faute de place à l'écran : les groupes séparés dans les réglages (jusqu'à 17 lignes), et le court en perspective en fond de l'écran titre |
| Les visages sont bons, mais on voit à travers le ventre (affiche VS) | Les bustes n'ont que 2 palettes chacun : l'ombre de la tenue tombait dans la palette du visage, qui ne la contient pas, et prenait la couleur du fond. Palettes refaites : A (sombre, peau, tenue : visage, bras croisés sur la tenue), B (tenue ombrée, tenue, blanc : plis et texte) ; plus de trou |
| Le tirage au sort n'est pas aléatoire : il aboutit toujours au même résultat | Le générateur de hasard partait toujours de la même valeur et n'avançait pas dans les menus. Désormais, hors du match, il avance à chaque trame et chaque appui y mêle l'instant où il a lieu : tirage (pile ou face), premier serveur et échanges différents d'une partie à l'autre |
| Sur les bustes de l'affiche VS, des traits blancs comme un carré ; textes doublés à gauche ; couleur des yeux et bas du polo des hommes | La peau tombait dans le blanc (traits en carré) et l'ombre du texte dans la tenue (texte « doublé ») : coûts de couleurs revus, ombre du texte sombre (noire si les cheveux sont clairs) ; buste coupé au-dessus de l'ourlet du polo |
| Polo des hommes : il laissait paraître les hanches sur les côtés (largeur) ; jupe des femmes trop rectangulaire en haut ; coupe de cheveux des hommes | **Polo** aussi large que les hanches, à la même longueur. **Jupe** arrondie : haut remonté à la taille, évasement progressif, ourlet plus large. **Coiffure masculine** refaite : volume sur le dessus, houppe, frange en mèches, côtés courts, mèches dessinées |
| Laisser paraître la forme de la poitrine des femmes, en particulier dans le VS ; puis moins marquée, plus fondue | Sur l'affiche VS, **ombre tramée** sous chaque sein (dessous et côté) : la forme se devine sans trait appuyé. **Margaux**, présentée comme une enfant, a une **silhouette de fillette** au buste droit (affiche VS et bande-annonce) |
| Le haut de la tête des hommes semblait coupé | Buste des hommes descendu dans le cadre : la tête est entière |
| La couleur des yeux est celle du fond de l'écran ; pour tous ? ; un personnage sans visage ; Papi pas centré pareil | Les yeux et contours prenaient la couleur 0, celle du fond (vert sur gazon) : ils prennent désormais la couleur sombre des cheveux, pour les **21 personnages**. **Margaux** (blonde) et **Papi** (cheveux gris, dont le visage disparaissait) ont leurs propres palettes : A (noir, peau, cheveux), B (noir, peau, tenue), le texte de leur côté prenant la palette blanche de l'autre côté. Buste de Papi cadré comme celui des autres hommes. Message du court Margaux recentré (il dépassait la colonne 30) |
| Le générique de début doit rester sur fond noir | Il prenait le fond vert des nouveaux menus : il garde son fond noir |
| Sur les écrans de compétition, les médailles or, argent et bronze ne se distinguaient pas beaucoup | Les trois étaient pâles (jaune pâle, blanc, pêche) : désormais **or** jaune franc, **argent** gris, **bronze** cuivré, chacune avec son reflet |

### Version 2.56

| Demande | Réalisation |
|---|---|
| Au lancement d'un tournoi Martinot, une animation d'environ 6 secondes enchaînant des gros plans percutants (frappe, glissade sur terre battue, bras levés, raquette frappée au sol...) sur une musique rock, et le nom de la compétition qui apparaît de manière cinématographique | **Bande-annonce** (Grand Martinot et Martinworld Tour, environ 7 s) : des plans en gros plan séparés par des coupes franches (sans fondu) : la frappe (la balle arrive, flash blanc à l'impact, elle repart en traînée), la glissade sur terre battue (gerbe de poussière, l'écran tremble), les bras levés au ciel (pluie de confettis, ovation), la raquette frappée au sol (choc, l'écran tremble et s'amortit, éclats) ; fond uni, une couleur par plan. Puis le titre : sur fond noir, entre deux filets dorés, le nom apparaît lettre par lettre en lettres doublées, chacune avec un éclair, un reflet doré le parcourt, puis le sous-titre (« 4 TOURNOIS - LE GRAND CHELEM », « 7 TOURNOIS - TOUTES LES SURFACES »). **Riff de rock original** (basse en croches, accords de quinte, batterie). A ou START la passe |
| Encore plus gros, personnages aussi détaillés que possible ; effets de caméra ou de zoom | **Personnages dessinés en haute définition** (nouveau générateur `tools/gen_teaser.py`) au lieu des sprites agrandis : membres effilés, mains, chaussures lacées, polo à col, short à coutures ou jupe plissée, raquette au tamis cordé, visage (yeux avec le blanc et l'iris, sourcils, nez, bouche selon l'expression : concentration, effort, joie, colère), ombres du côté opposé à la lumière ; jusqu'à 228 tuiles par image, en pied sur presque tout l'écran (190 px) ou en gros plan. Couleurs de chaque joueur, 3 palettes par image (peau et son ombre, tenue et son ombre, peau-tenue-blanc), version masculine et féminine (queue de cheval, jupe). **7 plans** : la frappe en gros plan, la glissade en pied puis zoomée, les bras levés en pied puis zoom sur le buste, la raquette fracassée au sol en pied puis zoom brutal sur le visage qui crie. **Caméra** : travellings éclair d'un plan à l'autre (le personnage file hors champ, le suivant arrive), lente dérive pendant le plan, tremblements ; les zooms sont des coupes à éclair blanc. 14 images compressées (34 Ko) dans les banques libres, décompressées en assembleur (environ 5 trames par plan) |
| Attention aux défauts d'apparence, notamment les visages (têtes étranges) ; bien distinguer femmes et hommes (silhouette, visage, habillement), courbes plus féminines, tenue classique de tennis | **Têtes redessinées** : crâne et mâchoire proportionnés (carrée chez l'homme, plus fine et arrondie chez la femme), oreilles près de la tête, cheveux sans rayures (raie, pattes chez l'homme ; frange de côté, chevelure qui encadre le visage et queue de cheval haute chez la femme), cou éclairé avec l'ombre sous le menton, bouche à la bonne hauteur (plus d'effet de barbiche), pupilles et intérieur de la bouche sombres. **Femmes** : épaules plus étroites, taille marquée, hanches plus larges, bras et jambes plus fins, poitrine suggérée par un pli du tissu, cils, sourcils fins et arqués, lèvres ; **robe de tennis** sans manches à encolure arrondie et jupe plissée, du même tissu. **Hommes** : polo à col et manches courtes, short ; l'écusson du polo, qui ressemblait à un téton, est retiré. En gros plan, une palette peau-ombre-tenue évite les marches d'escalier sur les bras ; zoom de joie sur fond rouge sombre |
| Revoir la tombée des habits (short, robe), les jambes des femmes (trop musclées, comme celles des hommes) ; les jupes paraissaient fendues sur le côté | **Jupe** évasée d'un seul tenant avec le haut de la robe : elle couvre désormais les hanches et le haut des cuisses (la cuisse dépassait sur le côté, d'où l'effet de fente), plis en éventail jusqu'à l'ourlet. **Jambes féminines** fines et galbées (cuisse effilée, genou fin, mollet arrondi, cheville fine), sans traits de rotule ni de mollet ; taille plus marquée. **Short** masculin ample, aux jambes droites plus larges que la cuisse, ourlet net, porté sous le **polo** qui tombe par-dessus (ourlet) au lieu d'être rentré |
| Les cuisses débordaient encore des jupes ; largeur d'épaules et poitrine de la femme ; têtes mal alignées avec le cou ; faire des recherches de proportions (femmes, puis hommes) ; bretelles jusqu'en haut des épaules ; poitrine plus volumineuse ; défaut à l'épaule | **Proportions du canon de dessin de figure** (Loomis, Richer) : les personnages mesuraient 6,35 têtes avec des jambes 40 % trop courtes. Ils sont désormais construits par **squelette** (longueurs de segments et angles des articulations) : femme 7,5 têtes, épaules 1,7 tête, hanches 1,8 ; homme 7,8 têtes, épaules 2,4, hanches 1,5, membres plus longs, cou plus épais ; entrejambe à mi-hauteur, cuisse et tibia d'environ 2 têtes. Tête dans l'axe du cou. Vêtements : le buste et ses vêtements passent toujours devant le haut des cuisses, les bras toujours devant le buste ; robe qui couvre les hanches, jupe enveloppant le haut des cuisses (contrôle automatique : plus de cuisse découverte) ; short à deux jambes qui suivent les cuisses, polo par-dessus. Robe : bretelles jusqu'en haut des épaules, poitrine plus volumineuse avec une ombre douce |
| Autour des images, une démarcation sur la couleur de fond | La NES colore l'écran par blocs de 16 x 16 pixels : les blocs qui touchent le personnage prennent ses palettes, où les bandes obliques du fond ne pouvaient pas apparaître, d'où un halo en escalier. **Fond uni** (une couleur par plan) : plus aucune démarcation ; le mouvement reste donné par les travellings, les tremblements et les effets |
| Dans la bande-annonce, la dernière image (titre jaune sur fond noir) affichée une seconde de plus ; l'image de présentation du tournoi juste avant le début du match, et non juste après la séquence rock | **Titre tenu une seconde de plus** (bande-annonce d'environ 7 s). **Image du tournoi déplacée** : après la bande-annonce vient directement l'écran de la compétition ; au premier match de chaque tournoi, l'image du tournoi et sa fanfare s'affichent après la présentation du match, juste avant son début (A lance le match) |

### Version 2.55

| Demande | Réalisation |
|---|---|
| Il subsiste un défaut de terrain au niveau du poteau gauche en perspective | Entre la chaise d'arbitre et le poteau gauche du filet, une bande de 3 px couleur du court (et un morceau de chaise en trop) descendait le long de la chaise. Cause : l'image en perspective est calculée en échantillonnant le court à plat ; à côté de la chaise, l'échantillonnage tombait sur la chaise dessinée à plat, ensuite redessinée à sa place. Sous la chaise, l'échantillonnage prend maintenant les abords (la rangée juste au-dessus). Vérifié dans les 6 lieux ; 5 tuiles de moins par lieu en perspective |

### Version 2.54

| Demande | Réalisation |
|---|---|
| En entrant sur le court, le joueur qui va au fond marche à reculons | Pendant les séquences (entrée, changement de côté, marche vers le filet), un joueur qui s'éloigne vers le haut est maintenant vu de dos, et un joueur qui vient vers le bas est vu de face ; en marche latérale, il est de profil. Une fois arrivé, il reprend la vue de son côté du court (`view_x`, lu par spr_person) |
| Au changement de côté : joueurs assis de profil ; chaises visibles dès la fin de l'échange, les joueurs s'y rendent pour s'installer | Les chaises (de profil, tuile permanente) apparaissent dès la fin de l'échange qui amène le changement de côté (prévu avec les règles du score : jeu gagné qui rend le total de jeux impair, ou 6 points au jeu décisif, hors fin de set) ; les joueurs s'y rendent et s'assoient **de profil**, tournés vers le court (5e image de profil, « assis » : seule la rangée des jambes est nouvelle), serviette en écharpe autour du cou, gourde à la bouche puis posée |
| Un vrai tirage au sort : l'arbitre attribue au hasard pile ou face à chaque joueur, lance la pièce, puis donne le résultat | Le bandeau annonce d'abord ce que l'arbitre attribue (« ALEX : FACE », « MAYA : PILE », au hasard), la pièce est lancée, puis le résultat tombe (« FACE ! ALEX SERT ») : le gagnant du tirage sert le premier jeu. Le serveur du premier jeu étant déjà tiré au hasard au début du match, la pièce tombe sur le côté attribué à ce joueur : résultat et service sont toujours cohérents |

### Version 2.53

| Demande | Réalisation |
|---|---|
| Grand Martinot et Martinworld Tour : format comme les vrais Grands Chelems (option 2) | Format imposé : tous les matchs en 3 sets, les finales de chaque tournoi en 5 sets. La ligne FORMAT disparaît du menu pour ces deux compétitions ; l'écran de présentation du match affiche le format réel (« 3 SETS » ou « 5 SETS »). Pour faire la place dans la banque des écrans (pleine), les lignes du menu sont ajoutées par une fonction commune (500 octets gagnés) |

### Version 2.52

| Demande | Réalisation |
|---|---|
| Plus de séquences animées : entrée des joueurs et tirage au sort, balle de match, poignée de main au filet, changement de côté (pour tous les matchs) ; remise des médailles (compétitions classiques) ; remise du trophée et conférence de presse (Grand Martinot, Martinworld Tour) | **Entrée** : les joueurs sortent du tunnel (en bas à droite), longent le court jusqu'à leur place, saluent le public, puis l'arbitre lance une pièce (« PILE ! » ou « FACE ! ») et annonce l'équipe au service. **Changement de côté** (sans fin de set) : les joueurs s'assoient sur leur chaise près de l'arbitre, serviette sur les épaules, gourde à la bouche puis posée, pendant le récapitulatif du score ; puis ils gagnent l'autre côté en passant à côté du poteau du filet. **Balle de match** : après la fête habituelle, les vainqueurs tombent à genoux, poings levés, puis s'allongent sur le dos, raquette lâchée au sol ; tous vont au filet se serrer la main, puis saluent l'arbitre. A ou START passe chaque séquence. Vue à plat et en perspective, simple et double ; jamais en démonstration ni à l'entraînement |
| Graphismes | 4 poses nouvelles (poignée de main, victoire à genoux, allongé sur le dos, assis) et 4 accessoires (chaise, gourde, serviette, raquette au sol) dans un 3e jeu de tuiles par phase, chargé seulement pendant les séquences (34 tuiles, comme les deux autres) ; la pièce est la tuile de la balle. Pour faire place aux tables des nouvelles poses dans la banque fixe, la frappe parfaite et les messages de timing de l'entraînement passent dans la banque 13 |
| Remise des médailles (compétitions classiques : tournoi, poules, championnat, saison, personnalisé) | Sur le podium : les joueurs apparaissent un à un sur leur marche (3e, 2e, puis le vainqueur), avec un flash ; les médailles de bronze, d'argent et d'or descendent sur leur ruban jusqu'au cou des joueurs ; hymne original (nouveau morceau) et flashs des photographes ; puis la coupe est brandie sous les feux d'artifice, comme avant |
| Remise du trophée et conférence de presse (Grand Martinot, Martinworld Tour) | **Remise du trophée** : tapis rouge jusqu'à la table des trophées, ramasseurs de balles en haie d'honneur, l'officiel à côté de la table ; le finaliste monte le tapis, reçoit sa plaque, salue et se range ; le vainqueur monte à son tour, reçoit la coupe, l'embrasse, puis la soulève sous les confettis et les feux d'artifice (« GRAND CHELEM ! » si tous les tournois sont gagnés). **Conférence de presse** : le champion derrière la table, micros, flashs des photographes, trois questions et leurs réponses (la famille s'invite : les goûters de Déborah, Augustin qui veut la coupe, Paul qui lance un défi au ping-pong, Margaux à l'arbitrage). A avance ; le générique de fin suit |
| Une barre couleur chair dans le dos des personnages ; puis « il manque un bras » ; « je veux la perfection » | Le dessin des personnages est commun aux deux vues (tuiles partagées) : de face, un bras qui passe devant le buste (préparations du coup droit et du revers, revers, volée, impact du service) est juste ; de dos, il barrait le dos. Une première correction (bras couleur maillot) faisait disparaître ce bras de face. Solution retenue, sans compromis : **chargement à la volée**, comme les jeux NES de fin de vie. Pour le joueur vu de dos dans un de ces 9 gestes, les 2 tuiles de son buste « bras caché » sont copiées dans une case réservée (2 cases, une par joueur vu de dos en double, à la fin de la zone partagée, libre pendant l'échange), dans la même image que le sprite ; de face, le bras reste visible. Aussi pendant les ralentis. La pose de colère est redessinée (raquette qui tape le sol à côté du pied) : ses bras ne passent plus devant le buste. Aucune tuile permanente en plus |
| En perspective, le terrain déborde encore au niveau du filet | Sous le filet, le fond était repeint de la couleur du court jusqu'aux poteaux, au-delà des couloirs : une encoche de court de chaque côté. Le fond reprend maintenant celui de la rangée juste au-dessous (court ou abords) |
| Défaut corrigé au passage | Margaux : sa chevelure était aussi dessinée dans la rangée du haut, vide, de la pose du plongeon (une mèche flottait au-dessus d'elle quand elle plongeait) |

### Version 2.51

| Demande | Réalisation |
|---|---|
| Défaut d'alignement des lignes du court des deux côtés du filet (vue en perspective) | En perspective, les lignes obliques ne se décalent d'un pixel qu'aux frontières de rangées de tuiles (8 px), pour économiser les tuiles. Pour les 4 lignes de côté, une de ces marches tombait derrière le filet : la ligne ressortait du filet un pixel plus loin qu'elle n'y entrait, et paraissait cassée. Désormais, chaque ligne ressort exactement où elle entre (mêmes colonnes juste au-dessus et juste au-dessous du filet) ; au-dessus du filet, l'escalier de la ligne est décalé d'une rangée jusqu'à rejoindre sa place, chaque marche restant d'un pixel. Mesuré sur les 4 lignes, tous lieux ; même nombre de tuiles |

### Version 2.50

| Demande | Réalisation |
|---|---|
| Revoir le plongeon : un vrai plongeon pour rattraper une balle, testé et ajusté | **Avant** : le plongeon partait après la frappe (le joueur frappait debout, puis s'envolait vers une balle déjà partie) et ne rattrapait aucune balle. **Maintenant** : il part avant la frappe, sur une balle trop loin sur le côté (environ 24 à 42 px), et la frappe part en vol : un lob de sauvetage, plus lent et moins précis. Vol de 18 px en arc (jusqu'à 9 px de haut), poussière à la réception (terre battue, plage, glace), puis le joueur reste au sol, se relève un genou à terre (nouvelle pose, 2 tuiles) et revient en garde : immobile environ une demi-seconde. Plus de plongeon automatique après une frappe ordinaire |
| Le plongeon se décide au lâcher du bouton | Joueur humain : au lâcher du bouton, sans direction à tenir ; bien placé, frappe normale ; balle trop loin, plongeon. L'ordinateur plonge quand il n'arriverait pas à temps en courant (souvent en Expert, rarement en Facile), jamais sur une balle qui sort |
| Mesures | Joueur humain, balle à 28-42 px : touchée en plongeant ; à 46-48 px : hors de portée ; bien placé : frappe normale. Ordinateur contre ordinateur (terre, gazon, dur, plage, glace) : environ un plongeon pour 50 frappes, 35 plongeons sur 35 ont touché la balle après l'ajout du filtre « balle qui sort » (avant ce filtre, 7 sur 11). Vue à plat et en perspective, simple et double |

### Version 2.49

| Demande | Réalisation |
|---|---|
| D'autres noms pour les 3 nouveaux tournois du Martinworld Tour | Glace : **Margaux-les-Neiges** ; plage : **Rio de Bruneiro** (Rio de Janeiro et Bruno) ; moquette : **Deborcy Masters** (Bercy et Déborah), dont l'image porte maintenant l'enseigne « DEBORCY ». Noms changés dans les écrans d'étape, les annonces, le mode d'emploi et la documentation |

### Version 2.48

| Demande | Réalisation |
|---|---|
| Un autre nom pour le Tour du monde : « Martinworld Tour » | La compétition s'appelle **MARTINWORLD TOUR** sur les écrans d'étape, les annonces des tournois, le podium et le générique de fin (« MARTINWORLD TOUR CHAMPION »). Dans le menu et le palmarès, où la place est comptée (14 signes), elle s'appelle **MARTINWORLD** |

### Version 2.47

| Demande | Réalisation |
|---|---|
| Une autre compétition, un Grand Chelem sur toutes les surfaces du jeu, 3 matchs par tournoi, avec le même principe et une image de présentation par tournoi ; on garde les 4 images déjà dessinées, on en crée d'autres, et on invente les noms | **Le Tour du monde** : 7 tournois de 8 joueurs (quarts, demis, finale ; 8 équipes en double), un par surface, toujours au stade, de jour : **Open d'Augustralie** (dur), **Saint-Martinotz** (glace), **Debogarros** (terre battue), **Martinbledon** (gazon), **Copamargaux** (plage), **US Opaul** (dur), **Bruno-Bercy** (moquette), dans l'ordre d'un calendrier. Points par tour (360 à 2 000), classement général, Grand Chelem si les 7 tournois sont gagnés (podium, générique de fin). Explication dans le menu ; lieu, conditions et surface imposés |
| Images des nouveaux tournois | Saint-Martinotz : les Alpes enneigées, sapins, chalet, court tracé sur une patinoire, **la neige tombe**. Copamargaux : la baie de Rio, le Pain de Sucre et son téléphérique, le Corcovado et sa statue, palmiers, filet sur la plage, **la mer scintille**, promenade aux vagues noires et blanches. Bruno-Bercy : le palais de Bercy la nuit, pentes de gazon, charpente, enseigne, **les projecteurs balaient le ciel**, la Seine. Écran : « TOURNOI 2/7 : GLACE », etc. |
| Fanfares | 3 fanfares originales : une tyrolienne avec grelots (glace), une petite samba (plage), une entrée des champions sous les projecteurs (moquette) |
| Codes de reprise | Le Tour du monde a ses codes (format de version 0, marqué) ; les anciens codes restent valables |
| Défaut corrigé | Le meilleur rang du Grand Martinot s'écrivait dans l'octet du club nudiste (une 3e place le débloquait). Les meilleurs rangs du Grand Martinot et du Tour du monde ont maintenant leur propre bloc de sauvegarde ; le palmarès affiche les 7 compétitions |

### Version 2.46

| Demande | Réalisation |
|---|---|
| Travailler davantage Martinbledon | Image de l'annonce redessinée : Big Ben avec son étage de l'horloge en pierre dorée, un grand cadran blanc (chiffres et petite aiguille ; la grande aiguille tourne), un beffroi gothique, une flèche et des pinacles ; le Parlement avec ses contreforts surmontés de tourelles et ses rangées de fenêtres ; la tour Victoria et son drapeau ; la grande roue (London Eye) ; la bâche violette de Wimbledon et un court de gazon rayé, lignes blanches. 158 tuiles, aucune fusionnée. La grande aiguille (sprite) est recentrée sur le nouveau cadran |

### Version 2.45

| Demande | Réalisation |
|---|---|
| Pour l'US Opaul, les immeubles ressemblent à une usine | Ligne d'horizon redessinée : tours fines séparées par le ciel, hauteurs très variées, trois silhouettes célèbres (Empire State Building en gradins avec son antenne et son feu, Chrysler Building avec sa couronne en arcs lumineuse et sa flèche, tour effilée façon One World Trade Center) ; fenêtres sur une grille fixe, éclairées selon 3 motifs, qui scintillent toujours en deux groupes (126 tuiles, sans fusion) |

### Version 2.44

| Demande | Réalisation |
|---|---|
| Un mode Grand Chelem : 4 tournois (dur, terre battue, gazon, dur), aux noms mêlant les vrais tournois et la famille ; 16 joueurs | **Le Grand Martinot** : **Open d'Augustralie** (dur), **Debogarros** (terre battue), **Martinbledon** (gazon), **US Opaul** (dur). Tournois de 16 joueurs (huitièmes, quarts, demis, finale) ; en double, 8 équipes (16 joueurs : il n'y a pas assez de personnages pour 16 équipes toutes différentes). Une défaite ne termine que le tournoi en cours. Points comme les vrais tournois du Grand Chelem : 180, 360, 720, 1 200, 2 000 ; classement général (les 7 premiers et votre place). Adversaire un cran plus fort en finale. Gagner les 4 tournois : « GRAND CHELEM ! » sur le podium, puis le générique de fin (« GRAND SLAM CHAMPION ») ; 1er au classement sans les 4 titres : podium, « CHAMPION ! », générique. Codes de reprise : nouveau format (version 0) pour ce mode, les anciens codes restent valables |
| Le Grand Martinot se déroule toujours au stade, sur ses surfaces, de jour, sans pluie | Lieu (stade) et conditions (jour) imposés pour ce mode ; les lignes LIEU et CONDITIONS ne sont plus proposées au menu |
| Un écran au début de chaque tournoi qui l'annonce, avec une image représentative ; une fanfare | Au premier tour de chaque tournoi du Grand Martinot : écran d'annonce avec une image pleine largeur (224 x 128 px), le nom du tournoi, sa surface et le tour. **Open d'Augustralie** : l'Opéra de Sydney et le pont du port au coucher du soleil, l'eau de la baie scintille. **Debogarros** : la tour Eiffel qui scintille au crépuscule, au-dessus d'un court de terre battue. **Martinbledon** : Big Ben et le Parlement, gazon rayé ; la grande aiguille de l'horloge tourne. **US Opaul** : la statue de la Liberté brandit une raquette devant les gratte-ciel de New York la nuit ; les fenêtres s'allument et s'éteignent. Chaque tournoi a sa fanfare originale (air sautillant, valse musette, cuivres, petit swing). Aux tours suivants, le nom du tournoi reste rappelé sur l'écran d'étape et sur la présentation du match |
| Expliquer brièvement et clairement le fonctionnement de chaque compétition dans le menu | Curseur sur la ligne COMPETITION : 4 lignes d'explication en bas du menu, à la place du style de l'adversaire et de l'aide des touches, qui reviennent dès que le curseur quitte la ligne |

### Version 2.43

| Demande | Réalisation |
|---|---|
| Le plongeon manque de fluidité et n'est pas très compréhensible | Le joueur passait d'un coup de debout à allongé au moment de la frappe, restait figé sur place, puis se relevait d'un coup. Le plongeon est maintenant découpé : **élan** en fente vers la balle (3 trames), **vol** (7 trames : corps à l'horizontale, décollé du sol jusqu'à 6 px, le joueur avance vraiment de 9 px vers la balle), **réception** avec poussière, sable ou éclats de glace, **au sol** (8 trames), **relevé** en fente (4 trames). Du plongeon au relevé, le joueur ne peut pas bouger : c'est le prix d'une balle rattrapée, comme en vrai. Mêmes règles pour le CPU |
| Ça ne glisse pas tellement sur terre battue | La glissade à l'arrêt passe d'environ 2 px à 6 px (le joueur garde les trois quarts de sa vitesse à chaque trame, au lieu de la moitié), et le joueur glisse aussi quand il fait demi-tour en pleine course (environ 4,5 px avant de repartir). Équilibre vérifié avec le robot « humain » : 83 % des points sur terre battue, 80 % sur gazon (dans le bruit de la mesure). **Défaut trouvé en mesurant** : pendant le chargement du jeu de tuiles du revers, la boucle du jeu tournait deux fois moins vite (copie trop lente en C) ; une routine en assembleur, placée à côté des tuiles en banque 13, charge maintenant 3 tuiles par trame sans ralentir le jeu (10 trames au lieu de 18 à mi-vitesse) |
| Il manque des morceaux de ligne blanche en perspective | Le court en perspective était obtenu en allant chercher, pour chaque pixel, la couleur du court à plat ; au fond, où le court est rétréci, ce prélèvement sautait parfois la colonne d'un pixel d'une ligne de côté (lignes du simple et du double, médiane de service). Les lignes verticales sont maintenant redessinées directement en perspective, ligne de pixels par ligne de pixels : plus aucun trou, sur les 7 lieux, sans tuile de plus |
| Analyser le geste de revers ; faire les 5 corrections | **Analyse** : seul l'armé était un revers à deux mains ; l'impact et l'accompagnement reprenaient le coup droit en miroir (une seule main, comme un gaucher), le revers coupé était dessiné à deux mains, la volée de revers à une main. **Nouvelles poses** : préparation du revers (raquette basse de l'autre côté, deux mains), impact à deux mains (bras tendus vers la balle), fin à deux mains (raquette au-dessus de l'épaule opposée), revers coupé à une main (raquette haute, bras libre en arrière ; impact et fin à une main), volée de revers à deux mains. **Place** : il ne restait que 5 tuiles de sprites ; les poses de célébration (déception, rebond avant le service, poing serré, colère, fatigue), jamais montrées pendant l'échange, et celles du revers, montrées seulement pendant l'échange, se partagent maintenant 34 tuiles (« jeu de tuiles par phase ») : le jeu utile est rechargé en mémoire graphique à chaque changement de phase, jusqu'à trois tuiles par trame (9 à 23 trames) ; d'ici là, la pose d'avant (secours) est montrée, jamais une tuile à moitié chargée. Le choix du coup a quitté la banque 3, pleine, pour la banque 13. **Défaut corrigé** : en 2.42, la tuile des éclats de glace (15) était aussi celle des cheveux de Margaux de profil ; elle a maintenant sa propre place |

### Version 2.42

| Demande | Réalisation |
|---|---|
| Garder le jeu actuel et ajouter au menu une option « perspective » pour jouer exactement au même jeu, aussi complet, en perspective (15 %) — étape 1 | Nouvelle ligne **VUE : À PLAT / PERSPECTIVE** dans le menu, dans tous les modes (match, compétition, entraînement), mémorisée avec les autres réglages. En perspective, le court se resserre de 15 % vers le fond : les 7 lieux ont un court redessiné (lignes, couloirs, filet, poteaux et chaise d'arbitre), les tribunes et le décor restent les mêmes. Les règles, la physique, l'IA et les commandes ne changent pas : seul l'affichage est projeté. Joueurs, balle et son ombre, ombres des joueurs, poussière, éclaboussures, repère d'atterrissage, goutte de sueur, traînée du service canon, balle du dribble, éclat de la frappe parfaite et ralentis suivent la projection. Les joueurs gardent pour l'instant leur taille (étape suivante : 5 à 6 tailles selon la distance). Coût mesuré : en simple, comme à plat ; en double (4 CPU), 508 trames d'échange en dépassement contre 477 à plat sur la même mesure. Hors match, les tuiles d'origine sont rétablies |
| Le public fait bizarre en perspective : solution 2 (tribunes en perspective) | Les tribunes des 7 lieux suivent maintenant la perspective : leur bord (la rambarde) reste parallèle aux lignes de côté et elles s'avancent vers le court au fond, au lieu de laisser une bande vide qui s'élargissait. Dans la moitié du fond, les spectateurs sont plus petits (environ 85 %). Le public continue de suivre la balle du regard et de faire la ola, y compris dans les colonnes ajoutées et avec les petits spectateurs. Chaque lieu garde son décor (gradins, bancs et haies du club et du parc, gradins et panneaux de la salle, balcons du château, cœurs du court Margaux, pelouse du club nudiste). Pour faire la place, les tuiles du court à plat, inutiles pendant un match en perspective, sont réutilisées puis rétablies ensuite |
| Analyser et retravailler le public de tous les courts pour qu'il soit plus harmonieux, avec davantage de tailles | **Analyse** : grille trop régulière au stade (4 dessins seulement), saut brutal entre deux tailles, aucune marche de gradin ; bancs du club et balcons du château en bandes sombres ; arbres coupés au bord des tribunes. **Refonte** : 4 modèles de spectateurs (cheveux courts, longs, casquette, cheveux clairs) au même style dans tous les lieux, décalés d'une place à l'autre ; marches des gradins visibles au stade, gradins rouges, cheveux noirs, peau claire. **En perspective, 4 tailles** : normale au premier plan, environ 90 %, 80 %, puis une foule lointaine à 4 petites têtes par tuile (plus loin, c'est plus petit et plus serré). Rambarde au pixel près. Le décor sans public (arbres, bannières, torches, spectateurs du club nudiste) glisse entier vers le court au lieu d'être coupé ; dans la salle, les gradins restent au bord et le mur s'élargit avant les panneaux. Le regard qui suit la balle et la ola animent toutes les tailles |
| Le plongeon semble bizarre et ne se réalise jamais (dans les deux vues) | **Cause** : il n'apparaissait que pendant la préparation du coup, quand la balle était encore à 20 px ou plus, et le joueur se redressait au moment même de la frappe ; on ne le voyait presque jamais. Le dessin (raquette pointée vers le ciel) faisait aussi bizarre. **Maintenant** : quand la balle est frappée à 20 px ou plus sur le côté, le joueur plonge vers elle (miroir selon le côté de la balle) et reste au sol au moins 8 trames (jusqu'à 22), puis se relève dès qu'il se remet à courir ; poussière sur terre battue et sur glace. Nouveau dessin : corps allongé au ras du sol, tête vers la balle, bras et raquette tendus devant, main d'appui au sol. Mesuré : environ 1 frappe sur 15, dans les deux vues. Le jeu lui-même (zone de frappe) ne change pas |
| Dérapages sur glace, sable et terre battue : la version visuelle et le vrai dérapage physique | **Physique** (`src/slip.s`, appliquée après les déplacements de tous les joueurs, humains et CPU, pendant l'échange) : **glace**, inertie (départ progressif, environ 5 px de glissade à l'arrêt ou au demi-tour) ; **terre battue**, courte glissade à l'arrêt (environ 2 px) ; **plage**, départ lourd (plein régime en 3 trames), arrêt net ; gazon, dur et moquette inchangés. Les bornes du court et du filet restent respectées. **Effets** : poussière sur terre, grains de sable clairs sur la plage, éclats de glace blancs, à chaque arrêt brusque ou demi-tour ; sur terre et glace, une trace au départ de la glissade. **Équilibre** mesuré avec le robot « humain » contre le CPU Normal : environ 75 % des points sur gazon, 84 % sur terre et sur glace, 73 % sur la plage après adoucissement du départ dans le sable (68 % au premier réglage). En double, chaque IA ne bougeant qu'une trame sur deux, ses trames de repos sont ignorées. Coût : environ 500 cycles par trame en double |
| Smash, smash sauté et volées influencés par les choix de coup : tout faire | Le choix du coup quitte le moteur (banque 0, presque pleine : 560 octets libérés) pour la banque 3 (`shot_select`). **Smash dirigé** : A à plat, A + HAUT vers la ligne de fond, A + BAS dans l'angle court, B smash coupé, B + BAS smash amorti. **Charge** : un smash armé est plus rapide (jusqu'à + 16) et moins précis. **Volées** : les choix restent (lob, amortie, coupé, à plat, croisé court, lifté), en plus court ; la volée à plat bien armée passe le plafond de vitesse (52 au lieu de 42) ; l'amortie volée meurt plus près du filet. **Seuil du smash** relevé de 20 à 24 px (au-dessus de la tête) : les volées à hauteur d'épaule restent des volées. **Geste** : le smash armé s'affiche dès 24 px, seulement avant le rebond (il apparaissait à partir de 40 px, et aussi sur des balles hautes après le rebond, qui ne sont pas des smashs). Le CPU profite des mêmes choix selon sa tactique (smash amorti contre un adversaire resté au fond, par exemple) |
| (défaut trouvé par les tests) | Depuis la version 2.40, le « OOOH » du public sur une balle dehors de peu et son soupir après une double faute étaient étouffés : le public se tait pendant la voix de l'arbitre. Ils viennent maintenant juste après l'annonce |

### Version 2.41

| Demande | Réalisation |
|---|---|
| La peau de l'arbitre et des spectateurs semble un peu verte sur certains lieux | Analyse : l'arbitre avait la peau en jaune pâle ($38) et les spectateurs du club, de la salle, du château et du parc en jaune ($28). Dans plusieurs palettes d'émulateur, ces deux couleurs ont plus de vert que de rouge (par exemple $28 = rouge 160, vert 170, bleu 0). Les deux passent au beige $37 (rouge 228, vert 196, bleu 144), teinte de peau déjà portée par plusieurs joueurs. Les spectateurs du stade ($27) et du club nudiste ($27) n'étaient pas concernés. La couronne et la baguette de la princesse, de la même couleur que la peau, deviennent beige doré |

### Version 2.40

| Demande | Réalisation |
|---|---|
| Je n'entends pas les annonces de l'arbitre, ce sont toujours les mêmes sons | Les échantillons jouaient bien (vérifié sur le canal DMC de l'émulateur), mais trop bas : ils n'utilisaient qu'un quart de la plage du canal, et partaient en même temps que le bip franc et le murmure du public. La voix utilise maintenant toute la plage, avec une saturation douce qui la rend plus présente ; pendant l'annonce, le bip est discret (volume 3) et le public se tait |

### Version 2.39

| Demande | Réalisation |
|---|---|
| Toutes les améliorations du son | **Musiques à 4 voix** : en plus de la mélodie et de la basse, une harmonie (« oum-pa » en contretemps pour la polka et la valse, accords tenus ailleurs) et des percussions (grosse caisse, caisse claire, charleston). Elles sont générées à partir des accords de chaque morceau ; un bruitage du menu ne coupe plus la musique que le temps du bip. **Voix de l'arbitre** : « Out! », « Fault! » et « Let! » en vraie voix (échantillons du canal DMC, synthétisés en anglais) ; voix plus aiguë pour la princesse du court Margaux. **« Pok » échantillonné** à chaque frappe. Pour faire la place des échantillons dans la banque fixe, les musiques passent dans une banque libre et sont copiées en mémoire au moment de jouer. Sur console, la lecture des manettes est doublée pendant un échantillon (défaut connu de la NES) |
| Des personnages en plus | MAX (puissance, tenue orange), LOLA (offensive, anthracite), PAPI (défensif, cheveux gris), TITOU (fantaisie, comme Margaux) : 21 personnages. Couleurs contrôlées sur tous les courts. Les codes de reprise les acceptent ; les anciens codes restent valables |
| Des lieux en plus | **Château** : murs de pierre, bannières royales, torches, spectateurs aux balcons (6 nouvelles tuiles de décor sur les 8 qui restaient). **Parc** : le décor du club aux couleurs de l'automne. Les codes de reprise utilisent un nouveau format pour ces deux lieux ; ceux des autres lieux ne changent pas |
| IA qui ajuste davantage sa tactique selon le score | Une fois par point : menée de 2 jeux ou plus, l'IA frappe plus fort, prend moins de marge et monte au filet ; devant, elle joue placé et fait deux fois moins de fautes ; face à une balle de set ou de match, elle défend (marge, peu de fautes) ; avec une balle de set ou de match, elle attaque ; sur une balle de break contre elle au service, elle assure sa première balle |

### Version 2.38

| Demande | Réalisation |
|---|---|
| Le jeu tourne-t-il sur console originale ? (correction proposée : sons muets) | Sur la vraie NES, l'unité de balayage d'un canal carré coupe toute note dont la période dépasse $3FF quand elle est réglée « montant, décalage 0 », même désactivée. Le bourdon grave (« ARE YOU SURE? », confirmation de l'effacement) et le « pop » des feux d'artifice auraient été muets. Le balayage inactif est maintenant réglé à $08 (descendant) pour tous les sons et la musique. jsnes ne reproduit pas ce comportement : un test le simule sur toutes les notes jouées (générique, menus, match, feux d'artifice) |

### Version 2.37

| Demande | Réalisation |
|---|---|
| Animation des joueurs : tout faire (frappes, entre les points, déplacements, finitions), même au-delà des capacités de la NES si l'émulateur suit | **Frappes** en 3 temps : préparation pendant que la balle arrive (coup droit ou revers à deux mains), impact bras tendu, accompagnement propre au coup. Gestes propres à la volée (raquette devant, geste sec), au smash (armé comme le service), à l'amortie (geste retenu) et au lob (raquette vers le ciel) ; plongeon sur une balle très lointaine. **Entre les points** : le serveur fait rebondir la balle deux fois avant de servir ; le receveur attend genoux fléchis en se balançant ; le gagnant fête à sa façon selon le personnage (poing serré, bras levés, poing serré en sautillant ; Margaux fait une pirouette) ; le perdant tape le sol avec sa raquette après une faute, ou met les mains sur les genoux après un long échange. **Déplacements** : course en 4 images de face, de dos et de profil, pas chassés près de la balle, glissade avec poussière sur terre battue et sur glace. Tout tient dans la NES : 22 poses dans 180 tuiles grâce au partage des rangées (253 tuiles de sprites sur 256), et le calcul des poses, en assembleur, a quitté la banque fixe. Charge mesurée : comme avant en simple ; en double, 257 trames d'échange en dépassement sur 11 416 (244 avant) |

### Version 2.36

| Demande | Réalisation |
|---|---|
| L'arbitre plus près du filet que dans le public | La chaise et l'arbitre (ou la princesse) quittent les tribunes : ils sont au bord du court, contre le poteau gauche du filet, toujours centrés sur lui, dans les 5 lieux, de jour, de nuit et sous la pluie. Pour faire la place sans toucher aux couleurs du court, la surface dépasse un peu moins des lignes de côté (6 px au lieu de 10) et les poteaux sont aux bouts du filet. Les spectateurs reprennent les places libérées |

### Version 2.35

| Demande | Réalisation |
|---|---|
| Dimensions de la balle : tout appliquer | La balle grossit maintenant en douceur, en 4 tailles : 4 px près du sol, 5 px à partir de 12 px de hauteur, 6 px (nouvelle taille) à partir de 20 px, 7 px à partir de 40 px. Avant, elle sautait de 5 à 7 px dès 30 px de hauteur et paraissait trop grosse en haut des échanges ordinaires ; la grande balle est maintenant réservée aux vrais lobs. La petite ombre suit le même palier (40 px). La taille au sol (4 px) ne change pas : c'est le minimum lisible en mouvement |

### Version 2.34

| Demande | Réalisation |
|---|---|
| Ombre de la balle selon la hauteur | L'ombre rétrécit quand la balle monte : grande près du sol, moyenne au-dessus de 12 px, petite au-dessus de 30 px. On juge mieux les lobs et les smashs |
| Rebond écrasé | Au moment du rebond, la balle est aplatie pendant 3 trames (pas quand elle roule) |
| Voir l'effet de la balle | La couture blanche tourne : dans le sens du déplacement pour un lifté, en sens inverse pour un coupé, plus vite pour une amortie ; immobile pour une balle à plat |
| La balle passe derrière le filet | Dans le camp du haut, à hauteur du filet, la balle est vue à travers les mailles (dessin en damier). Dans le camp du bas, elle passe devant |
| Couleur de la balle adaptée au terrain : vraie couleur quand c'est possible, couleur approchante sinon | Jaune de vraie balle de tennis (la teinte NES la plus proche) sur terre, gazon, dur, moquette, glace et court Margaux ; vert-jaune vif sur le sable jaune de la plage, où le jaune serait invisible. Écart de couleur d'au moins ΔE 40 partout, de jour, de nuit et sous la pluie. La balle utilise maintenant la palette des effets : l'arbitre garde sa couleur de peau. Les étoiles de la fête et des feux d'artifice sont blanches (avant : blanches et dorées) |

### Version 2.33

| Demande | Réalisation |
|---|---|
| Sur le court Margaux, l'arbitre est une princesse : couronne, baguette à étoile à la main, habits roses | Princesse assise sur la chaise : couronne dorée, longs cheveux noirs, robe rose, baguette dorée avec une étoile au bout. Au repos, elle tient sa baguette levée devant elle. Sur une faute ou une balle dehors, elle tend le bras, baguette pointée. Sur un let et en fin de match, elle lève sa baguette. Elle suit la balle du regard comme l'arbitre habituel, et reste visible pendant les ralentis. Sur les autres courts, rien ne change. Le rose remplace le blanc de la palette de l'arbitre sur ce court : le reflet de la balle y devient rose pâle |

### Version 2.32

| Demande | Réalisation |
|---|---|
| Supprimer les bruits de crissement | Supprimés : un demi-tour brusque ne fait plus aucun son (le test vérifie que le carré 2 reste muet). Les bruits de pas discrets, le souffle du geste, le « pok » et les bips restent |

### Version 2.31

| Demande | Réalisation |
|---|---|
| Comme des ratés dans certaines musiques | Le rythme a été vérifié trame par trame (générique, titre, menu, valse, présentation) : aucune note en retard ni sautée. La cause était dans le timbre : depuis la 2.29, le carré de la mélodie n'était relancé qu'aux notes qui changent d'octet de période, d'où un petit clic sur certaines notes seulement. Chaque note est de nouveau relancée de la même façon, comme dans la polka déjà validée. Les silences qui détachent les notes répétées passent de 2 trames à 1 |

### Version 2.30

| Demande | Réalisation |
|---|---|
| « Iiik » du crissement plus discret | Volume 6 au lieu de 11 (sur 15) et 67 ms au lieu de 83 : audible sans dominer. Toujours le même « iiik » qui monte, mêmes conditions (demi-tour brusque sur dur, moquette ou glace, au plus un toutes les 2,5 s) |

### Version 2.29

| Demande | Réalisation |
|---|---|
| Vérifier toutes les musiques : certaines semblent saccadées (court Margaux, générique d'ouverture) | Cause trouvée : la fanfare d'ouverture, la valse de Margaux, l'interlude de fin de set, le thème de la finale et la fanfare de victoire jouaient leur mélodie avec un son qui s'éteint tout seul en 0,3 à 0,5 s. Les notes longues mouraient avant la suivante. Mesuré dans l'émulateur : mélodie presque muette 134 trames sur 470 dans la fanfare, 69 sur 480 dans la valse. Ces morceaux sont maintenant joués à volume constant (2 et 18 trames : seulement les silences écrits) ; les notes répétées sont détachées par un silence de 2 trames ; le petit clic au début de chaque note est supprimé. La polka et l'hymne du générique de fin étaient déjà corrects |
| Le crissement ne s'entend pas | Il était trop faible et trop court (volume 5, 33 ms). Il passe au volume 11 et à 83 ms, et existe aussi sur la glace. Il reste réservé aux surfaces où il est réaliste (dur, moquette, glace : pas sur terre, gazon ni sable) et aux demi-tours brusques gauche-droite, au plus un toutes les 2,5 s |
| Un bip quand le service est faux, comme pour une balle dehors | Chaque faute de service (dehors ou dans le filet) fait le bip du juge électronique, à la place de l'ancien bourdonnement grave |

### Version 2.28

| Demande | Réalisation |
|---|---|
| Animation des joueurs : toutes les améliorations proposées | **Attente** : léger rebond sur place (1 px toutes les 16 trames, décalé d'un joueur à l'autre). **Split-step** : quand l'adversaire frappe, le joueur à l'arrêt fait un petit saut puis se réceptionne appuis écartés. **Course en 4 temps** : chaque foulée est basse puis relevée d'1 px. **Service en 3 temps** : lancer, impact bras tendu tout en haut, accompagnement. **Revers à deux mains** quand la balle est à gauche (au lieu d'un coup droit en miroir) et **extension** (fente, bras tendu, tamis bas) quand la balle passe loin du joueur. Margaux a sa chevelure dans toutes les poses. Pour faire de la place (la table de sprites était pleine), les rangées de tuiles identiques d'une pose à l'autre sont partagées : 11 poses tiennent dans 116 tuiles au lieu de 216 pour 8. Le choix de la pose est écrit en assembleur : le jeu n'est pas plus lent qu'avant, en simple comme en double. Les ralentis montrent aussi les nouvelles poses |

### Version 2.27

| Demande | Réalisation |
|---|---|
| Bruitages : tout faire | « Pok » de la frappe avec un corps au triangle, plus grave quand la frappe est forte (le triangle est libre pendant le match). Priorités sur le canal de bruit : un rebond ne coupe plus l'ovation, une frappe ne coupe plus le « OOOH ». « Bip » du juge électronique sur une balle dehors. Souffle au geste de raquette. Crissement des chaussures sur dur et moquette lors d'un demi-tour brusque, au plus un toutes les 2,5 s (environ 8 par minute). Le test des commandes relatives couvre maintenant aussi B (lob et amortie) |

### Version 2.26

| Demande | Réalisation |
|---|---|
| Public qui suit la balle | Spectateurs redessinés, un par place et plus lisibles : de face, on voit le visage ; de dos, la chevelure. Toute la tribune tourne la tête quand la balle change de moitié du court. Une première version, à deux spectateurs par case, était illisible (rayures au club et en salle) |
| La ola | Après les beaux points et à la fin du match (stade, club, salle), une vague de bras levés descend les tribunes, puis chaque rangée reprend son dessin d'origine |
| Plus de réactions | « OOOH » sur une balle dehors de peu (moins de 5 px), soupir sur une double faute, applaudissements rythmés avant une balle de match, silence au lancer du service |
| HAUT / BAS selon la place à l'écran | Pour les coups, HAUT veut dire « vers l'adversaire » (à plat, lob) et BAS « vers soi » (croisé court, amortie), en bas comme en haut de l'écran ; les commandes ne s'inversent plus après un changement de côté. Le mode d'emploi intégré le rappelle |
| Spectateurs en salle et au club | Salle : gradins sur le côté extérieur, panneaux publicitaires côté court. Club : 4 bancs de spectateurs de chaque côté au lieu de 2. Les deux sont en noir, jaune et blanc, bien visibles. En double, le coût en calcul reste dans le budget (12 trames en dépassement sur 6 000) |

### Version 2.25

| Demande | Réalisation |
|---|---|
| En cas de fatigue, garder le ralentissement mais frapper plus fort et moins précis, pour conclure le point | Fatigue (`tired`, 0 à 10, de la 11e à la 20e frappe) : en plus du ralentissement, la frappe gagne jusqu'à +23 % de vitesse et + 5 d'écart. La frappe parfaite n'est pas concernée et reste précise. Réglage choisi sur 16 matchs CPU contre CPU par version : les échanges de 15 frappes et plus se terminent plus vite (18,6 frappes en moyenne au lieu de 22,0) et plus souvent sur un coup gagnant (31 % au lieu de 20 %). Avec très peu d'écart en plus, plus aucun de ces échanges ne finissait sur un coup gagnant ; avec + 10, ils finissaient presque tous sur des fautes. Pour faire de la place, `win_party` et la table des bonds de joie passent de la banque du moteur à la banque fixe |

### Version 2.24

| Demande | Réalisation |
|---|---|
| Services canon et coups parfaits un peu plus puissants | Service canon : vitesse 96 au lieu de 84, soit environ 50 % plus rapide qu'un service normal au lieu de 32 % (mesuré : 352 contre 237). Il reste sûr : 36 essais sur 36 dans le carré. Frappe parfaite : +37,5 % au lieu de +25 % (mesuré : +41 % face à une frappe normale presque identique) |

### Version 2.23

| Demande | Réalisation |
|---|---|
| Arbitre et chaise trop grands, perspective à revoir (proposition 3 : profil réduit) | Chaise et arbitre redessinés de profil à environ 60 % de la taille (28 px de haut au lieu de 47). La zone de couleur propre à la chaise passe de 48 à 32 px de haut |
| L'arbitre disparaît pendant les ralentis | Il est maintenant dessiné aussi pendant les ralentis |
| Arbitre centré par rapport au filet | Chaise et arbitre remontés de 16 px : leur milieu est à la hauteur du filet (à 1 px près, la couleur du décor se réglant par blocs de 16 px) |
| Il ne fait pas de gestes | En plus des gestes existants (bras tendu sur une faute ou une balle dehors, bras levé sur un let et à la fin du match, désormais raccordé à l'épaule), il suit la balle du regard, vers le haut ou vers le bas du court, à chaque échange |

### Version 2.22

| Demande | Réalisation |
|---|---|
| Vrai croisé court (A + BAS) | Le coup part en angle et tombe dans le carré de service près de la ligne de côté (profondeur 30 au lieu de 46, lift plus marqué). Il sert à sortir l'adversaire du court ou à passer un joueur au filet, avec un risque de sortie sur le côté. Mesuré avec le joueur robot : toutes les balles qui rebondissent tombent en angle dans le carré (14 sur 14), contre aucune avant. Il compte aussi pour la frappe parfaite |
| Frappe à plat (A + HAUT) plus tranchée | Plus rapide (54 au lieu de 46 en vitesse de base) et plus tendue (gravité 4 au lieu de 5), mais moins précise (écart + 7 au lieu de + 4). Vitesse mesurée : 121 contre 97 pour le lifté (105 avant). L'équilibre des matchs CPU contre CPU est inchangé (0,88 faute et 0,12 coup gagnant par point, avant comme après) |

### Version 2.21

| Demande | Réalisation |
|---|---|
| Chaise d'arbitre, version classique avec toutes les améliorations | Nouvelle chaise vue de profil, à gauche du filet, dans les 5 lieux : haut dossier, siège avec plaque, repose-pieds, deux pieds et une traverse. Elle est blanche à contours noirs (le vert était impossible avec les palettes de la NES), avec les mêmes couleurs de jour, de nuit et sous la pluie. L'arbitre, de profil, est tourné vers le court : bras tendu sur une faute ou une balle dehors, bras qui s'agite sur un let, bras levé qui salue à la fin du match |

### Version 2.20

| Demande | Réalisation |
|---|---|
| Générique de fin ridicule après une saison gagnée | Après le podium d'une saison gagnée, un générique défile comme au cinéma, sur un hymne solennel joué au kazoo : champion de la saison, Bruno, Augustin (« CHIEF EXECUTIVE SON »), Margaux en invitée d'honneur, postes absurdes (le stagiaire « toujours désolé pour le N », Isaac Newton bénévole, le frigo familial au traiteur), mentions légales, « THE END »… puis « ROYAL MARTINOT 2 — COMING SOON (PROBABLY NOT) » et un applaudissement timide. A accélère ; à la fin, A ramène au titre |

### Version 2.19

| Demande | Réalisation |
|---|---|
| Toujours des traits à chaque changement d'image du générique | Seconde cause, trouvée : à l'allumage de chaque écran, la palette était écrite pendant que l'image se dessinait. Écran éteint, la NES affiche alors les couleurs en cours d'écriture, d'où une image striée à chaque transition. La palette est maintenant écrite au début de l'intervalle libre. Mesuré : 160 écritures fautives pendant le générique avant, 0 après (test permanent). L'émulateur des tests ne montre pas cet effet, d'où l'oubli |

### Version 2.18

| Demande | Réalisation |
|---|---|
| Traits horizontaux pendant les transitions et en faisant défiler les modes | Cause : la console envoyait, dans la même image, la palette (fondus, changement de personnage) et le texte à réécrire. Cela dépassait le court moment où l'écriture à l'écran est permise, et le haut de l'image était abîmé. Maintenant, la palette attend l'image suivante (1/60 s) quand il y a beaucoup de texte, et elle est envoyée plus vite hors fondu. Marge mesurée : au moins 600 cycles partout. Le changement de mode au menu ne coupe plus l'écran (plus d'éclair noir) : les lignes sont redessinées en 1/10 s |
| Raté dans les premières notes du générique | La musique prenait 2 à 3 images de retard à chaque changement de scène (12 au total, le premier vers 2,8 s), car elle n'avançait pas pendant la construction de l'écran. Elle avance maintenant dans l'interruption d'image, à chaque image : toutes les notes tombent à l'image près. Tous les changements d'écran du jeu en profitent |
| 1 joueur contre CPU en double = mode En équipe | Le choix simple/double est retiré du mode 1 joueur contre CPU (toujours en simple). Il reste en mode 2 joueurs adversaires. Le double avec un partenaire CPU se joue en mode En équipe |

### Version 2.17

| Demande | Réalisation |
|---|---|
| Les joueurs en grand, sur un podium sur le court, le tout centré | Écran de victoire d'une compétition refait : podium centré sur le court de la compétition, les 3 premiers en taille double (agrandis pixel par pixel au vol), vainqueur au centre sur la plus haute marche, coupe géante, confettis, 5 feux d'artifice, noms sous les marches. En double, les 2 vainqueurs montent ensemble ; pour les autres équipes, seul le capitaine monte (la NES n'a que 4 palettes pour le décor), et le nom de l'équipe complète est écrit dessous |
| (correction) | La lecture des motifs en mémoire graphique était décalée d'un octet (instruction supprimée par le compilateur) : corrigée aussi dans le générique |

### Version 2.16

| Demande | Réalisation |
|---|---|
| Fanfare de victoire plus longue et plus joyeuse | Nouvelle fanfare de 160 trames, exactement la durée de la fête : arpèges qui montent sur do puis fa, montée finale et accord de do ; elle s'achève sur l'écran des résultats. Utilisée aussi à la fin du tutoriel et pour une compétition gagnée |
| Feux d'artifice pendant la pluie d'étoiles | Trois fusées : sifflement qui monte, « pop », puis gerbe de 4 étoiles qui s'écartent au-dessus du court. En double, la fête coûte au plus une image sur 120 (invisible) |
| Feux d'artifice aussi pour la victoire finale d'une compétition | Pendant la levée de coupe, 5 feux d'artifice (au lieu de 3 sur le court), plus bas pour laisser lire « VAINQUEUR ! », avec la nouvelle fanfare ; puis le calme jusqu'à A |

### Version 2.15

| Demande | Réalisation |
|---|---|
| Trous dans la musique du titre et du menu | Le « kazoo » de la polka avait une enveloppe qui décroît : sur les notes longues, le son s'éteignait presque avant la note suivante. Il est maintenant à volume constant ; mesuré sur 29 s enregistrées, aucune baisse du niveau à aucune trame, y compris au passage du titre au menu |
| Petite animation simple quand on gagne un match | Pendant « JEU, SET ET MATCH ! », le vainqueur (et son partenaire en double) sautille de joie, raquette levée, sous une pluie d'étoiles blanches et jaunes et de confettis. Rien quand le CPU bat un humain |

### Version 2.14

| Demande | Réalisation |
|---|---|
| Musique ridicule mais sans raté, qui tourne bien, la même à l'écran titre et au menu, un peu moins énergique mais très rythmée et amusante | Petite polka de cirque (`MUS_TITLE`) : mélodie « kazoo » sautillante et liée, broderies chromatiques comiques, « oum-pa » de la basse qui saute du grave à l'octave. Aucun silence ni bégaiement, boucle de 12,8 s sans couture, et la musique continue sans repartir du début entre l'écran titre et le menu. Elle remplace l'ancien thème du titre et la ritournelle agaçante d'un premier essai ; jamais de musique en match |

### Version 2.13

| Demande | Réalisation |
|---|---|
| Musique complètement ridicule et autres trucs décalés dans le générique | Fanfare qui déraille (fausses notes, accélération, glissando, silence gêné, « pouet »), barre « LOADING PRESTIGE » qui monte à 117 % et déborde, mentions légales absurdes (« NO TENNIS BALLS WERE HARMED », « RATED A FOR AUGUSTIN », sponsor : le frigo familial), ovation réduite à un seul « *CLAP* », balle qui fait tomber le N du logo remis par le stagiaire (« OOPS. SORRY. », « (PRESTIGE RESTORED) »), bouton « passer » vexé (« ARE YOU SURE? THIS IS VERY PRESTIGIOUS. » puis « FINE. ») |

### Version 2.12

| Demande | Réalisation |
|---|---|
| Animation de lancement très prétentieuse, avec musique et plein de choses superficielles | Générique de 20 secondes sur une fanfare composée pour l'occasion : « BRUNO MARTINOT PRESENTS », « IN ASSOCIATION WITH AUGUSTIN MARTINOT, CHIEF EXECUTIVE SON », « A ROYAL MARTINOT PRODUCTION, FILMED IN GLORIOUS 8-BIT TECHNICOLOR », critiques cinq étoiles signées par la famille, prix « BEST GAME 2026 (CATEGORY: OUR LIVING ROOM) », puis révélation du logo lettre par lettre avec « boum », or scintillant, étoiles, confettis, ovation et « LIMITED EDITION : 1 COPY ». N'importe quel bouton le passe |

### Version 2.11

| Demande | Réalisation |
|---|---|
| Nouveau nom : sympa, un peu drôle, excessivement prétentieux, style marque de luxe, faux anglais | **ROYAL MARTINOT** — *Grand Slam Prestige Platinum Diamond Edition*. Logo géant sur deux lignes à l'écran titre, sous-titres « GRAND SLAM PRESTIGE » et « PLATINUM DIAMOND EDITION », « (C) 2026 BRUNO MARTINOT ». Nom repris sur la présentation des matchs, les écrans de compétition, la ROM (`dist/royal_martinot.nes`) et toute la documentation. Les nouvelles lettres du logo occupent les dernières tuiles libres du décor |

### Version 2.10

| Demande | Réalisation |
|---|---|
| Passer à la cartouche de 256 Ko | MMC1 (SNROM) à 16 banques de 16 Ko, la dernière fixe. Les codes de reprise quittent la banque fixe pour la banque 5 : 2,5 Ko libérés dans la banque fixe, et 8 banques (128 Ko) entièrement libres pour la suite. Toujours compatible avec les cartouches flash MMC1 |
| Panneau Margaux plus petit, coloré, avec du volume, le plus haut possible, à côté du score | Petit panneau de 32 × 16 px dans le bandeau, juste à droite du score de gauche : face magenta, lettres blanches, liseré rose clair, rebord et pieds. Il remplace la grande planche blanche |
| Étoile de la frappe parfaite | Placée au bout de la raquette, entièrement hors du sprite du joueur, et affichée 12 images au lieu de 8 : bien visible pour le joueur du bas comme pour celui du haut, sur toutes les surfaces |
| Panneau du club nudiste | Même traitement : petit panneau « BRONZES » (club « Les Bronzés ») jaune soleil à lettres noires, à côté du score, à la place du texte « CLUB NUDISTE » du bandeau |
| Filet : les 5 améliorations | Bande blanche de 2 px, mailles aérées, ombre portée sur le court, poteaux avec reflet et tête claire, sangle centrale ; le filet tremble quand la balle le touche |
| « CHANGEMENT DE CÔTÉ » centré | Toutes les annonces du bandeau sont centrées |

### Version 2.9

| Demande | Réalisation |
|---|---|
| Joueur du haut qui chevauche le bandeau des annonces au service | Quand sa tête est entièrement dans le bandeau, elle passe derrière le texte : les annonces restent lisibles, la tête reste visible entre les lettres. Terrain et position de service inchangés |

### Version 2.8

| Demande | Réalisation |
|---|---|
| 1. Rotation des sprites, seulement quand nécessaire | Activée uniquement en double quand au moins 3 joueurs partagent une bande de 24 px : un sprite en trop clignote au lieu de disparaître. Les ombres sont maintenant affichées aussi en double. Le test a révélé un débordement de la table des sprites (blocage), corrigé |
| 2. Affichage des personnages en assembleur | Images d'échange en retard en double : 27 sur 6 000 (0,45 %), contre 90 avant ; aucune en simple. Rendu identique |
| 4. Retour sur le timing à l'entraînement | « TROP TOT DE 3 IMAGES », « TROP TARD DE 1 IMAGE », « RELACHEZ LE BOUTON AVANT », « BALLE TROP PRES DU CORPS », « COUP PARFAIT ! », « SERVICE CANON » |
| 5. Ralentis des coups parfaits | Une frappe parfaite gagnante est rejouée, même en retour de service (les services canon gagnants le sont déjà, comme aces) |
| 6. Fondus entre les écrans | Fondu au noir (3 crans) et retour progressif (4 crans) à chaque changement d'écran, musique continue. Pas de fondu quand le même écran est reconstruit (menu, pages), pour garder une navigation instantanée |
| Petite musique sur l'écran de fin de set, et le fermer rapidement | Interlude calme (8 mesures en boucle, coupé à la reprise, jamais de musique en match). N'importe quel bouton des deux manettes ferme l'écran sans fondu : retour au court en 4 images |
| Récapitulatif du score entre chaque jeu, chaque set et à la fin du match | Après chaque jeu : score set par set dans le bandeau (« ALEX 6 3 / MAYA 4 1 »). Après chaque set : écran récapitulatif (tableau des sets, vainqueur du set, statistiques, joueur au service), A pour reprendre. En fin de match : tableau complet des sets en tête des résultats |

### Version 2.7

| Demande | Réalisation |
|---|---|
| L'équivalent du service canon pour les coups classiques | **Frappe parfaite** : coup lifté, à plat ou coupé, balle à côté du corps, et bouton relâché juste avant son arrivée (fenêtre de **3 images**, vérifiée par un test qui relâche à chaque image). Balle environ 25 % plus rapide, presque sans écart, « tchak » sec et éclat blanc près de la raquette ; bandeau « COUP PARFAIT ! » seulement si le coup est gagnant, pour ne pas surcharger l'écran. CPU Expert de temps en temps, Normal rarement, Facile jamais. Taux attendu pour un adulte : 15 à 20 % au début, 30 à 40 % avec l'habitude (estimation). Pourcentage au palmarès, nombre par match aux résultats. Mode d'emploi intégré mis à jour |

### Version 2.6

| Demande | Réalisation |
|---|---|
| Service particulièrement réussi et puissant (option 1 : timing parfait), pour un adulte classique | **Service canon** : A frappé pile au sommet du lancer, dans une fenêtre de **2 images** (1/30 s, vérifiée par un test qui frappe à chaque image du lancer). Balle environ 30 % plus rapide, presque sans écart, bandeau « SERVICE CANON », « boum » et traînée. Deuxième balle un peu plus risquée. Le CPU Expert en réussit parfois (environ 12 % de ses premières balles), Facile et Normal jamais. Taux attendu pour un adulte : environ 25 à 30 % au début, 35 à 40 % après une heure ou deux (estimation). Compteur au palmarès (nombre et % des services A) et dans les statistiques du match, pour vérifier en jouant |

### Version 2.5

| Demande | Réalisation |
|---|---|
| Joueuse « Margaux » | 17e personnage, style FANTAISIE (amorties, lobs, montées au filet), tenue rose clair, **cheveux longs, blonds et bouclés**. Ses têtes sont dessinées à part (de face, de dos, de profil, pendant les frappes et le service), le corps étant celui des joueuses. Cheveux blonds $28, blond clair sur la plage pour rester visibles sur le sable. Margaux ne va jamais au club nudiste et n'est jamais tirée au sort comme adversaire. Disponible partout : matchs, double, compétitions (aussi en partenaire) et codes de reprise. Les codes passent à un format qui accepte 17 personnages, sur 13 signes comme avant ; les anciens codes restent valables |
| Vigilance sur les couleurs | Le rose clair est contrôlé contre tous les courts (ΔE ≥ 42, y compris le court rose) et contre toutes les tenues. Trois tenues trop proches ont changé : IVAN gris (au lieu de lilas, ΔE 12,6), TOM crème (au lieu de lavande, ΔE 15,2), INES chocolat (au lieu de pervenche, ΔE 23). `tools/contrast.py` vérifie aussi les tenues entre elles et fait désormais partie de `make test` (0 paire sous ΔE 25). Abréviation « MX » pour Margaux, pour ne pas la confondre avec MAYA (« MA ») dans les panneaux de score |
| Fatigue légère, désactivable | Ligne FATIGUE (OUI par défaut). Au-delà de 10 frappes dans un échange, tous les joueurs, humains et CPU, ralentissent peu à peu jusqu'à environ 20 % ; une goutte de sueur clignote au-dessus d'eux. Tout repart à zéro au point suivant. Mesure : 2 800 → 2 200 unités de déplacement en 20 trames après 20 frappes |
| Liste de vérification pour la vraie console | `docs/VERIFICATION_CONSOLE.md` : matériel, puis environ 60 points à cocher (démarrage, menus, match, couleurs par lieu, double et manette 2, options, codes et sauvegarde, PAL/NTSC, endurance) et ce qu'il faut renvoyer en cas de problème |

### Version 2.4 (finitions)

| Demande | Réalisation |
|---|---|
| 1. Annonce tronquée au club nudiste | « TOUT NU, TOUT ACE » (17 caractères) devient « ACE AU NATUREL », et « EN TOUTE NUDITE » devient « TOUT EN NUDITE ». La relecture automatique (point 12) a trouvé 7 autres messages trop longs, tous corrigés |
| 2. Smash sauté absent des ralentis | Le saut est enregistré dans les bits libres de la pose (tampon inchangé, 16 octets par trame) et rejoué à l'identique |
| 3. Le CPU Expert ne sautait jamais | Sur un lob peu profond, il recule sous la trajectoire et smashe en sautant environ une fois sur deux (17 lobs sur 30 en mesure) ; jamais en Facile ni en Normal |
| 4. À-coups en double | Les deux IA d'une équipe décident en alternance, le tri des sprites est plus rapide, des multiplications sont remplacées par des décalages. Images en retard pendant les échanges : de 11 % à 1,5 % |
| 5. Ombres des joueurs | Ombre sous les pieds, qui reste au sol pendant le saut. Toujours en simple ; en double, seulement pendant un saut (limite de 8 sprites par ligne) |
| 6. Poussière et éclaboussures | Petit nuage au rebond sur la terre battue et le sable, éclaboussure sous la pluie (12 trames, deux tailles) |
| 7. Arbitre de chaise (stade) | Assis sur sa chaise, il tourne la tête vers la moitié du court où se trouve la balle |
| 8. Réglage du son | Ligne SON : musique + sons, bruitages seuls, aucun |
| 9. Rebond selon la surface | Bruit propre à chaque surface : terre sourde, gazon doux, dur sec, moquette mate, sable feutré, glace claquante |
| 10. Repère d'atterrissage | En vitesse Lent : petite croix au point de chute pendant une demi-seconde, pour les balles destinées à un joueur humain |
| 11. Réglages mémorisés | Les réglages du menu sont enregistrés à chaque partie et retrouvés à l'allumage (cartouche à pile). Ils sont dans un bloc séparé : le palmarès existant est conservé |
| 12. Relecture des textes et captures | Nouvel outil `tools/check_texts.py`, lancé par `make test` : caractères absents de la police, lignes trop longues, annonces de plus de 14 caractères. Il a trouvé l'**apostrophe absente de la police** (« MODE D'EMPLOI » s'affichait « MODE D EMPLOI »), désormais dessinée, et 7 messages tronqués (entraînement, tutoriel). Captures de 23 écrans relues : personnage du menu qui masquait la ligne SON (déplacé), lieu « Court Margaux » ajouté au mode d'emploi |

### Version 2.3

| Demande | Réalisation |
|---|---|
| Coups gagnants signalés, sans surcharger l'image ni le son | Le message existant « COUP GAGNANT ! » nomme maintenant le coup : SMASH SAUTÉ, SMASH, AMORTIE, LOB GAGNANT, VOLÉE GAGNANTE, PASSING (adversaire au filet) ou COUP GAGNANT. Il s'y ajoute un « ting » bref sur le canal des bruitages. Rien de plus à l'écran (même bandeau, même durée), et le public réagit comme avant. Au club nudiste, l'arbitre ajoute une remarque pour chacun |
| Smash sauté | Une balle très haute (40 à 62 px), avant le rebond et hors du fond du court, se smashe en sautant : le joueur s'élève de 12 px et retombe, la frappe est plus rapide et plus tendue. Réservé aux joueurs humains et au CPU Expert, pour que le lob reste efficace contre Facile et Normal |
| Réglage de la vitesse du jeu | Ligne VITESSE JEU dans tous les modes : Lent (75 %), Normal, Rapide (125 %). En lent, aucun appui n'est perdu. En rapide, la trame ajoutée ne redessine pas : en simple, l'image reste fluide (2 % de trames en retard) ; en double, de légers à-coups (10 %) |
| Nouvelles musiques, toujours sans musique en match | Valse « boîte à musique » du court Margaux (menu quand le lieu est choisi, puis présentation) ; marche solennelle avant chaque finale de compétition (écran d'étape et présentation). Silence dès le début du match, vérifié par les tests |

### Version 2.2

| Demande | Réalisation |
|---|---|
| Joueurs plus rapides | +17 % pour les humains (1,88 → 2,19 px/trame) et +9 à +15 % pour l'IA selon le niveau, pour garder l'équilibre |
| Lancer de service plus haut, sans sortir de l'écran ni changer le terrain | Sommet du lancer à ~56 px au lieu de ~47. Pour le serveur du bas, il est affiché à sa hauteur réelle (environ 2,4 fois plus haut à l'écran qu'avant) ; pour celui du haut, la hauteur affichée est réduite pour que la balle reste à l'écran (point le plus haut à 12 px du bord). Terrain inchangé |
| Mode d'emploi, page 1 : un mot repoussé sur la ligne suivante | Trois lignes dépassaient 28 caractères ; page reformatée, et toutes les lignes du mode d'emploi vérifiées |
| Raquettes plus grosses et visibles | Tamis ovale 5 × 7 avec cadre et cordage, dans les 8 poses (tenues normales et au naturel) |
| Balle dont la taille suit la hauteur | 3 tailles (4, 5 et 7 px) selon la hauteur ; l'ombre reste au sol |
| Balle ratée bloquée en haut ou en bas de l'écran | La balle ratée poursuit sa trajectoire et disparaît une fois sortie de l'écran ; une balle très haute n'est plus collée au bord supérieur (seule son ombre reste visible). Arbitrage inchangé |
| Serveur du haut trop loin de la ligne de fond | Pieds 2 px derrière la ligne (au lieu de 8) : le sprite se dessine au-dessus des pieds, le recul paraissait plus grand en haut. Le serveur du bas est inchangé |
| Affichage des jeux confus (« 0-3 » lu comme un score entre les deux joueurs) | Panneau repensé : « J 3 » pour les jeux du set, un rond plein par set gagné à côté des points, `*` pour le serveur |
| Normal un peu trop difficile | Difficulté recalibrée avec un robot « humain » (`tests/calib.mjs`) : Normal plus lent, plus tardif, moins puissant et plus fautif ; IA moins parfaite dans son anticipation ; écart Normal → Expert adouci (le robot passe de 0 % à ~30 % des points contre Expert) |
| Mode « 2 J. équipe » proposé en simple | Incohérence corrigée : ce mode est toujours un double (ligne JEU masquée), et JOUEUR 2 désigne le personnage de la manette 2 (et non plus l'adversaire) |
| En équipe : partenaire au choix | Mode renommé « En équipe » ; ligne « JOUE PAR : CPU / MANETTE 2 » pour le partenaire |
| Bruits de pas discrets | Un bruit sourd très faible à chaque foulée, coupé dès qu'un rebond, une frappe ou le public utilise le canal |
| Joueurs de profil en déplacement | Nouvelle course vue de côté (2 images, hommes/femmes, aussi au club nudiste), orientée selon le sens du déplacement |
| Jeu « encore très difficile » | IA Facile et Normal nettement assouplies (plus lentes, plus tardives, plus fautives, anticipation moins précise) ; Expert un peu adouci ; zone de frappe plus généreuse pour tous |
| Personnages qui se confondent avec le court (sans liseré) | Couleurs des 6 surfaces, des 16 tenues et assombrissement de nuit/pluie recalculés avec `tools/contrast.py` : plus aucune paire personnage/court sous ΔE 25 (plusieurs étaient identiques, ΔE 0) |
| Court rose « Margaux » au décor girly | Nouveau lieu « Court Margaux » disponible d'emblée : court rose, abords prune, cœurs, fleurs et étoiles qui scintillent, panneau au nom de Margaux. Couleurs contrôlées (aucune paire sous ΔE 25) |
| Balle peu visible sur le sable jaune de la plage | Balle blanche sur la plage uniquement (ΔE 73 au lieu de 30) ; jaune pâle ailleurs (ΔE ≥ 51 partout, vérifié par `tools/contrast.py`) |
| Amortie peu réaliste | Depuis le fond, l'amortie partait comme un lob géant (110 px de haut, 1,25 s de vol, balle hors de l'écran). Désormais : trajectoire basse et vive (~20 px, ~0,7 s), premier rebond 15 à 22 px derrière le filet, rebond très bas (2-3 px), balle freinée (~7 px avant le 2e rebond) |
| Amortie : point garanti | Risque selon la distance au filet : imprécision accrue et profondeur réduite quand on la joue de loin, balle un peu moins « morte ». Mesure depuis le fond : 7 points gagnés, 6 balles renvoyées, 2 fautes sur 13 amorties |

### Version 2.1

| Demande | Réalisation |
|---|---|
| Compétitions en double | Les 5 compétitions se jouent en simple ou en double, avec un partenaire joué par l'ordinateur **ou par la manette 2** (deux joueurs humains dans la même équipe). Partenaire choisi par le joueur ; 16 personnages au lieu de 10, donc 8 équipes toutes différentes ; code de reprise sur 13 signes (double + partenaire) |
| Levée de coupe | À la victoire d'une compétition, le joueur (et son partenaire) brandit la coupe sous les confettis |
| Mode nudiste à débloquer | Gagner une compétition (ou saisir un code avec un trophée) débloque le lieu « Club nudiste Les Bronzés » : <ul><li>joueurs au naturel, sans aucun détail anatomique, avec une serviette aux couleurs de l'équipe ;</li><li>public au naturel debout dans l'herbe, en pied, sans gradins ;</li><li>annonces humoristiques de l'arbitre.</li></ul> |

**Écart volontaire au CDC § 5.3** (« pas de déblocage ») : le club nudiste est un bonus demandé explicitement. Tout le contenu du CDC reste disponible dès le départ.

### Version 2.0

| Demande | Réalisation |
|---|---|
| Service difficile pour le joueur du haut (balle « immobile ») | Le lancer sortait du haut de l'écran et son affichage restait bloqué. Sa hauteur est maintenant affichée à moitié : la balle monte et redescend visiblement. Couvert par un test automatique |
| Mode d'emploi dans le jeu | 8 pages accessibles depuis le titre : commandes, coups, service, règles, double, modes, compétitions et codes, conseils |
| Plage : sable confondu avec les jambes | Sable passé du beige ($37, identique à une teinte de peau) au jaune ($28) |
| Tout le contenu manquant du CDC | Voir § 3 |

Pour loger ce contenu, la cartouche passe de NROM (32 Ko) à **MMC1** : 128 Ko de PRG, CHR-RAM et 8 Ko de PRG-RAM. Voir `TECHNIQUE.md` § 2.

## 2. Résultats des tests

| Suite | Résultat |
|---|---|
| Règles (`rules_test`) | 42/42 |
| Codes (`password_test`) | 200 000/200 000 aller-retour (formats v3 et v0, le Grand Martinot), ancien code v2 accepté, Margaux codée, Margaux au club nudiste refusée ; 99,98 % des codes altérés d'un signe refusés |
| Émulateur (`emu_test`) | 335/335 vérifications (environ 4 min sur 4 processus en parallèle, `make test`) |
| Profil CPU, simple, 6 000 trames | 0 trame d'échange en dépassement |
| Relecture des textes (`check_texts.py`) | 0 problème |
| Contraste (`contrast.py 25`) | 0 paire sous ΔE 25 (personnages, courts, balle, tenues) |
| Profil CPU, double, 6 000 trames | 27 trames d'échange en dépassement (0,45 %), sans défaut d'image |
| Compilation | Reproductible : SHA-256 `eefb381d45b13deeb4b76e49cfaeaeb185f8a67532e8db2e4d81c607b797b533` sur deux compilations propres |

## 3. Conformité par section du CDC

Légende : ✅ réalisé et testé en émulation · 🟡 partiel · 🔧 nécessite du matériel

| § | Exigence | État | Commentaire |
|---|---|---|---|
| 1.3 | Création originale | ✅ | Code, graphismes générés, musiques, nom |
| 2.1 | Compatibilité NES/Famicom | 🔧 | MMC1 standard, programme de démarrage dans chaque banque ; **non vérifiée sur console** |
| 2.2 | Contraintes matérielles | ✅ | ≤ 8 sprites par ligne (pluie multiplexée), VRAM ≤ 96 octets par VBlank, budget CPU mesuré |
| 2.3 | Étude des cartouches | ✅ | Comparatif, choix MMC1 justifié |
| 2.4 | NTSC/PAL | 🟡 | Détection + compensation 50 Hz ; testée en forçant le mode (jsnes ne fait que NTSC) |
| 3.1–3.6 | Déplacements, 11 types de frappe, contrôles, effets, service, physique | ✅ | Let et balle qui frôle la bande ajoutés |
| 3.7 | Pas d'endurance | ✅ | |
| 4.1 | Règles modernes, formats configurables | ✅ | |
| 4.2 | Formats de compétition (simple et double) | ✅ | Match amical, simple, double, élimination directe, plusieurs niveaux (progressif), poules, toutes rondes (championnat), saison, personnalisé, exhibition (démo), défis (exercices), entraînement |
| 4.3 | Multijoueur local | ✅ | 1 joueur, 2 adversaires, 2 en équipe en double (en match et en compétition), équipes CPU, revanche immédiate |
| 5.1–5.3 | Personnages | ✅ | 16 personnages (8 hommes, 8 femmes), tous disponibles ; seul le lieu bonus « club nudiste » est à débloquer |
| 5.2 | Animations | ✅ | Course, frappe, service, joie, **déception** |
| 6.1 | Surfaces | ✅ | Terre, gazon, dur, moquette + **plage** et **glace** (fantaisie) |
| 6.2 | Environnements | ✅ | **Stade, club extérieur, salle** ; jour/nuit ; + club nudiste (bonus) |
| 6.3 | Météo cohérente | ✅ | Pluie, sauf en salle |
| 6.4 | Influence des surfaces | ✅ | Restitution et adhérence propres à chacune des 6 surfaces |
| 7.3 | Public | ✅ | Réactions sonores + **animation visuelle** (spectateurs debout, bras levés, en CHR-RAM) |
| 7.5 | Présentations, victoire/défaite, ralentis | ✅ | Écran de présentation avant chaque match ; joie et déception à la fin ; levée de coupe ; **ralentis** des aces, coups gagnants et longs échanges (désactivables, passables, jamais pendant un échange) |
| 8.1–8.5 | Menus, sélection, réglages rapides, pause, affichage | ✅ | Menu adapté au mode ; pause CONTINUER/QUITTER ; score hors de la zone de jeu |
| 8.6 | Tutoriel des commandes | ✅ | Mode d'emploi intégré + tutoriel interactif guidé |
| 9 | Entraînement (10 points du CDC) | ✅ | Adversaire immobile, cibles, zones, personnalisable, service, frappes, déplacements, placement, précision, régularité |
| 10.1–10.3 | Niveaux, styles, tactique | ✅ | 3 niveaux × 8 styles propres aux personnages |
| 10.4 | IA adaptative | ✅ | Placement et lobs selon les habitudes observées du joueur ; borné, remis à zéro à chaque match |
| 10.5 | Coordination en double | ✅ | |
| 11 | Audio | ✅ | APU natif, pas de musique pendant les échanges, annonces visuelles et sifflet (pas de voix, facultative) |
| 12.1–12.2 | Statistiques, classements, palmarès, historique | ✅ | Statistiques de match ; palmarès cumulé (simple/double, points, aces…, titres, records, historique des 8 dernières compétitions, meilleur résultat par type, records d'entraînement) ; classements des championnats et saisons |
| 12.3 | Codes de sauvegarde | ✅ | 12 signes, contrôle sur 12 bits |
| 12.3 | Sauvegarde matérielle | ✅ (documentée) | Palmarès en PRG-RAM ; conservé à l'extinction **uniquement** si la cartouche a une pile. Les codes restent la sauvegarde principale |
| 13 | Architecture, optimisation | ✅ | Banques, trampolines, profileur par banque |
| 15 | Tests | 🟡 | Complets en émulation ; **aucun sur matériel** |
| 17 | Livrables | ✅ | ROM, sources, outils, manuel (fichier + intégré), documentation technique, rapport |

## 4. Fonctions dépendant d'une extension

- **Persistance du palmarès à l'extinction** : nécessite une cartouche MMC1 avec PRG-RAM sauvegardée par pile. Sans pile, tout le jeu fonctionne ; seul le palmarès est perdu à l'extinction. Les codes de reprise ne dépendent d'aucun matériel.
- Aucune extension audio n'est utilisée.

## 5. Limitations restantes

1. **Tests sur matériel réel** (priorité 1 du CDC). Il faut flasher `dist/royal_martinot.nes` sur une cartouche MMC1/SNROM (ou un linéaire EverDrive/PowerPak) et tester sur NES NTSC, NES PAL et Famicom. Points à surveiller :
   - l'état du MMC1 à la mise sous tension ;
   - le temps de la NMI ;
   - Dendy ;
   - le rognage de l'image selon les téléviseurs.

   Il faut aussi passer la ROM dans Mesen et Nestopia : seul jsnes a été utilisé.
2. **Dépassements CPU** : 0,2 % des trames d'échange en double (frappes, début de ralenti) et quelques trames de transition entre les points. Pas d'impact sur les commandes.
3. **Équilibrage** : calibré avec un robot, pas avec un panel de joueurs ; à confirmer en jouant.
4. **Tenues en double** : les partenaires portent la palette de leur équipe (4 palettes de sprites).
5. **Espace** : la banque du moteur (0) est remplie à 15 576 / 16 368 octets, après le déplacement du calcul de trajectoire vers la banque fixe. Toute évolution du moteur passera par la banque des annexes (3) ou par la banque libre (6).
