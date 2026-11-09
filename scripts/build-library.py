from pathlib import Path
import json,re,zipfile,datetime,hashlib
root=Path(__file__).resolve().parents[1];datafile=root/'docs/data.json';data=json.loads(datafile.read_text());stamp=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=9))).isoformat(timespec='seconds')
# Function contract: purpose, parameters, return, algorithm, errors, relevant FR/TC, category
M={
'elapsed':('経過時間判定','now: 現在ms、since: 基準ms、interval: 待機ms','bool: interval以上でtrue','uint32_tの差を取り、指定周期以上かを比較する。約49日でのmillis折り返しを差分で扱う。intervalは2^31未満で使う。','引数は単位msに統一。長期間放置や2周以上の時計差は対象外。','FR-012, TC-012','時間管理'),
'clampPwm':('PWM値の上限制限','value: 任意のint','uint8_t: 0〜255','負値は0、255超過は255、それ以外はuint8_tへ変換する。','飽和処理。負値を方向指定として渡さない。','FR-001, FR-002','モーター制御'),
'validParameters':('制御設定の整合性確認','p: Parameters設定','bool: 必須条件を満たすか','距離の有限性・正数・大小関係、速度の大小、主要なタイムアウトの整合を判定する。','falseなら統合制御は安全停止。値は公式仕様ではなく仮設定。','FR-014, TC-014','設定'),
'pulseToCm':('Echo幅を距離に換算','pulseUs: Echo HIGH幅µs','float: 距離cm、0µsはNAN','pulseUs × 0.0343 / 2で往復の伝播時間を距離へ換算する。温度補正は行わない。','timeoutの0を0cmの障害物と混同せずNANにする。','FR-005, TC-005, FC-002','超音波センサー'),
'validDistance':('距離の有効範囲判定','cm: 距離cm','bool: 有限で2〜200cm','センサー用の仮の有効範囲を含む判定を行う。','NAN・無限大・範囲外はfalse。実センサーに応じて範囲変更が必要。','FR-005, FR-013, TC-005, TC-013','超音波センサー'),
'frontEmergency':('緊急停止距離判定','cm: 距離cm、p: 停止設定','bool: 危険距離か','emergencyInclusive=trueは<=、falseは<で比較する。','5cm境界は仕様未確定。既定の<=は仮設定で、確認フラグを立てるまで実機駆動しない。','FR-008, TC-008, BUG-001, FC-002','緊急停止'),
'frontWarning':('前方警戒距離判定','cm: 距離cm、p: 警戒距離','bool: 有限かつ警戒距離以下','前方15cm以下を警戒とする。NANは別の異常判定で停止する。','有効性全体はsensorsFreshで判定し、単独利用で安全確認しない。','FR-005, FR-009, TC-009','障害物検知'),
'rearFromLevel':('後方赤外線の極性変換','levelHigh: HIGHならtrue、activeLow: LOWで検知する設定','bool: 障害物あり','アクティブLOW/HIGHを設定に従って論理的な検知有無へ変換する。','極性は実センサーで確認。デジタル入力のみでは断線を保証できない。','FR-006, TC-006, FC-003','赤外線センサー'),
'sensorsFresh':('センサー正常性・鮮度判定','s: 取得値、now: 現在ms、p: 最大経過時間','bool: 前後有効かつ期限内','前方valid・距離範囲・後方validを確認し、sampledAtからの経過がsensorMaxAgeMs未満か確認する。','期限ちょうどでfalse。後方のvalidはソフトウェア上の読取可否であり自己診断ではない。','FR-013, TC-013','安全制御'),
'remoteFresh':('リモコン通信期限判定','r: リモコン状態、now: 現在ms、p: 期限','bool: validかつ期限内','直近の受信からremoteTimeoutMs未満であることを確認する。','期限ちょうどでfalse。受信断は手動操作だけでなく自動回避も停止させる。','FR-001, FR-014, TC-014','赤外線リモコン'),
'motionAllowed':('移動方向の安全判定','m: 動作、s: 前後情報、p: 設定','bool: 動作許可','前進は前方警戒なし、後退は後方検知なし、旋回は前後とも安全な場合に許可する。停止は常に許可。','鮮度は呼出側で確認。側方センサーがないため旋回時の側面安全は保証しない。','FR-011, FR-014, TC-011, TC-014','安全制御'),
'wheelsFor':('車両動作を左右速度へ変換','m: 動作、pwm: 0〜255','WheelPair: 左右の符号付き速度','前進(+,+)、後退(-,-)、左(-,+)、右(+,-)、停止(0,0)へ変換する。','車輪・配線の極性はHardwareAdapterで調整する。','FR-001, FR-002, FR-003, FR-004, TC-001, TC-004','モーター制御'),
'median3':('計測表示用3点中央値','a,b,c: 3つの距離','float: 中央値、無効値を含む場合NAN','3値を比較して中央値を返す。診断・表示用で、緊急停止の生値判定には使用しない。','NAN混入時はNAN。フィルターによる検知遅れを安全判定へ持ち込まない。','FR-005, TC-005','計測補助'),
'legacyRearState':('原本の変数Mへ変換','blocked: 後方検知','uint8_t: 検知2、未検知3','原本FC-003のM=2/M=3をそのまま表現する。','M以外の原本変数の意味を推測して補わない。','FR-006, TC-006, FC-003','状態管理'),
'makeStop':('停止出力の生成','reason: 理由、warning: ブザー要求（既定false）','Output: Stop・PWM0・警告・理由','全モーションをStopにし、理由と警告のみを保持する。','出力を作るのみ。実モーター停止にはapplyOutputが必要。','FR-008, TC-008','緊急停止'),
'resetController':('制御コンテキスト初期化','c: 更新対象、now: 現在ms','void','Controllerの全状態を既定値へ戻し、phaseStartedをnowへ設定する。','運転中の無条件リセットは禁止。setupまたは検証時に使用。','FR-012, TC-012','状態管理'),
'enterPhase':('状態遷移の記録','c: 対象、p: 次状態、now: 時刻、sequence: リモコン世代','void','状態・開始時刻・抑止対象シーケンスを設定し、安全確認タイマーを解除する。','latchedReasonを初期化しない。呼出側が停止理由を設定する。','FR-009, FR-010, FC-004','状態管理'),
'clearStable':('安全状態の連続継続確認','c: タイマー保存先、clear: 安全判定、now: ms、holdMs: 継続時間','bool: 安全が所定時間継続','安全初回に開始時刻を保持し、途中で不安全なら解除する。所定時間を満たすまでfalse。','瞬間的な安全値で通常操作へ復帰させない。','FR-012, TC-012','状態管理'),
'latchStop':('解除条件つき停止の保持','c: 状態、reason: 原因、now: ms、seq: 世代','Output: 警告つき停止','LatchedStopへ遷移し、発生理由を保存して停止出力を返す。','有効なSTOPの再操作と前後安全の連続確認まで解除しない。','FR-008, FR-013, TC-008, TC-013','緊急停止'),
'acceptRemote':('受信コマンド・世代の更新','r: 更新先、command: 動作、now: ms、repeat: リピートか','void','新規フレームのみコマンドとsequenceを更新。リピートは受信時刻のみ延長する。','リピートの正当性と旧受信の鮮度はpollRemote側で検証する。','FR-001, FR-012, TC-012','赤外線リモコン'),
'buzzerGate':('ブザーの周期ON/OFF判定','warning: 要求、now: ms、periodMs: 周期（既定400ms）','bool: 今ONにするか','警告中に周期前半をON、後半をOFFとする。delayを使用しない。','アクティブブザー専用の出力に利用。受動ブザーへtoneを追加するとタイマー競合を再検討する。','FR-007, TC-007','ブザー'),
'updateController':('方式C・安全優先制御の統合','c: 状態、s: センサー、r: リモコン、p: 設定、now: ms','Output: 動作・PWM・ブザー・理由','設定/センサー異常、緊急距離、前後同時検知を停止。通信断とSTOPを回避より優先。警戒時は一旦停止→逆方向安全確認→低速退避。安全継続後は停止し、新規操作まで待機する。時間超過は保持停止。','緊急・異常停止は前後安全＋新規STOPを連続確認後に解除待ちへ移る。実時間応答はセンサー周期とpulseIn待機に制約される。','FR-008, FR-009, FR-010, FR-011, FR-012, FR-013, FR-014, TC-008, TC-009, TC-010, TC-011, TC-012, TC-013, TC-014, TC-015, FC-004','統合制御'),
'validPin':('UNOピン範囲確認','pin: ピン番号','bool: 2〜19','D0/D1をシリアル用に除外し、UNO相当の範囲を確認する。','機能対応や重複はpinsReadyで追加確認する。','FR-014, TC-014','設定'),
'pwmPin':('PWM・タイマー利用条件確認','pin: PWM候補','bool: 5/6/9/10か','IRremoteのTimer2利用と競合する3/11を除外したPWM候補を確認する。','実際の使用タイマーはライブラリ設定に依存。変更時は再確認する。','FR-014, TC-014','設定'),
'pinsReady':('配線・仕様確認ゲート','引数なし（BoardConfigを参照）','bool: 駆動初期化可能か','ピン範囲、11ピンの重複、PWM、配線・停止境界・アクティブブザーの確認フラグを検証する。','未設定=-1とfalseの既定値では失敗する。実モータードライバー仕様の確認も必要。','FR-014, TC-014','設定'),
'initializeHardware':('安全側からI/O初期化','引数なし','bool: 初期化成功か','pinsReady後、出力ラッチをLOWにしてOUTPUTへ変更し、PWM0、ブザーOFF、センサー入力を設定する。','失敗時はピンを書き換えない。未配線ピンから既存モーターを止められることは保証しない。','FR-008, TC-008','モーター制御'),
'writeWheel':('1輪分のHブリッジ出力','in1,in2,pwm: ピン、speed: -255〜255、inverted: 極性反転','void','PWMを0にして方向ピンを設定し、飽和した速度絶対値をPWMへ出力する。','IN1/IN2/PWM型ドライバーのみのアダプター例。停止は惰性停止想定で制動保証なし。','FR-001, FR-002, TC-001, TC-002','モーター制御'),
'stopMotors':('左右モーター停止','引数なし','void','pinsReadyを確認し、左右のwriteWheelへ0を渡す。','ピン未設定時はI/O操作しない。物理的な即時制動を意味しない。','FR-008, TC-008','緊急停止'),
'readUltrasonic':('超音波トリガ・Echo取得','引数なし','float: cm、未設定/timeoutはNAN','LOW2µs→HIGH10µs→LOWを出力し、pulseIn(HIGH,15000µs)で取得後にcmへ変換する。','完全な非同期ではなく最大約15msの待機がある。センサー方式が異なる場合は差し替える。','FR-005, TC-005, FC-002','超音波センサー'),
'sampleSensors':('前後センサースナップショット','now: 取得開始ms','SensorFrame: 距離・検知・有効性・時刻','超音波値を取得して有効範囲を判定し、後方デジタル入力を極性変換する。','rearValidは読取実行を示すだけ。断線・固定出力故障の検出には追加回路/試験が必要。','FR-005, FR-006, FR-013, TC-005, TC-006, TC-013','障害物検知'),
'applyOutput':('車両出力をI/Oへ反映','o: 制御出力、now: 現在ms','void','動作を左右速度へ分解し、writeWheelを呼び出し、周期警告をブザーへ出す。','pinsReady=falseなら何もしない。受動型ブザーは対象外。','FR-007, FR-014, TC-007, TC-014','モーター制御'),
'codesReady':('リモコン設定検証','引数なし（BoardConfig参照）','bool: 5キーが一意かつ確認済み','5コマンドの重複を拒否し、REMOTE_CODES_CONFIRMEDを確認する。','既定の全0ではfalse。実リモコンを採取して設定する。','FR-001, FR-002, TC-001, TC-002','赤外線リモコン'),
'decodeCommand':('IRコマンドを動作に変換','code: 受信値、out: 出力参照','bool: 対応するキーか','設定された5キーと照合し、Forward/Backward/Left/Right/Stopをoutへ設定する。','未登録キー・設定不備はfalse。outを動作出力へ直接渡さない。','FR-001, FR-002, FR-003, FR-004, TC-001, TC-004','赤外線リモコン'),
'pollRemote':('IR受信・検証・再受信開始','state: 状態、now: ms、p: 通信期限設定','void','decode後に受信結果をコピーしてresume。NEC/NEC2、address、overflow、対応キーを検証する。正当なリピートのみ期限を延長する。','他プロトコルは拒否。IRremote 4.x向け。新規コマンドだけsequenceを増やす。','FR-001, FR-012, FR-014, TC-012, TC-014','赤外線リモコン')}
# Replace only this generated library; preserve original records and later user additions.
data['records']=[r for r in data['records'] if r.get('generator')!='robot-library-v1']
data['files']=[r for r in data['files'] if r.get('generator')!='robot-library-v1']
common={'visibility':'public','updated':stamp,'generator':'robot-library-v1','owner':'設計例'}
allmd=['# ロボットカー 関数設計書\n\n対象：Arduino UNO / OSOYOO Model No.2019012400。コードは参考実装であり実機検証前。\n\n5cm以下/未満の境界、ピン配置、ドライバー、赤外線極性、受信プロトコルとキー値を実機で確認する。既定値ではモーターI/Oを初期化しない。\n\n']
code_id=0;functions=0
for path in sorted((root/'docs/code/RobotCar').glob('*')):
 if path.suffix not in ['.h','.ino']:continue
 code_id+=1;cid=f'CODE-{code_id:03d}'
 title={'BoardConfig.h':'設定・ピン割り当て（実機確認用）','RobotLogic.h':'制御ロジック一式（方式C）','HardwareAdapter.h':'モーター・センサーI/Oアダプター','RemoteAdapter.h':'赤外線リモコン受信アダプター','RobotCar.ino':'統合スケッチ：安全優先・自動回避'}[path.name]
 source=path.read_text();rel=path.relative_to(root/'docs').as_posix()
 full={'id':cid,'kind':'codes','title':title,'category':'統合・共通ファイル','filename':path.name,'code':source,'status':'実機未検証','content':'統合プロジェクトの構成ファイル。5ファイルをRobotCarフォルダに揃えて開く。初期設定は未確認状態で駆動を無効化。単体の関数抜粋を別スケッチとして実行しない。','pins':'BoardConfig.hで確認後に設定（既定-1）','libraries':'Arduino標準 / RemoteAdapterのみArduino-IRremote 4.x','links':'BD-LIBRARY, BUG-001',**common}
 data['records'].append(full)
 for name,body in re.findall(r'// @function (\w+)\n(.*?)// @endfunction',source,re.S):
  functions+=1;fid=f'FD-{functions:03d}';code_id+=1;scid=f'CODE-{code_id:03d}';purpose,args,ret,process,errors,related,cat=M[name]
  signature=body.split('{',1)[0].strip();ns='robot' if path.name=='RobotLogic.h' else 'hardware' if path.name=='HardwareAdapter.h' else 'remote'
  dependencies=', '.join(sorted(set(re.findall(r'\b(\w+)\s*\(',body))&set(M)-{name})) or 'なし（標準関数・演算のみ）'
  used=', '.join(sorted(set(re.findall(r'\b(?:c|s|r|state|p)\.\w+|board::\w+',body)))) or '引数と関数内の局所変数のみ'
  content=f'処理手順：{process}\n\n呼出先：{dependencies}\n\n前提条件：対応する共通型定義と設定を読み込む。単体抜粋のみではビルドできない。実機動作は未確認。\n\n副作用：'+('Controller/RemoteStateの参照またはI/Oを更新する。' if name in ['resetController','enterPhase','clearStable','latchStop','acceptRemote','updateController','initializeHardware','writeWheel','stopMotors','readUltrasonic','sampleSensors','applyOutput','decodeCommand','pollRemote'] else '引数を読み取り、判定値を返す。I/O操作なし。')+f'\n\n名前空間：{ns}\nソース：{path.name}\nプロトタイプ：{signature}'
  data['records'].append({'id':fid,'kind':'detailed','title':f'{name}：{purpose}','function':ns+'::'+name,'args':args,'returns':ret,'content':content,'errors':errors,'variables':used,'pins':'I/O関数のみBoardConfig.h参照。論理関数はピン未使用。','libraries':'Arduino-IRremote 4.x' if path.name=='RemoteAdapter.h' else 'C++11 / Arduino標準','category':cat,'status':'実機確認待ち','links':f'{scid}, {cid}, {related}',**common})
  data['records'].append({'id':scid,'kind':'codes','title':f'{name}：{purpose}','filename':name+'.h','code':'// 関数抜粋です。'+path.name+'と共通型定義が必要です。\n// 統合ZIP内の同名関数を参照してください。\n'+body.strip()+'\n','category':cat,'status':'実機未検証','content':process+'\n\n'+errors,'links':f'{fid}, {cid}, {related}',**common})
  full['links']+=', '+fid
  allmd.append(f'## {fid} {ns}::{name} — {purpose}\n\n|項目|内容|\n|---|---|\n|ソース|{path.name}|\n|引数|{args}|\n|戻り値|{ret}|\n|分類|{cat}|\n|関連|{scid}, {cid}, {related}|\n\n### 処理\n{process}\n\n### 使用変数・呼出先\n{used}\n\n呼出先：{dependencies}\n\n### 異常・制約\n{errors}\n\n```cpp\n{signature};\n```\n\n')
# Standalone diagnostics
for p in sorted((root/'docs/code/examples').rglob('*.ino')):
 code_id+=1;cid=f'CODE-{code_id:03d}';names={'01':'シリアル入力コマンド確認','02':'超音波距離モニター','03':'後方赤外線・M状態モニター','04':'赤外線リモコンのキー値採取','05':'アクティブブザー周期警告','06':'delayを使わない周期実行','07':'モーターベンチ診断・時間制限付き','08':'5cm・15cmの境界比較','09':'方式Cのセンサー模擬シナリオ'}
 data['records'].append({'id':cid,'kind':'codes','title':names[p.name[:2]],'category':'単独診断スケッチ','filename':p.name,'code':p.read_text(),'status':'実機未検証','content':'独立した診断例。同じ名前のフォルダをArduino IDEで開く。ピンが未設定の例は動作しない。モーター例は車輪を浮かせ、実ドライバーと配線を確認する。09のみ同梱のRobotLogic.hが必要。','links':'BD-LIBRARY, FILE-SOURCE, FILE-DESIGN','pins':'コード内の-1を実機に合わせて設定。06はUNO内蔵LEDのみ。','libraries':'04のみArduino-IRremote 4.x。ほかはArduino標準。',**common})
# add links from existing requirements and flowcharts
for r in data['records']:
 if r.get('generator')=='robot-library-v1':continue
 if r['id'].startswith(('FR-','FC-')):
  linked=[x['id'] for x in data['records'] if x.get('generator')=='robot-library-v1' and x['kind']=='detailed' and r['id'] in x.get('links','').split(', ')]
  r['links']=', '.join(dict.fromkeys(r.get('links','').split(', ')+linked))
(root/'docs/design/FunctionDesign.md').write_text(''.join(allmd))
overview='''# コードライブラリの使い方

## 収録内容
統合プロジェクト5ファイル、関数抜粋と関数設計書、単独診断スケッチ9本を収録。関数抜粋は単独スケッチではなく参照用。Arduino IDEではRobotCar/RobotCar.inoを開き、同じフォルダに4つの.hを揃える。

## 実機へ合わせる手順
1. モータードライバー、端子、回路、センサー型番を確認する。本アダプターは左右各IN1/IN2/PWM型を仮定。実ドライバーが異なる場合はHardwareAdapterを変更する。
2. 診断04でIRのprotocol/address/commandを採取し、NEC/NEC2かを確認する。他形式にはRemoteAdapterの変更が必要。
3. BoardConfigのピン、極性、キー値を入力する。D0/D1、重複ピン、IRremoteとPWMのタイマー競合を避ける。
4. BUG-001の5cm未満/以下を合意してParameters.emergencyInclusiveを設定する。既定値は仮に<=。確認フラグは実際の確認後に設定する。
5. アクティブブザーか確認する。受動ブザーとtone()を使用する場合、UNOでのIRremoteとのタイマー競合を再設計する。
6. 車輪を浮かせて方向・PWM0の停止を確認し、低速・管理された範囲でテストする。

## 動作と制約
STOPと通信断は自動回避より優先。危険距離・センサー異常・前後同時検知・回避タイムアウトは保持停止。安全が連続確認でき、新しいSTOP操作を受信したら解除待ちへ移り、その後の新規操作で復帰する。回避完了後も押しっぱなしでは再始動しない。

速度120/70、通信期限350ms、測定60ms、鮮度200ms、方向切替停止100ms、回避上限1500ms、安全確認400msは全て仮の初期値。実機で応答と制動距離を評価して決定する。

pulseInは最大約15ms待機する。全処理が非ブロッキングではない。停止指令と物理制動は異なる。側方の安全や後方デジタルセンサーの断線を保証できない。IRremoteのフレーム受信・測定周期・シリアル出力による遅延は実機で測定する。

## 確認済み範囲
ホストg++ C++11の論理テストを実行。境界、時計の折り返し、欠測、古いデータ、通信断、回避、STOP優先、タイムアウト保持停止、復帰、新規操作待ちを確認。Arduino UNO向けクロスコンパイル、実センサー、配線、実走行、物理的な停止距離は未検証。既存TCの実機合否は変更しない。

ZIP展開先では `g++ -std=c++11 -IRobotCar tests/robot_logic_test.cpp -o logic-test` で論理テストを実行できる（g++が必要）。I/Oアダプターの構文検査にはローカルのArduino/IRremote模擬ヘッダーを使用した。これはUNO向けビルドや実ライブラリの検証の代わりにはならない。

## 参照資料
- Arduino pulseIn: https://docs.arduino.cc/language-reference/en/functions/advanced-io/pulseIn/
- Arduino millis: https://docs.arduino.cc/language-reference/en/functions/time/millis/
- Arduino-IRremote公式: https://github.com/Arduino-IRremote/Arduino-IRremote

APIの参照日：2026-11-10。センサーとモータードライバーの仕様は実機資料で確認する。
'''
(root/'docs/design/LibraryGuide.md').write_text(overview)
data['records'].append({'id':'BD-LIBRARY','kind':'basic','title':'追加コード・関数設計書の構成と実機への合わせ方','category':'コード運用','content':overview,'status':'参考設計','links':'FILE-SOURCE, FILE-DESIGN, BUG-001',**common})
# downloadable files for full modules and standalone sketches
for p in sorted((root/'docs/code').rglob('*')):
 if not p.is_file() or p.suffix=='.zip':continue
 fid='SRC-'+hashlib.sha1(p.relative_to(root).as_posix().encode()).hexdigest()[:10]
 data['files'].append({'id':fid,'group_id':fid,'name':p.name,'title':p.parent.name+' / '+p.name,'url':p.relative_to(root/'docs').as_posix(),'size':p.stat().st_size,'category':'05_ソースコード','description':'参考実装・実機未検証。実機設定と同梱ガイドを確認。','created':stamp,'change':'参考コード初版','visibility':'public','generator':'robot-library-v1','links':'BD-LIBRARY'})
for fid,p,title,category in [('FILE-DESIGN',root/'docs/design/FunctionDesign.md','関数設計書（全関数）','03_詳細設計'),('FILE-GUIDE',root/'docs/design/LibraryGuide.md','コード利用・実機設定ガイド','12_参考資料')]:
 data['files'].append({'id':fid,'group_id':fid,'name':p.name,'title':title,'url':p.relative_to(root/'docs').as_posix(),'size':p.stat().st_size,'category':category,'description':title,'created':stamp,'change':'初版','visibility':'public','generator':'robot-library-v1','links':'BD-LIBRARY'})
zpath=root/'docs/code/RobotCarLibrary.zip'
with zipfile.ZipFile(zpath,'w',zipfile.ZIP_DEFLATED) as z:
 for p in sorted((root/'docs/code').rglob('*')):
  if p.is_file() and p!=zpath and p.suffix!='.zip':z.write(p,p.relative_to(root/'docs/code').as_posix())
 for p in sorted((root/'docs/design').glob('*.md')):z.write(p,'design/'+p.name)
 z.write(root/'tests/robot_logic_test.cpp','tests/robot_logic_test.cpp')
data['files'].append({'id':'FILE-SOURCE','group_id':'FILE-SOURCE','name':zpath.name,'title':'統合コード・診断スケッチ・関数設計書一式','url':'code/RobotCarLibrary.zip','size':zpath.stat().st_size,'category':'05_ソースコード','description':'全コードと関数設計書を一括取得。未設定の既定値では駆動無効。','created':stamp,'change':'初版','visibility':'public','generator':'robot-library-v1','links':'BD-LIBRARY, FILE-DESIGN'})
datafile.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n')
print(json.dumps({'codes':sum(r['kind']=='codes' for r in data['records']),'function_designs':functions,'diagnostic_sketches':9,'files':len(data['files'])}))
