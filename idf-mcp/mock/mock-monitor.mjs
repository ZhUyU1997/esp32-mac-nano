// Mock ESP-IDF serial monitor: prints fake device logs forever.
// Key handling mirrors esp-idf-monitor:
//   Ctrl-]  (0x1d)      -> exit
//   Ctrl-T Ctrl-R (0x14 0x12) -> reset the chip (menu + reset)
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

console.log("--- idf.py monitor (MOCK) ---");
let i = 0;

const tick = async () => {
  while (true) {
    i++;
    const ts = 1000 + i * 137;
    const levels = ["I", "W", "E"];
    const level = levels[i % 3];
    console.log(`${level} (${ts}) app_main: mock log line ${i}`);
    await sleep(500);
  }
};

const stop = () => {
  console.log("--- monitor stopped (mock) ---");
  process.exit(0);
};

// Raw-mode stdin, like esp-idf-monitor reads from the terminal.
// Tolerate non-TTY stdin (headless MCP testing): raw mode just isn't available.
try {
  process.stdin.setRawMode?.(true);
} catch {
  /* non-TTY stdin, keys not available */
}
process.stdin.resume();
let tail = "";
process.stdin.on("data", (d) => {
  const s = tail + d.toString();
  tail = d.toString().slice(-2);
  if (s.includes("\x1d")) stop();
  if (s.includes("\x14\x12")) console.log("--- chip reset (mock) ---");
});
process.on("SIGINT", stop);

tick();
