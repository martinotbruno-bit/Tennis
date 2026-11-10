# Royal Martinot — Rapport de réalisation (version 2.23)

Comme l'exige le CDC (§ 17.5), ce rapport distingue ce qui est **réalisé**, **testé**, **validé sur matériel réel**, **validé uniquement en émulation**, **dépendant d'une extension**, et les **limitations restantes**.

> **Validation matérielle : aucune.** Aucun test n'a été fait sur console réelle (NES, Famicom, PAL, cartouche MMC1). Toutes les validations ont eu lieu en émulation (jsnes) et par des tests unitaires sur PC. Les critères « vérifié sur console physique » (CDC § 2.1, 15.4, 16.A) **ne sont pas remplis**. Ce point ne peut pas être traité dans l'environnement de développement utilisé.

## 1. Évolutions

### Version 2.23

| Demande | Réalisation |
|---|---|
| Arbitre et chaise trop grands, perspective à revoir (proposition 3 : profil réduit) | Chaise et arbitre redessinés de profil à environ 60 % de la taille (28 px de haut au lieu de 47). La zone de couleur propre à la chaise passe de 48 à 32 px de haut |
| L'arbitre disparaît pendant les ralentis | Il est maintenant dessiné aussi pendant les ralentis |
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
| Codes (`password_test`) | 200 000/200 000 aller-retour (format v3), ancien code v2 accepté, Margaux codée, Margaux au club nudiste refusée ; 99,98 % des codes altérés d'un signe refusés |
| Émulateur (`emu_test`) | 216/216 vérifications (environ 21 min) |
| Profil CPU, simple, 6 000 trames | 0 trame d'échange en dépassement |
| Relecture des textes (`check_texts.py`) | 0 problème |
| Contraste (`contrast.py 25`) | 0 paire sous ΔE 25 (personnages, courts, balle, tenues) |
| Profil CPU, double, 6 000 trames | 27 trames d'échange en dépassement (0,45 %), sans défaut d'image |
| Compilation | Reproductible : SHA-256 `85a3003cfa36db90bd8c0715259d796ce799bf5c6b3083ff97ff03550bd6acca` sur deux compilations propres |

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
