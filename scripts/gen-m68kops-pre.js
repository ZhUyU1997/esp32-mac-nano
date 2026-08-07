/*
 * gen-m68kops-pre.js
 *
 * Generates a compact, ESP32-friendly opcode dispatch table for Musashi.
 *
 * Input:
 *   - main/macplus/cpu/musashi/opcode_table.json
 *     A JSON version of Musashi's opcode pattern table:
 *       [ handlerName, maskHexString, matchHexString, [cycles_68000, cycles_68010, cycles_68020] ]
 *
 * Output (to stdout):
 *   - C source that becomes main/macplus/cpu/musashi/m68kops_pre.c
 *
 * What gets generated:
 *   - m68ki_instruction_jump_table[]:
 *     Unique opcode handler function pointers (deduplicated).
 *   - m68ki_opcode_page_map[256] + m68ki_opcode_index_pages[pageCount][256]:
 *     Two-level opcode → handler-index mapping:
 *       page = opcode >> 8
 *       entry = opcode & 0xff
 *       packed u16 = (cycleIndex << 11) | handlerIndex
 *     Pages are deduplicated to reduce size.
 *   - m68ki_cycle_values[]:
 *     Lookup table for cycle counts (68000 only in this project).
 *
 * Compression notes:
 *   - We intentionally collapse a few rare large cycle values to reduce the
 *     number of distinct cycles from 36 down to 32 so the cycle index fits in
 *     5 bits (stored in the upper bits of the packed u16).
 *
 * Usage:
 *   node scripts/gen-m68kops-pre.js > main/macplus/cpu/musashi/m68kops_pre.c
 */

const fs = require("fs");
const path = require("path");

function parseArgs(argv) {
  let tablePath;
  let dataAttr = process.env.M68KOPS_DATA_ATTR;
  let jumpTableAttr = process.env.M68KOPS_JUMP_TABLE_ATTR;

  for (let i = 0; i < argv.length; i++) {
    const a = argv[i];
    if (!a.startsWith("--")) {
      if (tablePath !== undefined) {
        throw new Error(`Unexpected extra argument: ${a}`);
      }
      tablePath = a;
      continue;
    }

    const eq = a.indexOf("=");
    const key = eq === -1 ? a : a.slice(0, eq);
    const inlineValue = eq === -1 ? undefined : a.slice(eq + 1);

    function readValue() {
      if (inlineValue !== undefined) return inlineValue;
      if (i + 1 >= argv.length) throw new Error(`Missing value for ${key}`);
      i++;
      return argv[i];
    }

    if (key === "--data-attr") {
      dataAttr = readValue();
    } else if (key === "--jump-table-attr") {
      jumpTableAttr = readValue();
    } else if (key === "--no-data-attr") {
      dataAttr = "";
    } else if (key === "--no-jump-table-attr") {
      jumpTableAttr = "";
    } else if (key === "--help") {
      process.stdout.write(
        [
          "Usage:",
          "  node scripts/gen-m68kops-pre.js [opcode_table.json] [options]",
          "",
          "Options:",
          "  --data-attr <macro>           Attribute macro for data tables (default: FAST_DATA_ATTR)",
          "  --jump-table-attr <macro>     Attribute macro for jump table (default: same as --data-attr)",
          "  --no-data-attr                Omit attribute macro for data tables",
          "  --no-jump-table-attr          Omit attribute macro for jump table",
          "",
          "Environment:",
          "  M68KOPS_DATA_ATTR",
          "  M68KOPS_JUMP_TABLE_ATTR",
          "",
        ].join("\n")
      );
      process.exit(0);
    } else {
      throw new Error(`Unknown option: ${key}`);
    }
  }

  if (dataAttr === undefined) dataAttr = "FAST_DATA_ATTR";
  if (jumpTableAttr === undefined) jumpTableAttr = dataAttr;

  return { tablePath, dataAttr, jumpTableAttr };
}

function parseHex(s) {
  if (typeof s === "number") return s | 0;
  if (typeof s !== "string") throw new Error(`Invalid hex value: ${s}`);
  const v = Number.parseInt(s, 0);
  if (!Number.isFinite(v)) throw new Error(`Invalid hex string: ${s}`);
  return v | 0;
}

function loadOpcodeTable(jsonPath) {
  const raw = fs.readFileSync(jsonPath, "utf8");
  const data = JSON.parse(raw);
  if (!Array.isArray(data)) throw new Error("opcode_table.json must be an array");
  return data.map((row, i) => {
    if (!Array.isArray(row) || row.length !== 4) {
      throw new Error(`Invalid row at index ${i}`);
    }
    const [handler, mask, match, cycles] = row;
    if (typeof handler !== "string") throw new Error(`Invalid handler at index ${i}`);
    if (!Array.isArray(cycles) || cycles.length < 1) {
      throw new Error(`Invalid cycles at index ${i}`);
    }
    return {
      handler,
      mask: parseHex(mask),
      match: parseHex(match),
      cycles0: cycles[0] | 0,
    };
  });
}

function buildTables(entries) {
  const fn = new Array(0x10000);
  const cyc0 = new Array(0x10000);
  for (let i = 0; i < 0x10000; i++) {
    fn[i] = "m68k_op_illegal";
    cyc0[i] = 0;
  }

  let idx = 0;

  while (entries[idx].mask !== 0xff00) {
    const { handler, mask, match, cycles0 } = entries[idx];
    for (let op = 0; op < 0x10000; op++) {
      if (((op & mask) | 0) === match) {
        fn[op] = handler;
        cyc0[op] = cycles0;
      }
    }
    idx++;
  }

  while (entries[idx].mask === 0xff00) {
    const { handler, match, cycles0 } = entries[idx];
    for (let i = 0; i <= 0xff; i++) {
      const op = (match | i) & 0xffff;
      fn[op] = handler;
      cyc0[op] = cycles0;
    }
    idx++;
  }

  while (entries[idx].mask === 0xf1f8) {
    const { handler, match, cycles0 } = entries[idx];
    for (let i = 0; i < 8; i++) {
      for (let j = 0; j < 8; j++) {
        const op = (match | (i << 9) | j) & 0xffff;
        fn[op] = handler;
        cyc0[op] = cycles0;
        if ((op & 0xf000) === 0xe000 && (op & 0x20) === 0) {
          const cycleCost = (((((i - 1) & 7) + 1) << 1) | 0) >>> 0;
          cyc0[op] = (cyc0[op] + cycleCost) | 0;
        }
      }
    }
    idx++;
  }

  while (entries[idx].mask === 0xfff0) {
    const { handler, match, cycles0 } = entries[idx];
    for (let i = 0; i <= 0x0f; i++) {
      const op = (match | i) & 0xffff;
      fn[op] = handler;
      cyc0[op] = cycles0;
    }
    idx++;
  }

  while (entries[idx].mask === 0xf1ff) {
    const { handler, match, cycles0 } = entries[idx];
    for (let i = 0; i <= 0x07; i++) {
      const op = (match | (i << 9)) & 0xffff;
      fn[op] = handler;
      cyc0[op] = cycles0;
    }
    idx++;
  }

  while (entries[idx].mask === 0xfff8) {
    const { handler, match, cycles0 } = entries[idx];
    for (let i = 0; i <= 0x07; i++) {
      const op = (match | i) & 0xffff;
      fn[op] = handler;
      cyc0[op] = cycles0;
    }
    idx++;
  }

  while (entries[idx].mask === 0xffff) {
    const { handler, match, cycles0 } = entries[idx];
    const op = match & 0xffff;
    fn[op] = handler;
    cyc0[op] = cycles0;
    idx++;
  }

  return { fn, cyc0 };
}

function normalizeCyclesForCompression(cyc0) {
  const drop = new Set([164, 166, 168, 170]);
  const replacement = 162;
  for (let op = 0; op < 0x10000; op++) {
    if (drop.has(cyc0[op])) cyc0[op] = replacement;
  }
  return cyc0;
}

function buildCycleValueTable(cyc0) {
  normalizeCyclesForCompression(cyc0);
  const values = Array.from(new Set(cyc0)).sort((a, b) => a - b);
  if (values.length > 32) {
    throw new Error(`Too many unique cycle values: ${values.length} > 32`);
  }
  const valueToIndex = new Map();
  for (let i = 0; i < values.length; i++) valueToIndex.set(values[i], i);
  const opcodeToCycleIdx = new Array(0x10000);
  for (let op = 0; op < 0x10000; op++) {
    opcodeToCycleIdx[op] = valueToIndex.get(cyc0[op]);
  }
  return { cycleValues: values, opcodeToCycleIdx };
}

function buildCompactJumpTable(fn, opcodeToCycleIdx) {
  const unique = [];
  const uniqueIndex = new Map();
  const opcodeToUnique = new Array(0x10000);

  for (let op = 0; op < 0x10000; op++) {
    const name = fn[op];
    let idx = uniqueIndex.get(name);
    if (idx === undefined) {
      idx = unique.length;
      unique.push(name);
      uniqueIndex.set(name, idx);
    }
    opcodeToUnique[op] = idx;
  }

  const HANDLER_BITS = 11;
  const HANDLER_MASK = (1 << HANDLER_BITS) - 1;
  if (unique.length > HANDLER_MASK + 1) {
    throw new Error(
      `Too many unique handlers: ${unique.length} > ${HANDLER_MASK + 1}`
    );
  }

  const pages = [];
  const pageIndex = new Map();
  const pageMap = new Array(256);

  for (let page = 0; page < 256; page++) {
    const arr = new Array(256);
    for (let i = 0; i < 256; i++) {
      const op = (page << 8) | i;
      const handlerIdx = opcodeToUnique[op];
      const cycIdx = opcodeToCycleIdx[op];
      arr[i] = (handlerIdx & HANDLER_MASK) | ((cycIdx & 31) << HANDLER_BITS);
    }
    const key = arr.join(",");
    let pidx = pageIndex.get(key);
    if (pidx === undefined) {
      pidx = pages.length;
      pages.push(arr);
      pageIndex.set(key, pidx);
    }
    pageMap[page] = pidx;
  }

  return { unique, pageMap, pages };
}

function emitM68kopsPreC({ fn, cyc0 }, { dataAttr, jumpTableAttr }) {
  const { cycleValues, opcodeToCycleIdx } = buildCycleValueTable(cyc0);
  const compact = buildCompactJumpTable(fn, opcodeToCycleIdx);
  const out = [];
  const dataAttrSp = dataAttr ? `${dataAttr} ` : "";
  const jumpTableAttrSp = jumpTableAttr ? `${jumpTableAttr} ` : "";
  out.push('#include "m68kops.h"\n\n');
  out.push('#include "fast_attr.h"\n\n');
  out.push("//Opcodes are built on the host, this does not need to do anything.\n");
  out.push("void m68ki_build_opcode_table() {}\n\n");
  out.push(
    `const m68ki_instruction_jump_call ${jumpTableAttrSp}m68ki_instruction_jump_table[]={\n`
  );
  for (let i = 0; i < compact.unique.length; i++) {
    out.push("\t");
    out.push(compact.unique[i]);
    out.push(",\n");
  }
  out.push("};\n\n");

  out.push(`const unsigned char ${dataAttrSp}m68ki_opcode_page_map[256]={\n\t`);
  for (let i = 0; i < 256; i++) {
    out.push(String(compact.pageMap[i]));
    out.push(",");
    if ((i & 15) === 15 && i !== 255) out.push("\n\t");
  }
  out.push("\n};\n\n");

  out.push(
    `const unsigned short ${dataAttrSp}m68ki_opcode_index_pages[${compact.pages.length}][256]={\n`
  );
  for (let p = 0; p < compact.pages.length; p++) {
    out.push("\t{");
    for (let i = 0; i < 256; i++) {
      if ((i & 15) === 0) out.push("\n\t\t");
      out.push(String(compact.pages[p][i]));
      out.push(",");
    }
    out.push("\n\t},\n");
  }
  out.push("};\n\n");

  out.push(
    `const unsigned char ${dataAttrSp}m68ki_cycle_values[${cycleValues.length}]={\n\t`
  );
  for (let i = 0; i < cycleValues.length; i++) {
    out.push(String(cycleValues[i]));
    out.push(",");
    if ((i & 15) === 15 && i !== cycleValues.length - 1) out.push("\n\t");
  }
  out.push("\n};\n");
  return out.join("");
}

function main() {
  const repoRoot = path.resolve(__dirname, "..");
  const opts = parseArgs(process.argv.slice(2));
  const tablePath =
    opts.tablePath ||
    path.join(repoRoot, "main/core/macplus/cpu/musashi/opcode_table.json");
  const entries = loadOpcodeTable(tablePath);
  const tables = buildTables(entries);
  process.stdout.write(emitM68kopsPreC(tables, opts));
}

main();
