#!/usr/bin/env python3
"""Radial variant of the parallel BFS animation.

Same simulation of bfs_parallel_v3 as make_bfs_svg.py (contiguous ranges
begin = n*id/W, CAS claims, barrier, main joins outputs in worker order), but
on a larger generated graph drawn as rings around the source. Vertex ids are a
random permutation (as in a real graph file), so adjacency lists and therefore
frontiers are in id order, not angle order: each worker's slice is scattered
around the ring, and workers genuinely share neighbours and race on them.

Stdlib only. Usage: python tools/animations/bfs_radial.py docs/public/animations/parallel-bfs-radial.svg
"""
import math
import random
import sys

# ---- parameters ---------------------------------------------------------------
W = 4
RING_COUNTS = [1, 7, 14, 22, 30, 38]
SEED = 7
EXTRA_PARENT_P = 0.55   # chance a vertex also has an edge from its 2nd-nearest parent
THIRD_PARENT_P = 0.12   # ...and from its 3rd-nearest
SAME_RING_P = 0.18      # chance of an edge to the next vertex on the same ring
EMPTY_COST = 0.5        # work units to visit a vertex with no out-edges
UNIT_S = 0.30           # seconds per edge check before a round is capped
ROUND_MIN, ROUND_MAX = 1.3, 2.2
GAP = 1.1               # seconds between barrier and next round (main joins)
INTRO, HOLD, RESET = 0.6, 3.2, 1.0

WIDTH, HEIGHT = 600, 640
CX, CY = 300, 384
RING_DR = 45
VR = 7.5
TAU = 2 * math.pi

# ---- graph ---------------------------------------------------------------------
rng = random.Random(SEED)
ang, rad, ring_of, rings = [], [], [], []
for d, n in enumerate(RING_COUNTS):
    if d == 0:
        items = [(0.0, 0.0)]
    else:
        off = rng.uniform(0, TAU)
        items = sorted(((off + TAU * (i + rng.uniform(-0.25, 0.25)) / n) % TAU,
                        d * RING_DR + rng.uniform(-0.22, 0.22) * RING_DR) for i in range(n))
    ids = []
    for a, r in items:
        ids.append(len(ang))
        ang.append(a)
        rad.append(r)
        ring_of.append(d)
    rings.append(ids)
N = len(ang)
pos = [(CX + rad[v] * math.cos(ang[v]), CY + rad[v] * math.sin(ang[v])) for v in range(N)]


def adiff(a, b):
    return abs((a - b + math.pi) % TAU - math.pi)


ADJ = {v: [] for v in range(N)}
for d in range(1, len(rings)):
    for v in rings[d]:
        parents = sorted(rings[d - 1], key=lambda u: adiff(ang[u], ang[v]))
        ADJ[parents[0]].append(v)
        if d > 1 and rng.random() < EXTRA_PARENT_P:
            ADJ[parents[1]].append(v)
            if rng.random() < THIRD_PARENT_P:
                ADJ[parents[2]].append(v)
    if d > 1:
        ring = rings[d]
        for i, v in enumerate(ring):
            if rng.random() < SAME_RING_P:
                ADJ[v].append(ring[(i + 1) % len(ring)])
vid_rank = list(range(N))  # the vertex ids a graph file would give, unrelated to position
rng.shuffle(vid_rank)
for u in ADJ:  # CSR adjacency is in id order
    ADJ[u].sort(key=lambda v: vid_rank[v])


# ---- simulation (bfs_parallel_v3) --------------------------------------------
def simulate():
    dist = [-1] * N
    dist[0] = 0
    claimed_by = {}
    rounds = []
    curr = [0]
    t = INTRO
    depth = 0
    while curr:
        n = len(curr)
        ranges = [(n * w // W, n * (w + 1) // W) for w in range(W)]
        checks, busy = [], [0.0] * W
        for w, (b, e) in enumerate(ranges):
            k = 0.0
            for u in curr[b:e]:
                for v in ADJ[u]:
                    cost = round(rng.uniform(0.6, 1.4), 2)  # checks vary (cache misses etc.)
                    checks.append({"w": w, "u": u, "v": v, "k0": k, "k1": k + cost})
                    k += cost
                if not ADJ[u]:
                    k += EMPTY_COST
            busy[w] = k
        max_b = max(max(busy), EMPTY_COST)
        scale = min(max(max_b * UNIT_S, ROUND_MIN), ROUND_MAX) / max_b

        # a check loads distance[v] at k0 and CASes at k1: it skips if v was
        # claimed by k0, and loses the CAS if another worker claimed v in (k0, k1]
        outputs = [[] for _ in range(W)]
        claim_k = {}
        order = sorted(checks, key=lambda c: (c["k1"], c["w"]))
        races = 0
        i = 0
        while i < len(order):
            j = i
            while j < len(order) and order[j]["k1"] == order[i]["k1"]:
                j += 1
            claims = {}
            for c in order[i:j]:
                v = c["v"]
                if dist[v] != -1 and claim_k.get(v, -1) <= c["k0"]:
                    c["kind"] = "skip"
                elif dist[v] != -1:
                    c["kind"] = "lost"
                    races += 1
                else:
                    claims.setdefault(v, []).append(c)
            for v, cs in claims.items():
                win = rng.choice(cs)["w"]  # CAS: one of the simultaneous attempts wins
                for c in cs:
                    if c["w"] == win:
                        c["kind"] = "win"
                        dist[v] = depth + 1
                        claim_k[v] = c["k1"]
                        outputs[win].append(v)
                    else:
                        c["kind"] = "lost"
                        races += 1
            i = j
        for c in checks:
            c["t0"], c["t1"] = t + c["k0"] * scale, t + c["k1"] * scale
            if c["kind"] == "win":
                claimed_by[c["v"]] = (c["w"], c["t1"])
        end = t + max_b * scale
        rounds.append({"depth": depth, "curr": curr, "ranges": ranges, "checks": checks,
                       "busy": [b * scale for b in busy], "start": t, "end": end, "races": races})
        curr = [v for out in outputs for v in out]
        t = end + GAP
        depth += 1
    return rounds, claimed_by


rounds, claimed_by = simulate()
RUN_END = rounds[-1]["end"] + GAP
T_TOTAL = RUN_END + HOLD + RESET
RESET_START = T_TOTAL - RESET

# ---- animation helpers ------------------------------------------------------------
css_rules = []


def pct(t):
    return f"{max(0.0, min(100.0, t / T_TOTAL * 100)):.3f}%"


def animate(el_id, frames):
    body = "".join(f"{pct(t)}{{{decl}}}" for t, decl in frames)
    css_rules.append(f"@keyframes k{el_id}{{{body}}}#{el_id}{{animation-name:k{el_id}}}")


def show(el_id, on, off=None, fade=0.2, hi=1):
    f = [(0, "opacity:0"), (on, "opacity:0"), (on + fade, f"opacity:{hi}")]
    if off is not None:
        off = max(off, on + fade)
        f += [(off, f"opacity:{hi}"), (off + fade, "opacity:0"), (T_TOTAL, "opacity:0")]
    else:
        f += [(RESET_START, f"opacity:{hi}"), (T_TOTAL, "opacity:0")]
    animate(el_id, f)


def grow(el_id, t0, t1):
    s = lambda x: f"transform:scaleX({x})"
    animate(el_id, [(0, s(0)), (t0, s(0)), (t1, s(1)), (RESET_START, s(1)), (T_TOTAL, s(0))])


def wcol(w):
    return f"var(--w{w})"


out = []
add = out.append
uid = 0


def nid(prefix):
    global uid
    uid += 1
    return f"{prefix}{uid}"


def edge_path(u, v):
    (x1, y1), (x2, y2) = pos[u], pos[v]
    dx, dy = x2 - x1, y2 - y1
    L = math.hypot(dx, dy)
    ux, uy = dx / L, dy / L
    a = VR + 1.5
    return f"M{x1 + ux * a:.1f} {y1 + uy * a:.1f}L{x2 - ux * a:.1f} {y2 - uy * a:.1f}"


def recolour_time(ri):
    return 0.3 if ri == 0 else rounds[ri - 1]["end"] + GAP * 0.5


# ---- graph layer -------------------------------------------------------------------
for u, vs in ADJ.items():
    for v in vs:
        add(f'<path d="{edge_path(u, v)}" class="edge"/>')

lost = []
for rd in rounds:
    for c in rd["checks"]:
        eid = nid("e")
        t0, t1 = c["t0"], c["t1"]
        f = [(0, "opacity:0;stroke-dashoffset:1"), (t0, "opacity:0;stroke-dashoffset:1"),
             (t0 + 0.01, "opacity:1;stroke-dashoffset:1"), (t1, "opacity:1;stroke-dashoffset:0")]
        if c["kind"] == "win":
            tid = nid("t")
            show(tid, rd["end"], fade=0.35)
            add(f'<path id="{tid}" d="{edge_path(c["u"], c["v"])}" class="a tree"/>')
            f += [(rd["end"], "opacity:1;stroke-dashoffset:0"), (rd["end"] + 0.35, "opacity:0;stroke-dashoffset:0")]
        else:
            keep = 0.5 if c["kind"] == "lost" else 0.15
            f += [(t1 + keep, "opacity:1;stroke-dashoffset:0"), (t1 + keep + 0.3, "opacity:0;stroke-dashoffset:0")]
            if c["kind"] == "lost":
                lost.append(c)
        f.append((T_TOTAL, "opacity:0;stroke-dashoffset:1"))
        animate(eid, f)
        cls = {"skip": "ov skip", "lost": "ov lostedge"}.get(c["kind"], "ov")
        add(f'<path id="{eid}" d="{edge_path(c["u"], c["v"])}" pathLength="1" class="a {cls}" stroke="{wcol(c["w"])}"/>')

# vertices: colour = worker responsible right now; slate once its round is scanned
owner = {}
for ri, rd in enumerate(rounds):
    for w, (b, e) in enumerate(rd["ranges"]):
        for v in rd["curr"][b:e]:
            owner[v] = (ri, w)
FADE = 0.2
for v in range(N):
    x, y = pos[v]
    r = VR + 2 if v == 0 else VR
    add(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="{r}" class="node"/>')
    ri, w_scan = owner[v]
    segs = [(0, "var(--ink)")] if v == 0 else [(claimed_by[v][1], wcol(claimed_by[v][0]))]
    if v == 0 or w_scan != claimed_by[v][0]:
        segs.append((recolour_time(ri), wcol(w_scan)))
    segs.append((rounds[ri]["end"], "var(--done)"))
    for k, (on, fill) in enumerate(segs):
        vid = nid("v")
        off = segs[k + 1][0] + FADE if k + 1 < len(segs) else None
        if on == 0:
            animate(vid, [(0, "opacity:1"), (off, "opacity:1"), (off + 0.01, "opacity:0"), (T_TOTAL, "opacity:0")])
        else:
            show(vid, on, off, fade=FADE)
        add(f'<circle id="{vid}" cx="{x:.1f}" cy="{y:.1f}" r="{r}" fill="{fill}" class="a"/>')

for c in lost:  # a red flash where a CAS attempt lost the race
    x, y = pos[c["v"]]
    lid = nid("l")
    show(lid, c["t1"], c["t1"] + 0.5, fade=0.12)
    add(f'<circle id="{lid}" cx="{x:.1f}" cy="{y:.1f}" r="{VR + 4}" class="a lostring"/>')

# ---- header ------------------------------------------------------------------------
add(f'<text x="32" y="50" class="title">Parallel BFS <tspan class="title-sub">— {W} workers</tspan></text>')
lx = 32
for w in range(W):
    add(f'<circle cx="{lx + 6}" cy="78" r="6" fill="{wcol(w)}"/><text x="{lx + 17}" y="82" class="lbl">W{w}</text>')
    lx += 56
add(f'<circle cx="{lx + 6}" cy="78" r="5" class="lostring"/><text x="{lx + 17}" y="82" class="sub">lost CAS</text>')
lx += 80
add(f'<circle cx="{lx + 6}" cy="78" r="6" fill="var(--done)"/><text x="{lx + 17}" y="82" class="sub">done</text>')

for ri, rd in enumerate(rounds):
    gid = nid("f")
    on = 0 if ri == 0 else recolour_time(ri)
    off = (recolour_time(ri + 1) if ri + 1 < len(rounds) else RUN_END) - 0.2
    show(gid, on, off, fade=0.2)
    add(f'<g id="{gid}" class="a"><text x="{WIDTH - 32}" y="50" class="lvl" text-anchor="end">level {rd["depth"]}</text>'
        f'<text x="{WIDTH - 32}" y="82" class="sub" text-anchor="end">frontier {len(rd["curr"])}</text></g>')
gid = nid("f")
show(gid, RUN_END, fade=0.2)
add(f'<g id="{gid}" class="a"><text x="{WIDTH - 32}" y="50" class="lvl" text-anchor="end">done</text>'
    f'<text x="{WIDTH - 32}" y="82" class="sub" text-anchor="end">{N} vertices</text></g>')

style = f"""
svg{{--bg:#fbfbfd;--border:#e3e6ec;--ink:#1f2430;--muted:#6b7280;--edge:#e3e6ec;--node:#d9dde4;
--track:#eef0f4;--idle:#d9dde4;--lost:#d64545;--w0:#2f6fdb;--w1:#e08a00;--w2:#12a37f;--w3:#8b5cf6;
--done:#4b5563;--tree:#9aa3b2;--join:#7b8494}}
@media (prefers-color-scheme:dark){{svg{{--bg:#0f141b;--border:#263040;--ink:#e6e9ef;--muted:#8b95a5;--edge:#1f2833;
--node:#2a3341;--track:#18202b;--idle:#2a3442;--lost:#ff6b6b;--w0:#5b9bff;--w1:#ffad33;--w2:#34d1a6;--w3:#a78bfa;
--done:#aab4c3;--tree:#4a5668;--join:#6b7588}}}}
text{{font-family:ui-sans-serif,-apple-system,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;fill:var(--ink)}}
.title{{font-size:20px;font-weight:650}}.title-sub{{font-weight:500;fill:var(--muted)}}
.sub{{font-size:12px;fill:var(--muted)}}.note{{font-size:12.5px;fill:var(--ink);opacity:.8}}
.lbl{{font-size:12px;font-weight:600}}.lvl{{font-size:16px;font-weight:650}}
.edge{{fill:none;stroke:var(--edge);stroke-width:1}}
.tree{{fill:none;stroke:var(--tree);stroke-width:1.4;stroke-linecap:round}}
.ov{{fill:none;stroke-width:2.2;stroke-linecap:round;stroke-dasharray:1 1}}.ov.skip{{stroke-width:1.2;opacity:.6}}
.node{{fill:var(--node)}}
.lostring{{fill:none;stroke:var(--lost);stroke-width:2}}.ov.lostedge{{stroke:var(--lost)}}
.track{{fill:var(--track)}}.idle{{fill:var(--idle)}}.join{{fill:var(--join)}}
.bar{{transform-box:fill-box;transform-origin:left center}}
.barrier{{stroke:var(--ink);stroke-width:1.2;stroke-dasharray:3 3}}.cursor{{stroke:var(--muted);stroke-width:1.2}}
.a{{animation-duration:{T_TOTAL:.2f}s;animation-iteration-count:infinite;animation-timing-function:linear;animation-fill-mode:both}}
@media (prefers-reduced-motion:reduce){{.a{{animation-play-state:paused;animation-delay:-{RUN_END + 0.8:.2f}s}}}}
"""

svg = (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {WIDTH} {HEIGHT}" width="{WIDTH}" height="{HEIGHT}" '
       f'role="img" aria-label="Animation of level-synchronous parallel BFS with {W} workers on a ring-shaped graph">'
       f'<style>{style}{"".join(css_rules)}</style>'
       f'<rect x="1" y="1" width="{WIDTH - 2}" height="{HEIGHT - 2}" rx="14" fill="var(--bg)" stroke="var(--border)"/>'
       + "".join(out) + "</svg>")

path = sys.argv[1] if len(sys.argv) > 1 else "parallel-bfs-radial.svg"
with open(path, "w", encoding="utf-8") as fh:
    fh.write(svg)
lost_n = sum(r["races"] for r in rounds)
print(f"wrote {path}: {len(svg) / 1024:.1f} KiB, {N} vertices, loop {T_TOTAL:.1f}s, "
      f"{len(rounds)} rounds, {lost_n} lost CAS")
