// Standalone build config: flash.html web flasher (single-file, deployed to GitHub Pages)
// Separate from main config: multi-entry + singlefile codeSplitting=false conflict (vite 8)
import { defineConfig } from 'vite';
import preact from '@preact/preset-vite';
import { viteSingleFile } from 'vite-plugin-singlefile';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));

export default defineConfig({
  root: __dirname,
  plugins: [preact(), viteSingleFile()],
  build: {
    target: 'es2018',
    assetsInlineLimit: 100000000,
    cssCodeSplit: false,
    // Separate output dir to avoid wiping dist/ (index.html embedded in firmware)
    outDir: 'dist-flash',
    rollupOptions: {
      input: { flash: path.join(__dirname, 'flash.html') },
    },
  },
});
