#!/usr/bin/env node
/* Telnet-bash server with a real pty (full interactive bash).
 *
 * Usage:
 *   node telnet_bash_srv.js [port]                    interactive bash (default)
 *   node telnet_bash_srv.js [port] --test [pattern]   fixed-output benchmark
 *   node telnet_bash_srv.js [port] --art <dir|file>   ASCII-art gallery
 *   node telnet_bash_srv.js [port] --file <file>      single-file test mode
 *        (gallery options: --baud <rate> 90s dial-up pacing,
 *         --auto <sec> autoplay — advance after N s once streamed)
 *        (repeatable: merge several packs)
 *
 * Benchmark patterns (deterministic, for vterm render/blit profiling):
 *   fill   — clear + full 80x30 text screen every 500ms (full-render cost)
 *   scroll — one 80-char line every 50ms (scroll + partial-render cost)
 *   idle   — nothing (idle blit baseline)
 *   burst:<kind>:<mb> — send <mb> MiB of <kind> at full speed, then idle.
 *     kind = ascii | sgr | noscroll  (noscroll = CUP home, no scroll)
 *     Used to measure the telnet→ring→parse throughput end-to-end (no
 *     reflash needed: the ESP32 drain log reports parse/flush timing).
 *   --replay <file> — send a captured raw terminal stream (e.g. pi -c) on
 *     connect, as fast as possible. Deterministic A/B test: same bytes every
 *     run, so the ESP32 drain log is comparable across firmware versions.
 *
 * --art gallery mode: load every .ans/.ice/.utf8/.txt and .zip in the
 *   directory (or a single file/zip; 16colo.rs packs are stored as .zip
 *   and unzipped in memory with Node's zlib — no external tools), convert
 *   CP437 .ans/.ice to UTF-8 with an embedded table, strip SAUCE/COMNT
 *   records, skip pieces wider than 80 cols (they would wrap on the
 *   vterm), and cycle through them: the first piece is sent on connect.
 *   Gallery keys (the vterm sends arrow keys as ESC [ A-D):
 *     right/left — next/previous piece (ANS)
 *     up/down    — jump to the next/previous year (zip pack <year>/ prefix)
 *     any arrow  — leaves autoplay (--auto); Enter or space toggles
 *                  autoplay off/on (or, without --auto, Enter cycles to
 *                  the next piece).
 *
 * --file test mode: send ONE converted file on connect; Enter re-sends it
 *   (edit the file, press Enter to re-check on the vterm). No gallery
 *   navigation, no pack scanning.
 *
 * Requires: npm install node-pty
 */
const net = require('net');
const path = require('path');
const pty = require('node-pty');
const artlib = require('./art-lib');
const fs = require('fs');

const args = process.argv.slice(2);
let port = 2324;
let testMode = false;
let testPattern = 'fill';
let replayFile = null;
let artDirs = [];
let testFile = null;
let baudRate = 0;   /* 90s dial-up simulation: bytes/sec cap for art/file */
let autoSec = 0;    /* art autoplay: advance to the next piece after N s */
for (let i = 0; i < args.length; i++) {
	if (args[i] === '--test') {
		testMode = true;
		if (args[i + 1] && !args[i + 1].startsWith('--'))
			testPattern = args[++i];
	} else if (args[i] === '--replay') {
		replayFile = args[++i];
	} else if (args[i] === '--art') {
		artDirs.push(args[++i]);
	} else if (args[i] === '--file') {
		testFile = args[++i];
	} else if (args[i] === '--baud') {
		baudRate = parseInt(args[++i], 10) || 0;
	} else if (args[i] === '--auto') {
		autoSec = parseFloat(args[++i]) || 0;
	} else if (/^\d+$/.test(args[i])) {
		port = parseInt(args[i], 10);
	}
}

/* ---- ASCII-art gallery mode (--art) -------------------------------- */

/* Convert one art buffer: shared art-lib logic (strip SAUCE, CP437
 * -> UTF-8, cursor-simulated width check against the 80-column
 * gallery limit). Returns the piece, or null if it is too wide. */
function convertPiece(name, buf) {
	return artlib.convertPiece(name, buf, 80);
}

/* Pace a buffer out at 90s dial-up speed: --baud caps the byte rate
 * (8N1 serial = 10 bits per byte, so 9600 baud ≈ 960 B/s). baud <= 0
 * sends instantly; onDone fires when the whole buffer has been written.
 * Returns the interval handle (clear it to abort). */
function writePaced(sock, buf, baud, onDone) {
	if (baud <= 0) {
		sock.write(buf);
		if (onDone) onDone();
		return null;
	}
	const bytesPerSec = Math.floor(baud / 10);
	const chunk = Math.max(1, Math.floor(bytesPerSec / 20)); /* 50 ticks/s */
	let off = 0;
	const timer = setInterval(() => {
		const n = Math.min(chunk, buf.length - off);
		sock.write(buf.subarray(off, off + n));
		off += n;
		if (off >= buf.length) {
			clearInterval(timer);
			if (onDone) onDone();
		}
	}, 50);
	return timer;
}


/* deterministic byte pattern for a burst (all bytes < 256, latin1-safe) */
function makePattern(kind) {
	let s = '';
	if (kind === 'ascii') {
		let line = '';
		for (let col = 0; col < 80; col++)
			line += String.fromCharCode(65 + col % 26);
		s = line + '\r\n';
	} else {
		for (let i = 0; i < 40; i++) {
			if (kind === 'noscroll')
				s += '\x1b[1;1H';
			s += '\x1b[38;2;' + (i % 255) + ';' + ((i * 3) % 255) + ';' +
			     ((i * 5) % 255) + 'mword' + String(i).padStart(3, '0') +
			     ' \x1b[0m\x1b]8;;\x07';
			if (kind === 'sgr')
				s += '\r\r\n';
		}
	}
	return Buffer.from(s, 'latin1');
}

function fillScreen(row0) {
	/* 3J also clears the scrollback so scrolled-up views never show
	 * stale content from an earlier screen */
	let out = '\x1b[2J\x1b[3J\x1b[H';
	for (let row = 0; row < 30; row++) {
		let line = '';
		for (let col = 0; col < 80; col++)
			line += String.fromCharCode(65 + ((row0 + row) * 5 + col) % 26);
		out += line + '\r\n';
	}
	return out;
}

const artMode = artDirs.length > 0;
const artUnits = artMode ? artlib.collectUnits(artDirs) : null;
const fileMode = testFile !== null;
const filePiece = fileMode ? convertPiece(path.basename(testFile), fs.readFileSync(testFile)) : null;

const server = net.createServer((sock) => {
	if (replayFile) {
		console.log('client connected (replay: ' + replayFile + ')');
		const buf = fs.readFileSync(replayFile);
		console.log('replay: ' + buf.length + ' B');
		const t0 = Date.now();
		let off = 0;
		const writeNext = () => {
			while (off < buf.length) {
				if (!sock.write(buf.subarray(off, off + 4096))) {
					off += 4096;
					sock.once('drain', writeNext);
					return;
				}
				off += 4096;
			}
			const dt = Date.now() - t0;
			console.log('replay done: ' + buf.length + ' B in ' + dt + ' ms (' +
			            (buf.length / dt / 1000).toFixed(2) + ' MB/s, host-side)');
		};
		writeNext();
		sock.on('close', () => {});
		sock.on('error', () => {});
		return;
	}
	if (testMode) {
		console.log('client connected (test mode: ' + testPattern + ')');
		if (testPattern === 'idle') {
			sock.on('close', () => {});
			sock.on('error', () => {});
			return;
		}
		if (testPattern.startsWith('burst:')) {
			const parts = testPattern.split(':');
			const kind = parts[1] || 'ascii';
			const mb = parseFloat(parts[2] || '1');
			const buf = makePattern(kind);
			const total = Math.floor((mb * 1024 * 1024) / buf.length);
			console.log('burst ' + kind + ': pattern ' + buf.length + ' B x' + total +
			            ' = ' + (buf.length * total / 1048576).toFixed(2) + ' MiB');
			const t0 = Date.now();
			let i = 0;
			const writeNext = () => {
				while (i < total) {
					if (!sock.write(buf)) {
						i++;
						sock.once('drain', writeNext);
						return;
					}
					i++;
				}
				const dt = Date.now() - t0;
				console.log('burst done: ' + buf.length * total + ' B in ' + dt +
				            ' ms (' + (buf.length * total / dt / 1000).toFixed(1) + ' MB/s, host-side)');
			};
			writeNext();
			sock.on('close', () => {});
			sock.on('error', () => {});
			return;
		}
		let row0 = 0;
		const timer = testPattern === 'scroll'
			? setInterval(() => {
				let line = '';
				for (let col = 0; col < 80; col++)
					line += String.fromCharCode(65 + (row0 * 5 + col) % 26);
				sock.write(line + '\r\n');
				row0++;
			}, 50)
			: setInterval(() => {
				sock.write(fillScreen(row0));
				row0++;
			}, 500);
		sock.on('close', () => clearInterval(timer));
		sock.on('error', () => clearInterval(timer));
		return;
	}

	if (fileMode) {
		if (!filePiece) {
			console.log('client connected (file mode: piece skipped, nothing to send)');
			sock.on('close', () => {});
			sock.on('error', () => {});
			return;
		}
		console.log('client connected (file test: ' + filePiece.name + ' ' + filePiece.w + 'x' + filePiece.rows + ')');
		let streamTimer = null;
		const send = () => {
			if (streamTimer) {
				clearInterval(streamTimer);
				streamTimer = null;
			}
			sock.write('\x1b[2J\x1b[3J\x1b[H');
			streamTimer = writePaced(sock, filePiece.buf, baudRate);
		};
		send();
		/* Enter re-sends the (possibly edited) file */
		sock.on('data', (d) => {
			const s = d.toString('latin1');
			if (s.indexOf('\r') >= 0 || s.indexOf('\n') >= 0)
				send();
		});
		sock.on('close', () => {});
		sock.on('error', () => {});
		return;
	}

	if (artMode) {
		const nUnits = artUnits.length;
		if (nUnits === 0) {
			console.log('client connected (art gallery: no pieces found)');
			sock.on('close', () => {});
			sock.on('error', () => {});
			return;
		}
		console.log('client connected (art gallery: ' + nUnits + ' units)');
		let zipCache = null; /* the currently decompressed pack */
		let ui = 0;         /* current unit index */
		let pi = 0;         /* piece index within the unit */
		const loadPieces = (unit) => {
			/* decompress a pack lazily, keep only the current one */
			if (zipCache && zipCache.path === unit.path)
				return zipCache.pieces;
			console.log('art: pack ' + unit.name);
			const pieces = [];
			let entries;
			try {
				entries = artlib.readZip(fs.readFileSync(unit.path));
			} catch (e) {
				console.log('art: pack ' + unit.name + ' FAILED: ' + e.message);
				zipCache = { path: unit.path, pieces: [] };
				return zipCache.pieces;
			}
			for (const e of entries)
				if (/\.(ans|ice|utf8|txt)$/i.test(e.name)) {
					const p = convertPiece(path.basename(e.name), e.buf);
					if (p)
						pieces.push(p);
				}
			zipCache = { path: unit.path, pieces: pieces };
			return pieces;
		};
		const unitPieces = (unit) => {
			if (!unit.zip) {
				/* convert the single file lazily (width filter may drop it) */
				if (!unit.piece) {
					const p = convertPiece(unit.name, fs.readFileSync(unit.path));
					unit.piece = p || null;
				}
				return unit.piece ? [unit.piece] : [];
			}
			return loadPieces(unit);
		};
		/* advance past units with no displayable pieces (e.g. a pack whose
		 * entries all use an unsupported zip method); false if nothing left */
		const seekPiece = () => {
			for (let g = 0; g < nUnits; g++) {
				const unit = artUnits[ui];
				if (unitPieces(unit).length > 0)
					return true;
				ui = (ui + 1) % nUnits;
				pi = 0;
			}
			return false;
		};
		let streamTimer = null; /* paced-send interval (--baud), aborted on switch */
		let autoTimer = null;  /* autoplay countdown (--auto) */
		/* autoplay engaged; manual navigation (arrow keys) leaves it, Enter
		 * re-activates it */
		let autoActive = autoSec > 0;
		/* years of the zip packs, ascending; single files have no year */
		const years = [...new Set(artUnits.map(u => u.year).filter(y => y !== null))]
			.sort((a, b) => +a - +b);
		const pauseAuto = () => {
			if (!autoActive)
				return;
			autoActive = false;
			if (autoTimer) {
				clearTimeout(autoTimer);
				autoTimer = null;
			}
			console.log('art: autoplay off (manual)')
		};
		const resumeAuto = () => {
			if (autoSec <= 0)
				return;
			if (!autoActive)
				console.log('art: autoplay on');
			autoActive = true;
			/* restart the countdown from the current piece; if it is still
			 * streaming, show()'s completion callback registers the timer */
			if (autoTimer) {
				clearTimeout(autoTimer);
				autoTimer = null;
			}
			if (!streamTimer)
				autoTimer = setTimeout(next, autoSec * 1000);
		};
		/* Enter/space flips between paused and autoplay; false when the
		 * session has no --auto at all */
		const toggleAuto = () => {
			if (autoSec <= 0)
				return false;
			if (autoActive)
				pauseAuto();
			else
				resumeAuto();
			return true;
		};
		const show = () => {
			if (!seekPiece()) {
				console.log('art: no displayable pieces');
				return;
			}
			const unit = artUnits[ui];
			const pieces = unitPieces(unit);
			const p = pieces[Math.min(pi, pieces.length - 1)];
			console.log('art: show [' + (pi + 1) + '/' + pieces.length + '] ' + p.name +
			            ' (' + p.w + 'x' + p.rows + ')' +
			            (unit.year ? ' [' + unit.year + ']' : '') +
			            (unit.zip ? ' @' + unit.name : ''));
			if (streamTimer) {
				clearInterval(streamTimer);
				streamTimer = null;
			}
			if (autoTimer) {
				clearTimeout(autoTimer);
				autoTimer = null;
			}
			/* 3J clears the scrollback too: switching pieces must not leave
			 * the previous piece's scrolled-out rows behind (the vterm view
			 * reads them back on scroll-up, mixing old and new content) */
			sock.write('\x1b[2J\x1b[3J\x1b[H');
			streamTimer = writePaced(sock, p.buf, baudRate, () => {
				streamTimer = null;
				/* autoplay starts counting once the piece has fully streamed */
				if (autoSec > 0 && autoActive)
					autoTimer = setTimeout(next, autoSec * 1000);
			});
		};
		const next = () => {
			pi++;
			while (pi >= unitPieces(artUnits[ui]).length) {
				pi = 0;
				ui = (ui + 1) % nUnits;
			}
			show();
		};
		/* previous piece: step back, borrowing the last piece of the
		 * previous unit when the current one runs out */
		const prev = () => {
			pi--;
			for (let g = 0; g < nUnits; g++) {
				if (pi >= 0 && unitPieces(artUnits[ui]).length > 0)
					break; /* still inside the current unit */
				ui = (ui - 1 + nUnits) % nUnits;
				pi = unitPieces(artUnits[ui]).length - 1;
			}
			show();
		};
		/* jump to the first displayable unit of the neighbouring year; a
		 * unit without a year (single file) is not navigable by year */
		const jumpYear = (dir) => {
			const yi = years.indexOf(artUnits[ui].year);
			if (yi < 0)
				return;
			const target = years[(yi + dir + years.length) % years.length];
			for (let g = 0; g < nUnits; g++) {
				ui = (ui + 1) % nUnits;
				if (artUnits[ui].year === target && unitPieces(artUnits[ui]).length > 0) {
					pi = 0;
					show();
					return;
				}
			}
		};
		show();
		sock.on('data', (d) => {
			const s = d.toString('latin1');
			/* arrow keys navigate the gallery (right/left = next/prev
			 * piece, up/down = next/prev year) and leave autoplay; Enter
			 * or space toggles autoplay off/on (without --auto, Enter
			 * cycles to the next piece) */
			let i = 0;
			while (i < s.length) {
				if (s[i] === '\x1b' && i + 2 < s.length && s[i + 1] === '[' &&
				    'ABCD'.indexOf(s[i + 2]) >= 0) {
					switch (s[i + 2]) {
					case 'A': pauseAuto(); jumpYear(-1); break; /* up */
					case 'B': pauseAuto(); jumpYear(1); break;  /* down */
					case 'C': pauseAuto(); next(); break;       /* right */
					case 'D': pauseAuto(); prev(); break;       /* left */
					}
					i += 3;
				} else if (s[i] === '\r' || s[i] === '\n') {
					if (!toggleAuto())
						next(); /* no --auto: old Enter-advance behaviour */
					i++;
				} else if (s[i] === ' ') {
					toggleAuto();
					i++;
				} else {
					i++;
				}
			}
		});
		sock.on('close', () => {});
		sock.on('error', () => {});
		return;
	}

	console.log('client connected');
	const p = pty.spawn('/bin/bash', ['-i'], {
		name: 'xterm-256color',
		cols: 80,
		rows: 30,
		env: process.env,
	});
	p.onData((d) => {
		try { sock.write(d); } catch (e) {}
	});
	sock.on('data', (d) => p.write(d.toString('utf8')));
	sock.on('close', () => p.kill());
	sock.on('error', () => p.kill());
	p.onExit(() => {
		try { sock.destroy(); } catch (e) {}
	});
});

server.listen(port, '0.0.0.0', () => {
	let what = '';
	if (testMode)
		what = ' (test: ' + testPattern + ')';
	else if (fileMode && !filePiece) {
		console.log('--file piece was filtered out (too wide for the 80-col gallery); exiting');
		process.exit(1);
	} else if (fileMode)
		what = ' (file: ' + filePiece.name + ')';
	else if (artMode)
		what = ' (art gallery: ' + artUnits.length + ' units)';
	if (autoSec > 0)
		what += ' (autoplay ' + autoSec + 's)';
	if (baudRate > 0)
		what += ' (baud ' + baudRate + ')';
	console.log('listening on :' + port + what);
});
