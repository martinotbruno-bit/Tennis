# Pixel Open

Jeu de tennis 8 bits original pour **NES / Famicom** (NTSC et PAL), sur cartouche MMC1 :
- simple et double, à 1 ou 2 joueurs ;
- 17 personnages aux styles de jeu différents, dont Margaux, IA à 3 niveaux qui s'adapte au joueur ;
- 6 surfaces (dont plage et glace), 4 lieux (stade, club, salle, court rose « Margaux »), jour, nuit et pluie ;
- compétitions en simple ou en double (tournoi, poules, championnat, saison, personnalisé) avec codes de reprise et levée de coupe ;
- un lieu bonus à débloquer : le club nudiste « Les Bronzés » ;
- 9 exercices d'entraînement dont un tutoriel ;
- smash sauté, coups gagnants annoncés, vitesse du jeu réglable (lent, normal, rapide), réglage du son ;
- ombres, poussière et éclaboussures, arbitre de chaise, réglages mémorisés ;
- ralentis, palmarès et mode d'emploi intégré.

![Aperçu](docs/img/apercu.png)

## Jouer

ROM prête à lancer : [`dist/pixel_open.nes`](dist/pixel_open.nes) (iNES, mapper 1 / MMC1, 128 Ko). Elle se lance dans un émulateur NES ou sur console via une cartouche flash compatible MMC1.

Commandes de base :
- **Croix** : se déplacer ;
- **A** (maintenir puis relâcher) : frappe liftée ;
- **B** : coupé ;
- **+ HAUT/BAS** : frappe à plat, croisé court, lob, amortie ;
- **START** : pause.

Tout est expliqué dans le **mode d'emploi** du jeu (écran titre) et dans le [manuel](docs/MANUEL.md).

## Documentation

- [Cahier des charges](CAHIER_DES_CHARGES.md)
- [Manuel du joueur](docs/MANUEL.md)
- [Documentation technique](docs/TECHNIQUE.md) : cartouche, banques, mémoire, physique, IA, compétitions, codes, optimisations
- [Rapport de réalisation](docs/RAPPORT.md) : conformité au CDC, tests, limitations
- [Vérification sur vraie console](docs/VERIFICATION_CONSOLE.md) : liste à cocher, pas à pas

## Compiler et tester

Outils : `cc65` 2.19, `python3`, `make` ; pour les tests, `gcc` et `node` ≥ 18.

```sh
make          # build/pixel_open.nes (+ occupation de chaque banque)
make test     # règles + codes (natif) et la suite complète sur émulateur (jsnes)
```

## Organisation

```
src/        code du jeu (C cc65 + assembleur 6502), un module par banque MMC1
tools/      génération des graphismes, rapport d'occupation mémoire
assets/     CHR généré et aperçus
cfg/        carte mémoire ld65 (MMC1 SNROM)
tests/      tests unitaires, tests sur émulateur, profileur CPU, captures
docs/       manuel, documentation technique, rapport, captures
dist/       ROM finale
```

## État

Version 2.5 : tout le contenu du cahier des charges est réalisé et testé **en émulation**. Les tests sur consoles réelles restent à faire : suivez la [liste de vérification](docs/VERIFICATION_CONSOLE.md).
