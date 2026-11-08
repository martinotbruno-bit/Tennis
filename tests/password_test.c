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
        d.type = rand() % 5; d.stage = rand() % 16; d.seed = rand() & 255; d.chr = rand() % 16;
        d.surf = rand() % 6; d.fmt = rand() % 4; d.cond = rand() % 3; d.env = rand() % 4;
        d.lvl = rand() % 3; d.results = rand() & 255; d.opts = rand() % 8; d.trophies = rand() % 16;
        d.doubles = rand() & 1; d.partner = (d.chr + 1 + rand() % 15) % 16;
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
    printf("caractères invalides refusés\n");
    return (ok == total && rejected * 1000 >= mutations * 998) ? 0 : 1;
}
