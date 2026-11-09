import {build} from 'esbuild';
import {copyFile,mkdir} from 'node:fs/promises';
await mkdir('docs/assets',{recursive:true});
await build({entryPoints:['src/main.tsx'],bundle:true,minify:true,outfile:'docs/assets/app.js',platform:'browser',target:'es2022',jsx:'automatic',define:{'process.env.NODE_ENV':'"production"'}});
await copyFile('src/styles.css','docs/assets/app.css');
console.log('GitHub Pages static files built in docs/');
