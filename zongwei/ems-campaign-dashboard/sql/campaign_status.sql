-- Campaign-status queries for EMS. Read-only; safe to run on production.
-- Generated from src/queries.js by `npm run preview`. Do not edit by hand.
-- Usage: psql "$DATABASE_URL" -f sql/campaign_status.sql

BEGIN READ ONLY;

-- 1. One row per campaign (email sequence)
WITH sends AS (
  SELECT campaign_id,
         COUNT(*) FILTER (WHERE LOWER(status) IN ('sent', 'delivered', 'success'))                        AS sent,
         COUNT(*) FILTER (WHERE status IS NULL OR LOWER(status) NOT IN ('sent', 'delivered', 'success'))  AS not_sent,
         MIN(sent_at)                                                                  AS first_sent_at,
         MAX(sent_at)                                                                  AS last_sent_at
  FROM send_log
  GROUP BY 1
),
events AS (
  SELECT campaign_id,
         COUNT(DISTINCT user_id) FILTER (WHERE opened_at  IS NOT NULL) AS opened,
         COUNT(DISTINCT user_id) FILTER (WHERE clicked_at IS NOT NULL) AS clicked,
         COUNT(*)                                                     AS tracked_rows
  FROM analytics_events
  GROUP BY 1
),
-- Unsubscribes carry no campaign key. Each one is attributed to the last
-- send the user received at or before unsubscribed_at (inferred, not logged).
unsub AS (
  SELECT last.campaign_id,
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
  GROUP BY 1
)
SELECT k.campaign_id,
       c.name                         AS campaign_name,
       c.status                       AS campaign_status,
       COALESCE(sends.sent, 0)        AS sent,
       COALESCE(sends.not_sent, 0)    AS not_sent,
       COALESCE(events.opened, 0)     AS opened,
       COALESCE(events.clicked, 0)    AS clicked,
       COALESCE(unsub.unsubscribed, 0) AS unsubscribed,
       COALESCE(events.tracked_rows, 0) AS tracked_rows,
       sends.first_sent_at,
       sends.last_sent_at,
       c.total_sent    AS stored_total_sent,
       c.total_opened  AS stored_total_opened,
       c.total_clicked AS stored_total_clicked
FROM (
  SELECT campaign_id FROM sends
  UNION SELECT campaign_id FROM events
  UNION SELECT campaign_id FROM unsub
  UNION SELECT id AS campaign_id FROM campaigns
) k
LEFT JOIN sends  USING (campaign_id)
LEFT JOIN events USING (campaign_id)
LEFT JOIN unsub  USING (campaign_id)
LEFT JOIN campaigns c ON c.id = k.campaign_id
ORDER BY c.name NULLS LAST;

-- 2. Per-email breakdown: campaign x email_type x variant
WITH sends AS (
  SELECT campaign_id, COALESCE(email_type, '(none)') AS email_type, COALESCE(variant, '(none)') AS variant,
         COUNT(*) FILTER (WHERE LOWER(status) IN ('sent', 'delivered', 'success'))                        AS sent,
         COUNT(*) FILTER (WHERE status IS NULL OR LOWER(status) NOT IN ('sent', 'delivered', 'success'))  AS not_sent,
         MIN(sent_at)                                                                  AS first_sent_at,
         MAX(sent_at)                                                                  AS last_sent_at
  FROM send_log
  GROUP BY 1, 2, 3
),
events AS (
  SELECT campaign_id, COALESCE(email_type, '(none)') AS email_type, COALESCE(variant, '(none)') AS variant,
         COUNT(DISTINCT user_id) FILTER (WHERE opened_at  IS NOT NULL) AS opened,
         COUNT(DISTINCT user_id) FILTER (WHERE clicked_at IS NOT NULL) AS clicked,
         COUNT(*)                                                     AS tracked_rows
  FROM analytics_events
  GROUP BY 1, 2, 3
),
-- Unsubscribes carry no campaign key. Each one is attributed to the last
-- send the user received at or before unsubscribed_at (inferred, not logged).
unsub AS (
  SELECT last.campaign_id, COALESCE(last.email_type, '(none)') AS email_type, COALESCE(last.variant, '(none)') AS variant,
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
  GROUP BY 1, 2, 3
)
SELECT k.campaign_id, k.email_type, k.variant,
       c.name                         AS campaign_name,
       c.status                       AS campaign_status,
       COALESCE(sends.sent, 0)        AS sent,
       COALESCE(sends.not_sent, 0)    AS not_sent,
       COALESCE(events.opened, 0)     AS opened,
       COALESCE(events.clicked, 0)    AS clicked,
       COALESCE(unsub.unsubscribed, 0) AS unsubscribed,
       COALESCE(events.tracked_rows, 0) AS tracked_rows,
       sends.first_sent_at,
       sends.last_sent_at
FROM (
  SELECT campaign_id, email_type, variant FROM sends
  UNION SELECT campaign_id, email_type, variant FROM events
  UNION SELECT campaign_id, email_type, variant FROM unsub
) k
LEFT JOIN sends  USING (campaign_id, email_type, variant)
LEFT JOIN events USING (campaign_id, email_type, variant)
LEFT JOIN unsub  USING (campaign_id, email_type, variant)
LEFT JOIN campaigns c ON c.id = k.campaign_id
ORDER BY c.name NULLS LAST, k.email_type, k.variant;

-- 3. Data quality: tracking coverage, unattributed unsubscribes, suppression list
SELECT
  (SELECT COUNT(*) FROM send_log)                                              AS send_log_rows,
  (SELECT COUNT(*) FROM send_log WHERE LOWER(status) IN ('sent', 'delivered', 'success'))         AS sent_rows,
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

-- 4. send_log.status values actually in use
SELECT COALESCE(status, '(null)') AS status, COUNT(*) AS rows
FROM send_log GROUP BY 1 ORDER BY 2 DESC;

-- 5. suppression_list.reason values actually in use
SELECT COALESCE(reason, '(null)') AS reason, COUNT(*) AS rows
FROM suppression_list GROUP BY 1 ORDER BY 2 DESC;

COMMIT;
