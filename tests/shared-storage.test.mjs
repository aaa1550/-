import assert from 'node:assert/strict';import {build} from 'esbuild';
await build({entryPoints:['src/sharedStorage.ts'],bundle:true,platform:'node',format:'esm',outfile:'/tmp/shared-storage-check.mjs'});
const m=await import('/tmp/shared-storage-check.mjs');
assert.equal(m.fromBase64Utf8(Buffer.from('日本語').toString('base64')),'日本語');assert.equal(m.toBase64Bytes(new Uint8Array(Buffer.from('abc日本語'))),Buffer.from('abc日本語').toString('base64'));
let requests=[],blobs=[],trees=[],commits=[],patched=false,conflict=false,denied=false;const empty={schemaVersion:1,records:[],files:[]};
const realFetch=globalThis.fetch;
globalThis.fetch=async(url,options={})=>{assert(String(url).startsWith('https://api.github.com/repos/aaa1550/-/'));requests.push({url,options});const p=String(url).split('/repos/aaa1550/-')[1],body=options.body?JSON.parse(options.body):null;let result;
if(p==='/git/ref/heads/main')result={object:{sha:'OLDHEAD'}}
else if(p==='/git/commits/OLDHEAD')result={tree:{sha:'BASETREE'}}
else if(p.startsWith('/contents/'))result={encoding:'base64',content:Buffer.from(JSON.stringify(empty)).toString('base64')}
else if(p==='/git/blobs'){if(denied)return new Response('{}',{status:403});blobs.push(body);result={sha:'BLOB'+blobs.length};}
else if(p==='/git/trees'){trees.push(body);result={sha:'NEWTREE'}}
else if(p==='/git/commits'){commits.push(body);result={sha:'NEWHEAD'}}
else if(p==='/git/refs/heads/main'){if(conflict)return new Response('{}',{status:422});patched=true;assert.equal(body.force,false);result={object:{sha:'NEWHEAD'}};}
else throw Error('Unexpected URL '+p);
return new Response(JSON.stringify(result),{status:200,headers:{'Content-Type':'application/json'}})};
const batch={records:[{id:'CODE-SHAREDTEST',kind:'codes',title:'テスト',code:'void setup() {}',importedAt:'local',visibility:'private',attachmentId:'FILE-SHAREDTEST'}],files:[{id:'FILE-SHAREDTEST',name:'test.ino',storageKey:'FILE-SHAREDTEST',size:3}],blobs:[{id:'FILE-SHAREDTEST',blob:new Blob(['abc'])}]};
const result=await m.publishSharedBatch(batch,'TEST_SECRET_NEVER_SAVE','skip',{records:[],files:[]});assert(patched);assert.equal(result.count,2);assert.equal(result.catalog.records[0].visibility,'public');assert.equal(result.catalog.records[0].importedAt,undefined);assert.equal(result.catalog.files.length,2);assert(!JSON.stringify(result.catalog).includes('storageKey'));assert(!JSON.stringify(blobs).includes('TEST_SECRET_NEVER_SAVE'));assert.equal(trees[0].base_tree,'BASETREE');assert.deepEqual(commits[0].parents,['OLDHEAD']);assert(trees[0].tree.some(t=>t.path==='docs/shared/catalog.json'));const snap=JSON.parse(blobs.find(b=>b.encoding==='utf-8').content);assert.equal(snap.files[0].dataBase64,Buffer.from('abc').toString('base64'));assert(m.fileDownloadUrl(result.catalog.files[0]).startsWith('https://raw.githubusercontent.com/aaa1550/-/main/docs/shared/files/'));
patched=false;conflict=true;await assert.rejects(()=>m.publishSharedBatch(batch,'TEST_SECRET','skip',{records:[],files:[]}),/競合/);assert(!patched);conflict=false;denied=true;await assert.rejects(()=>m.publishSharedBatch(batch,'TEST_SECRET','skip',{records:[],files:[]}),/書き込み権限/);assert(!patched);denied=false;
await assert.rejects(()=>m.publishSharedBatch(batch,'','skip',{records:[],files:[]}),/認証キー/);
await assert.rejects(()=>m.publishSharedBatch({records:batch.records,files:[],blobs:[]},'TEST_SECRET','skip',{records:batch.records,files:[]}),/新しく公開/);
const combined=await m.withSharedData({records:[{id:'base'}],files:[]});assert.equal(combined.records[0].id,'base');
globalThis.fetch=realFetch;
console.log('PASS: authenticated public save, atomic catalog commit, binary bytes, downloadable JSON backup, no token persistence, skip duplicates, permissions failure, no-force conflict rejection, anonymous read');
