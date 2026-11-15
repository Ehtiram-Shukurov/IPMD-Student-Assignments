'use strict';

// Read-only campaign-status view for the EMS admin dashboard.
//
//   DATABASE_URL=postgres://readonly_user@host/db node server.js
//
//   GET /admin/campaigns/status       -> JSON
//   GET /admin/campaigns              -> HTML view
//
// Binds to 127.0.0.1 by default because it has no auth of its own. To mount it
// inside the EMS Node service instead, reuse createHandler() behind the
// existing admin auth guard.

const http = require('node:http');
const { Pool } = require('pg');
const { loadCampaignStatus } = require('./src/queries');
const { renderPage } = require('./src/render');

function createHandler(pool, { sourceLabel = '' } = {}) {
  return async function handler(req, res) {
    const url = new URL(req.url, 'http://localhost');
    if (req.method !== 'GET' || !['/admin/campaigns', '/admin/campaigns/status'].includes(url.pathname)) {
      res.writeHead(404, { 'content-type': 'text/plain' }).end('not found');
      return;
    }
    const client = await pool.connect();
    try {
      const data = await loadCampaignStatus((sql) => client.query(sql));
      if (url.pathname === '/admin/campaigns/status') {
        res.writeHead(200, { 'content-type': 'application/json' }).end(JSON.stringify(data, null, 2));
      } else {
        res.writeHead(200, { 'content-type': 'text/html; charset=utf-8' })
          .end(renderPage(data, { source: 'live', sourceLabel }));
      }
    } catch (err) {
      console.error(err);
      res.writeHead(500, { 'content-type': 'text/plain' }).end('query failed');
    } finally {
      client.release();
    }
  };
}

if (require.main === module) {
  if (!process.env.DATABASE_URL) {
    console.error('Set DATABASE_URL to a read-only EMS connection string.');
    process.exit(1);
  }
  // Belt and braces: every session is read-only even if the role is not.
  const pool = new Pool({
    connectionString: process.env.DATABASE_URL,
    options: '-c default_transaction_read_only=on',
    max: 2,
  });
  const host = process.env.HOST || '127.0.0.1';
  const port = Number(process.env.PORT || 4010);
  const label = new URL(process.env.DATABASE_URL).hostname;
  http.createServer(createHandler(pool, { sourceLabel: label })).listen(port, host, () => {
    console.log(`campaign status: http://${host}:${port}/admin/campaigns`);
  });
}

module.exports = { createHandler };
