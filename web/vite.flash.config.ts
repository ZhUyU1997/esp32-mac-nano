// 独立构建配置：flash.html 网页烧写器（单文件，部署到 GitHub Pages）
// 与主配置分离：多入口 + singlefile 的 codeSplitting=false 冲突（vite 8）
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
    // 独立输出目录，避免清空 dist/（index.html 嵌入固件）
    outDir: 'dist-flash',
    rollupOptions: {
      input: { flash: path.join(__dirname, 'flash.html') },
    },
  },
});
