'use strict';

// Renders the view against the synthetic fixture and writes preview/index.html.
// Also regenerates sql/campaign_status.sql from src/queries.js so the file
// someone runs with psql always matches what the server runs.

const fs = require('node:fs');
const path = require('node:path');
const { openFixtureDb } = require('./fixture-db');
const q = require('../src/queries');
const { renderPage } = require('../src/render');

async function main() {
  const root = path.join(__dirname, '..');

  const sqlFile = [
    '-- Campaign-status queries for EMS. Read-only; safe to run on production.',
    '-- Generated from src/queries.js by `npm run preview`. Do not edit by hand.',
    '-- Usage: psql "$DATABASE_URL" -f sql/campaign_status.sql',
    '',
    'BEGIN READ ONLY;',
    '',
    '-- 1. One row per campaign (email sequence)',
    q.statusSql('campaign'),
    '',
    '-- 2. Per-email breakdown: campaign x email_type x variant',
    q.statusSql('email'),
    '',
    '-- 3. Data quality: tracking coverage, unattributed unsubscribes, suppression list',
    q.dataQualitySql,
    '',
    '-- 4. send_log.status values actually in use',
    q.statusBreakdownSql,
    '',
    '-- 5. suppression_list.reason values actually in use',
    q.suppressionReasonsSql,
    '',
    'COMMIT;',
    '',
  ].join('\n');
  fs.writeFileSync(path.join(root, 'sql', 'campaign_status.sql'), sqlFile);

  const db = await openFixtureDb();
  const data = await q.loadCampaignStatus((sql) => db.query(sql));
  fs.mkdirSync(path.join(root, 'preview'), { recursive: true });
  fs.writeFileSync(path.join(root, 'preview', 'index.html'), renderPage(data, { source: 'fixture' }));
  fs.writeFileSync(path.join(root, 'preview', 'status.json'), JSON.stringify(data, null, 2) + '\n');
  console.log(JSON.stringify(data.totals));
  await db.close();
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
