// Mock ESP-IDF flash: prints fake esptool progress then exits.
// Used for testing without touching real hardware / real idf.py.
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

const steps = [
  "Connecting........_____....._____....._____....._____....._____....._____.....OK",
  "Chip is ESP32-S3 (revision v0.2)",
  "Features: WiFi, BLE",
  "MAC: 7c:df:a1:00:00:00",
  "Uploading stub...",
  "Running stub...",
  "Stub running...",
  "Changing baud rate to 460800",
  "Configuring flash size...",
  "Compressed 24576 bytes at 0x0000e000...",
  "Compressed 8192 bytes at 0x00010000...",
  "Compressed 1048576 bytes at 0x00010000... (100%)",
  "Wrote 1048576 bytes (1047717 compressed) at 0x00010000 in 22.4 seconds (effective 468.0 kbit/s)...",
  "Hash of data verified.",
  "Leaving...",
  "Hard resetting via RTS pin...",
  "Done - flash OK (mock)",
];

for (const s of steps) {
  console.log(s);
  await sleep(150);
}
