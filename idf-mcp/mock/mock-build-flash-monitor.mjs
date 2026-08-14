// Mock `idf.py build flash monitor --no-reset`: fake build, then fake flash,
// then fake serial logs forever (no second reset).
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

console.log("--- idf.py build (MOCK) ---");
for (const s of [
  "Running ninja in directory build",
  "Executing \"ninja -j 2\"...",
  "[1/3] Building C object CMakeFiles/app.dir/main.c.obj",
  "[2/3] Linking CXX executable app.elf",
  "[3/3] Generating binary image from app.elf",
  "Project build complete.",
]) {
  console.log(s);
  await sleep(100);
}

console.log("--- idf.py flash monitor (MOCK) ---");
for (const s of [
  "Connecting........_____....._____....._____.....OK",
  "Chip is ESP32-S3 (revision v0.2)",
  "Uploading stub...",
  "Compressed 1048576 bytes at 0x00010000... (100%)",
  "Hash of data verified.",
  "Leaving...",
  "Hard resetting via RTS pin...",
  "Executing action: monitor",
  "Running idf_monitor in directory /home/yzhu/esp32-mini-mac (mock)",
  "Done - flash OK (mock)",
]) {
  console.log(s);
  await sleep(120);
}

let i = 0;
const tick = async () => {
  while (true) {
    i++;
    console.log(`I (${1000 + i * 137}) app_main: mock log line ${i}`);
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
