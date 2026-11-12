import json,html,re,pathlib,datetime,zipfile
root=pathlib.Path(__file__).resolve().parents[1];docs=root/'docs';src=docs/'code/RemoteObstacleCar';design=docs/'design/RemoteObstacleCar';design.mkdir(parents=True,exist_ok=True)
now=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=9))).isoformat()
code=(src/'RemoteObstacleCar.ino').read_text();guide=(src/'README.md').read_text()
catalog=json.loads((docs/'shared/catalog.json').read_text());base=json.loads((docs/'data.json').read_text())
old={r['id']:r for r in base['records']};old.update({r['id']:r for r in catalog['records']})
records=[];files=[]
links='LATEST-CODE, LATEST-GUIDE, LATEST-DESIGN'
def record(id,kind,title,**kw):
 r=dict(id=id,kind=kind,title=title,visibility='public',updated=now,links=links,**kw);records.append(r);return r
record('LATEST-CODE','codes','今回の仕様 / RemoteObstacleCar.ino（define・1ファイル）',filename='RemoteObstacleCar.ino',category='今回の仕様・define版',code=code,content='前方15cm以内で距離に応じたブザー、5cm以下で強制後退、後方検知で前進。OK優先・前後同時停止。#defineで設定、独自ヘッダー不要。旧FlowchartRobotの5cm停止とは動作が異なる。',status='実機未検証',pins='ENA D5 / ENB D6 / Servo D9 / Echo D3 / Trig D10 / IR受信 D4 / ブザー D13 / 後方 D0,D2',libraries='IRremote 4.4.1 / Servo 1.2.2 / Arduino Uno')
record('LATEST-SPEC','basic','今回の仕様：15cmで警告・5cmで後退・後方検知で前進',category='今回の仕様',content=guide,status='設計済み')
info={
'setMotors':('左右モーターへ方向とPWMを出す','符号からIN1/IN2を設定し、絶対値をPWMへ出力。実際に出した左右速度を記録する。','rightSpeed/leftSpeed: -255〜255の符号付きPWM','なし','左右速度','方向ピン、PWM、appliedRight/Left','digitalWrite, analogWrite'),
'go_Advance':('前進する','左右に正のDriveSpeedを渡す。自動回避時も同じ関数を使用する。','なし','なし','DriveSpeed','左右前進','setMotors'),
'go_Back':('後退する','左右に負のDriveSpeedを渡す。前方5cm以下の強制後退でも使用する。','なし','なし','DriveSpeed','左右後退','setMotors'),
'go_Right':('右旋回する','右を逆転、左を正転させる。','なし','なし','DriveSpeed','右旋回','setMotors'),
'go_Left':('左旋回する','右を正転、左を逆転させる。','なし','なし','DriveSpeed','左旋回','setMotors'),
'stop_Stop':('モーターへ停止指令を出す','最初に両PWMを0、その後に方向ピンをLOWにする。前回の出力速度も0に更新。','なし','なし','固定ピン設定','左右PWM0、方向LOW','analogWrite, digitalWrite'),
'resetToStop':('自動回避と古い操作を解除する','Remote_NumとDrive_NumをSTOP_STOP、速度0、回避モードなしへ戻す。モーターへの出力は後段で行う。','なし','なし','制御状態','停止状態・旧指令解除','なし'),
'do_IR_Tick':('リモコンの方向指令を更新する','既知キー24/82/90/8/28を方向またはSTOPへ変換。リピートは新規指令にせず、未知キーは無視。オーバーフローでは停止。方向は次の操作まで保持する。','なし','なし','IRremoteのcommand/flags','Remote_Num、STOP時は回避状態も解除','IrReceiver.decode, resetToStop, IrReceiver.resume'),
'watch':('超音波距離を計測する','Trigに10μsパルスを出し、Echo HIGHを最大15000μs測定。パルス長×0.01657をcmとして返す。タイムアウト0は-1で返す。','なし','float: 距離cm、失敗時-1','Echo D3、Trig D10','丸め前の距離','digitalWrite, delayMicroseconds, pulseIn'),
'updateSensors':('前方距離と後方検知を更新する','60ms周期でwatchを実行。5cm以下をfront=1にする。後方は毎ループ、左右いずれかLOWならback=1。','なし','なし','Echo、後方D0/D2、millis','distanceCm、distanceValid、front、back','millis, watch, digitalRead'),
'checkSensorsAndObstacles':('今回の仕様で実行動作を決める','STOP→測定失敗→前方5cm以下と後方同時→回避上限→継続中の回避→5cm以下の後退→後方検知の前進→通常操作。前方回避は15cm超、後方回避は後方検知解除で停止。回避3秒で打ち切り。','なし','なし','Remote_Num、距離、front/back、回避状態、millis','Drive_Num、DriveSpeed、avoidMode、回避終了時の旧指令解除','millis, resetToStop'),
'getBuzzerPeriod':('近づくほど警告周期を短くする','15cm=600ms、5cm以下=100ms。間は線形補間し、10cm=350msになる。呼出側が15cm以内かを判定する。','cm: 前方距離cm','uint32_t: ブザー周期ms','前方距離','周期（ms）','なし'),
'warning':('距離に応じたブザーを鳴らす','15cm以内なら先頭35msだけONにし、距離に応じた周期で反復。15cm超または測定無効ならOFF。delay/toneは使わない。停止中も距離警告を続ける。','なし','なし','distanceCm、distanceValid、millis','D13のLOW/HIGH','millis, getBuzzerPeriod, digitalWrite'),
'do_Drive_Tick':('実行動作をモーターへ反映する','停止なら直ちにPWM0。反転を伴う場合もPWM0にして100ms待ち、その間は関数から戻る。時間経過後に方向別の関数を呼ぶ。','なし','なし','Drive_Num、DriveSpeed、前回出力、millis','モーター出力、反転待ち状態','millis, stop_Stop, go_Advance, go_Back, go_Right, go_Left'),
'setup':('停止状態で初期化する','I/Oを設定し停止。ブザーOFF、Trig LOW。サーボを90度へ向けて400ms待ち、IR受信を開始。','なし','なし','ピン設定','#defineに沿った初期状態','pinMode, stop_Stop, resetToStop, digitalWrite, Servo.attach, Servo.write, delay, IrReceiver.begin'),
'loop':('リモコン・センサー・回避・ブザー・モーターを繰り返し更新する','IR→センサー→IR再確認→車両状態→ブザー→モーター。超音波待機中に届いたSTOPも出力前に取り込む。','なし','なし','リモコン、センサー','モーター、ブザー','do_IR_Tick, updateSensors, checkSensorsAndObstacles, warning, do_Drive_Tick'),
}
forms={m.group(2):m.group(1)+' '+m.group(2)+'('+m.group(3)+')' for m in re.finditer(r'^(void|float|uint32_t)\s+(\w+)\(([^)]*)\)\s*\{',code,re.M)}
assert set(forms)==set(info),(set(forms)-set(info),set(info)-set(forms))
e=lambda s:html.escape(str(s)).replace('\n','<br>')
sheets=[]
for i,(name,form) in enumerate(forms.items(),1):
 overview,processing,args,returns,inp,out,called=info[name]
 record(f'LATEST-FD-{i:02}','detailed','今回の仕様 / '+name,function=name,category='今回の仕様・define版',functionForm=form,overview=overview,processing=processing,args=args,returns=returns,input=inp,output=out,sourceFile='RemoteObstacleCar/RemoteObstacleCar.ino',calledFunctions=called,designLayout='function-sheet-v1',content=overview+'\n'+processing,status='実機未検証')
 sheets.append('<section><h1>関数設計書</h1><h2>'+e(name)+'</h2><table><tr><th>機能概要</th><td colspan="2">'+e(overview)+'</td></tr><tr><th>処理内容</th><td colspan="2">'+e(processing)+'</td></tr><tr><th>関数形式</th><td colspan="2"><code>'+e(form)+'</code></td></tr><tr><th rowspan="4">入出力</th><th>引数</th><td>'+e(args)+'</td></tr><tr><th>戻り値</th><td>'+e(returns)+'</td></tr><tr><th>入力</th><td>'+e(inp)+'</td></tr><tr><th>出力</th><td>'+e(out)+'</td></tr><tr><th>ソースファイル</th><td colspan="2">RemoteObstacleCar/RemoteObstacleCar.ino</td></tr><tr><th>呼び出し関数</th><td colspan="2">'+e(called)+'</td></tr></table><p>今回の仕様：15cmで警告・5cmで強制後退・後方検知で前進 / 実機未検証</p></section>')
style='body{font-family:Meiryo,sans-serif;color:#17212e;margin:24px;background:#edf1f4}main,section{max-width:1040px;background:white;padding:28px;margin:0 auto 24px}h1{font-size:26px}h2{font-size:20px}table{width:100%;border-collapse:collapse}td,th{border:1px solid #aab4bf;padding:13px;text-align:left;vertical-align:top}th{background:#f1f5f8}td{line-height:1.7;overflow-wrap:anywhere}a,button{color:#086b70}nav{display:flex;flex-wrap:wrap;gap:18px;margin:20px 0}pre{overflow:auto;background:#101b29;color:#e3edf7;padding:20px;font-size:14px;line-height:1.65;border-radius:8px}.note{border-left:4px solid #d88a24;padding:12px;background:#fff6e8}@media(max-width:600px){body{margin:8px}main,section{padding:16px}table{font-size:13px}}@media print{body{background:white;margin:0}section{break-after:page}section:last-child{break-after:auto}nav,button{display:none}pre{white-space:pre-wrap;background:white;color:black}}'
htmlpage=lambda title,body:'<!doctype html><html lang="ja"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>'+html.escape(title)+'</title><style>'+style+'</style>'+body+'</html>'
(design/'FunctionDesign.html').write_text(htmlpage('今回の仕様版・関数設計書', ''.join(sheets)))
nav='<nav><a href="../../code/RemoteObstacleCar/RemoteObstacleCar.ino" download>この.inoをダウンロード</a><a href="../../code/RemoteObstacleCar/RemoteObstacleCar.zip" download>コード・手順・設計書一式</a><a href="FunctionDesign.html">関数設計書</a><a href="../../code/RemoteObstacleCar/README.md">配線・使用手順</a></nav>'
summary='<table><tr><th>条件</th><th>動作</th></tr><tr><td>上・下・右・左</td><td>リモコンで前進・後退・右旋回・左旋回</td></tr><tr><td>前方15cm以内</td><td>近づくほどブザー間隔を短くする（15cm=600ms、10cm=350ms、5cm以下=100ms）</td></tr><tr><td>前方5cm以下</td><td>強制後退。15cmより離れたら停止</td></tr><tr><td>後方障害物</td><td>自動前進。後方検知がなくなったら停止</td></tr><tr><td>OK・超音波失敗・前後がふさがった</td><td>停止。回避完了後は新しい方向操作で再開</td></tr></table>'
(design/'Code.html').write_text(htmlpage('今回の仕様：define版 RemoteObstacleCar', '<main><h1>今回の仕様：define宣言の1ファイル版</h1><p>RemoteObstacleCar.ino / Arduino UNO・OSOYOO Model X</p>'+summary+nav+'<p class="note">配線は前の修正版と同じです。元のcode.txtからは、右ENAをD9→D5、サーボ信号をD5→D9へ移してください。EchoはD3、後方はD0/D2です。UNOコンパイル・模擬I/Oテスト済み、実機走行は未検証。</p><button id="copy" type="button">コードをコピー</button><span id="message" role="status"></span><pre><code id="source">'+html.escape(code)+'</code></pre></main><script>document.getElementById("copy").addEventListener("click",async()=>{try{await navigator.clipboard.writeText(document.getElementById("source").textContent);document.getElementById("message").textContent=" コピーしました"}catch{document.getElementById("message").textContent=" コードを選択してコピーしてください"}});</script>'))
# 既存の要件・テストを今回のユーザー指定へ整合させる。
updates={
'FR-005':('前方超音波距離測定・15cmと5cmの判定','前方を60ms周期で測定。15cm以内で距離警告、5cm以下で強制後退。無応答は停止。'),
'FR-006':('後方赤外線障害物検知','後方左右いずれかのデジタル入力がLOWなら障害物あり。取り付け・極性は実機確認。'),
'FR-007':('距離に応じたブザー警告','15cm以内でブザー。15cm=600ms、10cm=350ms、5cm以下=100ms周期。ONは35ms、間は線形補間。'),
'FR-008':('前方5cm以下の強制後退','前方5cm以下でリモコン指令を上書きし後退。後方も検知なら停止。OKと測定失敗も停止優先。'),
'FR-009':('前方障害物からの自動後退','前方5cm以下→必要なら方向切替100ms停止→PWM80で後退。前方15cm超で停止し、次の方向操作を待つ。最大3秒で停止。'),
'FR-010':('後方障害物からの自動前進','後方いずれかLOWで自動前進。後方検知解除で停止。前方5cm以下も同時なら停止。'),
'FR-011':('前後がふさがった場合の停止','前方5cm以下かつ後方検知なら前進・後退・旋回せず停止。前方が5cm超15cm以内は警告域であり、後方回避の前進は可能。'),
'FR-012':('回避後の停止と新規操作待ち','回避終了・異常・打ち切りでは古いリモコン指令を解除。もう一度方向キーを押して操作を再開する。'),
'FR-014':('停止優先と強制回避','OK停止・測定失敗・前後同時停止を優先。次に前方5cm以下の後退または後方検知による前進、最後に手動操作。')}
for id,(title,content) in updates.items():
 r=dict(old[id]);r.update(title=title,content=content,status='実装中',updated=now,notes='今回のユーザー指定を反映。UNOコンパイル・模擬I/Oテスト済み、実機未検証。',links=links);records.append(r)
expectations={
'TC-005':'15cm超は警告なし、15cm以内は警告、5cm以下は後退。無応答は停止。',
'TC-007':'15/10/5cmで周期600/350/100ms、ON35ms。近づくほど間隔が短い。',
'TC-008':'前方5cm以下で強制後退する。後方も検知時・OK停止中は停止。',
'TC-009':'強制後退は前方15cm超で停止。後方検知が加われば停止。3秒上限も停止。',
'TC-010':'後方いずれかLOWで前進。検知解除で停止。前方5cm以下も同時なら停止。',
'TC-011':'前方5cm以下と後方同時検知で停止する。',
'TC-012':'回避終了・異常停止後は旧指令で走り出さず、新しい方向操作で再開する。',
'TC-014':'OKと異常・前後同時停止が最優先。自動回避中は方向指令より回避を優先。',
'TC-015':'今回の初期設定AUTO_MAX_MS=3000で、回避開始から3秒以上継続したら停止し、新しい方向操作まで再始動しない。'}
for id,expected in expectations.items():
 r=dict(old[id]);r.update(expected=expected,status='未実施',updated=now,actual='',notes='実機テスト未実施。模擬I/Oテストの結果はBUILD_CHECK.mdに分離。',links=links)
 if id=='TC-008':r['title']='前方5cm以下の強制後退'
 records.append(r)
r=dict(old['BUG-001']);r.update(title='5cm境界と動作の仕様を確定',status='解決済み',content='今回のユーザー指定により、5cm以下で強制後退する。前方5cm以下と後方同時、OK、測定失敗は停止。',action='RemoteObstacleCar.inoと要件・テスト期待値を今回の指定へ更新。実機試験は未実施。',updated=now,links=links);records.append(r)
# 前の生成版は参考資料として保持し、違う動作であることを一覧に明示。
for r0 in catalog['records']:
 if r0['id'].startswith('FCODE-') and r0['kind'] in ('codes','detailed','basic'):
  r=dict(r0)
  if not r['title'].startswith('旧版参考 / '):r['title']='旧版参考 / '+r['title']
  prefix='旧版参考（5cm以下停止の設計）。今回使うコードはRemoteObstacleCar.inoです。\n'
  if not str(r.get('content','')).startswith('旧版参考'):r['content']=prefix+r.get('content','')
  records.append(r)
zipfile_path=src/'RemoteObstacleCar.zip'
with zipfile.ZipFile(zipfile_path,'w',zipfile.ZIP_DEFLATED) as z:
 for f in src.iterdir():
  if f.is_file() and f.suffix!='.zip':z.write(f,'RemoteObstacleCar/'+f.name)
 z.write(design/'FunctionDesign.html','FunctionDesign.html')
 # テストはリポジトリと同じ相対配置を維持する。
 z.write(src/'RemoteObstacleCar.ino','docs/code/RemoteObstacleCar/RemoteObstacleCar.ino')
 for f in (root/'tests/remote-obstacle').rglob('*'):
  if f.is_file():z.write(f,str(f.relative_to(root)))
for id,title,path,category in [
('LATEST-INO','今回の仕様・define宣言の1ファイルコード','code/RemoteObstacleCar/RemoteObstacleCar.ino','05_ソースコード'),
('LATEST-ZIP','今回の仕様・コードと手順と設計書一式','code/RemoteObstacleCar/RemoteObstacleCar.zip','05_ソースコード'),
('LATEST-CODE-PAGE','今回の仕様・コード表示とコピー','design/RemoteObstacleCar/Code.html','05_ソースコード'),
('LATEST-GUIDE','今回の仕様・配線と使用手順','code/RemoteObstacleCar/README.md','12_参考資料'),
('LATEST-DESIGN','今回の仕様・表形式の関数設計書','design/RemoteObstacleCar/FunctionDesign.html','03_詳細設計')]:
 p=docs/path;files.append(dict(id=id,group_id=id,title=title,name=p.name,url=path,size=p.stat().st_size,category=category,description='15cmで段階警告・5cmで強制後退・後方検知で前進。配線と使用手順を確認。実機未検証。',visibility='public',created=now,shared=False))
ri={r['id'] for r in records};fi={f['id'] for f in files}
catalog['records']=[r for r in catalog['records'] if r['id'] not in ri]+records
catalog['files']=[f for f in catalog['files'] if f['id'] not in fi]+files
catalog['updatedAt']=now
(docs/'shared/catalog.json').write_text(json.dumps(catalog,ensure_ascii=False,indent=2)+'\n')
print('Generated latest single-file code page,',len(forms),'function sheets and',len(files),'downloads')
