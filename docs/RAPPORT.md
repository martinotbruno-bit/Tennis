# Pixel Open — Rapport de réalisation (version 1.0)

Ce rapport distingue, comme l'exige le CDC (§ 17.5), ce qui est **réalisé**, **testé**, **validé sur matériel réel**, **validé uniquement en émulation**, **dépendant d'une extension** et ce qui **reste à faire**.

> **Validation matérielle : aucune.** Aucun test n'a été effectué sur console réelle (NES, Famicom, PAL, cartouche). Toutes les validations ont été faites en émulation (jsnes) et par des tests unitaires sur PC. Les critères d'acceptation « vérifié sur console physique » du CDC (§ 2.1, 15.4, 16.A) ne sont donc **pas encore remplis**.

## 1. Synthèse

| Domaine | État |
|---|---|
| ROM fonctionnelle, sources, compilation reproductible | Fait |
| Gameplay simple et double, règles complètes | Fait, testé en émulation |
| IA 3 niveaux, coordination en double | Fait, testé en émulation |
| 4 surfaces, jour/nuit/pluie, 6 personnages | Fait |
| Tournoi, entraînement (2 exercices), démo | Fait, testé en émulation |
| Statistiques de match, codes de sauvegarde | Fait, testé |
| NTSC/PAL | Détection et compensation faites ; PAL testé uniquement en simulation logicielle |
| Tests sur matériel réel | **Non fait** |
| Ralentis/replays, réactions visuelles du public, IA adaptative, terrains fantaisie, classements cumulés | **Non fait** (voir § 4) |

## 2. Résultats des tests (version 1.0)

| Suite | Résultat |
|---|---|
| Règles (`rules_test`) | 42/42 |
| Codes (`password_test`) | 13 824/13 824 aller-retour ; 99,98 % des codes altérés refusés ; lettres invalides refusées |
| Émulateur (`emu_test`) | 62/62 vérifications (environ 6 min) |
| Profil CPU, double CPU, 8 000 trames | 4 trames d'échange en dépassement (0,05 %) |
| Profil CPU, simple, 8 000 trames | 0 trame d'échange en dépassement |
| Matchs CPU complets observés | 4 surfaces × 3 niveaux, simple et double, sans blocage ; échanges jusqu'à 53 frappes |
| Compilation | Reproductible (SHA-256 identique sur deux compilations propres) |

## 3. Conformité par section du CDC

Légende : ✅ réalisé et testé en émulation · 🟡 partiel · ❌ non réalisé · 🔧 hors de portée de l'environnement (matériel)

| § | Exigence | État | Commentaire |
|---|---|---|---|
| 1.3 | Création originale | ✅ | Code, graphismes (générés par `tools/gen_assets.py`), musiques et nom originaux |
| 2.1 | Compatibilité NES/Famicom | 🟡🔧 | Mapper 0 standard, aucun accessoire ; **non vérifiée sur console** |
| 2.2 | Respect des contraintes matérielles | ✅ | ≤ 8 sprites par ligne, palettes, VRAM écrite en VBlank uniquement, budget CPU mesuré |
| 2.3 | Étude des cartouches | ✅ | Comparatif dans TECHNIQUE.md § 2 ; NROM-256 retenu |
| 2.4 | NTSC / PAL | 🟡 | Détection + compensation 50 Hz ; testée en forçant le mode (jsnes est NTSC uniquement) |
| 2.5 | Manettes standard | ✅ | 2 manettes NES/Famicom |
| 3.1 | Déplacements libres (8 directions) | ✅ | |
| 3.2 | Coup droit, revers, volée, smash, lob, amortie, passing, puissance, placement, défense, réflexe | ✅ | Le type dépend des boutons, de la direction, de la hauteur et du rebond ; coup droit et revers sont visuels |
| 3.3 | Direction et puissance (maintien du bouton) | ✅ | |
| 3.4 | Effets lifté / coupé, trajectoires | ✅ | Effet réel sur la gravité et le rebond |
| 3.5 | Service complet | ✅ | Lancer, timing, direction, 1re/2e balle, faute, double faute ; « let » non géré (filet = faute) |
| 3.6 | Physique | ✅ | Gravité, rebond, filet, limites, in/out |
| 3.7 | Pas d'endurance, personnages = apparence | ✅ | |
| 4.1 | Règles modernes, configurables | ✅ | Jeu décisif, changements de côté, ordre des serveurs en double |
| 4.2 | Formats | 🟡 | Match simple/double, exhibition (démo), tournoi à élimination (3 tours), entraînement, défis (cibles). **Absents** : phase de groupes, toutes rondes, championnat, saison, tournoi personnalisé |
| 4.3 | Multijoueur local | ✅ | 1P/CPU, 2P adversaires, 2P en équipe (double), équipes CPU ; revanche immédiate |
| 5.1–5.3 | 2 à 4 personnages, hommes/femmes, tout débloqué | ✅ | 6 personnages ; tenue par équipe en double (limite : 4 palettes de sprites) |
| 5.2 | Animations | 🟡 | Course, frappe (2 temps), service, joie ; pas d'animation de déception dédiée |
| 6.1 | Surfaces | 🟡 | Terre, gazon, dur, moquette ; **pas de terrain fantaisie** |
| 6.2 | Environnements | 🟡 | Stade avec tribunes et chaise d'arbitre, jour/nuit ; un seul décor |
| 6.3 | Météo (pluie) | ✅ | Gouttes (multiplexées) et rebonds modifiés ; pas de vent (conforme) |
| 6.4 | Influence des surfaces | ✅ | Restitution et adhérence par surface |
| 7.3 | Réactions du public | 🟡 | Sonores (ace, coup gagnant, smash, long échange, fautes) ; **pas d'animation visuelle** |
| 7.5 | Présentations, ralentis, replays | ❌ | Écran de tournoi et annonces seulement ; **pas de replay** |
| 8 | Menus, sélection, pause, affichage discret | ✅ | Score dans les coins hors de la zone de jeu |
| 8.6 | Tutoriel des commandes | 🟡 | Aide sur l'écran de configuration et manuel ; pas de tutoriel interactif |
| 9 | Entraînement | 🟡 | Lanceur de balles avec cibles et score, travail du service. **Absents** : exercices personnalisables, déplacements, régularité |
| 10.1–10.3 | IA à niveaux, styles, tactique | ✅ | Écarts de décisions et de précision, lobs, amorties, montées au filet |
| 10.2 | Styles de jeu distincts | 🟡 | Les styles dépendent du niveau ; pas encore de profil « défenseur » / « attaquant » par adversaire |
| 10.4 | IA adaptative | ❌ | À étudier |
| 10.5 | Coordination en double | ✅ | Responsable de balle, couverture de la moitié, joueur au filet |
| 11.1–11.2 | Audio natif, pas de musique en échange | ✅ | |
| 11.3 | Annonces | ✅ | Visuelles + sifflet ; pas de voix (facultatif) |
| 11.4 | Extensions audio | ✅ (étude) | Non utilisées, conformément au CDC |
| 12.1 | Statistiques de match | ✅ | Points, aces, doubles fautes, coups gagnants, fautes, % 1res balles, plus long échange |
| 12.1–12.2 | Statistiques cumulées, classements, palmarès, historique | 🟡 | Seul le nombre de trophées est conservé (dans le code) |
| 12.3 | Codes de sauvegarde | ✅ | 6 lettres, somme de contrôle, détection d'erreurs |
| 13 | Architecture, optimisation CPU/PPU | ✅ | Mesurée par le profileur |
| 13.5 | Qualité adaptative déterministe | ✅ | Pluie multiplexée ; trame de frappe sans redessin ; IA étalée |
| 15 | Tests | 🟡 | Automatisés en émulation ; **pas de test sur matériel** |
| 17 | Livrables | ✅ | ROM, sources, scripts, manuel, documentation technique, rapport |

## 4. Limitations et suite proposée

1. **Tests sur matériel réel** (priorité 1 du CDC). Flasher `dist/pixel_open.nes` sur une cartouche (EverDrive, PowerPak, NROM reproduite) et tester sur NES NTSC, NES PAL et Famicom. Points à surveiller :
   - le temps de la NMI ;
   - le comportement de la détection de région sur Dendy ;
   - le rognage de l'image selon les téléviseurs.

   Il faut aussi passer la ROM dans des émulateurs de référence (Mesen, Nestopia) : seul jsnes a été utilisé.
2. **Espace ROM** : il reste environ 4,3 Ko de PRG. Il faudra passer à UNROM ou MMC1 pour ajouter les éléments suivants :
   - ralentis/replays (enregistrement de la trajectoire dans un tampon circulaire) ;
   - championnat ou saison ;
   - terrains fantaisie ;
   - animations du public.
3. **Formats manquants** : phase de groupes, toutes rondes, championnat, saison, tournoi personnalisé.
4. **IA adaptative** : mémoriser la direction préférée du joueur et ses montées au filet, puis ajuster le placement de l'IA.
5. **Faute « let »** au service (balle qui touche le filet puis tombe dans le carré) : actuellement comptée comme une faute uniquement si la balle ne franchit pas le filet.
6. **Équilibrage** : en CPU contre CPU, la plupart des points se terminent sur une faute ; les aces sont rares. Un joueur robot parfait bat le niveau expert : l'IA expert peut être rendue plus agressive.
7. **Dépassements CPU** : 0,05 % des trames d'échange en double ; quelques trames de transition entre les points.
8. **Tenues en double** : les partenaires partagent la palette de leur équipe (limite matérielle de 4 palettes de sprites).
