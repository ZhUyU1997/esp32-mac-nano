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
 *
 * Requires: npm install node-pty
 */
const net = require('net');
const pty = require('node-pty');

const args = process.argv.slice(2);
let port = 2324;
let testMode = false;
let testPattern = 'fill';
for (let i = 0; i < args.length; i++) {
	if (args[i] === '--test') {
		testMode = true;
		if (args[i + 1] && !args[i + 1].startsWith('--'))
			testPattern = args[++i];
	} else if (/^\d+$/.test(args[i])) {
		port = parseInt(args[i], 10);
	}
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
	if (testMode) {
		console.log('client connected (test mode: ' + testPattern + ')');
		if (testPattern === 'idle') {
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
