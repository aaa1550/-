# 別のPCでダウンロードするための「公開保存」

## 保存するPC
1. サイト右上の「公開保存」を押します。
2. ファイルを新しく選ぶか、「このブラウザに取り込んだデータを公開する」を選びます。
3. 公開する一覧と重複IDの扱いを確認します。
4. GitHubの公開保存キーを入力し、「誰でも取得できる場所に公開保存」を押します。
5. 完了すると資料原本と「公開保存データ一式」のJSONがGitHubに登録されます。

保存内容は一般公開です。閲覧者は認証なしで取得できます。保存操作はGitHubリポジトリ aaa1550/- への書き込み権限がある人だけが利用できます。

## 初回だけ必要なGitHub認証の準備
- https://github.com/settings/personal-access-tokens/new を開きます。
- Resource owner：aaa1550。
- Expiration：有効期限を設定。
- Repository access：Only select repositories → リポジトリ「-」だけを選択。
- Repository permissions：Contents → Read and write。
- Generate tokenで作ったキーをサイトの公開保存欄へ入力します。

認証キーはチャットや公開資料に書かないでください。サイトはキーを処理中のメモリにだけ保持し、公開データ、ソース、localStorage、IndexedDBへ保存しません。送信先は固定の https://api.github.com/repos/aaa1550/-/ 配下です。

## ダウンロードするPC
同じサイト https://aaa1550.github.io/-/ を開き、「ファイル管理」から必要な資料または「公開保存データ一式」をダウンロードします。ダウンロード側ではGitHubログイン・認証キーは不要です。内容が古い場合は「公開データを再読込」を押してください。

GitHub Pagesの更新には数分かかる場合があります。共有一覧は可能な場合GitHub APIから取得し、資料はGitHubの公開原本からダウンロードします。

## 既存のブラウザ取り込みデータ
取り込んだ元のPC・ブラウザで公開保存してください。別のPCから元PCのIndexedDBを読み取ることはできません。従来の「このPCに取込」は端末内保存として残しています。公開保存後も端末内のコピーは残ります。

## 方式
共有一覧は docs/shared/catalog.json。資料とバックアップは docs/shared/files と docs/shared/backups に保存します。資料・一覧・一括JSONを1つのGitコミットで反映します。競合時は強制上書きせずエラーにし、再確認を求めます。同じIDは既定でスキップし、明示的に選んだ場合のみ置換します。

上限は資料1件20MB・合計50MB、JSONバックアップ80MB、1回20ファイルです。コードファイルはコードライブラリにも登録します。Office/PDFは原本保存であり内容の自動抽出は行いません。
