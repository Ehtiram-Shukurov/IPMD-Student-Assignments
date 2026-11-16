'use strict';

// Renders the campaign-status view as one self-contained HTML page:
// totals first, then one row per campaign, then the per-email breakdown,
// then the data-quality notes that explain what the numbers can and cannot say.

const esc = (v) =>
  String(v ?? '').replace(/[&<>"']/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));

const pct = (num, den) => (den > 0 ? `${((100 * num) / den).toFixed(1)}%` : '—');

const fmtDate = (v) => {
  if (!v) return '—';
  const d = v instanceof Date ? v : new Date(v);
  return Number.isNaN(d.getTime()) ? esc(v) : d.toISOString().slice(0, 16).replace('T', ' ');
};

const CSS = `
:root { --fg:#1d1d1f; --muted:#6e6e73; --line:#e3e3e6; --bg:#fff; --head:#f5f5f7; --warn:#9a5b00; --warnbg:#fff6e5; --bad:#b3261e; }
* { box-sizing:border-box; }
body { margin:0; padding:24px; font:14px/1.45 -apple-system, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; color:var(--fg); background:var(--bg); }
h1 { font-size:20px; margin:0 0 4px; }
h2 { font-size:15px; margin:28px 0 8px; }
.sub { color:var(--muted); margin:0 0 16px; }
.banner { padding:8px 12px; border-radius:6px; margin-bottom:16px; font-weight:600; }
.banner.fixture { background:var(--warnbg); color:var(--warn); border:1px solid #f0d49a; }
.banner.live { background:#eaf6ec; color:#1e6b34; border:1px solid #b9dfc2; }
.kpis { display:grid; grid-template-columns:repeat(auto-fit, minmax(130px, 1fr)); gap:10px; }
.kpi { border:1px solid var(--line); border-radius:8px; padding:10px 12px; }
.kpi .n { font-size:24px; font-weight:650; font-variant-numeric:tabular-nums; }
.kpi .l { color:var(--muted); font-size:12px; }
table { border-collapse:collapse; width:100%; }
th, td { padding:6px 10px; border-bottom:1px solid var(--line); text-align:right; font-variant-numeric:tabular-nums; white-space:nowrap; }
th:first-child, td:first-child, th.l, td.l { text-align:left; }
th { background:var(--head); font-weight:600; font-size:12px; color:#3a3a3c; }
.flag { color:var(--bad); font-weight:600; }
.note { color:var(--muted); font-size:12px; }
ul.issues { margin:0; padding-left:18px; }
ul.issues li { margin:4px 0; }
.wrap { overflow-x:auto; }
`;

function kpis(t) {
  const items = [
    ['Sent', t.sent],
    ['Opened', t.opened, pct(t.opened, t.sent)],
    ['Clicked', t.clicked, pct(t.clicked, t.sent)],
    ['Unsubscribed', t.unsubscribed],
    ['Failed / other', t.not_sent],
  ];
  return `<div class="kpis">${items
    .map(([l, n, r]) => `<div class="kpi"><div class="n">${esc(n)}</div><div class="l">${esc(l)}${r ? ` · ${esc(r)}` : ''}</div></div>`)
    .join('')}</div>`;
}

function campaignTable(rows) {
  const body = rows
    .map((c) => `<tr>
  <td>${esc(c.campaign_name ?? c.campaign_id)}</td>
  <td class="l">${esc(c.campaign_status ?? '')}</td>
  <td>${c.sent}</td><td>${c.opened}</td><td>${pct(c.opened, c.sent)}</td>
  <td>${c.clicked}</td><td>${pct(c.clicked, c.sent)}</td>
  <td>${c.unsubscribed}</td><td>${c.not_sent}</td>
  <td class="${c.stored_totals_stale ? 'flag' : ''}">${c.stored_total_sent} / ${c.stored_total_opened} / ${c.stored_total_clicked}${c.stored_totals_stale ? ' ≠' : ''}</td>
  <td class="l">${fmtDate(c.last_sent_at)}</td>
</tr>`)
    .join('');
  return `<div class="wrap"><table>
<thead><tr><th>Campaign</th><th class="l">Status</th><th>Sent</th><th>Opened</th><th>Open rate</th><th>Clicked</th><th>Click rate</th><th>Unsub*</th><th>Failed / other</th><th>Stored totals (sent / opened / clicked)</th><th class="l">Last send (UTC)</th></tr></thead>
<tbody>${body || '<tr><td colspan="11" class="l">No campaigns.</td></tr>'}</tbody></table></div>`;
}

function emailTable(rows) {
  const body = rows
    .map((e) => `<tr>
  <td>${esc(e.campaign_name ?? e.campaign_id)}</td>
  <td class="l">${esc(e.email_type)}</td><td class="l">${esc(e.variant)}</td>
  <td>${e.sent}</td><td>${e.opened}</td><td>${pct(e.opened, e.sent)}</td>
  <td>${e.clicked}</td><td>${pct(e.clicked, e.sent)}</td>
  <td>${e.unsubscribed}</td><td>${e.not_sent}</td>
</tr>`)
    .join('');
  return `<div class="wrap"><table>
<thead><tr><th>Campaign</th><th class="l">Email type</th><th class="l">Variant</th><th>Sent</th><th>Opened</th><th>Open rate</th><th>Clicked</th><th>Click rate</th><th>Unsub*</th><th>Failed / other</th></tr></thead>
<tbody>${body || '<tr><td colspan="10" class="l">No sends logged.</td></tr>'}</tbody></table></div>`;
}

function issues(data) {
  const q = data.quality;
  const out = [];
  const stale = data.campaigns.filter((c) => c.stored_totals_stale);
  if (stale.length) {
    out.push(`<b>${stale.length} of ${data.campaigns.length} campaigns</b> have stored <code>campaigns.total_*</code> values that do not match the logs (marked ≠). The counts in this view are computed from <code>send_log</code> and <code>analytics_events</code>, not from those columns.`);
  }
  if (q.sent_rows > 0 && q.analytics_rows < q.sent_rows / 2) {
    out.push(`Open/click tracking looks incomplete: <b>${q.analytics_rows}</b> <code>analytics_events</code> rows for <b>${q.sent_rows}</b> successful sends. Low open and click numbers may mean tracking is not recording, not that nobody opened.`);
  }
  out.push(`*Unsubscribes have no campaign key in the schema. Each one is attributed to the last email the user received before <code>unsubscribed_at</code> (inferred). Unattributed: <b>${q.unattributed_unsubscribes}</b> of ${q.unsubscribed_users}.`);
  if (q.suppression_rows > 0) {
    out.push(`<code>suppression_list</code> has <b>${q.suppression_rows}</b> rows; <b>${q.suppressed_not_marked_unsubscribed}</b> of them have no matching unsubscribed user. They are not counted as unsubscribes here until their <code>reason</code> values are confirmed.`);
  }
  out.push(`A send counts as successful when <code>send_log.status</code> is one of: ${data.sentStatuses.map((s) => `<code>${esc(s)}</code>`).join(', ')}. Observed statuses: ${data.statuses.map((s) => `<code>${esc(s.status)}</code> × ${s.rows}`).join(', ') || 'none'}.`);
  return `<ul class="issues">${out.map((i) => `<li>${i}</li>`).join('')}</ul>`;
}

function renderPage(data, { source = 'fixture', sourceLabel = '', generatedAt = new Date() } = {}) {
  const banner =
    source === 'live'
      ? `<div class="banner live">Live EMS database${sourceLabel ? ` · ${esc(sourceLabel)}` : ''} · read-only</div>`
      : `<div class="banner fixture">FIXTURE DATA — not live EMS. Synthetic rows on the real EMS schema, for layout and query testing only.</div>`;
  return `<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>EMS Campaign Status</title><style>${CSS}</style></head>
<body>
${banner}
<h1>Campaign status</h1>
<p class="sub">All email sequences · generated ${fmtDate(generatedAt)} UTC</p>
${kpis(data.totals)}
<h2>By campaign</h2>
${campaignTable(data.campaigns)}
<h2>Per-email breakdown</h2>
${emailTable(data.emails)}
<h2>Data notes</h2>
${issues(data)}
<p class="note">Opened and clicked count distinct recipients per row group. Rates are against successful sends.</p>
</body></html>`;
}

module.exports = { renderPage };
