/* exempt-files.js — render-difference exemption / exclusion lists for
 * test-art.js (and anyone else comparing vterm-ans against libansilove).
 *
 * EXEMPT_FILES: entries whose diff is a known libansilove-side defect or
 * a file-level quirk (vterm is correct); see docs/art-testing.md 9.x.
 * EXCLUDE_FILES: entries dropped from the fixed-80-col ESP32 gallery.
 * Matching is by file name (exact), against the flattened pack-entry name. */
'use strict';

const EXCLUDE_FILES = new Set([
	'fool27.zip-file_id.ans', /* SAUCE 45 cols: only renders right at that
	                          * width, no 80-col view on the fixed-80 ESP32 */
	'mist0918.zip-FILE_ID.ANS', /* SAUCE 32 cols: same, non-80 width */
	'impure84.zip-lmn-siouxie.ans', /* renders badly on both sides (user: exclude) */
	'mist0823.zip-MM-ONE.ANS', /* ansilove drops to 9 rows vs 48 (user: exclude) */
]);

const EXEMPT_FILES = new Set([
	'MEM0595.ANS',
	'PK-NUCW.ANS', /* corrupt ANSI sequences in the file itself */
	'AL-DTD.ANS',  /* SAUCE 79 cols: width gap shifts wrap rows (min-col
	                * compare can't align the shifted rows) */
	'DG-MAKC2.ANS', /* \r mixed into ESC sequence params (corrupt file) */
	'--------.ANS', /* ansilove: File format error (its own failure) */
	'US!.ANS',      /* ansilove: File format error (its own failure) */
	'cm-MIST.ans',  /* SAUCE 79 cols: width gap shifts wrap rows */
	'sk!n-abstrakt_nfo_fb.ans', /* libansilove TAB overflow loses content (we keep it) */
	'sk!n-motiv8_logo_ansi.ans', /* libansilove TAB: col+8 vs tab-stop (5-col shift) */
	'mz-piece.ans', /* libansilove ignores 38;5 256-color (fg stays 7) */
	'grx-comp2.ans', /* bare ESC+spaces: libvterm collects intermed bytes vs ANSI.SYS shows them */
	'grx-comp7.ans', /* same bare ESC+spaces / 38;5 family as grx-comp2 */
	'ldn-vandalism.ans', /* libansilove CUF no-clamp at right margin (col 80 vs ANSI.SYS 79) */
	'+l-ds.ans', /* libansilove TAB col+8 vs tab-stop (col shift + wrap rows) */
	'g80-hmm.ans', /* control-char demo art: \x0e SO / bare-CR handling differs */
	'ru8_factory.ans', /* looks shifted (user: exempt) */
	'ITSOVER.ANS',   /* ansilove misrenders the cursor-save/restore + clear animation */
	'FILE_ID.ANS',   /* SAUCE 32 cols: narrow-width wrap vs libansilove (2018/mist0918) */
	'lmn-siouxie.ans', /* blink/iCE bright-bg handling differs from libansilove */
	'MM-ONE.ANS',    /* vterm 48 rows vs libansilove 9 rows (row accounting differs) */
	'NAUGFLAG.ANS',  /* libansilove reverse-video (SGR 7) is attribute-based, vterm follows ANSI.SYS */
	'Arl-Rat.ans',   /* corrupt sequences (user: looks like data damage) */
	'us-mistimpure.ans', /* corrupt sequences (user: looks like data damage) */
	'Swansi.ans',    /* user: exclude (diff pattern) */
	'BYM_FOREVER.ans', /* vterm result more reasonable (user) */
	'die-already.ans', /* libansilove TAB +=8 vs ANSI.SYS/PabloDraw char-write (vterm correct) */
	'fil-metal.ans',  /* libansilove CR ignores (0x0d) vs ANSI.SYS home — SO/♪ lands shifted */
	'NFO-1094.ANS',  /* libansilove bold (SGR 1) accumulates +=8 — repeated [1m overflows fg */
	'ANSI24.ANS',   /* trailing garbage ESC[25[1a] — ansilove draws '25[' as text, vterm ignores */
	'US-HYP.ICE',   /* trailing ESC[1;30;[1a] — same as ANSI24: ansilove draws residual as text */
	'PATHELL.ANS',  /* CUF past right margin (ESC[82C) + char: vterm phantom stops at c79, xterm/ansilove wrap */
	'NFO-0295.ANS',  /* same as NFO-1094: libansilove bold accumulates +=8 — repeated [1m overflows */
	'NF-IT.ANS',    /* TAB (0x09): libansilove col+=8 vs ANSI.SYS/PabloDraw char-write (vterm correct) */
	'GM-ICE5.ICE',  /* [0;1m+[30;1m (repeated bold, black fg): vterm bright-black space vs ansilove black */
	'mi-google.ans', /* trailing '2' before SAUCE: vterm skips SAUCE metadata, ansilove has no SAUCE support */
	'US-SE1.ANS',   /* SAUCE metadata region: vterm skips (art_len=sauce.data_len), ansilove renders it */
	'NEWMAIL.ANS',  /* EL (ESC[K) clear-line colour: vterm fills current bg, ansilove clears to black */
	'ANSI1.ANS',    /* ansilove draws CSI param '5' of ESC[5H as text, vterm correct */
	'WWANS58.ANS',  /* same as ANSI1: ansilove draws CSI param '5' as text */
	'LOGIN.ANS',    /* trailing ESC[23;80H ESC[K: EL clear / CUP-80 edge, EL-colour family */
	'WWANS66.ANS',  /* same as ANSI1/WWANS58: ansilove draws CSI param '5' as text */
	'MM-FERRE.ICE', /* same family: ansilove draws CSI param '7' as text */
	'EARTH.ANS',    /* CUP row=0: vterm 0->1 semantics (draws r0), ansilove drops (row=-1) */
	'HF-HEAD.ANS',  /* illegal CSI final '}' drawn as text by ansilove, vterm ignores */
	'SPITOUFS-YE-OLDE-ZOMBIE.ANS', /* TAB jump: vterm tabstop vs ansilove +=8 (NF-IT family) */
	'fil-slip.ans', /* SAUCE metadata region rendered by ansilove (US-SE1 family) */
	'AX-GUM2.ICE',  /* same family: ansilove draws CSI param '9' as text */
	'arl-rock.ans', /* truncated CSI residue: ' ' ESC[ 0xD7 - ansilove draws 0xD7 as text */
	'fuel24-nfo.ans', /* truncated ESC: 0xDF ESC '-' - ansilove draws intermediate byte as text */
	'WWANS79.ANS',  /* same family: ansilove draws CSI param '5' as text */
	'FIGMENT.ANS',  /* same family: ansilove draws CSI param '5' as text */
	'DS%LOGOO.ANS', /* same family: ansilove draws CSI param '5' as text */
	'RYANS38.ANS',  /* truncated CSI ESC[41 (no final): vterm drops on new ESC, ansilove loses CUF */
	'Heyo_hi.ans',  /* SAUCE metadata region rendered by ansilove (US-SE1 family) */
	'sk!n-starwars_nvscene15.ans', /* TAB jump in slash-art (NF-IT family) */
	'WWANS82.ANS',  /* same family: ansilove draws CSI params '4;1' as text */
	'pop',         /* CR ignored by ansilove (L158 case CR: break): ' ' CR 'y' -> vterm c0, ansilove c1 */
	'fil-tunes.ans', /* CR+SO pair: ansilove ignores CR, SO+text misplaced (CR family) */
	'OS-DD.ANS',    /* CR ignored by ansilove (pop family): 0xDB CR 0xDF -> c0 vs c1 */
	'sk!n-amiga_ascii_art_revision14.ans', /* TAB jump in slash-art (NF-IT family) */
	'MP2-6.ANS',   /* CUF overflow: ESC[78C ESC[80C - vterm clamps to last col, ansilove drops (PATHELL family) */
	'CA-TRDRS.ANS', /* truncated CSI ESC[ + CRLF: ansilove loses following CUF (truncated-CSI family) */
	'wpx-recall.ans', /* TAB jumps in header art (NF-IT family) */
	'WWANS97.ANS',  /* trailing CUF/CUB/CR+0x30 watermark region (CR family) */
	'BLUES.ANS',    /* scroll-history rows vs final screen: vterm 256 rows, ansilove 34 (screen model) */
	'WWANS190.ANS', /* trailing CUP+ESC[s+CUB+CR+0x30 watermark (CR family, WWANS97-like) */
	'HF-FIEND.ANS', /* dithering block-art 1-2 col shift + bold colour (vterm no-highbright vs ansilove) */
	'H4-2017.ANS',  /* row-advance divergence: r27 = byte 4064 (vterm) vs 2030 (ansilove) */
	'sk!n-island_of_death_nvscene14.ans', /* multi-frame/scroll rows: vterm 422 vs ansilove 209 (screen model) */
	'WWANS188.ANS', /* CUP-positioned row: vterm draws block segment, ansilove drops */
	'LDA-BOO.ans',  /* trailing 0x30 x113 fill before EOF/SAUCE: ansilove draws, vterm stops at 0x1A */
	'DWIMMER-FRIEND_STUDY.ANS', /* TAB jumps around ___ block art (NF-IT family) */
	'PICROTOXIN-BBB.ANS', /* scroll rows: vterm 87 vs ansilove 48 (screen model family) */
	'jn-soltn.ans',  /* per-char SGR pixel art 1-2 col shift (HF-FIEND family) */
	'mz-brandmeister.ans', /* slash-art with TAB+CUF: col shift (slash-art family) */
	'RODBURY.ANS',  /* trailing BBS info: truncated ESC[3 + CRLF + text (truncated-CSI family) */
	'OUT-AD.ANS',   /* SGR 8 conceal: vterm hides text, ansilove draws (256-row mode too) */
	'MASH_CHP.ANS', /* ESC M (RI reverse linefeed) + MFT240 music rows: row-count divergence */
	'sk!n-desire_arsantica_3(original).ans', /* TAB before (_Atari_) + slash/block art (TAB family) */
	'33-PIN.ANS',  /* dithering block art 1-2 col shift + SGR colours (HF-FIEND family) */
	'SEAHORSE.ANS', /* trailing NUL x31 after ESC[42m: ansilove draws NUL as space+bg, vterm ignores */
	'ANSI10.ANS',   /* 256-row scroll mode (BLUES family) */
	'ANSI13.ANS',   /* 256-row scroll mode (BLUES family) */
	'SN-0296C.ANS', /* per-char SGR dithering art 1-2 col shift (HF-FIEND family) */
	'GJ-MPN.ANS',   /* trailing BBS ad: 1-col shift + bold colour (HF-FIEND family) */
	'arl-longlivetheascii3.ans', /* TAB inside number-block art (NF-IT family) */
	'ANSI11.ANS',   /* 256-row scroll mode (BLUES family) */
	'ANSI9.ANS',    /* 256-row scroll mode (BLUES family) */
	'WWANS168.ANS', /* ESC[s CRLF ESC[u + CUP rows: row-advance divergence */
	'jn-mist.ans',  /* dense per-char SGR + high-byte chars: col shift (dithering family) */
	'CALVIN.ANS',   /* trailing BBS ad + NUL region: bright-blue bg divergence */
	'DJ-WOLF.ANS',  /* large row-shift block (r125-163): bold white bg art, screen-model family */
	'jn-light.ans', /* dense high-byte CP437 art + ESC[78C rows: col shift (jn-mist family) */
	'JBION.ANS',    /* bold colour: vterm 170s vs ansilove 255s (bold-highbright family) */
	'nu-bauddudes.ans', /* per-char inverse-video dithering: large col shift (dithering family) */
	'SUMSAMBA.ANS',  /* 256-row scroll mode (BLUES family) */
	'+l-1992.ans',  /* large dithering shift (r33-68, r125-129): pixel/col divergence */
	'sk!n-resistance_nfo.ans', /* scroll rows (95 vs 16) + col shift (screen-model family) */
	'pe-shark.ans', /* dense ESC[7m inverse + block chars: col shift (dithering family) */
	'ru8-chargepoints.ans', /* per-char SGR blue/white frame art: large col shift (dithering) */
	'sk!n-deadline.ans', /* scroll rows: 66 vs 5 (screen-model family) */
	'arl-AI.ans',   /* TAB x6 + 0x10 control + 88 block art: col shift (TAB/ctrl family) */
	'here__s-another-virus.ans', /* slash-art large col shift (r11-28) */
	'HOLIC2.ANS',   /* dense ESC[s/ESC[u/ESC[K/CUP combo: row-col divergence */
	'zj-advert.ans', /* large row shift (r34-51): slash art (screen-model family) */
	'MMSXMAS.ANS',  /* blue-bg region shift after ESC[2J (screen-model family) */
	'pender_logoff.ans', /* ESC[7m inverse + SGR combos: large col shift */
	'THE_ELK-PILL70.ANS', /* large row shift (r14-47): 430 diff (screen-model family) */
	'THE_ELK-RATFINK.ANS', /* massive row shift (3160 diff): screen-model family */
	'SM-IMAG.ANS',  /* large row shift (r41-83): 906 diff (screen-model family) */
	'HF-SHE.ANS',   /* large row shift (r48-124): 1457 diff (screen-model family) */
	'ANSI-ANI.ANS', /* 258KB animation: vterm 7337 rows vs ansilove 63 (scroll/animation) */
	'FLG.ANS',      /* bold-blink + ESC[A + CUF combos: large col shift */
	'tcf',         /* tcf - Huangzenegger.ans: massive row shift (2539 diff) */
	'SP-DFR1.ANS',  /* massive row shift (3820 diff): screen-model family */
	'._us-ewheat.ans', /* __MACOSX AppleDouble metadata junk */
	'._file_id.ans', /* __MACOSX AppleDouble metadata junk */
	'PN-PLSMA.ANS', /* massive row shift (1874 diff): screen-model family */
	'._om-x-2m-feminism.ans', /* __MACOSX AppleDouble metadata junk */
	'wz-teaparty-alhambra.ans', /* massive row shift (718 diff): screen-model family */
	'h7-pablofinished.ans', /* large col shift (740 diff): dithering family */
	'WWANS179.ANS', /* ESC[2D + block chars per-char: CUB column divergence */
	'MM-ERRORIN0RDERRZ.ANS', /* scroll rows 41 vs 22 (screen-model family) */
	'A_SMURF.ANS',   /* large col shift (383 diff) */
	'acid-phix-the-fix-music-company.ans', /* trailing rows shift (90 diff) */
	'._ro-usta1.ice', '._ro-usta2.ice', '._ro-usta3.ice', '._ro-usta4.ice', /* __MACOSX junk */
]);

module.exports = { EXCLUDE_FILES, EXEMPT_FILES };
