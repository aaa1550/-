# ロボットカー 関数設計書

対象：Arduino UNO / OSOYOO Model No.2019012400。コードは参考実装であり実機検証前。

5cm以下/未満の境界、ピン配置、ドライバー、赤外線極性、受信プロトコルとキー値を実機で確認する。既定値ではモーターI/Oを初期化しない。

## FD-001 hardware::validPin — UNOピン範囲確認

|項目|内容|
|---|---|
|ソース|HardwareAdapter.h|
|引数|pin: ピン番号|
|戻り値|bool: 2〜19|
|分類|設定|
|関連|CODE-003, CODE-002, FR-014, TC-014|

### 処理
D0/D1をシリアル用に除外し、UNO相当の範囲を確認する。

### 使用変数・呼出先
引数と関数内の局所変数のみ

呼出先：なし（標準関数・演算のみ）

### 異常・制約
機能対応や重複はpinsReadyで追加確認する。

```cpp
inline bool validPin(int8_t pin);
```

## FD-002 hardware::pwmPin — PWM・タイマー利用条件確認

|項目|内容|
|---|---|
|ソース|HardwareAdapter.h|
|引数|pin: PWM候補|
|戻り値|bool: 5/6/9/10か|
|分類|設定|
|関連|CODE-004, CODE-002, FR-014, TC-014|

### 処理
IRremoteのTimer2利用と競合する3/11を除外したPWM候補を確認する。

### 使用変数・呼出先
引数と関数内の局所変数のみ

呼出先：なし（標準関数・演算のみ）

### 異常・制約
実際の使用タイマーはライブラリ設定に依存。変更時は再確認する。

```cpp
inline bool pwmPin(int8_t pin);
```

## FD-003 hardware::pinsReady — 配線・仕様確認ゲート

|項目|内容|
|---|---|
|ソース|HardwareAdapter.h|
|引数|引数なし（BoardConfigを参照）|
|戻り値|bool: 駆動初期化可能か|
|分類|設定|
|関連|CODE-005, CODE-002, FR-014, TC-014|

### 処理
ピン範囲、11ピンの重複、PWM、配線・停止境界・アクティブブザーの確認フラグを検証する。

### 使用変数・呼出先
board::ACTIVE_BUZZER_CONFIRMED, board::BUZZER, board::ECHO, board::IR_RECEIVE, board::LEFT_IN1, board::LEFT_IN2, board::LEFT_PWM, board::REAR, board::RIGHT_IN1, board::RIGHT_IN2, board::RIGHT_PWM, board::STOP_BOUNDARY_CONFIRMED, board::TRIG, board::WIRING_CONFIRMED

呼出先：pwmPin, validPin

### 異常・制約
未設定=-1とfalseの既定値では失敗する。実モータードライバー仕様の確認も必要。

```cpp
inline bool pinsReady();
```

## FD-004 hardware::initializeHardware — 安全側からI/O初期化

|項目|内容|
|---|---|
|ソース|HardwareAdapter.h|
|引数|引数なし|
|戻り値|bool: 初期化成功か|
|分類|モーター制御|
|関連|CODE-006, CODE-002, FR-008, TC-008|

### 処理
pinsReady後、出力ラッチをLOWにしてOUTPUTへ変更し、PWM0、ブザーOFF、センサー入力を設定する。

### 使用変数・呼出先
board::BUZZER, board::BUZZER_ACTIVE_HIGH, board::ECHO, board::IR_RECEIVE, board::LEFT_IN1, board::LEFT_IN2, board::LEFT_PWM, board::REAR, board::RIGHT_IN1, board::RIGHT_IN2, board::RIGHT_PWM, board::TRIG

呼出先：pinsReady

### 異常・制約
失敗時はピンを書き換えない。未配線ピンから既存モーターを止められることは保証しない。

```cpp
inline bool initializeHardware();
```

## FD-005 hardware::writeWheel — 1輪分のHブリッジ出力

|項目|内容|
|---|---|
|ソース|HardwareAdapter.h|
|引数|in1,in2,pwm: ピン、speed: -255〜255、inverted: 極性反転|
|戻り値|void|
|分類|モーター制御|
|関連|CODE-007, CODE-002, FR-001, FR-002, TC-001, TC-002|

### 処理
PWMを0にして方向ピンを設定し、飽和した速度絶対値をPWMへ出力する。

### 使用変数・呼出先
引数と関数内の局所変数のみ

呼出先：clampPwm, pinsReady

### 異常・制約
IN1/IN2/PWM型ドライバーのみのアダプター例。停止は惰性停止想定で制動保証なし。

```cpp
inline void writeWheel(int8_t in1,int8_t in2,int8_t pwm,int16_t speed,bool inverted);
```

## FD-006 hardware::stopMotors — 左右モーター停止

|項目|内容|
|---|---|
|ソース|HardwareAdapter.h|
|引数|引数なし|
|戻り値|void|
|分類|緊急停止|
|関連|CODE-008, CODE-002, FR-008, TC-008|

### 処理
pinsReadyを確認し、左右のwriteWheelへ0を渡す。

### 使用変数・呼出先
board::LEFT_IN1, board::LEFT_IN2, board::LEFT_PWM, board::RIGHT_IN1, board::RIGHT_IN2, board::RIGHT_PWM

呼出先：pinsReady, writeWheel

### 異常・制約
ピン未設定時はI/O操作しない。物理的な即時制動を意味しない。

```cpp
inline void stopMotors();
```

## FD-007 hardware::readUltrasonic — 超音波トリガ・Echo取得

|項目|内容|
|---|---|
|ソース|HardwareAdapter.h|
|引数|引数なし|
|戻り値|float: cm、未設定/timeoutはNAN|
|分類|超音波センサー|
|関連|CODE-009, CODE-002, FR-005, TC-005, FC-002|

### 処理
LOW2µs→HIGH10µs→LOWを出力し、pulseIn(HIGH,15000µs)で取得後にcmへ変換する。

### 使用変数・呼出先
board::ECHO, board::ECHO_TIMEOUT_US, board::TRIG

呼出先：pinsReady, pulseToCm

### 異常・制約
完全な非同期ではなく最大約15msの待機がある。センサー方式が異なる場合は差し替える。

```cpp
inline float readUltrasonic();
```

## FD-008 hardware::sampleSensors — 前後センサースナップショット

|項目|内容|
|---|---|
|ソース|HardwareAdapter.h|
|引数|now: 取得開始ms|
|戻り値|SensorFrame: 距離・検知・有効性・時刻|
|分類|障害物検知|
|関連|CODE-010, CODE-002, FR-005, FR-006, FR-013, TC-005, TC-006, TC-013|

### 処理
超音波値を取得して有効範囲を判定し、後方デジタル入力を極性変換する。

### 使用変数・呼出先
board::REAR, board::REAR_ACTIVE_LOW, s.frontCm, s.frontValid, s.rearBlocked, s.rearValid, s.sampledAt

呼出先：pinsReady, readUltrasonic, rearFromLevel, validDistance

### 異常・制約
rearValidは読取実行を示すだけ。断線・固定出力故障の検出には追加回路/試験が必要。

```cpp
inline robot::SensorFrame sampleSensors(uint32_t now);
```

## FD-009 hardware::applyOutput — 車両出力をI/Oへ反映

|項目|内容|
|---|---|
|ソース|HardwareAdapter.h|
|引数|o: 制御出力、now: 現在ms|
|戻り値|void|
|分類|モーター制御|
|関連|CODE-011, CODE-002, FR-007, FR-014, TC-007, TC-014|

### 処理
動作を左右速度へ分解し、writeWheelを呼び出し、周期警告をブザーへ出す。

### 使用変数・呼出先
board::BUZZER, board::BUZZER_ACTIVE_HIGH, board::LEFT_IN1, board::LEFT_IN2, board::LEFT_INVERTED, board::LEFT_PWM, board::RIGHT_IN1, board::RIGHT_IN2, board::RIGHT_INVERTED, board::RIGHT_PWM

呼出先：buzzerGate, pinsReady, wheelsFor, writeWheel

### 異常・制約
pinsReady=falseなら何もしない。受動型ブザーは対象外。

```cpp
inline void applyOutput(const robot::Output& o,uint32_t now);
```

## FD-010 remote::codesReady — リモコン設定検証

|項目|内容|
|---|---|
|ソース|RemoteAdapter.h|
|引数|引数なし（BoardConfig参照）|
|戻り値|bool: 5キーが一意かつ確認済み|
|分類|赤外線リモコン|
|関連|CODE-013, CODE-012, FR-001, FR-002, TC-001, TC-002|

### 処理
5コマンドの重複を拒否し、REMOTE_CODES_CONFIRMEDを確認する。

### 使用変数・呼出先
board::KEY_BACKWARD, board::KEY_FORWARD, board::KEY_LEFT, board::KEY_RIGHT, board::KEY_STOP, board::REMOTE_CODES_CONFIRMED

呼出先：なし（標準関数・演算のみ）

### 異常・制約
既定の全0ではfalse。実リモコンを採取して設定する。

```cpp
inline bool codesReady();
```

## FD-011 remote::decodeCommand — IRコマンドを動作に変換

|項目|内容|
|---|---|
|ソース|RemoteAdapter.h|
|引数|code: 受信値、out: 出力参照|
|戻り値|bool: 対応するキーか|
|分類|赤外線リモコン|
|関連|CODE-014, CODE-012, FR-001, FR-002, FR-003, FR-004, TC-001, TC-004|

### 処理
設定された5キーと照合し、Forward/Backward/Left/Right/Stopをoutへ設定する。

### 使用変数・呼出先
board::KEY_BACKWARD, board::KEY_FORWARD, board::KEY_LEFT, board::KEY_RIGHT, board::KEY_STOP

呼出先：codesReady

### 異常・制約
未登録キー・設定不備はfalse。outを動作出力へ直接渡さない。

```cpp
inline bool decodeCommand(uint16_t code,robot::Motion& out);
```

## FD-012 remote::pollRemote — IR受信・検証・再受信開始

|項目|内容|
|---|---|
|ソース|RemoteAdapter.h|
|引数|state: 状態、now: ms、p: 通信期限設定|
|戻り値|void|
|分類|赤外線リモコン|
|関連|CODE-015, CODE-012, FR-001, FR-012, FR-014, TC-012, TC-014|

### 処理
decode後に受信結果をコピーしてresume。NEC/NEC2、address、overflow、対応キーを検証する。正当なリピートのみ期限を延長する。

### 使用変数・呼出先
board::REMOTE_ADDRESS, state.command

呼出先：acceptRemote, codesReady, decodeCommand, remoteFresh

### 異常・制約
他プロトコルは拒否。IRremote 4.x向け。新規コマンドだけsequenceを増やす。

```cpp
inline void pollRemote(robot::RemoteState& state,uint32_t now,const robot::Parameters& p);
```

## FD-013 robot::elapsed — 経過時間判定

|項目|内容|
|---|---|
|ソース|RobotLogic.h|
|引数|now: 現在ms、since: 基準ms、interval: 待機ms|
|戻り値|bool: interval以上でtrue|
|分類|時間管理|
|関連|CODE-018, CODE-017, FR-012, TC-012|

### 処理
uint32_tの差を取り、指定周期以上かを比較する。約49日でのmillis折り返しを差分で扱う。intervalは2^31未満で使う。

### 使用変数・呼出先
引数と関数内の局所変数のみ

呼出先：なし（標準関数・演算のみ）

### 異常・制約
引数は単位msに統一。長期間放置や2周以上の時計差は対象外。

```cpp
inline bool elapsed(uint32_t now,uint32_t since,uint32_t interval);
```

## FD-014 robot::clampPwm — PWM値の上限制限

|項目|内容|
|---|---|
|ソース|RobotLogic.h|
|引数|value: 任意のint|
|戻り値|uint8_t: 0〜255|
|分類|モーター制御|
|関連|CODE-019, CODE-017, FR-001, FR-002|

### 処理
負値は0、255超過は255、それ以外はuint8_tへ変換する。

### 使用変数・呼出先
引数と関数内の局所変数のみ

呼出先：なし（標準関数・演算のみ）

### 異常・制約
飽和処理。負値を方向指定として渡さない。

```cpp
inline uint8_t clampPwm(int value);
```

## FD-015 robot::validParameters — 制御設定の整合性確認

|項目|内容|
|---|---|
|ソース|RobotLogic.h|
|引数|p: Parameters設定|
|戻り値|bool: 必須条件を満たすか|
|分類|設定|
|関連|CODE-020, CODE-017, FR-014, TC-014|

### 処理
距離の有限性・正数・大小関係、速度の大小、主要なタイムアウトの整合を判定する。

### 使用変数・呼出先
p.clearHoldMs, p.emergencyCm, p.escapePwm, p.maxEscapeMs, p.normalPwm, p.remoteTimeoutMs, p.sensorMaxAgeMs, p.stopBeforeReverseMs, p.warningCm

呼出先：なし（標準関数・演算のみ）

### 異常・制約
falseなら統合制御は安全停止。値は公式仕様ではなく仮設定。

```cpp
inline bool validParameters(const Parameters& p);
```

## FD-016 robot::pulseToCm — Echo幅を距離に換算

|項目|内容|
|---|---|
|ソース|RobotLogic.h|
|引数|pulseUs: Echo HIGH幅µs|
|戻り値|float: 距離cm、0µsはNAN|
|分類|超音波センサー|
|関連|CODE-021, CODE-017, FR-005, TC-005, FC-002|

### 処理
pulseUs × 0.0343 / 2で往復の伝播時間を距離へ換算する。温度補正は行わない。

### 使用変数・呼出先
引数と関数内の局所変数のみ

呼出先：なし（標準関数・演算のみ）

### 異常・制約
timeoutの0を0cmの障害物と混同せずNANにする。

```cpp
inline float pulseToCm(uint32_t pulseUs);
```

## FD-017 robot::validDistance — 距離の有効範囲判定

|項目|内容|
|---|---|
|ソース|RobotLogic.h|
|引数|cm: 距離cm|
|戻り値|bool: 有限で2〜200cm|
|分類|超音波センサー|
|関連|CODE-022, CODE-017, FR-005, FR-013, TC-005, TC-013|

### 処理
センサー用の仮の有効範囲を含む判定を行う。

### 使用変数・呼出先
引数と関数内の局所変数のみ

呼出先：なし（標準関数・演算のみ）

### 異常・制約
NAN・無限大・範囲外はfalse。実センサーに応じて範囲変更が必要。

```cpp
inline bool validDistance(float cm);
```

## FD-018 robot::frontEmergency — 緊急停止距離判定

|項目|内容|
|---|---|
|ソース|RobotLogic.h|
|引数|cm: 距離cm、p: 停止設定|
|戻り値|bool: 危険距離か|
|分類|緊急停止|
|関連|CODE-023, CODE-017, FR-008, TC-008, BUG-001, FC-002|

### 処理
emergencyInclusive=trueは<=、falseは<で比較する。

### 使用変数・呼出先
p.emergencyCm, p.emergencyInclusive

呼出先：なし（標準関数・演算のみ）

### 異常・制約
5cm境界は仕様未確定。既定の<=は仮設定で、確認フラグを立てるまで実機駆動しない。

```cpp
inline bool frontEmergency(float cm,const Parameters& p);
```

## FD-019 robot::frontWarning — 前方警戒距離判定

|項目|内容|
|---|---|
|ソース|RobotLogic.h|
|引数|cm: 距離cm、p: 警戒距離|
|戻り値|bool: 有限かつ警戒距離以下|
|分類|障害物検知|
|関連|CODE-024, CODE-017, FR-005, FR-009, TC-009|

### 処理
前方15cm以下を警戒とする。NANは別の異常判定で停止する。

### 使用変数・呼出先
p.warningCm

呼出先：なし（標準関数・演算のみ）

### 異常・制約
有効性全体はsensorsFreshで判定し、単独利用で安全確認しない。

```cpp
inline bool frontWarning(float cm,const Parameters& p);
```

## FD-020 robot::rearFromLevel — 後方赤外線の極性変換

|項目|内容|
|---|---|
|ソース|RobotLogic.h|
|引数|levelHigh: HIGHならtrue、activeLow: LOWで検知する設定|
|戻り値|bool: 障害物あり|
|分類|赤外線センサー|
|関連|CODE-025, CODE-017, FR-006, TC-006, FC-003|

### 処理
アクティブLOW/HIGHを設定に従って論理的な検知有無へ変換する。

### 使用変数・呼出先
引数と関数内の局所変数のみ

呼出先：なし（標準関数・演算のみ）

### 異常・制約
極性は実センサーで確認。デジタル入力のみでは断線を保証できない。

```cpp
inline bool rearFromLevel(bool levelHigh,bool activeLow);
```

## FD-021 robot::sensorsFresh — センサー正常性・鮮度判定

|項目|内容|
|---|---|
|ソース|RobotLogic.h|
|引数|s: 取得値、now: 現在ms、p: 最大経過時間|
|戻り値|bool: 前後有効かつ期限内|
|分類|安全制御|
|関連|CODE-026, CODE-017, FR-013, TC-013|

### 処理
前方valid・距離範囲・後方validを確認し、sampledAtからの経過がsensorMaxAgeMs未満か確認する。

### 使用変数・呼出先
p.sensorMaxAgeMs, s.frontCm, s.frontValid, s.rearValid, s.sampledAt

呼出先：elapsed, validDistance

### 異常・制約
期限ちょうどでfalse。後方のvalidはソフトウェア上の読取可否であり自己診断ではない。

```cpp
inline bool sensorsFresh(const SensorFrame& s,uint32_t now,const Parameters& p);
```

## FD-022 robot::remoteFresh — リモコン通信期限判定

|項目|内容|
|---|---|
|ソース|RobotLogic.h|
|引数|r: リモコン状態、now: 現在ms、p: 期限|
|戻り値|bool: validかつ期限内|
|分類|赤外線リモコン|
|関連|CODE-027, CODE-017, FR-001, FR-014, TC-014|

### 処理
直近の受信からremoteTimeoutMs未満であることを確認する。

### 使用変数・呼出先
p.remoteTimeoutMs, r.receivedAt, r.valid

呼出先：elapsed

### 異常・制約
期限ちょうどでfalse。受信断は手動操作だけでなく自動回避も停止させる。

```cpp
inline bool remoteFresh(const RemoteState& r,uint32_t now,const Parameters& p);
```

## FD-023 robot::motionAllowed — 移動方向の安全判定

|項目|内容|
|---|---|
|ソース|RobotLogic.h|
|引数|m: 動作、s: 前後情報、p: 設定|
|戻り値|bool: 動作許可|
|分類|安全制御|
|関連|CODE-028, CODE-017, FR-011, FR-014, TC-011, TC-014|

### 処理
前進は前方警戒なし、後退は後方検知なし、旋回は前後とも安全な場合に許可する。停止は常に許可。

### 使用変数・呼出先
s.frontCm, s.frontValid, s.rearBlocked, s.rearValid

呼出先：frontWarning, validDistance

### 異常・制約
鮮度は呼出側で確認。側方センサーがないため旋回時の側面安全は保証しない。

```cpp
inline bool motionAllowed(Motion m,const SensorFrame& s,const Parameters& p);
```

## FD-024 robot::wheelsFor — 車両動作を左右速度へ変換

|項目|内容|
|---|---|
|ソース|RobotLogic.h|
|引数|m: 動作、pwm: 0〜255|
|戻り値|WheelPair: 左右の符号付き速度|
|分類|モーター制御|
|関連|CODE-029, CODE-017, FR-001, FR-002, FR-003, FR-004, TC-001, TC-004|

### 処理
前進(+,+)、後退(-,-)、左(-,+)、右(+,-)、停止(0,0)へ変換する。

### 使用変数・呼出先
引数と関数内の局所変数のみ

呼出先：なし（標準関数・演算のみ）

### 異常・制約
車輪・配線の極性はHardwareAdapterで調整する。

```cpp
inline WheelPair wheelsFor(Motion m,uint8_t pwm);
```

## FD-025 robot::median3 — 計測表示用3点中央値

|項目|内容|
|---|---|
|ソース|RobotLogic.h|
|引数|a,b,c: 3つの距離|
|戻り値|float: 中央値、無効値を含む場合NAN|
|分類|計測補助|
|関連|CODE-030, CODE-017, FR-005, TC-005|

### 処理
3値を比較して中央値を返す。診断・表示用で、緊急停止の生値判定には使用しない。

### 使用変数・呼出先
引数と関数内の局所変数のみ

呼出先：なし（標準関数・演算のみ）

### 異常・制約
NAN混入時はNAN。フィルターによる検知遅れを安全判定へ持ち込まない。

```cpp
inline float median3(float a,float b,float c);
```

## FD-026 robot::legacyRearState — 原本の変数Mへ変換

|項目|内容|
|---|---|
|ソース|RobotLogic.h|
|引数|blocked: 後方検知|
|戻り値|uint8_t: 検知2、未検知3|
|分類|状態管理|
|関連|CODE-031, CODE-017, FR-006, TC-006, FC-003|

### 処理
原本FC-003のM=2/M=3をそのまま表現する。

### 使用変数・呼出先
引数と関数内の局所変数のみ

呼出先：なし（標準関数・演算のみ）

### 異常・制約
M以外の原本変数の意味を推測して補わない。

```cpp
inline uint8_t legacyRearState(bool blocked);
```

## FD-027 robot::makeStop — 停止出力の生成

|項目|内容|
|---|---|
|ソース|RobotLogic.h|
|引数|reason: 理由、warning: ブザー要求（既定false）|
|戻り値|Output: Stop・PWM0・警告・理由|
|分類|緊急停止|
|関連|CODE-032, CODE-017, FR-008, TC-008|

### 処理
全モーションをStopにし、理由と警告のみを保持する。

### 使用変数・呼出先
引数と関数内の局所変数のみ

呼出先：なし（標準関数・演算のみ）

### 異常・制約
出力を作るのみ。実モーター停止にはapplyOutputが必要。

```cpp
inline Output makeStop(Reason reason,bool warning=false);
```

## FD-028 robot::resetController — 制御コンテキスト初期化

|項目|内容|
|---|---|
|ソース|RobotLogic.h|
|引数|c: 更新対象、now: 現在ms|
|戻り値|void|
|分類|状態管理|
|関連|CODE-033, CODE-017, FR-012, TC-012|

### 処理
Controllerの全状態を既定値へ戻し、phaseStartedをnowへ設定する。

### 使用変数・呼出先
c.phaseStarted

呼出先：なし（標準関数・演算のみ）

### 異常・制約
運転中の無条件リセットは禁止。setupまたは検証時に使用。

```cpp
inline void resetController(Controller& c,uint32_t now);
```

## FD-029 robot::enterPhase — 状態遷移の記録

|項目|内容|
|---|---|
|ソース|RobotLogic.h|
|引数|c: 対象、p: 次状態、now: 時刻、sequence: リモコン世代|
|戻り値|void|
|分類|状態管理|
|関連|CODE-034, CODE-017, FR-009, FR-010, FC-004|

### 処理
状態・開始時刻・抑止対象シーケンスを設定し、安全確認タイマーを解除する。

### 使用変数・呼出先
c.blockedSequence, c.clearTiming, c.phase, c.phaseStarted

呼出先：なし（標準関数・演算のみ）

### 異常・制約
latchedReasonを初期化しない。呼出側が停止理由を設定する。

```cpp
inline void enterPhase(Controller& c,Phase p,uint32_t now,uint32_t sequence);
```

## FD-030 robot::clearStable — 安全状態の連続継続確認

|項目|内容|
|---|---|
|ソース|RobotLogic.h|
|引数|c: タイマー保存先、clear: 安全判定、now: ms、holdMs: 継続時間|
|戻り値|bool: 安全が所定時間継続|
|分類|状態管理|
|関連|CODE-035, CODE-017, FR-012, TC-012|

### 処理
安全初回に開始時刻を保持し、途中で不安全なら解除する。所定時間を満たすまでfalse。

### 使用変数・呼出先
c.clearStarted, c.clearTiming

呼出先：elapsed

### 異常・制約
瞬間的な安全値で通常操作へ復帰させない。

```cpp
inline bool clearStable(Controller& c,bool clear,uint32_t now,uint32_t holdMs);
```

## FD-031 robot::latchStop — 解除条件つき停止の保持

|項目|内容|
|---|---|
|ソース|RobotLogic.h|
|引数|c: 状態、reason: 原因、now: ms、seq: 世代|
|戻り値|Output: 警告つき停止|
|分類|緊急停止|
|関連|CODE-036, CODE-017, FR-008, FR-013, TC-008, TC-013|

### 処理
LatchedStopへ遷移し、発生理由を保存して停止出力を返す。

### 使用変数・呼出先
c.latchedReason

呼出先：enterPhase, makeStop

### 異常・制約
有効なSTOPの再操作と前後安全の連続確認まで解除しない。

```cpp
inline Output latchStop(Controller& c,Reason reason,uint32_t now,uint32_t seq);
```

## FD-032 robot::acceptRemote — 受信コマンド・世代の更新

|項目|内容|
|---|---|
|ソース|RobotLogic.h|
|引数|r: 更新先、command: 動作、now: ms、repeat: リピートか|
|戻り値|void|
|分類|赤外線リモコン|
|関連|CODE-037, CODE-017, FR-001, FR-012, TC-012|

### 処理
新規フレームのみコマンドとsequenceを更新。リピートは受信時刻のみ延長する。

### 使用変数・呼出先
r.command, r.receivedAt, r.sequence, r.valid

呼出先：なし（標準関数・演算のみ）

### 異常・制約
リピートの正当性と旧受信の鮮度はpollRemote側で検証する。

```cpp
inline void acceptRemote(RemoteState& r,Motion command,uint32_t now,bool repeat);
```

## FD-033 robot::buzzerGate — ブザーの周期ON/OFF判定

|項目|内容|
|---|---|
|ソース|RobotLogic.h|
|引数|warning: 要求、now: ms、periodMs: 周期（既定400ms）|
|戻り値|bool: 今ONにするか|
|分類|ブザー|
|関連|CODE-038, CODE-017, FR-007, TC-007|

### 処理
警告中に周期前半をON、後半をOFFとする。delayを使用しない。

### 使用変数・呼出先
引数と関数内の局所変数のみ

呼出先：なし（標準関数・演算のみ）

### 異常・制約
アクティブブザー専用の出力に利用。受動ブザーへtoneを追加するとタイマー競合を再検討する。

```cpp
inline bool buzzerGate(bool warning,uint32_t now,uint32_t periodMs=400);
```

## FD-034 robot::updateController — 方式C・安全優先制御の統合

|項目|内容|
|---|---|
|ソース|RobotLogic.h|
|引数|c: 状態、s: センサー、r: リモコン、p: 設定、now: ms|
|戻り値|Output: 動作・PWM・ブザー・理由|
|分類|統合制御|
|関連|CODE-039, CODE-017, FR-008, FR-009, FR-010, FR-011, FR-012, FR-013, FR-014, TC-008, TC-009, TC-010, TC-011, TC-012, TC-013, TC-014, TC-015, FC-004|

### 処理
設定/センサー異常、緊急距離、前後同時検知を停止。通信断とSTOPを回避より優先。警戒時は一旦停止→逆方向安全確認→低速退避。安全継続後は停止し、新規操作まで待機する。時間超過は保持停止。

### 使用変数・呼出先
c.blockedSequence, c.clearTiming, c.latchedReason, c.phase, c.phaseStarted, p.clearHoldMs, p.escapePwm, p.maxEscapeMs, p.normalPwm, p.stopBeforeReverseMs, r.command, r.sequence, s.frontCm, s.rearBlocked

呼出先：clearStable, elapsed, enterPhase, frontEmergency, frontWarning, latchStop, makeStop, motionAllowed, remoteFresh, sensorsFresh, validParameters

### 異常・制約
緊急・異常停止は前後安全＋新規STOPを連続確認後に解除待ちへ移る。実時間応答はセンサー周期とpulseIn待機に制約される。

```cpp
inline Output updateController(Controller& c,const SensorFrame& s,const RemoteState& r,const Parameters& p,uint32_t now);
```

