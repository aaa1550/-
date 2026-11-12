import json,html,pathlib,datetime,zipfile,re
root=pathlib.Path(__file__).resolve().parents[1];d=root/'docs';src=d/'code/FlowchartRobot';out=d/'design/FlowchartRobot';out.mkdir(parents=True,exist_ok=True)

def markdown_html(text):
 def inline(s):
  s=html.escape(s)
  s=re.sub(r'`([^`]+)`',r'<code>\1</code>',s)
  return re.sub(r'\*\*([^*]+)\*\*',r'<strong>\1</strong>',s)
 lines=text.splitlines();out=[];i=0
 while i<len(lines):
  line=lines[i]
  if line.startswith('|'):
   rows=[]
   while i<len(lines) and lines[i].startswith('|'):
    cells=[c.strip() for c in lines[i].strip().strip('|').split('|')]
    if not all(re.fullmatch(r':?-+:?',c) for c in cells): rows.append(cells)
    i+=1
   out.append('<table>'+''.join('<tr>'+''.join(('<th>' if n==0 else '<td>')+inline(c)+('</th>' if n==0 else '</td>') for c in row)+'</tr>' for n,row in enumerate(rows))+'</table>')
   continue
  if line.startswith('#'):
   n=len(line)-len(line.lstrip('#'));out.append(f'<h{n}>'+inline(line[n:].strip())+f'</h{n}>')
  elif line.strip():out.append('<p>'+inline(line)+'</p>')
  i+=1
 return '\n'.join(out)

# 関数設計は掲載するソースの実装に対応。追加資料の診断を含む。
items=[
('setup','void setup()','初期状態を安全な停止にする','出力・入力ピンを初期化し、モーターとブザーを停止。サーボを90度へ合わせ、IR受信を開始する。','なし','なし','UNOのピン設定、起動状態','ピン設定、PWM0、サーボ90度、受信開始','FlowchartRobot.ino','stop_Stop, Servo.attach, Servo.write, IrReceiver.begin'),
('loop','void loop()','メインフローチャートの順番で処理を繰り返す','リモコン→超音波（60ms周期）→後方赤外線→車両状態→ブザー→行動。超音波待機後にも受信を確認する。','なし','なし','センサー・IR信号、millis()','モーター・ブザーへの出力','FlowchartRobot.ino','do_IR_Tick, updateUltrasonicState, updateRearIRState, updateVehicleState, warning, do_Drive_Tick'),
('do_IR_Tick','void do_IR_Tick()','リモコンをRMへ変換する','24=上、82=下、90=右、8=左、28=STOP。既知フレームで連番更新。リピートは受信時刻だけ更新。未知キーは無視、オーバーフローは操作無効。','なし','なし','IrReceiver.decodedIRData','RM、sequence、remoteAt、remoteSeen','FlowchartRobot.ino','IrReceiver.decode, IrReceiver.resume, millis'),
('watch','unsigned long watch()','超音波Echoのパルス長を計測する','Trigに10μsのパルスを出し、Echo HIGHを最大15000μs測定。タイムアウト時は0。','なし','Echo HIGHの時間（μs）、失敗時0','Echo D3、Trig D10','パルス幅','FlowchartRobot.ino','digitalWrite, delayMicroseconds, pulseIn'),
('updateUltrasonicState','void updateUltrasonicState()','前方の距離状態を更新する','watchの結果をclassifyFrontで分類し、sensors.frontへ保存する。','なし','なし','Echoパルス幅','CLEAR / CAUTION / EMERGENCY / INVALID','FlowchartRobot.ino','watch, flowcar::classifyFront'),
('updateRearIRState','void updateRearIRState()','後方障害物の有無を更新する','D0/D2のどちらかがLOWならrear=true。Mへ直接代入せず、前方状態と統合する。','なし','なし','左後方D0、右後方D2（検知LOW）','sensors.rear','FlowchartRobot.ino','digitalRead'),
('updateVehicleState','void updateVehicleState()','受信有効性とセンサーから車両状態を決める','受信から350ms以内ならlinkAlive。純粋制御関数へ状態を渡し、driveとMを更新する。','なし','なし','RM、sequence、センサー、受信時刻','drive、M、controller','FlowchartRobot.ino','millis, flowcar::update'),
('warning','void warning()','制御を止めずに警告音を出す','警告開始から50ms ON、50ms OFF、50ms ON。600ms周期で繰り返す。警告不要ならHIGH。toneとdelayは使わない。','なし','なし','drive.buzzer、millis()','D13（LOWで鳴動）','FlowchartRobot.ino','digitalWrite, millis'),
('setMotor','void setMotor(uint8_t in1, uint8_t in2, uint8_t pwm, int8_t percent)','符号付き百分率を方向とPWMへ変換する','正数=正転、負数=逆転、0=停止。絶対値×255/100を四捨五入してPWMへ出力する。','in1/in2:方向ピン、pwm:速度ピン、percent:-100〜100（呼出元で保証）','なし','モーターの符号付き速度','方向ピンとPWM','FlowchartRobot.ino','digitalWrite, analogWrite'),
('stop_Stop','void stop_Stop()','左右モーターへ停止指令を出す','まず両PWMを0にし、その後方向端子をLOWへ設定。前回出力値を0へ更新する。','なし','なし','固定ピン設定','PWM0、方向LOW、appliedRight/Left=0','FlowchartRobot.ino','analogWrite, digitalWrite'),
('do_Drive_Tick','void do_Drive_Tick()','車両状態を実際のモーター出力へ変える','左右0なら停止。前回と符号が反転する場合も先にPWM0。左右の速度をsetMotorへ渡す。自動回避の100ms停止は制御側が担当。','なし','なし','drive.right、drive.left','左右モーター出力','FlowchartRobot.ino','stop_Stop, setMotor'),
('flowcar::classifyFront','Front classifyFront(unsigned long echoUs)','距離境界を判定する','0はINVALID。元コードの係数0.01657cm/μsを維持。丸め前の距離で5cm以下はEMERGENCY、15cm以下はCAUTION、それ以外CLEAR。','echoUs:Echoパルス幅（μs）','Front列挙型','パルス幅','前方状態','FlowchartLogic.h','なし'),
('flowcar::stop','Output stop(bool alarm=false, uint8_t m=0)','停止出力を生成する','右0%、左0%、指定された警告とMをOutputに格納する。','alarm:警告、m:車両状態番号','Output','指定フラグ','停止出力値（I/O操作なし）','FlowchartLogic.h','なし'),
('flowcar::manual','Output manual(Remote rm)','手動操作の速度を決める','前進50%、後退-30%、左=(右30/左-30)、右=(右-30/左30)。未定義またはSTOPは0%。','rm:STOP/FORWARD/BACKWARD/RIGHT/LEFT','Output','RM','左右速度、M','FlowchartLogic.h','flowcar::stop'),
('flowcar::update','Output update(Controller& c, Sensors s, Remote rm, bool linkAlive, uint32_t sequence, uint32_t now)','優先停止と低速回避を管理する','STOP/通信切れ→測定異常→5cm以下→前後同時→新規操作待ち→回避→手動。回避開始100ms停止、反対方向10%、障害物解消または1500msで停止。再開は新しい操作連番が必要。','c:前回状態（更新）、s:センサー、rm:操作、linkAlive:受信有効、sequence:操作連番、now:ms','Output','状態、センサー、リモコン、時刻','制御状態の更新、左右速度、警告、M','FlowchartLogic.h','flowcar::stop, flowcar::manual'),
('setup [TrackerMonitor]','void setup()','追跡センサー診断の入力と表示を準備する','改訂版のモーター出力を停止、ブザーを停止し、画像のIR1〜IR5=A0〜A4を入力へ設定。Serialを115200bpsで開始。D0のセンサー信号線は外して使用する。','なし','なし','A0〜A4、固定ピン設定','モーター停止、シリアル見出し','examples/TrackerMonitor/TrackerMonitor.ino','pinMode, analogWrite, digitalWrite, Serial.begin, Serial.println'),
('loop [TrackerMonitor]','void loop()','5チャンネルの生値を表示する','millisの差分で200ms周期に読み取り、IR1〜IR5をCSV表示。LOW/HIGHの生値のみを示し、障害物や白黒を決めつけない。','なし','なし','A0〜A4のdigitalRead','115200bpsのCSV表示','examples/TrackerMonitor/TrackerMonitor.ino','millis, digitalRead, Serial.print, Serial.println'),

]
now=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=9))).isoformat();records=[];sheets=[]
for i,(name,form,overview,proc,args,returns,inp,output,source,called) in enumerate(items,1):
 r=dict(id=f'FCODE-FD-{i:02}',kind='detailed',title=f'フローチャート版 / {name}',function=name,overview=overview,processing=proc,functionForm=form,args=args,returns=returns,input=inp,output=output,sourceFile=(source if source.startswith('examples/') else 'FlowchartRobot/'+source),calledFunctions=called,content=overview+'\n'+proc,status='実機未検証',designLayout='function-sheet-v1',visibility='public',updated=now,links='FCODE-SOURCE, FC-001, FC-002, FC-003, FC-004')
 records.append(r)
 def e(x):return html.escape(x).replace('\n','<br>')
 sheets.append('<section><h1>関数設計書</h1><h2>'+e(name)+'</h2><table><tr><th>機能概要</th><td colspan="2">'+e(overview)+'</td></tr><tr><th>処理内容</th><td colspan="2">'+e(proc)+'</td></tr><tr><th>関数形式</th><td colspan="2"><code>'+e(form)+'</code></td></tr><tr><th rowspan="4">入出力</th><th>引数</th><td>'+e(args)+'</td></tr><tr><th>戻り値</th><td>'+e(returns)+'</td></tr><tr><th>入力</th><td>'+e(inp)+'</td></tr><tr><th>出力</th><td>'+e(output)+'</td></tr><tr><th>ソースファイル</th><td colspan="2">'+e(source if source.startswith('examples/') else 'FlowchartRobot/'+source)+'</td></tr><tr><th>呼び出し関数</th><td colspan="2">'+e(called)+'</td></tr></table><p>code.txt参考・提供フローチャート対応版 / 実機未検証</p></section>')
(out/'FunctionDesign.html').write_text('<!doctype html><html lang="ja"><meta charset="utf-8"><title>フローチャート版 関数設計書</title><style>body{font-family:Meiryo,sans-serif;background:#eee;color:#111;margin:24px}section{max-width:960px;background:white;padding:32px;margin:0 auto 30px}h1{text-align:center}h2{font-size:19px}table{width:100%;border-collapse:collapse}th,td{border:1px solid;padding:14px;vertical-align:top;text-align:left}th{width:20%}td{line-height:1.7;overflow-wrap:anywhere}code{font-size:14px}p{font-size:12px}@media print{body{background:white;margin:0}section{break-after:page;margin:0;padding:12mm;box-sizing:border-box}section:last-child{break-after:auto}}</style>'+''.join(sheets)+'</html>',encoding='utf8')
for i,name in enumerate(['FlowchartRobot.ino','FlowchartLogic.h','BoardPins.h'],1):
 records.append(dict(id=f'FCODE-CODE-{i}',kind='codes',title='フローチャート準拠版 / '+name,filename=name,code=(src/name).read_text(),content='code.txt・フローチャート・追加画像7枚を照合して作成。ENA D9→D5、サーボD5→D9の配線変更が必要。READMEを確認。',status='実機未検証',libraries='IRremote 4.4.1 / Servo 1.2.2 / Arduino UNO',pins='右ENA D5、左ENB D6、サーボD9。ほかREADME参照',visibility='public',updated=now,links='FCODE-SOURCE, FCODE-GUIDE'))
records.append(dict(id='FCODE-BASIC',kind='basic',title='code.txt参考・フローチャート版の配線と仕様',category='コード運用',content=(src/'README.md').read_text(),status='実機未検証',visibility='public',updated=now,links='FCODE-SOURCE, FCODE-GUIDE'))
records.append(dict(id='FCODE-TRACKER-CODE',kind='codes',title='画像5.jpg対応 / 5チャンネル追跡センサー診断',filename='TrackerMonitor.ino',code=(d/'code/examples/TrackerMonitor/TrackerMonitor.ino').read_text(),content='画像のIR1=A0、IR2=A1、IR3=A2、IR4=A3、IR5=A4を200ms周期で読み取り、115200bpsのCSVで表示。D0のセンサー線を外して使用。モーター電源を切って診断。',status='実機未検証',visibility='public',updated=now,links='FCODE-REVIEW, FCODE-TRACKER'))
records.append(dict(id='FCODE-REVIEW-BASIC',kind='basic',title='追加画像7枚・配線照合とコードへの反映',category='配線・資料照合',content=(src/'SourceReview.md').read_text(),status='実機未検証',visibility='public',updated=now,links='FCODE-REVIEW, FCODE-PINS, FCODE-TRACKER'))
report=markdown_html((src/'SourceReview.md').read_text())
images=[('２.jpg','ec82b19d-6a2e-47aa-a61d-8a01114b8820.jpg'),('3.jpg','875dafc8-113b-4a5e-8b3e-18c9cade67d7.jpg'),('4.jpg','105520db-e24b-4b38-b629-aeddf2082fe1.jpg'),('配線.jpg','ddf534b4-1e18-4a0a-b15e-aa49d9b97461.jpg'),('5.jpg','298acfa8-98a0-4e8a-9e7f-4cf9d3681eec.jpg'),('6.jpg','18bbd032-6892-43e5-9321-d0d5ea4bc7d8.jpg'),('7.png','a16569dd-494d-44fa-9ad8-470d0008739c.png')]
# ZIP展開時にも公開資料へアクセスできるよう、原本画像は絶対URLで参照する。
report += '<h2>確認した原本画像</h2>'+''.join('<figure><figcaption>'+html.escape(name)+'</figcaption><a href="https://aaa1550.github.io/-/shared/files/'+path+'"><img loading="lazy" alt="'+html.escape(name)+'" src="https://aaa1550.github.io/-/shared/files/'+path+'"></a></figure>' for name,path in images)
(out/'SourceReview.html').write_text('<!doctype html><html lang="ja"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>追加資料の照合・コード反映</title><style>body{font-family:Meiryo,sans-serif;max-width:1100px;margin:36px auto;padding:0 20px;line-height:1.8;color:#17212e}table{border-collapse:collapse;width:100%;font-size:14px}td,th{border:1px solid #aab3bd;padding:12px;text-align:left;vertical-align:top}th{background:#edf2f6}code{background:#edf2f6}img{max-width:100%;height:auto}figure{margin:24px 0;padding:16px;border:1px solid #ccc}h1{font-size:28px}a{color:#126572}@media(max-width:700px){table{display:block;overflow-x:auto}td{min-width:150px}}@media print{img{max-height:20cm}figure{break-inside:avoid}}</style>'+report+'</html>')

# ZIP内はArduino IDEで直接開ける同名フォルダ。
zip_path=d/'code/FlowchartRobot.zip'
with zipfile.ZipFile(zip_path,'w',zipfile.ZIP_DEFLATED) as z:
 for f in src.iterdir():z.write(f,'FlowchartRobot/'+f.name)
 z.write(out/'FunctionDesign.html','FunctionDesign.html')
 z.write(out/'SourceReview.html','SourceReview.html')
 z.write(d/'code/examples/TrackerMonitor/TrackerMonitor.ino','examples/TrackerMonitor/TrackerMonitor.ino')
 z.write(root/'tests/flowchart/logic_test.cpp','tests/logic_test.cpp')
files=[]
for id,title,path,category in [
 ('FCODE-REVIEW','追加資料7枚・配線照合と反映内容','design/FlowchartRobot/SourceReview.html','02_基本設計'),
 ('FCODE-PINS','統合コード・配線設定ヘッダー','code/FlowchartRobot/BoardPins.h','05_ソースコード'),
 ('FCODE-TRACKER','画像5.jpg対応・追跡センサー診断','code/examples/TrackerMonitor/TrackerMonitor.ino','05_ソースコード'),
 ('FCODE-SOURCE','code.txt参考・フローチャート準拠コード一式','code/FlowchartRobot.zip','05_ソースコード'),
 ('FCODE-GUIDE','フローチャート版・配線変更と使い方','code/FlowchartRobot/README.md','12_参考資料'),
 ('FCODE-DESIGN','フローチャート版・関数設計書（表形式）','design/FlowchartRobot/FunctionDesign.html','03_詳細設計'),
 ('FCODE-INO','フローチャート版・Arduinoスケッチ','code/FlowchartRobot/FlowchartRobot.ino','05_ソースコード'),
 ('FCODE-LOGIC','フローチャート版・制御ロジック','code/FlowchartRobot/FlowchartLogic.h','05_ソースコード')]:
 f=d/path;files.append(dict(id=id,group_id=id,title=title,name=f.name,url=path,size=f.stat().st_size,category=category,description='code.txtと元フローチャート対応。READMEの配線変更を確認。実機未検証。',visibility='public',created=now,shared=False))
catalog=json.loads((d/'shared/catalog.json').read_text())
record_ids={r['id'] for r in records};file_ids={r['id'] for r in files}
catalog['records']=[r for r in catalog['records'] if r['id'] not in record_ids]+records
catalog['files']=[r for r in catalog['files'] if r['id'] not in file_ids]+files
catalog['updatedAt']=datetime.datetime.now(datetime.timezone.utc).isoformat()
(d/'shared/catalog.json').write_text(json.dumps(catalog,ensure_ascii=False,indent=2)+'\n')
print('Generated',len(items),'function designs,',len(records),'records,',len(files),'download files')
