# 公開テンプレートに沿った関数設計書

公開保存された関数設計書_sample.xlsxを使用し、CompletedObstacleCar.inoの15関数を記入しました。元のSheet1を保ち、6つの枠を埋め、同じ枠を9つ追加しています。

項目は、機能概要、処理内容、関数形式、入出力（種類・形式・内容）、ソースファイル、呼び出し関数、備考です。引数・戻り値と、引数以外の状態入力・副作用の出力を分けて記載しています。

対象ソース固定版：
https://github.com/aaa1550/-/blob/bb244ca71bca4e2cfc58cb1afbd611874b4c1abb/docs/code/CompletedObstacleCar/CompletedObstacleCar.ino

前方10cmで回避後退、後方検知で回避前進、通常PWM180・回避PWM100、サーボなしの版です。コード・ピン・既存のテスト結果は変更していません。後方赤外線センサーの検知遅延・接触の実機事象は未解決として、該当関数の備考に記入しています。

Excelとサイト版で同じ内容を閲覧できます。以前の関数設計HTMLはFunctionDesignLegacy.htmlとして保持しています。
