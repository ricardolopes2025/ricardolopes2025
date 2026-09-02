#!/usr/bin/env node
const fs = require('fs');
const path = require('path');
const { Octokit } = require('@octokit/rest');

const token = process.env.GITHUB_TOKEN;
const owner = process.env.GITHUB_ACTOR || 'ricardolopes2025';
if (!token) {
  console.warn('Warning: GITHUB_TOKEN not provided; unauthenticated requests may be rate-limited.');
}
const octokit = new Octokit({ auth: token });

function escapeXml(s) {
  return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
}

(async function main() {
  try {
    console.log('Listing repos for', owner);
    const repos = await octokit.paginate(octokit.repos.listForUser, { username: owner, per_page: 100 });
    const filtered = repos.filter(r => !r.fork && !r.archived && r.visibility !== 'private');

    const langTotals = {};
    for (const repo of filtered) {
      console.log('Fetching languages for', repo.name);
      const { data } = await octokit.repos.listLanguages({ owner, repo: repo.name });
      for (const [lang, bytes] of Object.entries(data || {})) {
        langTotals[lang] = (langTotals[lang] || 0) + bytes;
      }
    }

    const totalBytes = Object.values(langTotals).reduce((a, b) => a + b, 0);
    if (totalBytes === 0) {
      const emptySvg = `<svg xmlns="http://www.w3.org/2000/svg" width="480" height="120">
  <style>text{font-family:Arial, Helvetica, sans-serif; fill:#ff6b92}</style>
  <rect width="100%" height="100%" fill="#0b1221"/>
  <text x="24" y="62" font-size="20">Most Used Languages</text>
  <text x="24" y="92" font-size="14">No languages data.</text>
</svg>`;
      fs.mkdirSync(path.dirname('dist/top-langs.svg'), { recursive: true });
      fs.writeFileSync('dist/top-langs.svg', emptySvg, 'utf8');
      console.log('Wrote dist/top-langs.svg (no data)');
      return;
    }

    const sorted = Object.entries(langTotals).sort((a, b) => b[1] - a[1]).slice(0, 8);
    const width = 480;
    const rowHeight = 22;
    const height = 48 + sorted.length * rowHeight;

    const rows = sorted.map(([lang, bytes], i) => {
      const pct = (bytes / totalBytes) * 100;
      const barWidth = Math.max(1, Math.round((pct / 100) * (width - 220)));
      const y = 40 + i * rowHeight;
      return {
        lang: escapeXml(lang),
        pct: pct.toFixed(1),
        barWidth,
        y
      };
    });

    const barsSvg = rows.map(r => `
  <text x="16" y="${r.y + 14}" font-size="12" fill="#fff">${r.lang}</text>
  <rect x="140" y="${r.y + 2}" width="${r.barWidth}" height="14" fill="#ff6b92" rx="3" ry="3"></rect>
  <text x="${140 + (width - 220) + 8}" y="${r.y + 14}" font-size="12" fill="#cbd5e1">${r.pct}%</text>
`).join('\n');

    const svg = `<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="${width}" height="${height}">
  <rect width="100%" height="100%" fill="#0b1221"/>
  <g font-family="Arial, Helvetica, sans-serif">
    <text x="16" y="24" font-size="18" fill="#ff6b92">Most Used Languages</text>
    ${barsSvg}
  </g>
</svg>`;

    fs.mkdirSync(path.dirname('dist/top-langs.svg'), { recursive: true });
    fs.writeFileSync('dist/top-langs.svg', svg, 'utf8');
    console.log('Wrote dist/top-langs.svg');
  } catch (err) {
    console.error('Error:', err);
    process.exit(1);
  }
})();