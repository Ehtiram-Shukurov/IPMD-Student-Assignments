-- Local mirror of the EMS tables, for testing only. Never run against EMS.
--
-- Table and column names come from Gongju's read-only inspection of the live
-- EMS container on 2026-11-14. That inspection listed names only, so every
-- type below is inferred from the column name and may differ from production.
-- The queries in src/queries.js do not depend on these exact types.

CREATE TABLE users (
  id                 UUID PRIMARY KEY,
  email              TEXT NOT NULL,
  first_name         TEXT,
  source             TEXT,
  created_at         TIMESTAMPTZ,
  paid               BOOLEAN,
  donation_amount    NUMERIC,
  last_email_type    TEXT,
  scheduled_day      INTEGER,
  ab_variant         TEXT,
  engagement_score   NUMERIC,
  tags               JSONB,
  unsubscribed       BOOLEAN DEFAULT FALSE,
  unsubscribed_at    TIMESTAMPTZ,
  bounce_count       INTEGER DEFAULT 0,
  hard_bounced       BOOLEAN DEFAULT FALSE,
  last_opened_at     TIMESTAMPTZ,
  last_clicked_at    TIMESTAMPTZ,
  click_count        INTEGER DEFAULT 0,
  total_emails_sent  INTEGER DEFAULT 0,
  reserved           BOOLEAN,
  reserved_at        TIMESTAMPTZ
);

CREATE TABLE campaigns (
  id              SERIAL PRIMARY KEY,
  name            TEXT NOT NULL,
  status          TEXT,
  created_at      TIMESTAMPTZ,
  subject_a       TEXT,
  subject_b       TEXT,
  winner_variant  TEXT,
  total_sent      INTEGER DEFAULT 0,
  total_opened    INTEGER DEFAULT 0,
  total_clicked   INTEGER DEFAULT 0,
  total_donated   NUMERIC DEFAULT 0,
  revenue_usd     NUMERIC DEFAULT 0
);

CREATE TABLE send_log (
  id           SERIAL PRIMARY KEY,
  user_id      UUID,
  campaign_id  INTEGER,
  sent_at      TIMESTAMPTZ,
  email_type   TEXT,
  variant      TEXT,
  channel      TEXT,
  status       TEXT,
  error        TEXT
);

CREATE TABLE analytics_events (
  id           SERIAL PRIMARY KEY,
  user_id      UUID,
  campaign_id  INTEGER,
  sent_at      TIMESTAMPTZ,
  email_type   TEXT,
  variant      TEXT,
  channel      TEXT,
  status       TEXT,
  opened_at    TIMESTAMPTZ,
  clicked_at   TIMESTAMPTZ,
  error        TEXT
);

CREATE TABLE suppression_list (
  id        SERIAL PRIMARY KEY,
  email     TEXT NOT NULL,
  reason    TEXT,
  added_at  TIMESTAMPTZ,
  source    TEXT
);
