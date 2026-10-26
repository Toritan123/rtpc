#!/usr/bin/env python3
"""A local page for listening to replacement candidates and choosing them.

    tools/samplematch/review.py OUTDIR [PORT]      -> http://localhost:PORT/

Lists every listening file under OUTDIR (listen/ from match.py: accepted
matches; listen_similar/: only spectrally similar; and any ai_*/listen/ from
audiosr_batch.py), with the scores from matches.csv. Each choice (use / do
not use) is written to OUTDIR/review.json at once; apply.py turns those into
the final replacement folder.
"""
import csv, glob, html, json, os, sys, threading
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from functools import partial

GROUPS = [
    ('listen', '同じ録音の高音質版（採用済み）', '元の音 → 差し替え後', 'use'),
    ('listen_similar', '似ている音（録り直しの可能性）', '元の音 → 候補', ''),
]


def load(outdir):
    rows = {}
    p = os.path.join(outdir, 'matches.csv')
    if os.path.exists(p):
        for r in csv.DictReader(open(p, encoding='utf-8')):
            rows[r['sample']] = r
    review = {}
    p = os.path.join(outdir, 'review.json')
    if os.path.exists(p):
        try:
            review = json.load(open(p, encoding='utf-8'))
        except ValueError:
            # A file broken by the old server (two saves interleaved): keep
            # it aside and start from what the page sends next.
            os.replace(p, p + '.broken')
    return rows, review


_lock = threading.Lock()


def save(outdir, changes):
    """Merge changes into review.json: one writer at a time, written to a
    temporary file and renamed, so a reader never sees half a file."""
    with _lock:
        _, review = load(outdir)
        review.update(changes)
        p = os.path.join(outdir, 'review.json')
        with open(p + '.tmp', 'w', encoding='utf-8') as f:
            json.dump(review, f, ensure_ascii=False, indent=1, sort_keys=True)
        os.replace(p + '.tmp', p)
        return len(review)


def page(outdir):
    rows, review = load(outdir)
    groups = list(GROUPS)
    for d in sorted(glob.glob(os.path.join(outdir, 'ai_*', 'listen'))):
        rel = os.path.relpath(d, outdir)
        model = rel.split(os.sep)[0][3:]
        groups.append((rel, 'AI 高音質化（%s）' % model,
                       '元の音 → AI そのまま → 元の音＋AI の高音' if model == 'audiosr'
                       else '元の音 → AI そのまま → タイミングと音量を元に合わせたもの（これが差し替えに使われます）', ''))
    out = []
    for folder, title, order, default in groups:
        files = sorted(glob.glob(os.path.join(outdir, folder, 'sample_*.wav')))
        if not files:
            continue
        out.append('<h2>%s <small>%d 個 ・ 再生順: %s</small></h2>' % (html.escape(title), len(files), order))
        for f in files:
            name = os.path.basename(f)[:-4]
            key = folder + '/' + name
            r = rows.get(name, {})
            info = ''
            if r and folder in ('listen', 'listen_similar'):
                info = '一致度 %s ・ %s Hz → %s Hz ・ %s' % (
                    r.get('wave_corr', ''), r.get('gba_rate', ''), r.get('cand_rate', ''),
                    html.escape(os.path.relpath(r.get('candidate', ''), '/Volumes/SSD-SCTU3A')))
                if r.get('note'):
                    info += ' ・ ' + html.escape(r['note'])
            state = review.get(key, default)
            out.append(
                '<div class="row" data-key="%s"><div class="name">%s</div>'
                '<audio controls preload="none" src="%s"></audio>'
                '<div class="btns"><button data-v="use" class="%s">使う</button>'
                '<button data-v="no" class="%s">使わない</button></div>'
                '<div class="info">%s</div></div>' % (
                    html.escape(key), name, html.escape(folder + '/' + os.path.basename(f)),
                    'on' if state == 'use' else '', 'on' if state == 'no' else '', info))
    return TEMPLATE.replace('%BODY%', '\n'.join(out))


TEMPLATE = """<!doctype html><html lang="ja"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Sample Review</title><style>
:root { --bg:#fafaf8; --fg:#1d1d1b; --mut:#6b6b66; --line:#e2e2dc; --on:#2f6f4f; --no:#a33b2f; }
@media (prefers-color-scheme: dark) { :root { --bg:#1b1b1a; --fg:#ecece8; --mut:#9a9a94; --line:#33332f; --on:#5fae86; --no:#d9705f; } }
body { background:var(--bg); color:var(--fg); font:14px/1.5 -apple-system, "Hiragino Sans", sans-serif; margin:0 auto; padding:16px; max-width:1100px; }
h1 { font-size:20px; } h2 { font-size:16px; margin-top:28px; border-bottom:1px solid var(--line); padding-bottom:4px; }
small { color:var(--mut); font-weight:normal; }
.row { display:grid; grid-template-columns: 110px minmax(200px, 320px) auto; gap:8px 12px; align-items:center; padding:8px 0; border-bottom:1px solid var(--line); }
.info { grid-column: 1 / -1; color:var(--mut); font-size:12px; word-break:break-all; }
.name { font-family: ui-monospace, monospace; }
audio { width:100%; height:32px; }
button { font:inherit; padding:4px 12px; border:1px solid var(--line); background:transparent; color:var(--fg); border-radius:6px; cursor:pointer; }
button.on[data-v=use] { background:var(--on); border-color:var(--on); color:#fff; }
button.on[data-v=no] { background:var(--no); border-color:var(--no); color:#fff; }
#st { position:fixed; right:16px; bottom:16px; color:var(--mut); background:var(--bg); padding:4px 8px; }
#st.err { color:#fff; background:var(--no); }
@media (max-width: 600px) { .row { grid-template-columns: 1fr; } }
</style></head><body>
<h1>差し替え候補の試聴</h1>
<p>各行の音を聞いて「使う／使わない」を選んでください。選ぶたびに保存されます。</p>
%BODY%
<div id="st"></div>
<script>
document.querySelectorAll('.row').forEach(function (row) {
  row.querySelectorAll('button').forEach(function (b) {
    b.addEventListener('click', function () {
      row.querySelectorAll('button').forEach(function (x) { x.classList.toggle('on', x === b); });
      // Send every choice on the page, not just this one, so a save that
      // failed is made good by the next.
      var all = {};
      document.querySelectorAll('.row').forEach(function (x) {
        var on = x.querySelector('button.on');
        if (on) all[x.dataset.key] = on.dataset.v;
      });
      var st = document.getElementById('st');
      fetch('save', { method: 'POST', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(all) })
        .then(function (r) { if (!r.ok) throw new Error(r.status); return r.json(); })
        .then(function (j) { st.className = ''; st.textContent = '保存しました（' + j.saved + ' 件）'; })
        .catch(function (e) { st.className = 'err'; st.textContent = '保存に失敗しました: ' + e; });
    });
  });
  row.querySelector('audio').addEventListener('play', function (ev) {
    document.querySelectorAll('audio').forEach(function (a) { if (a !== ev.target) a.pause(); });
  });
});
</script></body></html>"""


class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *a, outdir=None, **k):
        self.outdir = outdir
        super().__init__(*a, directory=outdir, **k)

    def do_GET(self):
        if self.path in ('/', '/index.html'):
            body = page(self.outdir).encode('utf-8')
            self.send_response(200)
            self.send_header('Content-Type', 'text/html; charset=utf-8')
            self.send_header('Content-Length', str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        else:
            super().do_GET()

    def do_POST(self):
        if self.path != '/save':
            self.send_error(404)
            return
        data = json.loads(self.rfile.read(int(self.headers.get('Content-Length', 0))))
        n = save(self.outdir, data)
        body = json.dumps({'saved': n}).encode()
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, *a):
        pass


if __name__ == '__main__':
    outdir = os.path.abspath(sys.argv[1])
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 8765
    print('http://localhost:%d/' % port, flush=True)
    ThreadingHTTPServer.request_queue_size = 64   # the default 5 refused bursts of clicks
    ThreadingHTTPServer(('127.0.0.1', port), partial(Handler, outdir=outdir)).serve_forever()
