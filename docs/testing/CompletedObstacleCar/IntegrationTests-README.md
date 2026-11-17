# 結合テスト（36件・8分野）

2026/11/17に公開保存の「結合テスト仕様兼結果書_0207.xlsx」を編集。
元の8シート・緑の見出し・表の構成を保持。内容は完成コードに合わせて置換。
元のH8ローバー・メロディー・TeraTerm用記入例と実測画像は除去。

## 実行範囲
製品のCompletedObstacleCar.inoは変更せずsetup()/loop()をホストで実行。
36件すべて期待値一致。これはホスト模擬のソフトウェア結合確認です。
実機結果は未実施。後方接触の申告がある3ケースは保留。
後方赤外線はLOWのデジタル検知で、検知距離をcmで設定するコードではありません。

スタブはmillis、pulseIn、GPIO、IR受信を置換します。pulseInやI/O処理は模擬時間を進めません。
実センサ応答、IR波形、割り込み競合、実際の周期精度、モータ慣性、検知距離、接触は対象外です。
実機では定規・動画・波形計測で距離、100msの停止、警告周期、接触有無を記録してください。
D0が後方センサ入力のため、製品にSerial監視は追加しません。

## 再実行
展開したディレクトリで以下を実行（g++が必要）。
```sh
g++ -std=c++17 -Wall -Wextra -Werror -I tests/stubs -I tests tests/integration_tests.cpp -o integration-tests
./integration-tests > results.ndjson
```
tests/CompletedObstacleCar.inoは作成時の原文コピー。
ソース版：4c9b73020a81f5f390ba3b7491e64dd19b0ff66b
各シナリオはreset→setupから開始し、指定時刻に入力してloopのみを呼び出します。
results.ndjsonのpurposeがIT-001〜IT-036の対応IDです。
