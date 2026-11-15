'use strict';

// Read-only queries behind the EMS campaign-status view.
//
// Table and column names follow the live EMS schema as inspected by Gongju on
// 2026-11-14 (users, campaigns, send_log, analytics_events, suppression_list).
// Column types and status values were not part of that inspection, so the
// queries avoid depending on them: keys are compared as-is and send statuses
// are configurable.

// send_log.status values that count as a successful send. Unknown until
// someone with DB access runs statusBreakdownSql; adjust here if needed.
const SENT_STATUSES = ['sent', 'delivered', 'success'];

const sentList = SENT_STATUSES.map((s) => `'${s}'`).join(', ');

// Normalised grouping keys, so NULL email_type / variant still join.
const KEY = {
  campaign: ['campaign_id'],
  email: ['campaign_id', 'email_type', 'variant'],
};

function keyCols(level, alias) {
  const p = alias ? `${alias}.` : '';
  return KEY[level]
    .map((k) => (k === 'campaign_id' ? `${p}campaign_id` : `COALESCE(${p}${k}, '(none)') AS ${k}`))
    .join(', ');
}

function groupBy(level) {
  return KEY[level].map((_, i) => i + 1).join(', ');
}

// One status query per level. Each source table is aggregated on its own
// before joining, so rows never multiply across tables.
function statusSql(level) {
  if (!KEY[level]) throw new Error(`unknown level: ${level}`);
  const keys = KEY[level];
  const using = keys.join(', ');

  return `
WITH sends AS (
  SELECT ${keyCols(level)},
         COUNT(*) FILTER (WHERE LOWER(status) IN (${sentList}))                        AS sent,
         COUNT(*) FILTER (WHERE status IS NULL OR LOWER(status) NOT IN (${sentList}))  AS not_sent,
         MIN(sent_at)                                                                  AS first_sent_at,
         MAX(sent_at)                                                                  AS last_sent_at
  FROM send_log
  GROUP BY ${groupBy(level)}
),
events AS (
  SELECT ${keyCols(level)},
         COUNT(DISTINCT user_id) FILTER (WHERE opened_at  IS NOT NULL) AS opened,
         COUNT(DISTINCT user_id) FILTER (WHERE clicked_at IS NOT NULL) AS clicked,
         COUNT(*)                                                     AS tracked_rows
  FROM analytics_events
  GROUP BY ${groupBy(level)}
),
-- Unsubscribes carry no campaign key. Each one is attributed to the last
-- send the user received at or before unsubscribed_at (inferred, not logged).
unsub AS (
  SELECT ${keyCols(level, 'last')},
         COUNT(DISTINCT u.id) AS unsubscribed
  FROM users u
  JOIN LATERAL (
    SELECT s.campaign_id, s.email_type, s.variant
    FROM send_log s
    WHERE s.user_id = u.id AND s.sent_at <= u.unsubscribed_at
    ORDER BY s.sent_at DESC
    LIMIT 1
  ) last ON TRUE
  WHERE u.unsubscribed IS TRUE AND u.unsubscribed_at IS NOT NULL
  GROUP BY ${groupBy(level)}
)
SELECT ${keys.map((k) => `k.${k}`).join(', ')},
       c.name                         AS campaign_name,
       c.status                       AS campaign_status,
       COALESCE(sends.sent, 0)        AS sent,
       COALESCE(sends.not_sent, 0)    AS not_sent,
       COALESCE(events.opened, 0)     AS opened,
       COALESCE(events.clicked, 0)    AS clicked,
       COALESCE(unsub.unsubscribed, 0) AS unsubscribed,
       COALESCE(events.tracked_rows, 0) AS tracked_rows,
       sends.first_sent_at,
       sends.last_sent_at${level === 'campaign' ? `,
       c.total_sent    AS stored_total_sent,
       c.total_opened  AS stored_total_opened,
       c.total_clicked AS stored_total_clicked` : ''}
FROM (
  SELECT ${using} FROM sends
  UNION SELECT ${using} FROM events
  UNION SELECT ${using} FROM unsub${level === 'campaign' ? `
  UNION SELECT id AS campaign_id FROM campaigns` : ''}
) k
LEFT JOIN sends  USING (${using})
LEFT JOIN events USING (${using})
LEFT JOIN unsub  USING (${using})
LEFT JOIN campaigns c ON c.id = k.campaign_id
ORDER BY c.name NULLS LAST${level === 'email' ? ', k.email_type, k.variant' : ''};
`.trim();
}

// Facts the view needs to show alongside the counts, so nobody mistakes a
// data gap for a campaign result.
const dataQualitySql = `
SELECT
  (SELECT COUNT(*) FROM send_log)                                              AS send_log_rows,
  (SELECT COUNT(*) FROM send_log WHERE LOWER(status) IN (${sentList}))         AS sent_rows,
  (SELECT COUNT(*) FROM analytics_events)                                      AS analytics_rows,
  (SELECT COUNT(*) FROM users WHERE unsubscribed IS TRUE)                      AS unsubscribed_users,
  (SELECT COUNT(*) FROM users u
     WHERE u.unsubscribed IS TRUE
       AND NOT EXISTS (SELECT 1 FROM send_log s
                       WHERE s.user_id = u.id
                         AND u.unsubscribed_at IS NOT NULL
                         AND s.sent_at <= u.unsubscribed_at))                  AS unattributed_unsubscribes,
  (SELECT COUNT(*) FROM suppression_list)                                      AS suppression_rows,
  (SELECT COUNT(*) FROM suppression_list sl
     WHERE NOT EXISTS (SELECT 1 FROM users u
                       WHERE LOWER(u.email) = LOWER(sl.email)
                         AND u.unsubscribed IS TRUE))                          AS suppressed_not_marked_unsubscribed;
`.trim();

const statusBreakdownSql = `
SELECT COALESCE(status, '(null)') AS status, COUNT(*) AS rows
FROM send_log GROUP BY 1 ORDER BY 2 DESC;
`.trim();

const suppressionReasonsSql = `
SELECT COALESCE(reason, '(null)') AS reason, COUNT(*) AS rows
FROM suppression_list GROUP BY 1 ORDER BY 2 DESC;
`.trim();

const toInt = (v) => (v === null || v === undefined ? 0 : Number(v));

const COUNT_FIELDS = [
  'sent', 'not_sent', 'opened', 'clicked', 'unsubscribed', 'tracked_rows',
  'stored_total_sent', 'stored_total_opened', 'stored_total_clicked',
];

function normalise(row) {
  const out = { ...row };
  for (const f of COUNT_FIELDS) if (f in out) out[f] = toInt(out[f]);
  return out;
}

// `query` is any function (sql) => Promise<{ rows }>, e.g. pg's client.query
// or PGlite's db.query. Everything runs inside one READ ONLY transaction.
async function loadCampaignStatus(query) {
  await query('BEGIN READ ONLY');
  try {
    const campaigns = (await query(statusSql('campaign'))).rows.map(normalise);
    const emails = (await query(statusSql('email'))).rows.map(normalise);
    const quality = normalise((await query(dataQualitySql)).rows[0]);
    for (const k of Object.keys(quality)) quality[k] = toInt(quality[k]);
    const statuses = (await query(statusBreakdownSql)).rows.map((r) => ({ ...r, rows: toInt(r.rows) }));
    const reasons = (await query(suppressionReasonsSql)).rows.map((r) => ({ ...r, rows: toInt(r.rows) }));
    await query('COMMIT');

    for (const c of campaigns) {
      c.stored_totals_stale =
        c.stored_total_sent !== c.sent ||
        c.stored_total_opened !== c.opened ||
        c.stored_total_clicked !== c.clicked;
    }

    const sum = (f) => campaigns.reduce((a, c) => a + c[f], 0);
    const totals = {
      sent: sum('sent'),
      not_sent: sum('not_sent'),
      opened: sum('opened'),
      clicked: sum('clicked'),
      unsubscribed: sum('unsubscribed') + quality.unattributed_unsubscribes,
      unsubscribed_attributed: sum('unsubscribed'),
      unsubscribed_unattributed: quality.unattributed_unsubscribes,
    };

    return { totals, campaigns, emails, quality, statuses, reasons, sentStatuses: SENT_STATUSES };
  } catch (err) {
    await query('ROLLBACK').catch(() => {});
    throw err;
  }
}

module.exports = {
  SENT_STATUSES,
  statusSql,
  dataQualitySql,
  statusBreakdownSql,
  suppressionReasonsSql,
  loadCampaignStatus,
};
