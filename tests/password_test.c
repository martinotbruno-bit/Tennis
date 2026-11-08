/* Tests des codes de sauvegarde : aller-retour exhaustif + détection d'erreurs */
#include <stdio.h>
#include "../src/password.h"
int main(void)
{
    char code[7] = {0}, bad[7];
    int ok = 0, total = 0, rejected = 0, mutations = 0, i, k;
    pu8 r, c, s, f, n, w, R, C, S, F, N, W;
    for (R = 0; R < 3; ++R) for (C = 0; C < 6; ++C) for (S = 0; S < 4; ++S)
    for (F = 0; F < 4; ++F) for (N = 0; N < 3; ++N) for (W = 0; W < 16; ++W) {
        pw_encode(code, R, C, S, F, N, W);
        ++total;
        if (pw_decode(code, &r, &c, &s, &f, &n, &w) && r == R && c == C && s == S && f == F && n == N && w == W) ++ok;
        /* une lettre modifiée doit être refusée (ou décoder vers autre chose) */
        for (i = 0; i < 6; ++i) for (k = 0; k < 16; ++k) {
            for (int j = 0; j < 7; ++j) bad[j] = code[j];
            if (PW_ALPHA[k] == code[i]) continue;
            bad[i] = PW_ALPHA[k];
            ++mutations;
            if (!pw_decode(bad, &r, &c, &s, &f, &n, &w)) ++rejected;
        }
    }
    printf("codes : %d/%d aller-retour corrects ; %d/%d codes altérés refusés (%.1f%%)\n",
           ok, total, rejected, mutations, rejected * 100.0 / mutations);
    if (!pw_decode("ZZZZZZ", &r, &c, &s, &f, &n, &w)) printf("lettres invalides refusées\n"); else return 1;
    return (ok == total && rejected * 100 >= mutations * 95) ? 0 : 1;
}
