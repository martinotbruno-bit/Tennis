# Pixel Open — Rapport de réalisation (version 2.1)

Comme l'exige le CDC (§ 17.5), ce rapport distingue ce qui est **réalisé**, **testé**, **validé sur matériel réel**, **validé uniquement en émulation**, **dépendant d'une extension**, et les **limitations restantes**.

> **Validation matérielle : aucune.** Aucun test n'a été fait sur console réelle (NES, Famicom, PAL, cartouche MMC1). Toutes les validations ont eu lieu en émulation (jsnes) et par des tests unitaires sur PC. Les critères « vérifié sur console physique » (CDC § 2.1, 15.4, 16.A) **ne sont pas remplis**. Ce point ne peut pas être traité dans l'environnement de développement utilisé.

## 1. Évolutions

### Version 2.1

| Demande | Réalisation |
|---|---|
| Compétitions en double | Les 5 compétitions se jouent en simple ou en double. Partenaire choisi par le joueur ; 16 personnages au lieu de 10, donc 8 équipes toutes différentes ; code de reprise sur 13 signes (double + partenaire) |
| Levée de coupe | À la victoire d'une compétition, le joueur (et son partenaire) brandit la coupe sous les confettis |
| Mode nudiste à débloquer | Gagner une compétition (ou saisir un code avec un trophée) débloque le lieu « Club nudiste Les Bronzés » : <ul><li>joueurs au naturel, sans aucun détail anatomique, avec une serviette aux couleurs de l'équipe ;</li><li>public au naturel sur des serviettes dans l'herbe, sans gradins ;</li><li>annonces humoristiques de l'arbitre.</li></ul> |

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
| Codes (`password_test`) | 200 000/200 000 aller-retour ; 99,95 % des codes altérés d'un signe refusés |
| Émulateur (`emu_test`) | 103/103 vérifications (environ 12 min) |
| Profil CPU, simple, 6 000 trames | 1 trame d'échange en dépassement |
| Profil CPU, double, 6 000 trames | 11 trames d'échange en dépassement (0,2 %), sans défaut d'image |
| Compilation | Reproductible : SHA-256 `50fb34d69b4636461656cefb2111dd8123520b6f35e2ec9478c70d15f5d9fb4f` sur deux compilations propres |

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
| 4.3 | Multijoueur local | ✅ | 1 joueur, 2 adversaires, 2 en équipe en double, équipes CPU, revanche immédiate |
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

1. **Tests sur matériel réel** (priorité 1 du CDC). Il faut flasher `dist/pixel_open.nes` sur une cartouche MMC1/SNROM (ou un linéaire EverDrive/PowerPak) et tester sur NES NTSC, NES PAL et Famicom. Points à surveiller :
   - l'état du MMC1 à la mise sous tension ;
   - le temps de la NMI ;
   - Dendy ;
   - le rognage de l'image selon les téléviseurs.

   Il faut aussi passer la ROM dans Mesen et Nestopia : seul jsnes a été utilisé.
2. **Dépassements CPU** : 0,2 % des trames d'échange en double (frappes, début de ralenti) et quelques trames de transition entre les points. Pas d'impact sur les commandes.
3. **Équilibrage** : un joueur robot parfait bat le niveau expert ; l'expert pourrait être plus agressif.
4. **Tenues en double** : les partenaires portent la palette de leur équipe (4 palettes de sprites).
5. **Espace** : la banque du moteur (0) est presque pleine (16 314 / 16 368 octets). Toute évolution du moteur passera par la banque des annexes (3) ou par la banque libre (6).
