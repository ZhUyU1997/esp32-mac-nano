/* decode-worker.js — offload PNG decode (sharp/libvips) + cellDiff to a
 * worker thread: both are CPU-bound and would otherwise serialize on the
 * main event loop of the test-art worker pool. */
'use strict';
const { parentPort } = require('worker_threads');
const diff = require('./lib/diff-lib');

parentPort.on('message', async (m) => {
	try {
		if (m.kind === 'decode') {
			const img = await diff.decodePngRgbAsync(Buffer.from(m.buf));
			parentPort.postMessage({ ok: true, kind: 'decode', id: m.id,
			                         w: img.w, h: img.h, data: img.data.buffer },
			                        [img.data.buffer]);
		} else if (m.kind === 'compare') {
			const v = await diff.decodePngRgbAsync(Buffer.from(m.vBuf));
			const a = await diff.decodePngRgbAsync(Buffer.from(m.aBuf));
			const cmp = diff.cellDiff(v, a, !!m.withMap);
			parentPort.postMessage({ ok: true, kind: 'compare', id: m.id,
			                         cols: cmp.cols, rows: cmp.rows, diff: cmp.diff,
			                         total: cmp.total, rate: cmp.rate, bright: cmp.bright,
			                         map: cmp.map ? cmp.map.buffer : null },
			                        cmp.map ? [cmp.map.buffer] : []);
		}
	} catch (e) {
		parentPort.postMessage({ ok: false, kind: m.kind, id: m.id, error: e.message });
	}
});
