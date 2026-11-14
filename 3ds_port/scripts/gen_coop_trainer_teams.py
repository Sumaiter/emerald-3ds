"""Derive diverse co-op additions from the ROM's real trainer rosters.

Keep every original Pokemon. Prefer the trainer's own identity/rematches,
then its class, then its established types. Do not import a species earlier
than its earliest ordinary trainer appearance, or add legendary Pokemon.
"""
import argparse
from pathlib import Path
import re
import json
from collections import defaultdict

ROOT = Path(__file__).resolve().parents[2]
LEGENDARY = set('ARTICUNO ZAPDOS MOLTRES MEWTWO MEW RAIKOU ENTEI SUICUNE LUGIA HO_OH CELEBI REGIROCK REGICE REGISTEEL LATIAS LATIOS KYOGRE GROUDON RAYQUAZA JIRACHI DEOXYS'.split())
STARTERS = set('BULBASAUR IVYSAUR VENUSAUR CHARMANDER CHARMELEON CHARIZARD SQUIRTLE WARTORTLE BLASTOISE CHIKORITA BAYLEEF MEGANIUM CYNDAQUIL QUILAVA TYPHLOSION TOTODILE CROCONAW FERALIGATR TREECKO GROVYLE SCEPTILE TORCHIC COMBUSKEN BLAZIKEN MUDKIP MARSHTOMP SWAMPERT'.split())
FACILITY = {'TRAINER_CLASS_'+s for s in 'SALON_MAIDEN DOME_ACE PALACE_MAVEN ARENA_TYCOON FACTORY_HEAD PIKE_QUEEN PYRAMID_KING'.split()}

def generate(upstream):
    parties = {}
    text = (upstream/'src/data/trainer_parties.h').read_text(encoding='utf-8')
    for m in re.finditer(r'static const struct \w+ (\w+)\[\] = \{(.*?)\n\};', text, re.S):
        parties[m[1]] = [(s, int(l)) for l, s in re.findall(r'\.lvl\s*=\s*(\d+).*?\.species\s*=\s*(SPECIES_\w+)', m[2], re.S)]
    text = (upstream/'src/data/trainers.h').read_text(encoding='utf-8')
    trainers = []
    for m in re.finditer(r'\[(TRAINER_\w+)\]\s*=\s*\{(.*?)\n    \}', text, re.S):
        # Debug fixtures must not change the species/levels chosen for story trainers.
        if m[1].startswith('TRAINER_ROUTE101_TEST_'):
            continue
        body = m[2]
        party = re.search(r'\.party\s*=\s*\w+\((\w+)\)', body)
        cls = re.search(r'\.trainerClass\s*=\s*(\w+)', body)
        pic = re.search(r'\.trainerPic\s*=\s*(\w+)', body)
        if party and cls and pic and party[1] in parties:
            trainers.append((m[1], cls[1], pic[1], parties[party[1]], 'doubleBattle = TRUE' in body))
    species_text = (upstream/'src/data/pokemon/species_info.h').read_text(encoding='utf-8')
    types = {}
    for m in re.finditer(r'\[(SPECIES_\w+)\]\s*=\s*\{(.*?)\n    \}', species_text, re.S):
        t = re.search(r'\.types\s*=\s*\{(.*?)\}', m[2])
        if t: types[m[1]] = set(re.findall(r'TYPE_\w+', t[1]))
    first_level = {}
    observations = defaultdict(list)
    for ident, cls, pic, party, double in trainers:
        for species, level in party:
            first_level[species] = min(first_level.get(species, 100), level)
            observations[species].append((cls, pic, level))
    def wild_levels(value):
        if isinstance(value, dict):
            if 'species' in value and 'min_level' in value:
                s=value['species']; first_level[s]=min(first_level.get(s,100), value['min_level'])
            for child in value.values(): wild_levels(child)
        elif isinstance(value, list):
            for child in value: wild_levels(child)
    wild_levels(json.loads((upstream/'src/data/wild_encounters.json').read_text(encoding='utf-8')))
    # Early Bug Catchers otherwise have only Wurmple: an unevolved Surskit
    # adds a different, class-appropriate Pokemon without level-3 evolutions.
    first_level['SPECIES_SURSKIT'] = min(first_level.get('SPECIES_SURSKIT',100), 3)
    result = []
    for ident, cls, pic, party, double in trainers:
        if not party or double or cls in FACILITY or ident.endswith('_PLACEHOLDER') or any(s == 'SPECIES_NONE' or not l for s,l in party): continue
        original = {s for s, _ in party}
        level = max(l for _, l in party)
        target = min(6, max(2, len(party)*2))
        theme = set().union(*(types.get(s, set()) for s, _ in party))
        candidates = []
        for species, obs in observations.items():
            short = species.removeprefix('SPECIES_')
            if species in original or short in LEGENDARY or short in STARTERS: continue
            if first_level[species] > level: continue
            same_identity = any(c == cls and p == pic for c, p, _ in obs)
            same_class = any(c == cls for c, _, _ in obs)
            matches_type = bool(types.get(species, set()) & theme)
            if not (same_class or matches_type): continue
            distance = min(abs(l-level) for c,p,l in obs if c == cls) if same_class else abs(first_level[species]-level)
            # Stable diversity between individual trainers of the same class.
            tie = sum((i+1)*ord(c) for i,c in enumerate(ident+species)) % 997
            candidates.append(((not same_identity, not same_class, not matches_type, distance, tie), species))
        extras = [s for _,s in sorted(candidates)[:target-len(party)]]
        result.append((ident, party, extras, level))
    assert len(trainers) > 800 and len(result) > 500, (len(trainers),len(result))
    assert all(len(set(s for s,_ in party)|set(extras)) == len(set(s for s,_ in party))+len(extras) for _,party,extras,_ in result)
    assert all(len(party)+len(extras) <= 6 for _,party,extras,_ in result)
    assert all(extras or len(party) == 6 for _,party,extras,_ in result), [i for i,p,e,_ in result if not e and len(p)<6]
    lines = ['/* Generated by scripts/gen_coop_trainer_teams.py from native rosters. */',
             'static const u16 sCoopTrainerAdditions[TRAINERS_COUNT][3] = {']
    for ident, _, extras, _ in result:
        if extras: lines.append('    ['+ident+'] = {'+', '.join(extras)+'},')
    lines.append('};\n')
    return '\n'.join(lines), result

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--upstream', type=Path, default=ROOT/'build/upstream')
    p.add_argument('--output', type=Path, default=ROOT/'3ds_port/include/3ds_coop_trainer_teams.h')
    p.add_argument('--check', action='store_true')
    a = p.parse_args()
    text, rows = generate(a.upstream)
    if a.check:
        assert a.output.read_text(encoding='utf-8') == text, 'Trainer additions need regeneration'
    else:
        a.output.write_text(text, encoding='utf-8', newline='\n')
    print(f'{len(rows)} ordinary single-trainer teams: original rosters preserved, additions unique, maximum six')

if __name__ == '__main__': main()
