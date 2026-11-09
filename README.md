# GitHub Pages公開手順

このパッケージはブラウザのみで表示できる静的サイトです。ChatGPTへの接続は必要ありません。

## 公開
1. GitHubで新しいPublicリポジトリ（例：osoyoo-robot-car）を作ります。READMEを追加して作成するとアップロード先のmainブランチができます。
2. このZIPを展開し、docsフォルダ・srcフォルダ・build.mjs・package.json・README.mdをリポジトリに追加します。ZIPそのものをアップロードする方法では公開できません。
3. リポジトリの Settings → Pages → Build and deployment → Source で「Deploy from a branch」を選びます。
4. Branchをmain、フォルダを/docsにしてSaveします。
5. 公開完了後、Settings → Pagesに表示される「Visit site」から開きます。公開に数分かかる場合があります。

## 利用範囲
- 要件、設計、コード、テスト、不具合、進捗、記録を閲覧・検索できます。
- 提供されたフローチャート4枚を原本のまま同梱しています。
- 画面内アップロード・編集・非公開ファイル・サーバーDBには対応しません。
- 更新はGitHub側で行います。データや資料はリポジトリとサイト上で公開されます。
- 検索エンジンへの掲載時期は保証されません。公開URLの直接入力でアクセスできます。

## データ更新
- docs/data.json のrecords配列に要件・コードなどを登録します。
- 資料ファイルを docs/files に追加し、files配列に情報を登録します。
- 相対URLを使います（files/example.pdfなど）。先頭に/を付けません。
- data.jsonの更新だけなら再ビルドは不要です。

### Arduinoコードの登録例
records配列に次の形式で追加します。コードは実機確認済みのものを登録してください。
```json
{"id":"CODE-001","kind":"codes","title":"メインプログラム","filename":"main.ino","code":"// ここに実機確認済みコードを保存","status":"下書き","visibility":"public","links":"FR-014, FC-001"}
```

### Office資料の登録例
ファイルをdocs/filesへ追加し、files配列に次を追加します。sizeはバイト数です。
```json
{"id":"FILE-001","group_id":"FILE-001","title":"発表資料","name":"presentation.pptx","url":"files/presentation.pptx","category":"10_発表資料","size":1000,"created":"2026-11-10T00:00:00+09:00","change":"初版","visibility":"public","links":"FR-001"}
```
新版は別名で追加し、group_idを同じにすると一覧上で版を識別できます。旧ファイルも残します。

## UIを変更する場合
Node.jsを用意し、プロジェクト直下で実行します。
```bash
npm install
npm run build
```
生成されたdocs/assetsを含めてコミットします。GitHub Pagesはdocs以下だけを配信します。
