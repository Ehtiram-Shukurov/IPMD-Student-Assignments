'use strict';

// Checks the queries against the synthetic fixture, where every expected
// number can be worked out by hand from sql/fixture.sql.

const test = require('node:test');
const assert = require('node:assert/strict');
const { openFixtureDb } = require('../scripts/fixture-db');
const { loadCampaignStatus } = require('../src/queries');
const { renderPage } = require('../src/render');

let db;
let data;

test.before(async () => {
  db = await openFixtureDb();
  data = await loadCampaignStatus((sql) => db.query(sql));
});

test.after(async () => {
  await db.close();
});

const byName = (rows, name) => rows.filter((r) => r.campaign_name === name);

test('totals: 26 successful sends, 2 failed, 3 opens, 1 click, 3 unsubscribes', () => {
  assert.deepEqual(data.totals, {
    sent: 26,
    not_sent: 2,
    opened: 3,
    clicked: 1,
    unsubscribed: 3,
    unsubscribed_attributed: 2,
    unsubscribed_unattributed: 1,
  });
});

test('one row per campaign, sends counted from send_log', () => {
  const sent = Object.fromEntries(data.campaigns.map((c) => [c.campaign_name, c.sent]));
  assert.deepEqual(sent, {
    follow_kickstarter: 4,
    launch_instructions: 5,
    still_live: 6,
    vip_private_link: 5,
    we_are_live: 6,
  });
});

test('stored totals of 0 are flagged as stale, not reported as results', () => {
  assert.equal(data.campaigns.length, 5);
  assert.ok(data.campaigns.every((c) => c.stored_totals_stale));
  assert.ok(data.campaigns.every((c) => c.stored_total_sent === 0));
});

test('A/B variants are split in the per-email breakdown', () => {
  const rows = byName(data.emails, 'we_are_live');
  const v = Object.fromEntries(rows.map((r) => [r.variant, { sent: r.sent, opened: r.opened, clicked: r.clicked }]));
  assert.deepEqual(v, { a: { sent: 3, opened: 1, clicked: 1 }, b: { sent: 3, opened: 1, clicked: 0 } });
});

test('unsubscribe goes to the last email received before unsubscribing', () => {
  // u3 unsubscribed between still_live and we_are_live; u5 between vip and launch.
  assert.equal(byName(data.campaigns, 'still_live')[0].unsubscribed, 1);
  assert.equal(byName(data.campaigns, 'vip_private_link')[0].unsubscribed, 1);
  assert.equal(byName(data.campaigns, 'we_are_live')[0].unsubscribed, 0);
  assert.equal(data.quality.unattributed_unsubscribes, 1);
});

test('data quality facts', () => {
  assert.equal(data.quality.send_log_rows, 28);
  assert.equal(data.quality.sent_rows, 26);
  assert.equal(data.quality.analytics_rows, 3);
  assert.equal(data.quality.suppression_rows, 5);
  assert.equal(data.quality.suppressed_not_marked_unsubscribed, 3);
});

test('queries run inside a read-only transaction', async () => {
  await db.query('BEGIN READ ONLY');
  await assert.rejects(db.query("UPDATE campaigns SET total_sent = 1"), /read-only/);
  await db.query('ROLLBACK');
});

test('page shows the fixture banner and the warnings', () => {
  const html = renderPage(data, { source: 'fixture' });
  assert.match(html, /FIXTURE DATA — not live EMS/);
  assert.match(html, /5 of 5 campaigns/);
  assert.match(html, /tracking looks incomplete/);
  assert.match(html, /Unattributed: <b>1<\/b>/);
});
