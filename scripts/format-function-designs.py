"""Format existing function contracts as the user-supplied design sheet; never change firmware."""
from pathlib import Path
import json,re,html,zipfile,io,datetime
ROOT=Path(__file__).resolve().parents[1]
NOW=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=9))).isoformat(timespec='seconds')
NOINPUT='なし（引数のみ。外部I/O・共有設定の読み取りなし）'
NOOUTPUT='なし（戻り値のみ。状態更新・外部I/O出力なし）'
IO={
'pinsReady':('BoardConfig.hのピン番号・PWM設定・配線確認・停止境界確認・ブザー確認フラグ。',NOOUTPUT),
'initializeHardware':('BoardConfig.hのピン番号・各確認フラグ。','方向/PWM/TRIGをLOW、ブザーをOFFに初期化。センサー入力モードを設定し、左右PWMへ0を出力。'),
'writeWheel':('BoardConfig.hの配線/確認フラグ（pinsReadyで参照）。','指定輪のPWMを0にし、方向端子と飽和後の速度PWMへ出力する。'),
'stopMotors':('BoardConfig.hの左右モーター端子と確認フラグ。','左右PWMに0、左右方向端子にLOWを出力（惰性停止想定）。'),
'readUltrasonic':('Echo端子のパルス幅、BoardConfig.hのTRIG/ECHO/タイムアウトと確認フラグ。','TRIG端子へLOW→HIGH→LOWの測定パルスを出力する。'),
'sampleSensors':('前方Echo信号・後方赤外線端子のデジタルレベル、BoardConfig.hのピン/極性設定。','前方測定用TRIGパルスを出力。SensorFrameは戻り値として返す。'),
'applyOutput':('BoardConfig.hのモーター/ブザー端子・極性・確認フラグ。','左右モーターの方向/PWMと、ブザーON/OFFを出力する。'),
'codesReady':('BoardConfig.hの5キー値とREMOTE_CODES_CONFIRMED。',NOOUTPUT),
'decodeCommand':('BoardConfig.hのリモコンキー値と確認フラグ。','認識成功時のみ参照引数outへrobot::Motionを代入する。'),
'pollRemote':('IrReceiverの受信データ（protocol/address/command/flags）、BoardConfig.hのキー/アドレス設定。','受信器をresumeし、正当な受信時にstateのcommand/receivedAt/sequence/validを更新する。'),
'resetController':(NOINPUT,'参照引数cの全メンバーを初期化し、phaseStartedをnowへ更新する。'),
'enterPhase':(NOINPUT,'参照引数cのphase/phaseStarted/clearTiming/blockedSequenceを更新する。'),
'clearStable':(NOINPUT,'参照引数cのclearTiming/clearStartedを更新する。'),
'latchStop':(NOINPUT,'参照引数cをLatchedStopへ遷移し、latchedReasonを更新する。停止指令は戻り値。'),
'acceptRemote':(NOINPUT,'参照引数rのcommand/sequence（新規受信のみ）とreceivedAt/validを更新する。'),
'updateController':(NOINPUT,'参照引数cの状態/タイマー/停止理由を更新する。動作指令Outputは戻り値であり、この関数自体はI/Oを駆動しない。')}
def enrich(data):
 byid={r['id']:r for r in data['records']}
 for r in data['records']:
  if r['kind']!='detailed':continue
  content=r.get('content','');name=r.get('function','').split('::')[-1]
  p=re.search(r'プロトタイプ：([^\n]+)',content);sf=re.search(r'ソース：([^\n]+)',content)
  if p:
   form=re.sub(r'^inline\s+','',p[1]).strip()
   form=re.sub(r'\b'+re.escape(name)+r'\s*\(',r['function']+'(',form,count=1)
   form=re.sub(r'(?<![\w:])\b(Parameters|Motion|SensorFrame|Controller|RemoteState|Reason|Output|WheelPair|Phase)\b',r'robot::\1',form)
   if not form.endswith(';'):form+=';'
  else:form=(r.get('function','未確定')+'（関数形式は未確定）')
  steps=content.split('\n\n呼出先：',1)[0].removeprefix('処理手順：')
  sentences=[s.strip()+'。' for s in steps.split('。') if s.strip()]
  if len(sentences)>1:steps='\n'.join(f'{i+1}. {s}' for i,s in enumerate(sentences))
  code=''
  for link in r.get('links','').split(', '):
   item=byid.get(link,{})
   if item.get('kind')=='codes' and item.get('title','').startswith(name+'：'):
    code=item.get('code','');break
  if code:
   # Read actual call expressions from the linked function, dropping comments and control/type casts.
   body=re.sub(r'//[^\n]*','',code)
   calls=sorted(set(re.findall(r'\b([A-Za-z_]\w*(?:(?:::|\.)\w+)*)\s*\(',body))-{'if','for','while','switch','return','sizeof','uint32_t','uint8_t','uint16_t','int16_t','F',name})
   calltext='\n'.join(calls) if calls else 'なし'
  else:calltext='未確定（関数の実装は未登録）'
  inp,out=IO.get(name,(NOINPUT,NOOUTPUT))
  if not p:inp='未確定（関数設計案）';out='未確定（関数設計案）'
  r.update(overview=r['title'].split('：',1)[-1],processing=steps,functionForm=form,input=inp,output=out,sourceFile=sf[1] if sf else '未確定（ソース未登録）',calledFunctions=calltext,updated=NOW,designLayout='function-sheet-v1')
 return data
LABELS=['機能概要','処理内容','関数形式','引数','戻り値','入力','出力','ソースファイル','呼び出し関数']
KEYS=['overview','processing','functionForm','args','returns','input','output','sourceFile','calledFunctions']
def markup(r):
 e=lambda v:html.escape(str(v or 'なし'))
 out=f'<article class="function-sheet"><div class="function-sheet-top"><span>{e(r["id"])}</span><span>{e(r.get("status"))}</span></div><h2>関数設計書</h2><p class="sheet-name">{e(r.get("function",r["title"]))}</p><div class="function-sheet-scroll"><table aria-label="{e(r.get("function"))} 関数設計書"><colgroup><col/><col/><col/></colgroup><tbody>'
 for i,(label,key) in enumerate(zip(LABELS,KEYS)):
  v=e(r.get(key));v='<code>'+v+'</code>' if key=='functionForm' else v
  if 3<=i<=6:out+='<tr>'+('<th rowspan="4" scope="rowgroup">入出力</th>' if i==3 else '')+f'<th scope="row">{label}</th><td>{v}</td></tr>'
  else:out+=f'<tr><th colspan="2" scope="row">{label}</th><td>{v}</td></tr>'
 return out+'</tbody></table></div><p class="sheet-footnote">入力：引数以外に読み取る設定・状態・センサー情報。出力：戻り値以外の状態更新・外部機器への出力。</p></article>'
def main():
 path=ROOT/'docs/data.json';data=enrich(json.loads(path.read_text()));items=[r for r in data['records'] if r['kind']=='detailed'];design=ROOT/'docs/design';design.mkdir(parents=True,exist_ok=True)
 css=re.search(r'export const sheetCss = String.raw`(.*?)`;', (ROOT/'src/FunctionDesign.tsx').read_text(),re.S)[1]
 doc='<!doctype html><html lang="ja"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>関数設計書</title><style>'+css+'</style></head><body class="function-document">'+''.join(markup(r) for r in items)+'</body></html>'
 (design/'FunctionDesign.html').write_text(doc)
 # Markdown supports the same 3-column table with merged I/O rows through embedded HTML.
 (design/'FunctionDesign.md').write_text('# 関数設計書\n\n写真の書式に合わせた帳票。引数・戻り値と、外部入力・副作用の出力を区別する。実機確認待ちの記録はそのまま保持する。\n\n'+'\n\n'.join(markup(r) for r in items)+'\n')
 for f in data['files']:
  if f['id']=='FILE-DESIGN':f.update(size=(design/'FunctionDesign.md').stat().st_size,change='画像に合わせた表形式へ変更')
 htmlitem={'id':'FILE-DESIGN-HTML','group_id':'FILE-DESIGN-HTML','name':'FunctionDesign.html','title':'関数設計書（表形式・印刷用HTML）','url':'design/FunctionDesign.html','size':(design/'FunctionDesign.html').stat().st_size,'category':'03_詳細設計','description':'機能概要・処理内容・関数形式・入出力・ソースファイル・呼び出し関数。ブラウザの印刷からPDF保存可能。','created':NOW,'change':'表形式版','visibility':'public','generator':'robot-library-v1','links':'FILE-DESIGN'}
 data['files']=[f for f in data['files'] if f['id']!='FILE-DESIGN-HTML']+[htmlitem]
 zpath=ROOT/'docs/code/RobotCarLibrary.zip'
 if zpath.exists():
  source=zpath.read_bytes()
  with zipfile.ZipFile(io.BytesIO(source)) as zin,zipfile.ZipFile(zpath,'w',zipfile.ZIP_DEFLATED) as zout:
   for old in zin.infolist():
    if old.filename not in ['design/FunctionDesign.md','design/FunctionDesign.html']:zout.writestr(old,zin.read(old.filename))
   zout.write(design/'FunctionDesign.md','design/FunctionDesign.md');zout.write(design/'FunctionDesign.html','design/FunctionDesign.html')
  for f in data['files']:
   if f['id']=='FILE-SOURCE':f['size']=zpath.stat().st_size;f['change']='関数設計書を表形式へ更新（コード内容は変更なし）'
 path.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n')
 print(f'Formatted {len(items)} function design sheets ({sum(r["id"].startswith("FD-") for r in items)} implemented function contracts).')
if __name__=='__main__':main()
