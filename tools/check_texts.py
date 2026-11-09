#!/usr/bin/env python3
"""Pixel Open — relecture automatique des textes affichés.

Vérifie dans les sources C :
- que chaque caractère des chaînes existe dans la police (sinon il
  s'afficherait comme un blanc) ;
- que les textes placés par txt_at(x, y, "...") ne dépassent pas la colonne 30
  (les colonnes 0-1 et 30-31 peuvent être rognées par les téléviseurs) ;
- que les lignes du mode d'emploi font au plus 28 caractères ;
- que les annonces du bandeau (set_msg, set2, tmsg, win_label) font au plus
  14 caractères.
Code de sortie 1 si un problème est trouvé."""
import re, sys, pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
gen = (ROOT / 'tools' / 'gen_assets.py').read_text(encoding='utf-8')
font_src = gen[gen.index('FONT = {'):gen.index('\n}', gen.index('FONT = {'))]
glyphs = set(re.findall(r"""^ ["'](.)["']:""", font_src, re.M)) | {' '}

STR = r'"((?:[^"\\]|\\.)*)"'
problems = []
for f in sorted((ROOT / 'src').glob('*.c')):
    if f.name.startswith('assets'):
        continue
    src = f.read_text(encoding='utf-8')
    code = re.sub(r'/\*.*?\*/', '', src, flags=re.S)       # sans les commentaires
    code = re.sub(r'#\s*(include|pragma).*', '', code)
    for m in re.finditer(STR, code):
        t = m.group(1)
        if f.name == 'stats.c' or t in ('BANK', 'RODATA'):
            continue
        bad = sorted({c for c in t if c not in glyphs})
        if bad:
            problems.append(f'{f.name}: « {t} » : caractère(s) absent(s) de la police {bad}')
    for m in re.finditer(r'txt_at\(\s*(\d+)\s*,\s*[^,]+,\s*' + STR, code):
        x, t = int(m.group(1)), m.group(2)
        if x + len(t) > 30:
            problems.append(f'{f.name}: txt_at({x}, …, « {t} ») dépasse la colonne 30')
    for m in re.finditer(r'(?:set_msg|set2|tmsg)\(\s*' + STR + r'(?:\s*,\s*' + STR + r')?', code):
        for t in m.groups():
            if t is not None and len(t) > 14:
                problems.append(f'{f.name}: annonce « {t} » : {len(t)} caractères (14 au plus)')
    if f.name == 'extras.c':
        # tableaux de messages affichés dans le bandeau (tutoriel)
        for m in re.finditer(r'(tut_l[12])\[\d+\]\s*=\s*\{(.*?)\};', code, re.S):
            for t in re.findall(STR, m.group(2)):
                if len(t) > 14:
                    problems.append(f'extras.c: {m.group(1)} « {t} » : {len(t)} caractères (14 au plus)')
    if f.name == 'gamefix.c':
        i = code.index('win_label'); j = code.index('\n}', i)
        for t in re.findall(STR, code[i:j]):
            if len(t) > 14:
                problems.append(f'gamefix.c: annonce « {t} » trop longue')
    if f.name == 'manual.c':
        i = code.index('page_text'); j = code.index('};', i)
        for t in re.findall(STR, code[i:j]):
            if len(t) > 28:
                problems.append(f'manual.c: ligne « {t} » : {len(t)} caractères (28 au plus)')

for p in problems:
    print('  ' + p)
print(f'textes : {len(problems)} problème(s)')
sys.exit(1 if problems else 0)
