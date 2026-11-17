# 今回貼付コードの単体・結合テスト

作成：2026/11/17。対象：SubmittedCode.ino（製品コードの変更なし）。
SHA-256: ff9344beaeac794e86e95a178ad36e40b9c1656ae9d1c0dffba0ac8165b77bf8

単体111件・結合40件を今回コンパイルし実行。すべてホスト模擬で期待値一致。
単体の15関数のうちloopの11件はI/Oを模擬した全体処理の確認も兼ねます。
既存のケースを再実行し、単体6件・結合4件の境界条件を追加しています。
ホスト上でArduino・IR受信を模擬した結果です。実機の距離精度、受信波形、タイマー競合、停止距離、接触有無は未確認です。後方接触の申告は未解決として残しています。

## 再実行
展開したルートで以下を実行（g++が必要）。
```sh
g++ -std=c++17 -Wall -Wextra -Werror -I tests/stubs -I tests tests/unit/unit_tests.cpp -o unit-tests
./unit-tests > unit-results.ndjson
g++ -std=c++17 -Wall -Wextra -Werror -I tests/stubs -I tests tests/integration/integration_tests.cpp -o integration-tests
./integration-tests > integration-results.ndjson
```

tests/CompletedObstacleCar.inoはSubmittedCode.inoと同じ内容です。
スタブはmillis、GPIO、pulseIn、IR受信を置換。IRremoteライブラリの実装自体は実行しません。
fakePulseAdvanceはpulseIn内の模擬経過時間で、通常0、IT-040のみ10msです。
実機の実際の割り込み・波形・処理時間を再現した証拠ではありません。
結合結果欄のOK/NGは実機用で空欄。模擬結果はエビデンス欄とNDJSONに別記。
以前の後方接触申告は記録として保持。今回の実施日・実測値は未記入。
単体Excelの入力・出力欄には関数名と引数／戻り値／参照・更新する状態を明記。
参照・更新はグローバル状態で、void関数の戻り値を表してはいません。
