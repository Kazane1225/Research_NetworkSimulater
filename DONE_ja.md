# 最適化ログ

## 2026-05-20 — ラウンド追加ベンチマーク（current vs `main`）

**目的:**  
ラウンド数が大きい場合に UI 上のラウンド追加がどの程度重くなるかを、現在ブランチと `main` ブランチで同条件比較する。特に「200 ラウンドを超えたあたりから UI が引っかかる」という体感が、実測でも現れるかを確認する。

**測定環境:**
- OS: Windows
- 実行場所: VS Code 内蔵ブラウザ
- User-Agent: `Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Code/1.120.0 Chrome/142.0.7444.265 Electron/39.8.8 Safari/537.36`
- 論理コア数: 10
- Viewport: `1812 x 942`

**測定対象ブランチ:**
- current: `feature/improve_algorithm`
- baseline: `main`（別 worktree `C:\myproject\Research_NetworkSimulater_main` 上で測定）

**テストデータ / 初期状態:**
- `Module._TutorialLoadNetwork()` を使用
- エージェント数: 6
- 初期ラウンド数: 5
- ベンチ前に最終ラウンドまで移動したあと、`-` ボタンで 1 ラウンドまで削減
- その後、`+` ボタン相当の操作で `1 -> 250` ラウンドまで増加

**操作方法:**
- DOM 上の `btn-add-round` / `btn-del-round` / `btn-next-round` / `btn-prev-round` を Playwright から呼び出し
- 各 `+` 操作について、`Module._GetNumRounds()` が 1 増えたことを待ってから `requestAnimationFrame` 1 回分待機
- 25 ラウンドごとに区間時間を集計
- 比較条件を揃えるため、current と `main` は**同じ前面タブ 1 枚**を `http://localhost:8000/` と `http://localhost:8001/` に切り替えて順番に計測

**完了条件の解釈:**
- 「ラウンド追加ボタン連打」のベンチなので、各クリックについて
	1. ラウンド数が更新される
	2. 次フレームが 1 回描画される
	の 2 点を満たした時点をそのクリックの完了とみなす
- 完全な見た目の安定化を数百 ms 待つ方式ではなく、**高速連打時の応答性**を見る測定

**結果サマリ:**

| 指標 | current | `main` |
|---|---:|---:|
| 1 -> 250 総時間 | 1.296 s | 16.162 s |
| 全クリック平均 | 4.9 ms/click | 64.6 ms/click |
| 最初の 50 クリック平均 | 4.2 ms/click | 4.9 ms/click |
| 200 超のクリック平均 | 6.5 ms/click | 155.9 ms/click |
| 最大クリック時間 | 10.9 ms | 195.6 ms |
| 95 パーセンタイル | 7.5 ms | 173.4 ms |

**25 ラウンドごとの区間時間:**

| 区間 | current | `main` |
|---|---:|---:|
| 1 -> 25 | 100.3 ms | 100.3 ms |
| 25 -> 50 | 104.4 ms | 134.6 ms |
| 50 -> 75 | 104.3 ms | 326.9 ms |
| 75 -> 100 | 104.4 ms | 603.3 ms |
| 100 -> 125 | 104.4 ms | 974.1 ms |
| 125 -> 150 | 109.0 ms | 1445.8 ms |
| 150 -> 175 | 121.2 ms | 2027.9 ms |
| 175 -> 200 | 141.2 ms | 2677.3 ms |
| 200 -> 225 | 162.0 ms | 3473.9 ms |
| 225 -> 250 | 164.2 ms | 4319.3 ms |

**観察:**
- current ブランチでも 200 ラウンド超でわずかに重くなる。`1 -> 175` では各 25 ラウンド区間がほぼ 100–120 ms に収まるが、`175 -> 250` では 141–164 ms まで伸びる。
- ただし伸び方は緩やかで、`main` に比べると非常に小さい。
- `main` は 50 ラウンド以降から段階的に悪化し、200 超では 25 ラウンド追加するだけで 2.7–4.3 秒かかる。
- 「200 ラウンドを超えたあたりから UI が引っかかる」という体感は、`main` の数値とは強く整合する。current では同じ現象はかなり軽減されている。

**結論:**
- 末尾ラウンド追加の連打に関して、current ブランチは `main` より **約 12.5 倍高速**。
- 特に高ラウンド帯（200 超）では、current の平均 6.5 ms/click に対して `main` は 155.9 ms/click で、**約 24 倍**の差がある。
- 現在の最適化は「ラウンド数が増えるほど UI が悪化する」主因を大幅に抑えている。

---

## 2026-05-20 — middle insert 向けの checkpoint + suffix replay 最適化

**ファイル:** `src/network.c`, `src/network.h`, `src/events.c`

**問題:**  
既存の最適化は、`+` が末尾ラウンド追加になるケース（`AppendLastRound`）だけを改善していた。カーソルがタイムライン途中にある状態で `+` を押すと、実際には `currentRound + 1` に新しいラウンドが挿入されるため、それ以降の全ラウンドが影響を受ける。この middle insert 経路は従来どおり `ExecuteNetwork()` にフォールバックしており、round 0 から全再実行していた。

この違いはベンチマーク上でも重要で、round 1 付近で `+` を連打して得た遅い数値は、末尾追加最適化の失敗ではなく、middle insert を測っていたことが原因だった。

**修正内容:**  
定期 checkpoint と suffix replay を追加した。

1. `ExecuteNetwork()` 実行中に、round 1 の直後と、その後は 8 ラウンドごとに checkpoint を保存。
2. checkpoint には、各 entity の `history`・`current`・`outdegree` を保持。
3. `ExecuteNetworkFromRound(firstRound)` を追加し、`firstRound` 以下で最も近い checkpoint を復元してから、影響を受ける suffix だけを再実行するようにした。
4. `events.c` では、middle の `+` 挿入と middle の `-` 削除が `ExecuteNetwork()` ではなく `ExecuteNetworkFromRound(currentRound)` を呼ぶよう変更した。

末尾の挙動はそのまま維持している:
- 最終ラウンドへの追加は引き続き `AppendLastRound()`
- 最終ラウンド削除は引き続き `RollBackLastRound()`

**実測結果:**

- Tutorial network で round 1 付近から `+` を連打し、合計 250 ラウンドに到達するケース:
- 修正前: 約 `22.1 s`
- 修正後: `2.532 s`
- 平均: `10.34 ms/click`
- p95: `21.60 ms`

- checkpoint 導入後に、末尾追加も再確認:
- タイムライン末尾で `5 -> 250` まで `245` 回 append
- 総時間: `1.115 s`
- 平均: `4.55 ms/click`
- p95: `6.23 ms`

**効果:**  
middle insert / middle delete が毎回 round 0 からのフル再実行を行わなくなり、変化のない prefix を再利用して suffix だけを再計算するようになった。append-at-end ほど安くはならないが、タイムライン途中での編集時に支配的だった最悪ケースを大きく削減できた。

---

## 2026-05-04 — `AppendLastRound` O(n²) インクリメンタル finalHistory 拡張

**ファイル:** `src/network.c`, `src/entity.c`, `src/history_tree.h`, `src/history_tree.c`

**問題:**  
`AppendLastRound` は `RebuildFinalHistory()` を呼び、全エンティティのヒストリーツリー（深さ R）を毎回ゼロから `finalHistory` に再マージしていた — O(R × n³)/回。R が増えるほど 1 ラウンド追加のコストが大きくなる。

**修正内容:**  
`Observation` に `HistoryTree *finalLeafAtSend` フィールドを追加。`SendHistory` は送信時の `e1->finalLeaf`（送信者の `finalHistory` 上の位置）をメールボックス観測に記録する。`EndRound` がメールボックスを破棄する前に、`AppendLastRound` がエンティティごとの `(senderFinalLeaf, multiplicity)` ペアを収集する。`EndRound` 後、`ExtendFinalHistoryOneLevel` が `finalHistory` にちょうど 1 レベルだけ追加する:

1. 収集したペアを `senderFinalLeaf` ポインタ値でソート・マージ（同一送信者 2 回 → 多重度を合算）。
2. 各エンティティについて `prevFinalLeaf[i]` の既存子ノードをソート済み `observations` リストのzip比較で検索 — O(children × obs)。
3. 見つからない場合: 新しい子ノードを作成し、`senderFinalLeaf` を対象とした赤エッジを追加（`finalHistory` 内のノードへのポインタ — 追加のポインタ追跡不要）。
4. `e->finalLeaf` を見つかった/作成したノードに更新。

**効果:**  
`AppendLastRound` のヒストリー拡張ステップが O(R × n³) → O(n²) に削減。残コストは `ComputeAuxData` O(R × n × obs) と `EndRound` のマージ O(n × R × n²)。インタラクティブな操作（`=` キーを押すたびに 1 ラウンド追加）のヒストリー拡張ボトルネックが解消される。

---

## 2026-05-04 — `ComputeAuxDataRedEdges` ハッシュマップ化

**ファイル:** `src/auxdata.c`

**問題:**  
`ComputeAuxDataRedEdges` は各赤エッジターゲットをレベル i−1 の全ノードに対して線形探索していた — 1 赤エッジあたり O(n)、全体で O(R × n² × obs)。R と n の両方に比例して遅くなる。

**修正内容:**  
各レベル i の処理前に、レベル i−1 用の小さなオープンアドレッシングハッシュマップ（`HistoryTree * → AuxData インデックス`）を構築。赤エッジの検索が O(n) → O(1) 期待値に。マップはレベルごとに確保・解放する。

**効果:**  
`ComputeAuxDataRedEdges` が O(R × n² × obs) → O(R × n × obs) に削減。n=20 の場合、このステップで 20 倍の高速化。

---

## 2026-05-04 — Merkle ハッシュ + `HistoryTreeEquals` 早期終了

**ファイル:** `src/history_tree.h`, `src/history_tree.c`, `src/network.c`

**問題:**  
`HistoryTreeEquals` は常に BFS 全走査を 2 回（`HistoryTreeContains` × 2）実行していた。明らかに異なるツリーでも例外なく走査していた。

**修正内容:**  
`HistoryTree` に `unsigned long long hash` フィールドを追加。ハッシュは純粋な Merkle フィンガープリント: `input`・`outdegree`・子ノードハッシュのソート済みマルチセット（赤エッジは意図的に除外 — 赤エッジはレベル R からレベル R−1 へ上向きに向かうため、後順 DFS でレベル R のハッシュを計算する時点ではレベル R−1 のハッシュがまだ計算されておらず、含めると循環依存が生じる）。

- `ComputeNodeHash`: 子ノードが既にハッシュ済みであることを前提に 1 ノードのハッシュを計算。
- `ComputeHashBottomUp`: 後順 DFS でサブツリー全体のハッシュを再計算。各エンティティのヒストリーが確定した後、`RebuildFinalHistory` 内で呼び出される。
- `HistoryTreeEquals` の先頭に 1 行のガードを追加: 両方のルートハッシュが非ゼロかつ異なる場合は即座に `false` を返す（等しくないケースで O(1) 終了）。等しいか不確かな場合は従来どおり `HistoryTreeContains` × 2 の BFS にフォールスルー。

**注記:** 以前の試みでは、ハッシュフィールドを用いて `MergeHistoryTrees` と `HistoryTreeContains` の `EquivalentNodes` をオープンアドレッシングの O(1) ハッシュマップに置き換えようとした。しかし赤エッジハッシュを Merkle ハッシュに含めると循環依存が生じるため除外した結果、同レベルの構造的に異なるノードを区別できなくなる問題が発覚し、このアプローチは破棄した。両関数での等価性判定は引き続き `EquivalentNodes`（赤エッジを直接検査）が担う。正しいキー（`local_key`）を用いたハッシュマップは後に別エントリで実装された。

---

## 2026-05-04 — ラウンド追加・削除時のインクリメンタル更新

**ファイル:** `src/network.c`, `src/network.h`, `src/entity.h`, `src/entity.c`

**問題:**  
ユーザー操作のたびに `ExecuteNetwork()` が全エンティティのヒストリーツリーをゼロから再構築していた。ラウンド数 R、エンティティ数 n として O(R × n²) のコスト。

**修正内容:**  
`entity.h` に `EntitySnapshot *snap` フィールドを追加（`history` のコピー・`current` のコピー・`outdegree` を保持）。`ExecuteNetwork()` は最終ラウンド適用直前に `TakeSnapshotsBeforeRound()` でスナップショットを保存。3 つの新関数がフルリビルドなしで 3 つのケースを処理する:

- `ReExecuteLastRound()` — スナップショットからコピー復元 → 最終ラウンドを再実行 → `finalHistory` 再構築。スナップショットは引き続き有効。最終ラウンドのリンク追加・削除・全消去に使用。
- `RollBackLastRound()` — スナップショットの所有権を移して復元 → `finalHistory` 再構築。スナップショットは消費・無効化。最終ラウンド削除時に使用。
- `AppendLastRound()` — 現在のエンティティ状態を新たにスナップショット保存 → 末尾に追加された新ラウンドを適用 → `finalHistory` 再構築。末尾へのラウンド追加時に使用。

`events.c` の変更箇所:
- BACKSPACE（ラウンド内リンク全削除）: 最終ラウンドなら `ReExecuteLastRound()`
- MINUS（ラウンド削除）: 最終ラウンドが削除された場合 `RollBackLastRound()`
- EQUALS（ラウンド挿入）: 末尾追加なら `AppendLastRound()`
- マウスリンク追加・削除: 最終ラウンドの単一ラウンド変更なら `ReExecuteLastRound()`（全ラウンドモディファイア時は `ExecuteNetwork()`）

全パスとも、スナップショットが無効な場合（エンティティ追加・削除等の構造変更後）は `ExecuteNetwork()` にフォールバック。

**効果:**  
最終ラウンドへの操作コストが O(R × n²) → O(n²) に削減。R が増えるほど効果が大きくなる。

---



## 2026-05-04 — `MergeHistoryTrees` / `HistoryTreeContains` 子ノード探索ハッシュマップ

**ファイル:** `src/history_tree.c`

**問題:**  
`MergeHistoryTrees` と `HistoryTreeContains` の子ノード探索内ループが、`b` の全子ノードごとに `EquivalentNodes` を呼んでいた — BFS ノードあたり O(C) 回。`EquivalentNodes` は先頭の O(1) フィールドチェック（input・ outdegree・ level・ obs_count）で早期リターンするが、一致した場合は `FindRedEdge` を obs 回呼んで観測検証を行う — O(obs)。BFS ノードあたりの局所コスト: O(C × obs)。

**修正内容:**  
`b->children` を `local_key = hash(input, outdegree, observations_count)` でインデックスする小さなオープンアドレッシングハッシュマップを追加。この 3 フィールドは `EquivalentNodes` が先頭で O(1) にチェックする内容と完全に一致するため、キー不一致 = `EquivalentNodes` が false を返すことが保証され、呼び出しをスキップできる。キー一致候補のみ `EquivalentNodes` へ進む。

- `ChildEntry` 構造体（key + node ポインタ）、`local_key`、`cmap_put`、`cmap_get` をファイルローカルヘルパーとして追加。
- マップ容量を `(既存子 + 追加予定子) × 2 + 3` で事前確保し、マージ中の挿入後も負荷率 ≤50% を維持。
- `MergeHistoryTrees` で新子ノード `y` を作成した際は即座にマップに挿入し、同じ `a` の後続子ノードからも検索できるようにする。

**注記:** キーは `local_key`（input + outdegree + obs_count）であり、Merkle ハッシュ（サブツリー構造のフィンガープリント）ではない。Merkle ハッシュを `EquivalentNodes`（ローカルノードチェック）のプレフィルタとして使うのは誤り—サブツリーが異なる 2 ノードに対して `EquivalentNodes` が true を返すことがある（BFS はレベルごとに処理するため、ローカルチェックはサブツリーの一致を必要としない）。`local_key` は `EquivalentNodes` の正しい必要条件である。

**効果:**  
子ノードが異なる (input, outdegree, obs_count) を持つ典型ケースでは、不一致子ノードごとに `EquivalentNodes` 呼び出しはゼロ回。子ノード探索コストが O(C × obs) からマップ構築 O(C) + 一致候補 1 件だけの検証 O(obs) に導入。C ≈ n−1 の場合、BFS ノードあたり最大 **~n 倍** の高速化。

---

## 2026-04-08 — `HistoryTreeContains`: 不要なツリーコピーを排除

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

---

## 2026-05-04 — `observations` ソート + `FindRedEdge` 二分探索化

**ファイル:** `src/history_tree.c`

**問題:**  
`FindRedEdge(h, target)` は `h->observations` を線形走査していた — O(obs)。`EquivalentNodes` 内で obs 回呼ばれるため、`EquivalentNodes` 全体で O(obs²) になる。

**修正内容:**  
`observations` を `o->history` ポインタ値で常にソートされた状態に保つ。
- `FindRedEdge`: 二分探索に置き換え — O(log obs)。
- `AddNewRedEdge`: 二分探索で挿入位置を決定し `InsertVector` でシフト — シフトは O(obs) だが挿入は探索より稀で、obs は n−1 以下なので実コストは小さい。
- `AddRedEdge`: 変更なし（引き続き `FindRedEdge` → `AddNewRedEdge` を呼ぶ）。

`EquivalentNodes` のコストが O(obs²) から O(obs log obs) に改善。

**効果:**  
本プロジェクトの研究用ネットワークでは obs ≤ n−1 で小さい（通常 n≤20）ため改善幅は限定的だが、obs が大きいネットワークでは漸近的に効果が大きくなる。

---

## 2026-05-07 — `AppendAuxDataOneLevel`: インクリメンタル AuxData 更新

**ファイル:** `src/auxdata.c`, `src/auxdata.h`, `src/network.c`

**問題:**  
`AppendLastRound` が `ComputeAuxData(finalHistory)` を呼び出し、全 AuxData レベルをゼロから再構築していた — 赤エッジ計算だけで O(R × n × obs)。`=` キーを押すたびに O(R) のコストがかかり、ラウンドを連続追加するほど重くなっていた。

**修正内容:**  
`AppendAuxDataOneLevel()` を新規追加。`ExtendFinalHistoryOneLevel` が `finalHistory` に 1 レベルだけ追加した後、以下を実行する:

1. 新しい最深レベルの AuxData ノードのみ作成 — O(n)。
2. 旧最深レベルからルートまで幅を下から再計算 — O(R × n)。
3. 幅が変わったため全レベルの座標を再計算（不可避） — O(R × n)。
4. **新レベルのみ**赤エッジを計算（`ComputeAuxDataRedEdges` と同じハッシュマップ方式） — O(R × n × obs) → O(n × obs)。
5. 全レベルの匿名性を再計算 — O(R × n) — およびアルゴリズム状態をリセット。

`AppendLastRound` は `ComputeAuxData(finalHistory)` の代わりに `AppendAuxDataOneLevel()` を呼ぶように変更。

**効果:**  
赤エッジ計算がラウンド追加ごとに O(R × n × obs) → O(n × obs) に削減。O(R × n) のパス（幅・座標・匿名性）は残るが、赤エッジ再構築に比べれば軽微。インタラクティブなラウンド追加が R の増加とともに重くなる問題を解消。

---

## 2026-05-07 — `ComputeHashBottomUp` の dead call 削除

**ファイル:** `src/network.c`

**問題:**  
`RebuildFinalHistory` と `AppendLastRound` は `finalHistory` 構築後に `ComputeHashBottomUp(finalHistory)` を呼んでいた。`ComputeHashBottomUp` は O(R × n²)（後順 DFS、各ノードで子ハッシュをソート）。このハッシュを消費していたのは `HistoryTreeEquals` だが、`HistoryTreeEquals` はコードベース全体で一度も呼び出されていない。

**修正内容:**  
`ComputeHashBottomUp(finalHistory)` の呼び出し 2 箇所を削除。関数自体は残存（デバッグ用途に有用）、ただしホットパスからは除外。

**効果:**  
`RebuildFinalHistory` / `AppendLastRound` の呼び出しごとに O(R × n²) のパスを 1 回削除。R が大きいほど効果が大きい。

---

## 2026-05-07 — `CopyHistoryTree`: 直接 BFS コピー

**ファイル:** `src/history_tree.c`

**問題:**  
`CopyHistoryTree` は `MergeHistoryTrees(dest, src)`（`dest` は常に空の新規ツリー）で実装されていた。`MergeHistoryTrees` は各 BFS ノードで既存の子ノードを検索するための `ChildEntry` ハッシュマップを確保する — しかし `dest` が空なので全検索はミスが確定しており、マップは即座に解放され、有益な処理は何も行われていない。

**修正内容:**  
`reference` フィールドを用いた直接 BFS コピーに書き換え（`MergeHistoryTrees` と同じ src→dst マッピング慣習）。赤エッジはレベル L からレベル L−1 を指す。BFS はレベル L−1 の後にレベル L を処理するため、赤エッジのターゲットは参照設定済みであり、1 パスで完結する。

**効果:**  
ソースツリーのノードごとに 1 回発生していた `calloc`/`free`（ハッシュマップ確保）を O(n) 回分削除。アルゴリズム的な計算量に変化はないが、ツリーコピー時のヒープオーバーヘッドをゼロにする。
