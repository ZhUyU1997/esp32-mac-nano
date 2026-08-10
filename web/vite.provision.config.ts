// Second build pass for the standalone provision page (provision.html).
// Runs after the main build; keeps dist/index.html (emptyOutDir: false).
// Single-file output like the main page (vite-plugin-singlefile).
import { defineConfig } from 'vite';
import preact from '@preact/preset-vite';
import { viteSingleFile } from 'vite-plugin-singlefile';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));

export default defineConfig({
  root: __dirname,
  plugins: [preact(), viteSingleFile({ useRecommendedBuildConfig: false })],
  build: {
    outDir: 'dist',
    emptyOutDir: false, // keep index.html from the main build
    target: 'es2018',
    assetsInlineLimit: 100000000,
    cssCodeSplit: true,
    rollupOptions: {
      input: path.join(__dirname, 'provision.html'),
    },
  },
});
