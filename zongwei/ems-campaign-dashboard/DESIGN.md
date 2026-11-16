# EMS campaign-status view: design draft

Status: draft, written before access to the EMS code and database. Table and column names below are placeholders to be replaced once the real schema arrives.

## What the screen shows

One screen, read top to bottom.

1. Totals row for the selected sequence, or all sequences: sent, opened, clicked, unsubscribed, plus open rate and click rate.
2. Sequence table: one row per email sequence with the same four counts and two rates. Clicking a row filters the section below.
3. Per-email breakdown: one row per email step in the sequence, in send order, with the same counts.

No charts in v1. Numbers first, plain table styling.

## Counting rules

- Sent: messages the engine handed off successfully. Exclude failed or bounced sends if the schema separates them.
- Opened and clicked: count distinct recipients per email, not raw events, so repeated opens do not inflate the number. Show raw event counts only if asked.
- Unsubscribed: distinct recipients who unsubscribed and whose unsubscribe is attributed to that email or sequence.
- Rates: opened / sent and clicked / sent, computed in the query, shown as percentages.

## Query sketch

Assumes an events table keyed by message, and messages linked to an email step and a sequence. Names are placeholders.

```sql
SELECT
  s.id    AS sequence_id,
  s.name  AS sequence_name,
  COUNT(DISTINCT m.id) FILTER (WHERE m.status = 'sent')                       AS sent,
  COUNT(DISTINCT m.recipient_id) FILTER (WHERE e.type = 'open')               AS opened,
  COUNT(DISTINCT m.recipient_id) FILTER (WHERE e.type = 'click')              AS clicked,
  COUNT(DISTINCT m.recipient_id) FILTER (WHERE e.type = 'unsubscribe')        AS unsubscribed
FROM sequences s
JOIN email_steps st ON st.sequence_id = s.id
JOIN messages    m  ON m.step_id      = st.id
LEFT JOIN events e  ON e.message_id   = m.id
GROUP BY s.id, s.name
ORDER BY s.name;
```

The per-email breakdown is the same query grouped by `st.id` within one sequence, ordered by step position.

## Implementation plan once access lands

1. Read the EMS schema and map the placeholders to real tables and event types.
2. Add one read-only admin endpoint, for example `GET /admin/campaigns/status?sequence_id=`, returning totals, per-sequence rows and per-email rows.
3. Add the admin page in the EMS's existing frontend stack.
4. Verify the counts against a few manual queries on the real database, then take the screenshot for the report.

## Open questions for Qingzhe

- Which table records sends, and how are failed sends and bounces marked?
- Are opens and clicks stored as events per message, or as counters?
- How is an unsubscribe linked to an email or sequence?
- Is there an existing admin auth guard the new endpoint should use?
