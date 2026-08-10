// gzip the two built single-file pages for embedding in firmware.
// The device serves them with Content-Encoding: gzip — browsers and the
// iOS CNA WebSheet transparently decompress.
import { gzipSync } from 'node:zlib';
import { readFileSync, writeFileSync } from 'node:fs';

for (const f of ['dist/index.html', 'dist/provision.html']) {
  const raw = readFileSync(f);
  const gz = gzipSync(raw, { level: 9 });
  writeFileSync(f + '.gz', gz);
  console.log(`${f}.gz ${raw.length} -> ${gz.length} bytes`);
}
