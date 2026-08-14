// Mock `idf.py flash monitor --no-reset`: fake esptool flash, then fake
// serial logs forever (no second reset — the mock just continues).
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

console.log("--- idf.py flash monitor (MOCK) ---");

const flashSteps = [
  "Connecting........_____....._____....._____....._____.....OK",
  "Chip is ESP32-S3 (revision v0.2)",
  "Uploading stub...",
  "Compressed 1048576 bytes at 0x00010000... (100%)",
  "Wrote 1048576 bytes at 0x00010000 in 1.2 seconds",
  "Hash of data verified.",
  "Leaving...",
  "Hard resetting via RTS pin...",
  "Executing action: monitor",
  "Running idf_monitor in directory /home/yzhu/esp32-mini-mac (mock)",
  "Done - flash OK (mock)",
];

for (const s of flashSteps) {
  console.log(s);
  await sleep(120);
}

let i = 0;
const tick = async () => {
  while (true) {
    i++;
    const ts = 1000 + i * 137;
    console.log(`I (${ts}) app_main: mock log line ${i}`);
    await sleep(500);
  }
};

const stop = () => {
  console.log("--- monitor stopped (mock) ---");
  process.exit(0);
};

process.stdin.setRawMode?.(true);
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
