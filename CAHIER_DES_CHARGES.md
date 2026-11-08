# CAHIER DES CHARGES PROFESSIONNEL
## Création d'un jeu de tennis NES/Famicom de nouvelle génération

**Nom du projet :** Pixel Open  
**Nature du projet :** Création autonome d'un jeu vidéo de tennis 8 bits pour Nintendo Entertainment System (NES) et Famicom.  
**Destinataire :** IA de développement autonome.  
**Objectif :** Concevoir, programmer, optimiser, tester et livrer un jeu de tennis 8 bits original et ambitieux, dans la tradition des jeux de tennis de la NES, avec un contenu considérablement enrichi et une finition maximale, tout en conservant une compatibilité réelle avec le matériel d'origine.

| Version | Date | Statut |
|---|---|---|
| 1.0 | 2026-11-08 | Version initiale |
| 1.1 | 2026-11-08 | Nom du projet : Pixel Open |

## Sommaire

1. [Vision et objectifs du projet](#1-vision-et-objectifs-du-projet)
2. [Plateformes et contraintes matérielles](#2-plateformes-et-contraintes-matérielles)
3. [Gameplay et mécaniques de tennis](#3-gameplay-et-mécaniques-de-tennis)
4. [Règles et formats de compétition](#4-règles-et-formats-de-compétition)
5. [Personnages et apparence](#5-personnages-et-apparence)
6. [Terrains et environnements](#6-terrains-et-environnements)
7. [Graphismes et animations](#7-graphismes-et-animations)
8. [Interface et menus](#8-interface-et-menus)
9. [Modes d'entraînement](#9-modes-dentraînement)
10. [Intelligence artificielle](#10-intelligence-artificielle)
11. [Audio et musique](#11-audio-et-musique)
12. [Statistiques, résultats et sauvegardes](#12-statistiques-résultats-et-sauvegardes)
13. [Architecture logicielle et optimisation](#13-architecture-logicielle-et-optimisation)
14. [Méthode de développement autonome](#14-méthode-de-développement-autonome)
15. [Tests et validation](#15-tests-et-validation)
16. [Critères d'acceptation](#16-critères-dacceptation)
17. [Livrables obligatoires](#17-livrables-obligatoires)
18. [Priorités en cas de compromis](#18-priorités-en-cas-de-compromis)
19. [Directive finale à l'IA de développement](#19-directive-finale-à-lia-de-développement)

---

# 1. VISION ET OBJECTIFS DU PROJET

## 1.1 Objectif principal

Développer un jeu de tennis original, inspiré des jeux de tennis classiques de la NES, mais considérablement plus riche dans tous les domaines possibles :

- Gameplay et profondeur des échanges.
- Physique de la balle.
- Déplacements et animations des joueurs.
- Intelligence artificielle.
- Modes de jeu.
- Terrains et environnements.
- Graphismes et effets visuels.
- Sons et musiques.
- Menus et ergonomie.
- Multijoueur local.
- Statistiques et compétitions.
- Optimisation technique et stabilité.

Le résultat doit conserver l'identité d'un jeu de tennis 8 bits, tout en offrant une expérience beaucoup plus complète, moderne dans sa conception et particulièrement soignée.

Il ne s'agit pas de produire une simple démonstration technique ou un prototype. Le résultat attendu est un véritable jeu finalisé.

## 1.2 Principes fondamentaux

Les objectifs suivants sont prioritaires :

1. Compatibilité avec les consoles NES et Famicom d'origine.
2. Gameplay précis, réactif et agréable.
3. Fluidité maximale possible sur le matériel standard.
4. Richesse du contenu et diversité des modes.
5. Qualité graphique et sonore maximale dans les limites techniques.
6. Intelligence artificielle complète et crédible.
7. Stabilité pendant les parties et les longues sessions.
8. Livraison d'une ROM fonctionnelle, des sources et de la documentation.

En cas de conflit entre plusieurs objectifs, l'IA devra mesurer les conséquences techniques et rechercher le meilleur équilibre global.

Aucune amélioration graphique ne devra rendre les commandes imprécises ou dégrader les fonctions essentielles du jeu.

## 1.3 Création originale

Le jeu doit être une réalisation indépendante inspirée des principes de gameplay des jeux de tennis 8 bits.

Ne pas réutiliser le code source original, les graphismes propriétaires, les musiques, les sons ou les ressources protégées du jeu d'origine sans autorisation.

Créer des graphismes, animations, musiques, effets sonores, interfaces et ressources propres au nouveau jeu.

---

# 2. PLATEFORMES ET CONTRAINTES MATÉRIELLES

## 2.1 Compatibilité obligatoire

Le jeu doit être conçu pour fonctionner sur :

- Nintendo Entertainment System (NES) d'origine.
- Nintendo Famicom d'origine.
- Consoles compatibles utilisant des adaptateurs courants.
- Émulateurs NES et Famicom.

La compatibilité matérielle doit être vérifiée sur des consoles physiques, et non simplement supposée à partir d'une compilation réussie ou d'un fonctionnement en émulation.

## 2.2 Matériel standard

La version principale doit fonctionner sans modification de la console et sans accessoire indispensable.

L'IA doit respecter les caractéristiques et contraintes du matériel :

- Processeur Ricoh 2A03/2A07 selon la région.
- PPU et fonctionnement vidéo de la NES.
- APU audio natif.
- Mémoire vive disponible.
- Mémoire vidéo et mémoire graphique.
- Limites de sprites par ligne.
- Nombre de palettes disponibles.
- Contraintes de synchronisation CPU/PPU.
- Particularités des systèmes NTSC et PAL.

L'architecture de cartouche devra être sélectionnée après une étude technique comparative.

## 2.3 Cartouche haut de gamme

Étudier les possibilités des cartouches avancées et des différents mappers compatibles.

Objectifs :

- Maximiser la mémoire disponible.
- Optimiser l'organisation des banques graphiques.
- Permettre des animations plus riches.
- Améliorer la gestion du contenu.
- Étudier les possibilités d'extension audio.
- Sélectionner une architecture adaptée à la production d'une cartouche physique.

La capacité mémoire ne doit pas être considérée comme illimitée. Chaque choix devra être justifié par sa compatibilité réelle avec les consoles visées.

Toute fonction dépendant d'une extension matérielle devra être identifiée séparément. Le jeu doit rester fonctionnel sans cette extension.

## 2.4 Gestion NTSC et PAL

Prendre en charge les deux grandes familles de fonctionnement.

L'IA devra :

- Détecter ou identifier la région lorsque cela est possible.
- Adapter la synchronisation du jeu.
- Maintenir une vitesse de jeu cohérente.
- Éviter les différences injustifiées de vitesse entre régions.
- Tester les timings, animations, commandes et sons dans les deux configurations.

Ne pas imposer artificiellement 60 images par seconde sur un système PAL qui ne possède pas cette fréquence vidéo.

La priorité est d'obtenir la meilleure fluidité réellement possible sur chaque système.

## 2.5 Contrôleurs

Prise en charge obligatoire des manettes NES/Famicom standard.

Étudier également les variantes de contrôleurs compatibles.

Le fonctionnement du jeu ne doit pas dépendre d'un accessoire particulier.

---

# 3. GAMEPLAY ET MÉCANIQUES DE TENNIS

## 3.1 Déplacements

Le joueur doit pouvoir se déplacer librement sur l'ensemble de sa moitié de terrain.

Le système de déplacement doit permettre :

- Déplacements horizontaux.
- Déplacements verticaux.
- Déplacements diagonaux.
- Course et ajustement de position.
- Déplacements vers le filet.
- Recul défensif.
- Placement avant la frappe.
- Déplacements adaptés à la réception de la balle.

Les commandes doivent être précises et immédiatement compréhensibles.

## 3.2 Système de frappe

Le gameplay doit proposer une véritable profondeur de jeu.

Les frappes doivent notamment comprendre :

- Coup droit.
- Revers.
- Volée.
- Smash.
- Lob.
- Amortie.
- Passing-shot.
- Frappe puissante.
- Frappe placée.
- Frappe défensive.
- Frappe réflexe.

Le système devra déterminer le type de frappe selon la position, le timing, la situation et les commandes du joueur.

## 3.3 Contrôles

La direction de la frappe doit pouvoir être influencée par la croix directionnelle.

La puissance doit pouvoir être modulée par le maintien d'un bouton.

Le système doit proposer :

- Contrôle de direction.
- Contrôle de puissance.
- Choix ou détection intelligente du type de frappe.
- Différences de timing.
- Gestion des frappes en déplacement.
- Possibilités de frappe offensive et défensive.

Les commandes devront être documentées clairement dans le manuel.

## 3.4 Effets de balle

Implémenter les effets de balle techniquement réalisables :

- Effet lifté.
- Effet coupé.
- Trajectoires plus ou moins tendues.
- Variations de hauteur.
- Variations de vitesse.
- Trajectoires croisées.
- Trajectoires longues et courtes.

Ces effets devront avoir une influence réelle sur la trajectoire et le rebond, et pas seulement sur l'apparence visuelle.

## 3.5 Service

Créer un système de service complet :

- Choix de la direction.
- Contrôle de puissance.
- Lancer de balle.
- Timing de frappe.
- Première balle.
- Deuxième balle.
- Fautes de service.
- Double faute.
- Service puissant.
- Service placé.
- Variations tactiques.

Les règles du service devront respecter les règles de tennis sélectionnées.

## 3.6 Physique de la balle

La physique constitue l'un des systèmes centraux du projet.

Elle doit gérer :

- Position et vitesse.
- Accélération éventuelle.
- Gravité simulée.
- Hauteur de balle.
- Rebond.
- Vitesse après rebond.
- Effets appliqués.
- Collision avec les limites du terrain.
- Collision avec le filet.
- Collision avec les raquettes.
- Détermination des balles bonnes et fautes.

La balle doit avoir une trajectoire visuellement fluide et des rebonds cohérents.

La simulation doit être suffisamment précise pour permettre des échanges variés et tactiques, sans dépasser les capacités de calcul de la console.

## 3.7 Endurance et caractéristiques des joueurs

Aucun système de fatigue ou d'endurance ne doit être nécessaire.

Les personnages jouables doivent principalement se distinguer par leur apparence.

Les différences de comportement des adversaires contrôlés par l'IA peuvent en revanche dépendre de leur niveau de difficulté et de leur stratégie, sans imposer des statistiques individuelles aux personnages.

---

# 4. RÈGLES ET FORMATS DE COMPÉTITION

## 4.1 Règles

Utiliser les règles modernes du tennis comme référence.

Prendre en charge :

- Points.
- Jeux.
- Sets.
- Matchs.
- Avantage.
- Égalité.
- Tie-break.
- Fautes.
- Double faute.
- Balle bonne ou faute.
- Service alterné.
- Changement de côté.
- Ordre des serveurs.
- Règles du double.

Les règles doivent être cohérentes et appliquées par tous les modes concernés.

Les formats de match doivent être configurables : nombre de sets, règles de départage et durée.

## 4.2 Formats de jeu

Prévoir les formats suivants :

- Match amical.
- Match simple.
- Match double.
- Tournoi à élimination directe.
- Tournoi à plusieurs niveaux.
- Phase de groupes.
- Compétition toutes rondes.
- Championnat.
- Saison.
- Tournoi personnalisé.
- Match d'exhibition.
- Défis et mini-jeux.
- Entraînement.

## 4.3 Multijoueur

Le multijoueur local à deux joueurs humains est une priorité majeure.

Prévoir :

- Un joueur humain contre un adversaire contrôlé par l'IA.
- Deux joueurs humains en simple.
- Doubles avec différentes combinaisons humain/IA.
- Deux joueurs humains dans la même équipe.
- Deux joueurs humains adversaires.
- Équipes composées de personnages contrôlés par l'IA.

Permettre une configuration complète avant chaque match.

Proposer un rematch instantané après une partie.

Le nombre de personnages présents sur le court peut atteindre quatre, sans imposer la prise en charge obligatoire de quatre joueurs humains.

---

# 5. PERSONNAGES ET APPARENCE

## 5.1 Personnages

Prévoir entre deux et quatre personnages présents sur le terrain selon le format.

Les joueurs doivent pouvoir choisir des personnages masculins et féminins.

Les apparences doivent être variées.

## 5.2 Graphismes des joueurs

Créer une identité visuelle 8 bits soignée.

Étudier :

- Apparence fidèle à l'esprit des jeux de tennis 8 bits, avec une identité visuelle propre.
- Sprites 8 bits détaillés.
- Plusieurs tenues.
- Variantes de couleurs.
- Animations de course.
- Animations de préparation.
- Animations de frappe.
- Animations de service.
- Animations de célébration.
- Réactions de déception.

Les animations doivent être riches, mais leur coût technique devra être contrôlé.

## 5.3 Disponibilité du contenu

Tous les personnages et contenus prévus doivent être accessibles dès le début.

Ne pas imposer de progression ou de déblocage obligatoire.

---

# 6. TERRAINS ET ENVIRONNEMENTS

## 6.1 Nombre et variété

Créer le plus grand nombre de terrains possible dans les limites du matériel.

Prévoir notamment :

- Terre battue.
- Gazon.
- Terrain dur.
- Moquette.
- Terrains fantaisie.

## 6.2 Environnements

Créer plusieurs environnements :

- Stade classique.
- Grand stade avec tribunes.
- Terrain extérieur.
- Terrain intérieur.
- Match de jour.
- Match de nuit.
- Décors variés.

Chaque terrain doit posséder une identité visuelle propre.

## 6.3 Conditions météorologiques

Prévoir des conditions météorologiques variables, notamment la pluie.

La météo doit être cohérente avec le type de terrain et l'environnement.

Les effets visuels de pluie doivent être étudiés en fonction du coût en sprites, en cycles CPU et en ressources graphiques.

Ne pas introduire de vent comme mécanique obligatoire.

## 6.4 Influence des surfaces

Les surfaces doivent pouvoir influencer la physique :

- Vitesse de déplacement de la balle.
- Hauteur du rebond.
- Comportement après rebond.
- Vitesse des échanges.
- Stratégies de jeu.

Les différences doivent être perceptibles et testables.

---

# 7. GRAPHISMES ET ANIMATIONS

## 7.1 Ambition graphique

Exploiter au maximum les possibilités graphiques de la NES.

L'objectif est d'obtenir un jeu visuellement très riche pour une console 8 bits.

## 7.2 Terrain

Prévoir tous les éléments compatibles :

- Lignes de jeu.
- Filet détaillé.
- Tribunes.
- Public.
- Arbitres.
- Éléments de décor.
- Variations d'environnement.
- Animations d'ambiance.

## 7.3 Public

Le public doit pouvoir réagir aux événements du match :

- Point gagné.
- Point perdu.
- Échange long.
- Smash.
- Faute.
- Balle décisive.
- Fin de match.

Les réactions doivent varier visuellement et sonorement lorsque les ressources le permettent.

## 7.4 Balle

Prévoir :

- Déplacement visuel fluide.
- Trajectoire lisible.
- Rebond animé.
- Changements de hauteur cohérents.

Les effets visuels supplémentaires ne devront pas nuire à la lisibilité de la balle.

## 7.5 Présentations et ralentis

Prévoir :

- Présentation avant match.
- Présentation des joueurs.
- Présentation des terrains.
- Animations de victoire.
- Animations de défaite.
- Ralentis et replays après les points importants.

Les ralentis ne doivent jamais interrompre ou ralentir les commandes pendant un échange actif.

Les replays doivent être facultatifs ou intégrés dans les temps morts.

---

# 8. INTERFACE ET MENUS

## 8.1 Menu principal

Créer un menu principal très élaboré avec présentation visuelle des terrains et des joueurs.

Prévoir des transitions soignées et une navigation intuitive.

## 8.2 Sélection

Créer un écran complet permettant de choisir :

- Joueurs.
- Personnages.
- Équipes.
- Terrains.
- Conditions de jeu.
- Mode de jeu.

## 8.3 Configuration des matchs

Proposer des paramètres prédéfinis faciles à sélectionner.

Les choix doivent permettre de lancer rapidement une partie sans devoir modifier systématiquement de nombreux paramètres.

## 8.4 Pause

Créer des menus de pause adaptés aux différents modes.

Les options disponibles doivent être cohérentes avec le type de partie.

## 8.5 Affichage en match

L'affichage doit rester lisible et discret.

Informations principales :

- Score.
- Jeux remportés.

Ne pas surcharger l'écran de statistiques pendant les échanges.

Les statistiques détaillées doivent être accessibles après le match ou dans les menus.

## 8.6 Accessibilité et ergonomie

Prévoir :

- Commandes explicites.
- Menus intuitifs.
- Navigation simple.
- Informations contextuelles utiles.
- Tutoriel simple des commandes.

L'ensemble doit rester adapté à la manette NES.

---

# 9. MODES D'ENTRAÎNEMENT

Créer un véritable espace d'entraînement.

Prévoir :

1. Entraînement contre un adversaire immobile.
2. Entraînement avec cibles.
3. Zones de précision.
4. Exercices personnalisables.
5. Travail du service.
6. Travail des frappes.
7. Travail des déplacements.
8. Exercices de placement.
9. Défis de précision.
10. Défis de régularité.

Les exercices doivent être suffisamment variés pour améliorer réellement la maîtrise du jeu.

---

# 10. INTELLIGENCE ARTIFICIELLE

## 10.1 Niveaux de difficulté

Créer plusieurs niveaux de difficulté clairement différenciés.

L'écart doit provenir de décisions et de comportements de jeu, et non uniquement d'une augmentation artificielle de la vitesse.

## 10.2 Styles de jeu

Prévoir différents comportements :

- Jeu offensif.
- Jeu défensif.
- Jeu de fond de court.
- Jeu au filet.
- Jeu de placement.
- Jeu de puissance.
- Jeu tactique.
- Jeu équilibré.

## 10.3 Intelligence tactique

L'IA doit pouvoir :

- Lire la trajectoire de la balle.
- Anticiper les rebonds.
- Se positionner.
- Choisir une frappe.
- Adapter sa puissance.
- Exploiter les espaces.
- Varier ses trajectoires.
- Défendre et attaquer.
- Gérer les situations difficiles.
- Commettre certaines erreurs en fonction du niveau.

## 10.4 Adaptation

Étudier une IA capable d'adapter progressivement son comportement au style du joueur humain.

Cette adaptation doit rester équilibrée et ne pas produire de comportement injuste ou artificiel.

## 10.5 Doubles

L'IA doit gérer le placement et la coordination de deux personnages.

Éviter les collisions inutiles et les situations où les deux partenaires tentent systématiquement de jouer la même balle.

---

# 11. AUDIO ET MUSIQUE

## 11.1 Audio natif

Exploiter les capacités audio natives de la NES.

Créer une identité sonore originale.

Prévoir :

- Musiques de menu.
- Sons de transition.
- Sons de navigation.
- Bruitages de course.
- Impacts de balle sur raquette.
- Impacts différenciés selon les frappes.
- Rebonds.
- Sifflets d'arbitrage.
- Réactions du public.
- Sons de victoire et de défaite.

## 11.2 Audio pendant les matchs

Aucune musique de fond pendant les échanges.

Les sons du jeu doivent renforcer la perception des actions et du rythme du match.

## 11.3 Annonces

Prévoir des annonces relatives :

- Au score.
- Aux fautes.
- Aux services.
- À la fin du match.

Utiliser des signaux sonores et des indications visuelles compatibles avec le matériel.

Les annonces vocales éventuelles ne devront être étudiées que si leur qualité et leur coût technique sont acceptables. Elles ne sont pas une condition de fonctionnement du jeu.

## 11.4 Extensions audio

Étudier les extensions audio de certaines cartouches.

Ne pas rendre une extension audio indispensable à l'expérience principale.

---

# 12. STATISTIQUES, RÉSULTATS ET SAUVEGARDES

## 12.1 Statistiques

Créer des statistiques détaillées par joueur et par compétition.

Prévoir notamment :

- Matchs joués.
- Matchs gagnés.
- Matchs perdus.
- Points gagnés.
- Jeux gagnés.
- Sets gagnés.
- Aces.
- Doubles fautes.
- Fautes directes.
- Coups gagnants.
- Pourcentage de premières balles.
- Résultats en simple et en double.
- Historique des compétitions.

Les statistiques doivent être accessibles dans les menus et après les matchs.

## 12.2 Classements et palmarès

Prévoir :

- Classements locaux.
- Palmarès.
- Résultats des tournois.
- Historique des compétitions.
- Statistiques cumulées.

## 12.3 Sauvegardes

Le système principal de sauvegarde doit utiliser des codes saisis manuellement.

Les codes doivent pouvoir contenir les informations nécessaires à la restauration des données compatibles avec la capacité mémoire disponible.

Prévoir :

- Vérification des codes.
- Détection des erreurs de saisie.
- Protection contre les codes invalides.
- Génération de nouveaux codes.
- Instructions simples pour les utiliser.

La taille des données devra être optimisée.

Ne pas supposer qu'une NES standard dispose d'une mémoire de sauvegarde permanente. Toute solution matérielle supplémentaire devra être identifiée et documentée.

## 12.4 Profils

Aucun système de profils joueurs permanents n'est demandé.

Les statistiques et résultats peuvent être conservés dans le système de sauvegarde sans imposer de profils complexes.

---

# 13. ARCHITECTURE LOGICIELLE ET OPTIMISATION

## 13.1 Architecture préalable

Avant de programmer, l'IA doit concevoir une architecture technique complète.

Cette architecture devra couvrir :

- Gestion des entrées.
- Déplacements.
- Physique de balle.
- Collisions.
- Règles du tennis.
- IA.
- Modes de jeu.
- Gestion des terrains.
- Graphismes.
- Animations.
- Audio.
- Menus.
- Sauvegardes.
- Synchronisation NTSC/PAL.

L'architecture pourra être adaptée pendant le développement si les tests démontrent qu'une autre approche est meilleure.

## 13.2 Optimisation CPU

Optimiser :

- Les calculs de trajectoire.
- Les collisions.
- Les déplacements.
- L'IA.
- Les mises à jour d'animation.
- Les traitements de menus.
- Les interruptions et synchronisations.

L'IA doit mesurer les coûts CPU et éviter les traitements inutiles pendant les phases de jeu.

## 13.3 Optimisation PPU

Respecter les contraintes graphiques natives.

Étudier :

- L'organisation des sprites.
- Les limites de sprites par ligne.
- Les palettes.
- Les banques graphiques.
- Les changements de banques.
- Les possibilités de scrolling.
- Les contraintes de rendu.
- La stabilité de l'affichage.

## 13.4 Gestion des ressources

Mettre en place une gestion rigoureuse de la mémoire.

Chaque ressource devra être dimensionnée et justifiée.

Éviter les duplications inutiles de données.

## 13.5 Qualité adaptative

Lorsque les ressources deviennent insuffisantes, l'IA devra privilégier les fonctions essentielles.

Les effets secondaires pourront être simplifiés ou supprimés automatiquement selon une stratégie préalablement définie.

Les commandes, la précision des frappes, les règles et la stabilité ne devront pas être sacrifiées au profit des effets visuels.

Le comportement adaptatif devra être déterministe et reproductible, afin d'éviter des différences imprévisibles entre deux parties.

---

# 14. MÉTHODE DE DÉVELOPPEMENT AUTONOME

## 14.1 Autonomie complète

L'IA doit gérer le projet du début à la fin.

Elle doit prendre les décisions techniques nécessaires, organiser son travail et résoudre les problèmes sans attendre une validation humaine à chaque étape.

## 14.2 Étapes de réalisation

L'ordre exact pourra être choisi par l'IA, mais le processus devra au minimum comporter :

1. Analyse des contraintes matérielles.
2. Étude des architectures de cartouche.
3. Conception technique.
4. Définition des systèmes de jeu.
5. Développement du moteur de jeu.
6. Développement du gameplay.
7. Développement des modes.
8. Développement de l'IA.
9. Création des graphismes et animations.
10. Création des sons et musiques.
11. Création des menus.
12. Intégration des statistiques et sauvegardes.
13. Optimisation.
14. Tests automatisés.
15. Tests sur émulateurs.
16. Tests sur matériel réel.
17. Corrections.
18. Compilation finale.
19. Vérification des livrables.
20. Documentation.

## 14.3 Gestion des erreurs

À chaque erreur détectée, l'IA doit :

- Identifier la cause.
- Évaluer les conséquences.
- Corriger le problème.
- Reconstruire le projet.
- Exécuter les tests concernés.
- Exécuter les tests de régression.
- Vérifier l'absence d'effets secondaires.
- Documenter la correction.

Ne pas masquer les erreurs ou déclarer une fonction terminée sans vérification.

## 14.4 Code professionnel

Le code doit être :

- Structuré.
- Modulaire.
- Documenté.
- Optimisé.
- Maintenable.
- Reproductible.
- Dépourvu de dépendances inutiles.

Les commentaires techniques doivent expliquer les optimisations spécifiques au matériel NES.

---

# 15. TESTS ET VALIDATION

## 15.1 Tests fonctionnels

Tester toutes les fonctions :

- Menus.
- Navigation.
- Sélection des personnages.
- Sélection des terrains.
- Configuration des matchs.
- Commandes.
- Frappes.
- Services.
- Règles.
- Modes.
- Tournois.
- Doubles.
- Entraînement.
- IA.
- Statistiques.
- Sauvegardes.
- Pause.
- Rematch.

## 15.2 Tests de gameplay

Vérifier :

- Réactivité des commandes.
- Précision des directions.
- Cohérence des frappes.
- Trajectoires.
- Rebond.
- Collisions.
- Comportement du filet.
- Équilibre des niveaux d'IA.
- Fonctionnement des doubles.
- Application des règles.
- Lisibilité de la balle.

## 15.3 Tests de stabilité

Tester :

- Longues sessions.
- Matchs complets.
- Tournois successifs.
- Changements de mode.
- Retours aux menus.
- Redémarrages.
- Sauvegardes et restaurations.
- Scénarios inhabituels.
- Cas limites.
- Risques de blocage.

## 15.4 Tests techniques

Tester sur :

- Émulateurs pertinents.
- NES physique.
- Famicom physique.
- Systèmes NTSC.
- Systèmes PAL.
- Cartouches et adaptateurs correspondant à l'architecture retenue.

Contrôler :

- Performances CPU.
- Stabilité vidéo.
- Synchronisation.
- Audio.
- Mémoire.
- Compatibilité.
- Absence de ralentissements anormaux.
- Absence de corruption de données.

## 15.5 Tests de régression

Après chaque correction importante, relancer les tests pertinents.

Avant livraison, exécuter une campagne complète de validation.

---

# 16. CRITÈRES D'ACCEPTATION

Le projet ne pourra être déclaré terminé que lorsque les critères suivants seront satisfaits.

### A. Compatibilité

- ROM fonctionnelle sur les configurations matérielles annoncées.
- Compatibilité NES/Famicom vérifiée.
- Fonctionnement NTSC et PAL vérifié.
- Absence de dépendance obligatoire à une extension non standard.

### B. Gameplay

- Commandes réactives.
- Déplacements fonctionnels.
- Système de frappe complet.
- Physique de balle cohérente.
- Règles correctement appliquées.
- Matchs jouables jusqu'à leur conclusion.
- Modes simples et doubles fonctionnels.

### C. Contenu

- Tous les modes annoncés terminés.
- Tous les terrains prévus intégrés.
- Personnages et apparences finalisés.
- Entraînement fonctionnel.
- IA opérationnelle à tous les niveaux prévus.
- Menus et interfaces terminés.
- Statistiques et sauvegardes opérationnelles.

### D. Qualité

- Graphismes finalisés.
- Animations finalisées.
- Audio finalisé.
- Aucun ralentissement anormal dans les scénarios de référence.
- Aucun blocage connu dans les scénarios validés.
- Aucun défaut critique connu.

### E. Livraison

- ROM finale vérifiée.
- Sources complètes.
- Documentation disponible.
- Compilation reproductible.
- Tests documentés.
- Limitations connues clairement indiquées.

Une fonctionnalité annoncée ne doit pas être laissée sous forme de prototype, de simple placeholder ou de menu sans contenu opérationnel.

Si une fonctionnalité se révèle techniquement impossible sur le matériel standard, l'IA devra le signaler explicitement, documenter la raison et proposer la meilleure alternative réalisable.

---

# 17. LIVRABLES OBLIGATOIRES

## 17.1 Jeu final

Fournir :

- ROM finale prête à lancer.
- Format de ROM adapté à l'architecture retenue.
- Variantes de ROM utiles à l'émulation ou à la cartouche physique, si nécessaires.
- Identification claire de la version finale.

Le projet doit aboutir à une seule version finale principale, et non à plusieurs éditions concurrentes.

## 17.2 Code source

Fournir :

- Code source complet.
- Organisation des fichiers.
- Ressources originales.
- Scripts de compilation.
- Paramètres de compilation.
- Outils nécessaires à la reconstruction.

## 17.3 Documentation utilisateur

Créer un manuel complet contenant :

- Présentation du jeu.
- Commandes.
- Règles.
- Modes de jeu.
- Terrains.
- Personnages.
- Entraînement.
- Fonctionnement du multijoueur.
- Statistiques.
- Sauvegardes.
- Conseils d'utilisation.

## 17.4 Documentation technique

Fournir également :

- Architecture logicielle.
- Choix du mapper.
- Organisation mémoire.
- Organisation graphique.
- Gestion audio.
- Gestion NTSC/PAL.
- Contraintes matérielles.
- Méthodes d'optimisation.
- Instructions de compilation.
- Résultats des tests.
- Limitations connues.

## 17.5 Rapport final

Le rapport devra distinguer clairement :

- Fonctionnalités réalisées.
- Fonctionnalités testées.
- Fonctionnalités validées sur matériel réel.
- Fonctionnalités validées uniquement en émulation.
- Fonctions nécessitant une extension.
- Limitations restantes.

Ne jamais présenter une fonctionnalité comme validée sur console réelle si elle n'a pas effectivement été testée sur ce matériel.

---

# 18. PRIORITÉS EN CAS DE COMPROMIS

Lorsque toutes les ambitions ne peuvent pas être réalisées simultanément, appliquer les principes suivants :

**Priorité 1 :** compatibilité matérielle et stabilité.

**Priorité 2 :** réactivité et précision du gameplay.

**Priorité 3 :** cohérence de la physique et des règles.

**Priorité 4 :** fonctionnement complet des modes et fonctionnalités.

**Priorité 5 :** qualité des animations, graphismes et environnements.

**Priorité 6 :** effets visuels secondaires et enrichissements facultatifs.

Ces priorités ne signifient pas qu'il faut négliger les éléments graphiques. Elles définissent les arbitrages à effectuer lorsque les limites matérielles imposent un choix.

L'IA devra mesurer les performances avant de décider de réduire une fonction.

---

# 19. DIRECTIVE FINALE À L'IA DE DÉVELOPPEMENT

Tu es responsable de la réalisation complète de ce projet.

Tu dois agir comme une équipe professionnelle réunissant les compétences suivantes :

- Développeur spécialisé en programmation NES.
- Ingénieur en architecture de cartouches et mappers.
- Programmeur gameplay.
- Spécialiste de la physique de jeu.
- Développeur d'intelligence artificielle.
- Graphiste pixel art.
- Animateur 2D.
- Compositeur et concepteur sonore 8 bits.
- Ingénieur optimisation.
- Testeur logiciel.
- Responsable qualité.

Tu dois prendre les décisions techniques nécessaires sans demander systématiquement l'avis de l'utilisateur.

Tu dois étudier les contraintes réelles du matériel avant de définir les fonctionnalités et leur implémentation.

Tu dois construire un jeu complet, et non une démonstration.

Tu dois exploiter au maximum les capacités de la NES et de la Famicom d'origine, sans prétendre que leurs ressources sont illimitées.

Tu dois optimiser chaque système, mesurer les performances et corriger les défauts détectés.

Tu dois garantir que les fonctions essentielles restent disponibles sur le matériel standard.

Tu dois documenter toute limitation technique et proposer la meilleure alternative possible.

Tu ne dois jamais déclarer le projet terminé tant que les critères d'acceptation et les livrables obligatoires ne sont pas vérifiés.

**Résultat final attendu : un jeu de tennis 8 bits exceptionnel, considérablement plus riche que les jeux de tennis 8 bits classiques en contenu, en gameplay et en finition, exploitant au maximum les possibilités techniques de la NES/Famicom, tout en restant réellement compatible avec les consoles d'origine.**

**FIN DU CAHIER DES CHARGES**