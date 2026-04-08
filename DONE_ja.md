# 最適化ログ

## 2026-04-08 — `HistoryTreeContains`: 不要なツリーコピーを除去

**ファイル:** `src/history_tree.c`

**問題:**  
`HistoryTreeContains(h1, h2)` は h1 ツリー全体をコピー（O(n) 時間・メモリ）してから h2 をマージし、コピーを破棄する実装だった。`HistoryTreeEquals` は `HistoryTreeContains` を 2 回呼ぶため、等値判定のたびにフルコピーが 2 回走っていた。

**修正内容:**  
h1 を変更せずに直接 BFS で同型性を判定する実装に書き換えた。`MergeHistoryTrees` と同じ `reference` フィールドの慣習でマッピングを追跡し、終了後に `ResetReferences` でリセットする。  
これにより、呼び出しごとに発生していた O(n) のヒープ確保＋コピーがゼロになる。

**検討した他のアイデア:**
- `observations` のハッシュマップ化（`FindRedEdge` を O(obs→1) に）: 変更範囲は `history_tree.c` のみだが、obs は n に対して有界（研究用ネットワークでは通常 n≤20）のため効果は軽微と判断し見送り。
- `MergeHistoryTrees` の子ノード探索ハッシュ化: O(C × obs²) → O(obs) になるが変更範囲が広いため保留。
- サブツリーの Merkle ハッシュ化: `HistoryTreeEquals` が O(1) になるが実装コスト大。
- `malloc`-per-node をアリーナアロケータに変更: 実装コスト小・ラウンド数が多い場合に高い効果が期待できる。
