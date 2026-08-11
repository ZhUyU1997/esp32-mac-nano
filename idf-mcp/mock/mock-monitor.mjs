// Mock ESP-IDF serial monitor: prints fake device logs forever.
// Mirrors `idf.py monitor` key handling: the exit key is Ctrl-] (0x1d),
// which esp-idf-monitor reads from the terminal in raw mode. Ctrl-C (SIGINT)
// is also honored for direct kills.
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

// Raw-mode stdin: read the exit key Ctrl-] (0x1d) like esp-idf-monitor does.
process.stdin.setRawMode?.(true);
process.stdin.resume();
process.stdin.on("data", (d) => {
  if (d.includes("\x1d")) stop();
});
process.on("SIGINT", stop);

tick();
