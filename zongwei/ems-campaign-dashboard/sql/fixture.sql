-- SYNTHETIC test data on the EMS schema. These are NOT EMS results.
--
-- Shaped after what Gongju reported about the live database (5 campaigns, all
-- status 'sent', stored totals all 0, analytics_events nearly empty), so the
-- view's data-quality warnings can be exercised. Every user, send and event
-- here is made up; emails use the reserved example.test domain.

INSERT INTO campaigns (id, name, status, created_at, subject_a, subject_b, winner_variant) VALUES
  (1, 'still_live',          'sent', '2026-10-20T16:00Z', 'Still live?',          'We are still here',   NULL),
  (2, 'we_are_live',         'sent', '2026-10-22T16:00Z', 'We are live',          'It is happening',     'a'),
  (3, 'vip_private_link',    'sent', '2026-10-25T16:00Z', 'Your private link',    'VIP access inside',   NULL),
  (4, 'launch_instructions', 'sent', '2026-10-28T16:00Z', 'How to back us',       'Launch day steps',    NULL),
  (5, 'follow_kickstarter',  'sent', '2026-11-01T16:00Z', 'Follow us',            'One click to follow', NULL);

INSERT INTO users (id, email, first_name, created_at, unsubscribed, unsubscribed_at) VALUES
  ('00000000-0000-4000-8000-000000000001', 'u1@example.test', 'Test1', '2026-10-01T00:00Z', FALSE, NULL),
  ('00000000-0000-4000-8000-000000000002', 'u2@example.test', 'Test2', '2026-10-01T00:00Z', FALSE, NULL),
  ('00000000-0000-4000-8000-000000000003', 'u3@example.test', 'Test3', '2026-10-01T00:00Z', TRUE,  '2026-10-23T09:00Z'),
  ('00000000-0000-4000-8000-000000000004', 'u4@example.test', 'Test4', '2026-10-01T00:00Z', FALSE, NULL),
  ('00000000-0000-4000-8000-000000000005', 'u5@example.test', 'Test5', '2026-10-01T00:00Z', TRUE,  '2026-10-29T12:00Z'),
  ('00000000-0000-4000-8000-000000000006', 'u6@example.test', 'Test6', '2026-10-01T00:00Z', FALSE, NULL),
  ('00000000-0000-4000-8000-000000000007', 'u7@example.test', 'Test7', '2026-10-01T00:00Z', FALSE, NULL),
  ('00000000-0000-4000-8000-000000000008', 'u8@example.test', 'Test8', '2026-10-01T00:00Z', TRUE,  '2026-10-10T00:00Z');

-- Campaigns 1-4 go to users 1-6; campaign 5 goes to users 1-4. Campaign 2 is
-- an A/B test. A few sends fail. User 8 unsubscribed before any send, so that
-- unsubscribe cannot be attributed to a campaign.
INSERT INTO send_log (user_id, campaign_id, sent_at, email_type, variant, channel, status, error)
SELECT ('00000000-0000-4000-8000-00000000000' || u)::uuid,
       c,
       ('2026-10-20T16:00Z'::timestamptz + ((c - 1) * 3 || ' days')::interval + (u || ' minutes')::interval),
       CASE c WHEN 1 THEN 'reminder' WHEN 2 THEN 'announcement' WHEN 3 THEN 'vip' WHEN 4 THEN 'instructions' ELSE 'follow' END,
       CASE WHEN c = 2 THEN CASE WHEN u % 2 = 0 THEN 'b' ELSE 'a' END ELSE 'a' END,
       'email',
       CASE WHEN (c = 3 AND u = 6) OR (c = 4 AND u = 2) THEN 'failed' ELSE 'sent' END,
       CASE WHEN (c = 3 AND u = 6) OR (c = 4 AND u = 2) THEN 'SMTP 550 mailbox unavailable' ELSE NULL END
FROM generate_series(1, 5) AS c, generate_series(1, 6) AS u
WHERE NOT (c = 5 AND u > 4);

-- Sparse tracking, like the live database: only a handful of events.
INSERT INTO analytics_events (user_id, campaign_id, sent_at, email_type, variant, channel, status, opened_at, clicked_at) VALUES
  ('00000000-0000-4000-8000-000000000001', 2, '2026-10-23T16:01Z', 'announcement', 'a', 'email', 'opened',  '2026-10-23T18:00Z', '2026-10-23T18:02Z'),
  ('00000000-0000-4000-8000-000000000002', 2, '2026-10-23T16:02Z', 'announcement', 'b', 'email', 'opened',  '2026-10-23T19:00Z', NULL),
  ('00000000-0000-4000-8000-000000000004', 3, '2026-10-26T16:04Z', 'vip',          'a', 'email', 'opened',  '2026-10-26T20:00Z', NULL);

INSERT INTO suppression_list (email, reason, added_at, source) VALUES
  ('u3@example.test',      'unsubscribe', '2026-10-23T09:00Z', 'link'),
  ('u5@example.test',      'unsubscribe', '2026-10-29T12:00Z', 'link'),
  ('bounce1@example.test', 'hard_bounce', '2026-10-21T00:00Z', 'smtp'),
  ('bounce2@example.test', 'hard_bounce', '2026-10-21T00:00Z', 'smtp'),
  ('import@example.test',  'imported',    '2026-10-01T00:00Z', 'csv');
