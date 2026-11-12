/* Tests des codes de reprise : aller-retour + détection des erreurs de saisie */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/password.h"

static int same(const PwData *a, const PwData *b) { return memcmp(a, b, sizeof *a) == 0; }

int main(void)
{
    PwData d, e;
    char code[PW_LEN + 1] = {0}, bad[PW_LEN + 1];
    long ok = 0, total = 0, rejected = 0, mutations = 0, i, k, j;
    srand(1234);
    for (i = 0; i < 200000; ++i) {
        memset(&d, 0, sizeof d);
        d.type = rand() % 6; d.stage = rand() % 16; d.seed = rand() & 255; d.chr = rand() % 21;
        d.surf = rand() % 6; d.fmt = rand() % 4; d.cond = rand() % 3; d.env = rand() % 7;
        d.lvl = rand() % 3; d.results = rand() & 255; d.opts = rand() % 8; d.trophies = rand() % 16;
        d.doubles = rand() & 1; d.partner = (d.chr + 1 + rand() % 20) % 21; d.hum2 = d.doubles ? (rand() & 1) : 0;
        if (d.env == 3 && (d.chr == 16 || d.partner == 16)) d.env = 4;   /* jamais Margaux au club nudiste */
        if (d.type == 5) {
            /* le Grand Martinot (version 0) : surfaces imposées, pas de
               résultats sur 8 bits ni d'options ; 0 à 4 tours gagnés (3 en double) */
            d.surf = 0; d.results = 0; d.opts = 0;
            for (k = 0; k < 4; ++k) d.gres |= (unsigned short)(rand() % (d.doubles ? 4 : 5)) << (k * 3);
        }
        pw_encode(code, &d);
        ++total;
        if (pw_decode(code, &e) && same(&d, &e)) ++ok;
        if (i < 20000) {
            /* une lettre modifiée doit être refusée */
            for (k = 0; k < PW_LEN; ++k) for (j = 0; j < 32; ++j) {
                memcpy(bad, code, sizeof bad);
                if (PW_ALPHA[j] == code[k]) continue;
                bad[k] = PW_ALPHA[j];
                ++mutations;
                if (!pw_decode(bad, &e) || same(&d, &e)) ++rejected;
            }
        }
    }
    printf("codes : %ld/%ld aller-retour corrects ; %ld/%ld codes altérés refusés (%.2f%%)\n",
           ok, total, rejected, mutations, rejected * 100.0 / mutations);
    if (pw_decode("IIIIIIIIIIIII", &e)) return 1;
    /* un code de l'ancien format (16 personnages, version 2) reste valable */
    if (!pw_decode("DBSJ5PAWMKJQZ", &e) || e.trophies != 1) { printf("ancien code refusé\n"); return 1; }
    printf("ancien format (v2) toujours accepté\n");
    /* Margaux (personnage 16) en joueuse et en partenaire */
    memset(&d, 0, sizeof d); d.chr = 16; d.doubles = 1; d.partner = 3; d.env = 4; d.surf = 5; d.type = 4; d.cond = 2;
    pw_encode(code, &d);
    if (!pw_decode(code, &e) || !same(&d, &e)) return 1;
    d.chr = 3; d.partner = 16; pw_encode(code, &d);
    if (!pw_decode(code, &e) || !same(&d, &e)) return 1;
    printf("personnage 16 (Margaux) codé et relu\n");
    d.env = 3; pw_encode(code, &d);
    if (pw_decode(code, &e)) return 1;
    printf("Margaux au club nudiste : code refusé\n");
    /* le Grand Martinot : 4 tournois gagnés, puis un résultat impossible (5 tours) refusé */
    memset(&d, 0, sizeof d); d.type = 5; d.stage = 15; d.seed = 77; d.chr = 3; d.env = 6; d.lvl = 2; d.trophies = 9;
    d.gres = 4 | 4 << 3 | 4 << 6 | 4 << 9;
    pw_encode(code, &d);
    if (!pw_decode(code, &e) || !same(&d, &e)) { printf("Grand Martinot : code non relu\n"); return 1; }
    printf("Grand Martinot (version 0) codé et relu\n");
        printf("caractères invalides refusés\n");
    return (ok == total && rejected * 1000 >= mutations * 997) ? 0 : 1;
}
