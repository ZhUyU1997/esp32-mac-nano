#!/usr/bin/env node
/* Telnet-bash server with a real pty (full interactive bash).
 *
 * Usage:
 *   node telnet_bash_srv.js [port]                    interactive bash (default)
 *   node telnet_bash_srv.js [port] --test [pattern]   fixed-output benchmark
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
 * Requires: npm install node-pty
 */
const net = require('net');
const pty = require('node-pty');
const fs = require('fs');

const args = process.argv.slice(2);
let port = 2324;
let testMode = false;
let testPattern = 'fill';
let replayFile = null;
for (let i = 0; i < args.length; i++) {
	if (args[i] === '--test') {
		testMode = true;
		if (args[i + 1] && !args[i + 1].startsWith('--'))
			testPattern = args[++i];
	} else if (args[i] === '--replay') {
		replayFile = args[++i];
	} else if (/^\d+$/.test(args[i])) {
		port = parseInt(args[i], 10);
	}
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
	let out = '\x1b[2J\x1b[H';
	for (let row = 0; row < 30; row++) {
		let line = '';
		for (let col = 0; col < 80; col++)
			line += String.fromCharCode(65 + ((row0 + row) * 5 + col) % 26);
		out += line + '\r\n';
	}
	return out;
}

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
	console.log('listening on :' + port + (testMode ? ' (test: ' + testPattern + ')' : ''));
});
