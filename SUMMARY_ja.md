# サマリー

## ベンチマーク結果

### 末尾ラウンド追加

Tutorial network を 1 ラウンドまで減らし、その後 `1 -> 250` まで追加した。各追加について、ラウンド数の増加と 1 フレーム描画を完了条件として計測した。

| 指標 | current | `main` |
|---|---:|---:|
| 総時間 | 1.296 s | 16.162 s |
| 平均 | 4.9 ms/click | 64.6 ms/click |
| 最初の 50 クリック | 4.2 ms/click | 4.9 ms/click |
| 200 超 | 6.5 ms/click | 155.9 ms/click |
| 最大 | 10.9 ms | 195.6 ms |
| p95 | 7.5 ms | 173.4 ms |

### round 1 付近での middle insert

Tutorial network の round 1 付近から round 挿入を繰り返し、合計 250 ラウンドまで増やした。

| 指標 | current | `main` |
|---|---:|---:|
| 総時間 | 2.532 s | 16.926 s |
| 平均 | 10.34 ms/click | 69.09 ms/click |
| p95 | 21.60 ms | 181.95 ms |
| 最大 | 23.67 ms | 203.17 ms |

### current の limit 計測（`> 1 s / click`）

current ブランチのみを対象に、1 回の追加が `1000 ms` を超える最初のラウンド数を limit として測定した。

| シナリオ | ブランチ | limit 定義 | 結果 |
|---|---|---|---|
| 末尾追加 | current | 1 回の追加が `> 1000 ms` | 5000 ラウンドでも未到達 |
| round 1 付近の middle insert | current | 1 回の追加が `> 1000 ms` | 1993 ラウンド到達前に crash したため未到達 |

## 要点

- current ブランチは、末尾ラウンド追加で `main` より約 12.5 倍高速。
- 200 ラウンド超でも current は応答性を維持する一方、`main` は急激に悪化する。
- middle insert / middle delete は round 0 から全再実行せず、checkpoint から suffix だけを再計算するようになった。
- 末尾追加の主要ボトルネックだった `finalHistory` 全再構築を廃止し、1 レベルだけの増分更新に置き換えた。

## 主な最適化

### 1. 最終ラウンド操作の増分実行

- snapshot を使って、最終ラウンドの再実行・巻き戻し・追加を高速化。
- 効果: よく使う最終ラウンド操作が `O(R x n^2)` から `O(n^2)` に低下。

### 2. `finalHistory` の増分拡張

- append 時の `RebuildFinalHistory()` をやめ、1 レベルだけ伸ばす方式に変更。
- 効果: append 時の履歴更新コストが `O(R x n^3)` から `O(n^2)` に低下。

### 3. checkpoint からの suffix replay

- 定期 checkpoint と `ExecuteNetworkFromRound(firstRound)` を追加。
- 効果: middle insert / delete で、影響のある suffix だけを再実行するようになった。

### 4. 補助データ計算と木探索の削減

- `ComputeAuxDataRedEdges` をハッシュ化。
- history tree の等値判定と子探索の無駄を削減。
- `observations` をソートし、赤エッジ探索を二分探索化。

## History Tree の hash

- 各 `HistoryTree` ノードは、Merkle 風の構造 hash を持つ。
- hash は `input`、`outdegree`、子ノード hash のソート済みマルチセットから作る。
- red edge は、下から上への参照で bottom-up 計算時に循環依存になるため、意図的に含めていない。
- この hash は「違う木を早く弾く」ための前処理であり、最終的な同型判定そのものではない。

## Crash 改善方針

- middle insert は、以前の `1000` ラウンド前後での crash は改善したが、現在も `1993` ラウンド到達前に crash する。
- 今後も大ラウンド帯で機能を弱めるのではなく、checkpoint / replay / restore 経路そのものを安定化させる方針。
- 理由は、残っている不安定さが deep middle insert の replay 経路にあり、末尾追加側は引き続き安定かつ高速だからである。

## 測定メモ

- ベンチマーク値は、実行マシン、ブラウザ、viewport、前面/背面タブ状態の影響を受けるため、絶対値は環境依存として読む必要がある。
- そのため、もっとも信頼しやすいのは、同じマシン・同じブラウザ条件での `current` と `main` の相対比較である。
- 環境: Windows、VS Code 内蔵ブラウザ。
- 比較対象: `feature/improve_algorithm` と、別 worktree 上の `main`。
- テストデータ: `Module._TutorialLoadNetwork()`。
- 末尾追加ベンチは、バックグラウンドタブの影響を避けるため、同じ前面タブで両ビルドを順番に測定した。

実装の詳細や時系列の記録は `DONE.md` / `DONE_ja.md` を参照。