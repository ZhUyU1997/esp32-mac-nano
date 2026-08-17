/* config.js — shared paths/constants for the art toolchain. */
'use strict';

const path = require('path');

const ROOT = path.join(__dirname, '..', '..', '..'); /* tools/art/lib -> project root */

module.exports = {
	ROOT,
	PACKS: path.join(ROOT, 'scripts/art/packs'),
	ANSILOVE: process.env.ANSILOVE_BIN || '/usr/bin/ansilove',
	VTERM: path.join(ROOT, 'build/linux/x86_64/release/vterm-ans'),
};
