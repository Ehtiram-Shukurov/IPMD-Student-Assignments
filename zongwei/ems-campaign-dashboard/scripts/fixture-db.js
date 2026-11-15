'use strict';

// In-process Postgres (PGlite) loaded with sql/schema.sql and sql/fixture.sql.
// Used by the tests and the preview; needs no server and no network.

const fs = require('node:fs');
const path = require('node:path');

async function openFixtureDb() {
  const { PGlite } = await import('@electric-sql/pglite');
  const db = new PGlite();
  const sqlDir = path.join(__dirname, '..', 'sql');
  await db.exec(fs.readFileSync(path.join(sqlDir, 'schema.sql'), 'utf8'));
  await db.exec(fs.readFileSync(path.join(sqlDir, 'fixture.sql'), 'utf8'));
  return db;
}

module.exports = { openFixtureDb };
