#!/usr/bin/env python3
"""Generate an animated SVG of level-synchronous parallel BFS.

The animation is driven by a small simulation of bfs_parallel_v3: each round the
frontier is split into contiguous ranges (begin = n*id/W), workers scan their
range one edge per tick, unvisited neighbours are claimed with a CAS, and all
workers wait at a barrier before the next frontier is assembled.

Stdlib only. Usage: python tools/animations/bfs_layered.py docs/public/animations/parallel-bfs-v3.svg
"""
import math
import sys

# ---- graph + simulation parameters -----------------------------------------
W = 3
N = 21
ADJ = {
    0: [1, 2, 3],
    1: [5, 4], 2: [5, 6, 7], 3: [8, 9],
    4: [10], 5: [11], 6: [11, 12], 7: [13], 8: [14, 15], 9: [15, 16],
    10: [17], 11: [18], 13: [18], 14: [19], 16: [19, 20],
}
RACE_WINNER = {5: 1}  # simultaneous CAS on the same vertex: who wins
EMPTY_COST = 0.5      # ticks to visit a vertex with no out-edges
GAP = 2.0             # ticks between the barrier and the next round (main joins outputs)
INTRO = 1.0
HOLD = 5.0
RESET = 1.2
TICK = 0.55           # seconds per tick


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
        checks, rings, busy = [], [], [0.0] * W
        for w, (b, e) in enumerate(ranges):
            k = 0.0
            for u in curr[b:e]:
                k0 = k
                nbrs = ADJ.get(u, [])
                for v in nbrs:
                    checks.append({"w": w, "u": u, "v": v, "t0": t + k, "t1": t + k + 1})
                    k += 1
                if not nbrs:
                    k += EMPTY_COST
                rings.append((w, u, t + k0, t + k))
            busy[w] = k

        # resolve claims in time order; equal finish times on one vertex race
        outputs = [[] for _ in range(W)]
        order = sorted(checks, key=lambda c: (c["t1"], c["w"]))
        i = 0
        while i < len(order):
            j = i
            while j < len(order) and order[j]["t1"] == order[i]["t1"]:
                j += 1
            claims = {}
            for c in order[i:j]:
                if dist[c["v"]] != -1:
                    c["kind"] = "skip"
                else:
                    claims.setdefault(c["v"], []).append(c)
            for v, cs in claims.items():
                win = RACE_WINNER.get(v, cs[0]["w"])
                for c in cs:
                    if c["w"] == win:
                        c["kind"] = "win"
                        dist[v] = depth + 1
                        claimed_by[v] = (win, c["t1"])
                        outputs[win].append(v)
                    else:
                        c["kind"] = "lost"
            i = j

        end = t + max(max(busy), EMPTY_COST)
        rounds.append({"depth": depth, "curr": curr, "ranges": ranges, "checks": checks,
                       "rings": rings, "busy": busy, "start": t, "end": end})
        curr = [v for out in outputs for v in out]
        t = end + GAP
        depth += 1
    return rounds, dist, claimed_by


rounds, dist, claimed_by = simulate()
RUN_END = rounds[-1]["end"] + GAP  # includes the final join that finds an empty frontier
T_TICKS = RUN_END + HOLD + RESET
RESET_START = T_TICKS - RESET
DURATION = T_TICKS * TICK

# ---- layout -----------------------------------------------------------------
WIDTH, HEIGHT = 880, 628
R = 16
COL_X0, COL_DX = 110, 165
GRAPH_TOP, GRAPH_BOT = 140, 430
LANE_X0, LANE_X1 = 70, 852
LANE_Y0, LANE_DY, LANE_H = 502, 26, 14

# each column is drawn in frontier-array order, so top-to-bottom matches the
# strip: W0's range, then W1's, then W2's
levels = {rd["depth"]: rd["curr"] for rd in rounds}
pos = {}
for d, vs in levels.items():
    gap = 47
    mid = (GRAPH_TOP + GRAPH_BOT) / 2
    for i, v in enumerate(vs):
        pos[v] = (COL_X0 + d * COL_DX, mid + (i - (len(vs) - 1) / 2) * gap)


def lane_x(t):
    return LANE_X0 + (t / RUN_END) * (LANE_X1 - LANE_X0)


# ---- animation helpers -------------------------------------------------------
css_rules = []


def pct(tk):
    return f"{max(0.0, min(100.0, tk / T_TICKS * 100)):.3f}%"


def animate(el_id, frames):
    """frames: list of (tick, css declarations); ticks must be non-decreasing."""
    body = "".join(f"{pct(tk)}{{{decl}}}" for tk, decl in frames)
    css_rules.append(f"@keyframes k{el_id}{{{body}}}#{el_id}{{animation-name:k{el_id}}}")


def show(el_id, on, off=None, fade=0.25, hi=1):
    f = [(0, "opacity:0"), (on, "opacity:0"), (on + fade, f"opacity:{hi}")]
    if off is not None:
        f += [(max(off, on + fade), f"opacity:{hi}"), (max(off, on + fade) + fade, "opacity:0"),
              (T_TICKS, "opacity:0")]
    else:
        f += [(RESET_START, f"opacity:{hi}"), (T_TICKS, "opacity:0")]
    animate(el_id, f)


def grow(el_id, t0, t1):
    s = lambda x: f"transform:scaleX({x})"
    animate(el_id, [(0, s(0)), (t0, s(0)), (t1, s(1)), (RESET_START, s(1)), (T_TICKS, s(0))])


def wcol(w):
    return f"var(--w{w})"


# ---- build svg --------------------------------------------------------------
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
    a, b = R + 2, R + 3
    return f"M{x1 + ux * a:.1f} {y1 + uy * a:.1f}L{x2 - ux * b:.1f} {y2 - uy * b:.1f}"


# header
add(f'<text x="28" y="42" class="title">Parallel BFS <tspan class="title-sub">— {W} workers</tspan></text>')
lx = WIDTH - 28 - W * 62
for w in range(W):
    add(f'<rect x="{lx + w * 62}" y="31" width="12" height="12" rx="3" fill="{wcol(w)}"/>')
    add(f'<text x="{lx + w * 62 + 18}" y="41.5" class="lbl">W{w}</text>')
add(f'<text x="{WIDTH - 28}" y="62" class="sub" text-anchor="end">frontier is re-split among workers at every barrier</text>')


def recolour_time(ri):
    """When round ri's frontier is split and its vertices take their new owner's colour."""
    return 0.5 if ri == 0 else rounds[ri - 1]["end"] + GAP * 0.5


# frontier strip per round
cell_x = {}
for ri, rd in enumerate(rounds):
    gid = nid("f")
    on = 0 if ri == 0 else recolour_time(ri)
    off = (recolour_time(ri + 1) if ri + 1 < len(rounds) else RUN_END) - 0.2
    show(gid, on, off, fade=0.2)
    add(f'<g id="{gid}" class="a">')
    add(f'<text x="28" y="104" class="lvl">level {rd["depth"]}</text>')
    add('<text x="110" y="104" class="sub">frontier</text>')
    x = 170
    for w, (b, e) in enumerate(rd["ranges"]):
        for v in rd["curr"][b:e]:
            cell_x[(ri, v)] = x
            add(f'<rect x="{x}" y="88" width="28" height="22" rx="5" fill="{wcol(w)}"/>')
            add(f'<text x="{x + 14}" y="103.5" class="cell" text-anchor="middle">{v}</text>')
            x += 32
        if e > b:
            x += 10
    add("</g>")
gid = nid("f")
show(gid, RUN_END, fade=0.2)
add(f'<g id="{gid}" class="a"><text x="28" y="104" class="lvl">done</text>'
    '<text x="110" y="104" class="sub">next frontier is empty, so every vertex has its distance</text></g>')

# depth labels
for d in levels:
    add(f'<text x="{COL_X0 + d * COL_DX}" y="{GRAPH_BOT + 40}" class="sub" text-anchor="middle">depth {d}</text>')

# base edges
for u, vs in ADJ.items():
    for v in vs:
        add(f'<path d="{edge_path(u, v)}" class="edge"/>')

# animated edge overlays
lost_labels = []
for rd in rounds:
    for c in rd["checks"]:
        eid = nid("e")
        t0, t1 = c["t0"], c["t1"]
        f = [(0, "opacity:0;stroke-dashoffset:1"), (t0, "opacity:0;stroke-dashoffset:1"),
             (t0 + 0.02, "opacity:1;stroke-dashoffset:1"), (t1, "opacity:1;stroke-dashoffset:0")]
        if c["kind"] == "win":
            f += [(rd["end"], "opacity:1;stroke-dashoffset:0"), (rd["end"] + 0.4, "opacity:0;stroke-dashoffset:0"),
                  (T_TICKS, "opacity:0;stroke-dashoffset:1")]
        else:
            keep = 1.4 if c["kind"] == "lost" else 0.3
            f += [(t1 + keep, "opacity:1;stroke-dashoffset:0"), (t1 + keep + 0.5, "opacity:0;stroke-dashoffset:0"),
                  (T_TICKS, "opacity:0;stroke-dashoffset:1")]
        animate(eid, f)
        cls = "ov skip" if c["kind"] == "skip" else "ov"
        if c["kind"] == "win":
            # once the round ends the claimed edge settles into the grey BFS tree
            tid = nid("t")
            show(tid, rd["end"], fade=0.4)
            add(f'<path id="{tid}" d="{edge_path(c["u"], c["v"])}" class="a tree"/>')
        add(f'<path id="{eid}" d="{edge_path(c["u"], c["v"])}" pathLength="1" class="a {cls}" stroke="{wcol(c["w"])}"/>')
        if c["kind"] == "lost":
            lost_labels.append(c)

# scanning rings
for ri, rd in enumerate(rounds):
    for w, u, t0, t1 in rd["rings"]:
        rid = nid("r")
        show(rid, t0, t1, fade=0.15)
        x, y = pos[u]
        add(f'<circle id="{rid}" cx="{x}" cy="{y}" r="{R + 5}" class="a ring" stroke="{wcol(w)}"/>')
        # the same vertex's cell in the frontier array, so the range scan is visible
        hid = nid("h")
        show(hid, t0, t1, fade=0.15)
        add(f'<rect id="{hid}" x="{cell_x[(ri, u)] - 3}" y="85" width="34" height="28" rx="7" class="a ring" stroke="{wcol(w)}"/>')

# vertices: colour = the worker responsible for it right now.
#   claimed      -> colour of the CAS winner
#   at barrier   -> recoloured to the worker whose range it lands in
#   after its round is scanned -> grey (finished)
owner = {}
for ri, rd in enumerate(rounds):
    for w, (b, e) in enumerate(rd["ranges"]):
        for v in rd["curr"][b:e]:
            owner[v] = (ri, w)
FADE = 0.25
for v in range(N):
    x, y = pos[v]
    add(f'<circle cx="{x}" cy="{y}" r="{R}" class="node"/>')
    add(f'<text x="{x}" y="{y + 4}" class="id" text-anchor="middle">{v}</text>')
    ri, w_scan = owner[v]
    reassign = recolour_time(ri)
    segs = []  # (on, fill, text class)
    if v == 0:
        segs.append((0, "var(--ink)", "id on"))
    else:
        w_claim, t1 = claimed_by[v]
        segs.append((t1, wcol(w_claim), "id on"))
    if v == 0 or w_scan != claimed_by[v][0]:
        segs.append((reassign, wcol(w_scan), "id on"))
    segs.append((rounds[ri]["end"], "var(--done)", "id done"))
    for k, (on, fill, tcls) in enumerate(segs):
        vid = nid("v")
        # keep the lower layer until the next one has fully faded in
        off = segs[k + 1][0] + FADE if k + 1 < len(segs) else None
        if on == 0 and off is not None:
            animate(vid, [(0, "opacity:1"), (off, "opacity:1"), (off + 0.01, "opacity:0"), (T_TICKS, "opacity:0")])
        else:
            show(vid, on, off, fade=FADE)
        add(f'<g id="{vid}" class="a"><circle cx="{x}" cy="{y}" r="{R}" fill="{fill}"/>'
            f'<text x="{x}" y="{y + 4}" class="{tcls}" text-anchor="middle">{v}</text></g>')

# CAS-lost callouts
for c in lost_labels:
    x, y = pos[c["v"]]
    lid = nid("l")
    show(lid, c["t1"], c["t1"] + 1.6, fade=0.15)
    add(f'<g id="{lid}" class="a"><rect x="{x - 90}" y="{y - 10}" width="64" height="20" rx="10" class="pill"/>'
        f'<text x="{x - 58}" y="{y + 3.5}" class="lost" text-anchor="middle">W{c["w"]} lost CAS</text></g>')

# worker timeline
add(f'<text x="28" y="{LANE_Y0 - 12}" class="sub">timeline: early finishers idle until the barrier, then main joins the outputs into the next frontier</text>')
y = LANE_Y0 + W * LANE_DY
add(f'<text x="28" y="{y + 11}" class="lbl">main</text>')
add(f'<rect x="{LANE_X0}" y="{y}" width="{LANE_X1 - LANE_X0}" height="{LANE_H}" rx="3" class="track"/>')
for rd in rounds:
    s, e = rd["end"], rd["end"] + GAP
    mid_ = nid("m")
    grow(mid_, s, e)
    add(f'<rect id="{mid_}" x="{lane_x(s):.1f}" y="{y}" width="{lane_x(e) - lane_x(s):.1f}" height="{LANE_H}" rx="3" class="a bar join"/>')
for w in range(W):
    y = LANE_Y0 + w * LANE_DY
    add(f'<text x="28" y="{y + 11}" class="lbl">W{w}</text>')
    add(f'<rect x="{LANE_X0}" y="{y}" width="{LANE_X1 - LANE_X0}" height="{LANE_H}" rx="3" class="track"/>')
    for rd in rounds:
        s, b, e = rd["start"], rd["start"] + rd["busy"][w], rd["end"]
        if b > s:
            bid = nid("b")
            grow(bid, s, b)
            add(f'<rect id="{bid}" x="{lane_x(s):.1f}" y="{y}" width="{lane_x(b) - lane_x(s):.1f}" height="{LANE_H}" rx="3" class="a bar" fill="{wcol(w)}"/>')
        if e > b:
            iid = nid("i")
            grow(iid, b, e)
            add(f'<rect id="{iid}" x="{lane_x(b):.1f}" y="{y}" width="{lane_x(e) - lane_x(b):.1f}" height="{LANE_H}" rx="3" class="a bar idle"/>')
for rd in rounds:
    bid = nid("w")
    show(bid, rd["end"], fade=0.2)
    x = lane_x(rd["end"])
    add(f'<line id="{bid}" x1="{x:.1f}" x2="{x:.1f}" y1="{LANE_Y0 - 5}" y2="{LANE_Y0 + W * LANE_DY + LANE_H + 5}" class="a barrier"/>')
cid = "cur"
end_dx = LANE_X1 - LANE_X0
tx = lambda d: f"transform:translateX({d:.1f}px)"
animate(cid, [(0, tx(0)), (RUN_END, tx(end_dx)), (RESET_START, tx(end_dx)), (T_TICKS, tx(0))])
add(f'<line id="{cid}" x1="{LANE_X0}" x2="{LANE_X0}" y1="{LANE_Y0 - 7}" y2="{LANE_Y0 + W * LANE_DY + LANE_H + 7}" class="a cursor"/>')

style = f"""
svg{{--bg:#fbfbfd;--border:#e3e6ec;--ink:#1f2430;--muted:#6b7280;--edge:#d5d9e0;--node:#ffffff;--nodeline:#b8bfca;
--on:#ffffff;--track:#eef0f4;--idle:#d9dde4;--lost:#d64545;--w0:#2f6fdb;--w1:#e08a00;--w2:#12a37f;--done:#5f6b7c;--donetext:#ffffff;--tree:#8792a3;--join:#7b8494}}
@media (prefers-color-scheme:dark){{svg{{--bg:#0f141b;--border:#263040;--ink:#e6e9ef;--muted:#8b95a5;--edge:#2c3644;
--node:#161c25;--nodeline:#445064;--on:#0f141b;--track:#18202b;--idle:#2a3442;--lost:#ff6b6b;--w0:#5b9bff;--w1:#ffad33;--w2:#34d1a6;--done:#8a96a8;--donetext:#0f141b;--tree:#5d6a7e;--join:#6b7588}}}}
text{{font-family:ui-sans-serif,-apple-system,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;fill:var(--ink)}}
.title{{font-size:19px;font-weight:650}}.sub{{font-size:12px;fill:var(--muted)}}.lbl{{font-size:12px;font-weight:600}}
.title-sub{{font-weight:500;fill:var(--muted)}}
.lvl{{font-size:15px;font-weight:650}}
.cell,.id{{font-family:ui-monospace,"Cascadia Mono",Consolas,Menlo,monospace;font-size:11px;font-weight:600}}.id{{font-size:12.5px}}
.cell,.id.on{{fill:var(--on)}}.id{{fill:var(--muted)}}
.id.done{{fill:var(--donetext)}}.tree{{fill:none;stroke:var(--tree);stroke-width:2.2;stroke-linecap:round}}
.edge{{fill:none;stroke:var(--edge);stroke-width:1.6}}
.ov{{fill:none;stroke-width:2.6;stroke-linecap:round;stroke-dasharray:1 1}}.ov.skip{{stroke-width:1.8;stroke-dasharray:0.04 0.05}}
.node{{fill:var(--node);stroke:var(--nodeline);stroke-width:1.4}}
.ring{{fill:none;stroke-width:2.4}}
.pill{{fill:var(--bg);stroke:var(--lost);stroke-width:1.2}}.lost{{font-size:10.5px;font-weight:650;fill:var(--lost)}}
.track{{fill:var(--track)}}.idle{{fill:var(--idle)}}
.join{{fill:var(--join)}}
.bar{{transform-box:fill-box;transform-origin:left center}}
.barrier{{stroke:var(--ink);stroke-width:1.4;stroke-dasharray:3 3}}.cursor{{stroke:var(--muted);stroke-width:1.2}}
.a{{animation-duration:{DURATION:.2f}s;animation-iteration-count:infinite;animation-timing-function:linear;animation-fill-mode:both}}
@media (prefers-reduced-motion:reduce){{.a{{animation-play-state:paused;animation-delay:-{(RUN_END + 1) * TICK:.2f}s}}}}
"""

svg = (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {WIDTH} {HEIGHT}" width="{WIDTH}" height="{HEIGHT}" '
       f'role="img" aria-label="Animation of level-synchronous parallel BFS with {W} workers">'
       f'<style>{style}{"".join(css_rules)}</style>'
       f'<rect x="1" y="1" width="{WIDTH - 2}" height="{HEIGHT - 2}" rx="14" fill="var(--bg)" stroke="var(--border)"/>'
       + "".join(out) + "</svg>")

path = sys.argv[1] if len(sys.argv) > 1 else "parallel-bfs.svg"
with open(path, "w", encoding="utf-8") as fh:
    fh.write(svg)
print(f"wrote {path}: {len(svg) / 1024:.1f} KiB, loop {DURATION:.1f}s, {len(rounds)} rounds")
