// ── Japanese locale ────────────────────────────────────────────────────────
// すべてのユーザー向け日本語テキスト。新しいキーを追加する場合は en.js にも追加してください。

const LOCALE_JA = {

    algoInfo: [
        null,
        {
            title:      '安定化カウントアルゴリズム',
            desc:       '各ラウンド、エージェントは自分の履歴木をブロードキャストします。アルゴリズムは「露出ペア」—互いを観測し合った2つのエージェント—を見つけ出し、リーダーから外側へ匿名性の推定値を伝播させます。最初は誤った推定値を出力することがありますが、ラウンド 2n \u2212 2 までに正しいカウント n に安定することが保証されています。',
            boundLabel: '安定するラウンド',
            bound:      n => n > 0 ? `2n \u2212 2 = ${2 * n - 2}` : '\u2014',
        },
        {
            title:      '終了型カウントアルゴリズム',
            desc:       'より強力なバリアント：アルゴリズムはカウントが正しいと確信したとき、明示的に停止して「完了」を通知します。結果を発表する前に、カウント済みエージェントの完全な「島（isle）」を構築するため、誤った出力がコミットされることは一切ありません。',
            boundLabel: '終了するラウンド',
            bound:      n => n > 0 ? `3n \u2212 3 = ${3 * n - 3}` : '\u2014',
        },
    ],

    tutorial: [
        { title: 'ツアーへようこそ',         text: 'メインUIエリアの簡単なガイドです。「次へ」を押して進む、「\u2190前へ」で戻る、または「\u2715」でいつでも閉じることができます。',                                                                                                                                                                                                     sel: null              },
        { title: 'キャンバス',               text: 'メインエリアは2つに分かれています。左半分：ネットワーク—円はエージェント（固有IDを持たない匿名の計算主体）です。右半分：履歴木—各エージェントがラウンドごとに観測した内容を記録します。2つのエージェントは、履歴木が同一の場合にのみ区別不可能です。',                                                                            sel: '#canvas-wrap'    },
        { title: 'ダイナミックラウンド',      text: 'エッジはラウンドごとに変化します—それがネットワークを動的にしている理由です。ツールバーの\u25b2/\u25bcまたはキャンバス上のスクロールホイールを使ってラウンドを移動し、トポロジの変化を観察してください。',                                                                                                                   sel: '#btn-prev-round' },
        { title: 'カウントアルゴリズム',      text: '「安定化」または「終了型」を有効にしてカウントアルゴリズムを実行します。ツールバー下にアルゴリズムの説明と理論上のラウンド上界を示すバナーが表示されます。',                                                                                                                                                                 sel: '.algo-group'     },
        { title: 'ステップの実行',            text: 'エージェントまたは履歴木ノードを選択し、\u25b6ステップを押す（またはSpace）とアルゴリズムが1ステップ進みます。アルゴリズムがネットワークサイズを推定するにつれて、履歴木ノードの色が変化します。',                                                                                                                       sel: '#btn-step'       },
        { title: 'ネットワークの読み込み',    text: '「読込」をクリックして networks/ フォルダからサンプルを開きます。DefaultNetwork2.txt が入門として最適です。BoldiVigna.txt は研究文献でよく知られた例です。',                                                                                                                                                               sel: '#btn-load'       },
        { title: 'ネットワーク統計',          text: '左下の「ネットワーク統計」パネルにはリアルタイム情報が表示されます：エージェント数 n、ラウンド数 T、匿名性クラス数、および現時点で一意に特定されたエージェント数。',                                                                                                                                                        sel: '#net-stats'      },
        { title: '完了！',                   text: 'これで探索の準備ができました！「用語集」で重要な用語を確認し、ヘルプ（H）でキーボードショートカットを参照してください。さまざまなネットワークを読み込んで、2つのアルゴリズムの動作を比較してみましょう。',                                                                                                                    sel: null              },
    ],

    guidedStab: [
        { title: 'ようこそ — カウントを始めよう！',           body: 'サンプルネットワークを読み込みました：5ラウンドにわたる6エージェントです。各エージェントのIDを知ることなく、安定化カウントアルゴリズムがどのように機能するかを理解することが目標です。',                                                                                                          action: null,                                               highlight: null            },
        { title: 'エージェントは匿名',                        body: '左パネルの円がエージェントです。名前もIDもありません。唯一の例外はL（リーダー）で、特別な初期入力値0を持ちます。他のすべてのエージェントは入力値1から始まり、最初は完全に同一です。',                                                                                                                   action: null,                                               highlight: null            },
        { title: 'ラウンド1：リング構造',                     body: 'ネットワークはリング状から始まります：L — 1 — 2 — 3 — 4 — 5 — L。エージェント1と5は対称（どちらもLの隣）、エージェント2・3・4も対称です。アルゴリズムはこの曖昧さを解消しなければなりません！',                                                                                                    action: null,                                               highlight: null            },
        { title: 'ラウンド2へ進む',                           body: '\u2193キーかキャンバス上のスクロールホイールでラウンド2へ移動してください。リーダーLが接続するエージェントに注目—トポロジがすでに変化しています！',                                                                                                                                                   action: '\u2193キーかスクロールで進む',                       highlight: '#btn-next-round' },
        { title: 'ネットワークは動的！',                       body: 'リンクが変化しました—異なる接続セットが有効になっています。これがネットワークを動的にしている理由です：敵対者は各ラウンドでネットワークを再接続できます。右パネルの履歴木が1レベル成長したことに注目してください。',                                                                                     action: null,                                               highlight: null            },
        { title: 'ラウンド3へ進む',                           body: 'もう一度\u2193を押してラウンド3へ進んでください。リーダーLは以前到達できなかったエージェントに接続するようになります—これがネットワークを予測困難にする仕組みです。',                                                                                                                                  action: '\u2193キーかスクロールで進む',                       highlight: '#btn-next-round' },
        { title: '5つのダイナミックラウンド',                  body: 'このネットワークは5ラウンドあります。↓を押してラウンド5まで進んでください—アルゴリズムが正しく機能するには全ラウンドの情報が必要です。履歴木はラウンドごとに1レベル成長し、観測パターンが異なるエージェントは区別可能になります。',                                                              action: '↓キーかスクロールでラウンド5まで進む',               highlight: '#btn-next-round' },
        { title: '安定化アルゴリズムを有効化',                 body: 'カウントを開始します！ツールバーの「安定化」ボタンをクリックしてください。アルゴリズムの説明と理論的な保証を示すバナーが表示されます。',                                                                                                                                                              action: '「安定化」ボタンをクリック',                         highlight: '.algo-group'   },
        { title: 'エージェントを選択',                        body: '左パネルで任意のエージェント（円）を左クリックして選択してください。アルゴリズムは出発点となる視点を必要とします。',                                                                                                                                                                              action: '左パネルで任意のエージェントを左クリック',            highlight: null            },
        { title: '1ステップ実行',                             body: 'ツールバーの\u25b6ステップ（またはSpace）を押してください。アルゴリズムは「露出ペア」—互いを観測し合ったエージェント—を探し、匿名性の推定値を割り当て始めます。',                                                                                                                                       action: '\u25b6ステップまたはSpaceを押す',                    highlight: '#btn-step'     },
        { title: '色が変わった！',                            body: '履歴木を見てください：L（リーダー）はすぐに緑になります（一意だから）。エージェント1・5とエージェント2・3・4はYELLOW（黄色）から始まります—ラウンド1では区別不可能なため、推定しかできません。ステップを続けて曖昧さが解消される様子を観察してください。',                                             action: null,                                               highlight: null            },
        { title: '完了まで繰り返す',                          body: '\u25b6ステップを何度か押してください。履歴木のルートノードに注目—緑になり「6」が表示されたとき、アルゴリズムは正しく n = 6 を決定したことを意味します！',                                                                                                                                              action: 'ルートノードが緑になるまで\u25b6ステップを押し続ける', highlight: '#btn-step'     },
        { title: '\ud83c\udf89 n = 6 のカウント成功！',        body: 'ルートノードに6が表示されました—安定化アルゴリズムが正しく n = 6 を決定しました！上の「終了型」タブを試して、異なる保証を持つより強力なバリアントを確認してみましょう。',                                                                                                                           action: null,                                               highlight: null            },
    ],

    guidedTerm: [
        { title: '終了型 — より強い保証',                     body: '安定化（一時的に誤った値を出力することがある）とは異なり、終了型アルゴリズムは明示的に停止し、常に正しい答えのみを出力します。代償：終了は 2n \u2212 2 ではなく 3n \u2212 3 ラウンドまでかかります。同じ6エージェントネットワークが読み込まれています。',                                                action: null,                                               highlight: null            },
        { title: '終了型アルゴリズムを有効化',                 body: 'ツールバーの「終了型」ボタンをクリックしてください。バナーが更新され、3n \u2212 3 の上界が表示されます。n = 6 の場合、最大15ラウンドです。',                                                                                                                                                          action: '「終了型」ボタンをクリック',                         highlight: '.algo-group'   },
        { title: 'エージェントを選択',                        body: '左パネルで任意のエージェントを左クリックしてください。アルゴリズムはネットワークサイズの検証可能な証拠を探すため、選択エージェントの履歴木を解析します。',                                                                                                                                           action: '任意のエージェントを左クリック',                      highlight: null            },
        { title: '最初のステップ — 色に注目',                  body: '\u25b6ステップを押してください。履歴木の色に注目：シアン＝初期レベルの推測（未確定）、オレンジ＝中間的な絞り込み。重要：ノードが赤になることはありません—アルゴリズムは誤った答えをコミットしません。',                                                                                              action: '\u25b6ステップまたはSpaceを押す',                    highlight: '#btn-step'     },
        { title: '島（Isle）の構築',                          body: '終了型アルゴリズムは「島」を構築します—カウントを相互に確認し合うエージェントのグループです。オレンジノードは絞り込み中の島です。島が完全に検証されたときだけ、ルートがコミット（緑になる）します。ステップを続けてください。',                                                                     action: null,                                               highlight: null            },
        { title: '終了まで繰り返す',                          body: '\u25b6ステップを繰り返し押してください。各ステップで新たに観測されたエージェントからの証拠が伝播します。アルゴリズムが確信を持ったとき、ルートが緑になって停止します—それ以降のステップで値が変わることはありません。',                                                                              action: 'ルートが n = 6 で緑になるまで\u25b6ステップを押す',   highlight: '#btn-step'     },
        { title: '\ud83c\udf89 終了 — n = 6！',               body: '終了型アルゴリズムが証明可能な正しい答え n = 6 で停止しました。安定化とは異なり、誤った出力は一度も現れませんでした。「安定化」タブに戻って、ステップ数とラウンド上界を比較してみましょう。',                                                                                                     action: null,                                               highlight: null            },
    ],

    commentary: {
        uniqueIdentified: (du, unique, n) => `+${du} エージェントを一意に特定  (${unique} / ${n})`,
        classesChanged:   (from, to)       => `匿名性クラス: ${from} \u2192 ${to}`,
        rootGuessChanged: (was, now, ok)   => `ルート推定値: ${was} \u2192 ${now}${ok ? '  \u2713 正解！' : ''}`,
        nodesGuessed:     (dg, total)      => `${dg} ノードに推定値を割り当て  (計 ${total} ノード)`,
        noChange:         '変化なし — 別のエージェントを選択して試してください。',
    },

    ui: {
        // ツールバーラベル
        tbRound:     'ラウンド',
        tbAlgorithm: 'アルゴリズム',
        tbEdit:      '編集',
        // アルゴリズムボタン
        btnNone:        'なし',
        btnStabilizing: '安定化',
        btnTerminating: '終了型',
        // アイコン付きボタン（テキスト部分のみ）
        btnStep:     'ステップ',
        btnAgent:    'エージェント',
        btnDeselect: '選択解除',
        btnLoad:     '読込',
        btnSave:     '保存',
        btnOptions:  'オプション',
        btnGlossary: '用語集',
        btnTour:     'ツアー',
        btnLearn:    '学習',
        btnHelp:     'ヘルプ',
        // オプションパネル
        optOutdegree:   '出次数認識を切替',
        optArrows:      '矢印の表示切替',
        optRoundNodes:  'ノードの形を切替（丸/四角）',
        optHighlight:   '現在レベルのハイライト切替',
        optRedEdge:     '赤エッジの描画モード切替',
        optGrid:        'エージェントをグリッドに整列',
        optCaps:        'デフォルトを双方向リンクにする（Caps Lock）',
        optStudentMode: '学習モード',
        // キャンバスラベル
        labelNetwork: 'ネットワーク',
        labelHistory: '履歴木',
        // ノードカラー凡例
        legendTitle:   'ノードの色',
        legYellow:     '未推定',
        legGreen:      '正しく推定/カウント済み',
        legStabRed:    '誤った推定',
        legTermCyan:   '初期レベルの推測',
        legTermOrange: '中間推測',
        legTagStab:    '（安定化）',
        legTagTerm:    '（終了型）',
        // ネットワーク統計パネル
        statsTitle:   'ネットワーク統計',
        statsAgents:  'エージェント数',
        statsLeaders: 'リーダー数',
        statsRounds:  'ラウンド数',
        statsLinks:   'リンク数（現ラウンド）',
        statsClasses: '匿名性クラス数',
        statsUnique:  '一意に特定済み',
        // アルゴリズムバナー
        bannerHint:    'エージェントまたは履歴木ノードを選択し、<kbd>Space</kbd>（または<strong>\u25b6 ステップ</strong>）を押してステップを実行してください。',
        bannerUniq:    '一意に特定済み',
        bannerAgents:  'エージェント',
        bannerClasses: '匿名性クラス数',
        // ツアーパネル
        tutPrev: '\u2190 前へ',
        tutNext: '次へ \u2192',
        tutDone: '\u2713 完了',
        // 学習パネル
        guidedBadge: '\ud83c\udf93 学習',
        guidedSkip:  'スキップ \u2192\u2192',
        guidedNext:  '次へ \u2192',
        guidedDone:  '\u2713 完了',
        // その他メッセージ
        wasmNotReady: 'WASMがまだ準備できていません — しばらく待ってから再試行してください。',
        // ヘルプモーダル
        helpTitle: 'Anonymous Dynamic Networks — 操作ガイド',
        helpClose: '\u2715 閉じる',
        helpContent: `<h3>マウス操作 — ネットワークパネル</h3>
<table>
<tr><td>エージェント上で左クリック＋ドラッグ</td><td>そのエージェントから新しい有向リンクを描画</td></tr>
<tr><td>エージェント上で右クリック＋ドラッグ</td><td>エージェントを移動</td></tr>
<tr><td>空白領域で右クリック</td><td>新しいエージェントを作成</td></tr>
<tr><td>エージェント/ノードを左クリック</td><td>選択/選択解除</td></tr>
<tr><td>マウスホイール</td><td>現在のラウンドを変更</td></tr>
</table>
<h3>リンクの描画・削除時の修飾キー</h3>
<table>
<tr><td>Ctrl または Q</td><td>作成ではなくリンクを削除</td></tr>
<tr><td>Shift</td><td>リンクを双方向（無向）にする</td></tr>
<tr><td>Alt</td><td>すべてのラウンドに変更を適用</td></tr>
<tr><td>Caps Lock</td><td>切替：双方向をデフォルト方向にする</td></tr>
</table>
<h3>ナビゲーション</h3>
<table>
<tr><td>\u2191 / \u2193</td><td>前/次のラウンド</td></tr>
<tr><td>\u2190 / \u2192</td><td>現在のビューで前/次の匿名性クラス</td></tr>
<tr><td>ESC</td><td>エージェントの選択解除とビュー選択のクリア</td></tr>
</table>
<h3>ネットワーク編集</h3>
<table>
<tr><td>Del または U</td><td>選択したエージェントを削除</td></tr>
<tr><td>+</td><td>現在のラウンドの後に新しいラウンドを挿入</td></tr>
<tr><td>\u2212</td><td>現在のラウンドを削除</td></tr>
<tr><td>Backspace</td><td>現在のラウンドのすべてのリンクを削除</td></tr>
<tr><td>0〜9</td><td>選択エージェントの入力値を設定（0 = リーダー L）</td></tr>
<tr><td>L</td><td>ファイルからネットワークを読み込む</td></tr>
<tr><td>S</td><td>ファイルにネットワークを保存</td></tr>
<tr><td>G</td><td>すべてのエージェントをグリッドに整列</td></tr>
</table>
<h3>カウントアルゴリズム（TABで切替、Spaceでステップ実行）</h3>
<table>
<tr><td>Tab</td><td>切替：なし \u2192 安定化 \u2192 終了型 \u2192 なし</td></tr>
<tr><td>Space</td><td>アルゴリズムを1ステップ実行（先にエージェントまたは履歴ノードを選択）</td></tr>
</table>
<table style="margin-top:6px">
<tr><td style="color:#ffff00">黄色ノード</td><td>未推定</td></tr>
<tr><td style="color:#80ff80">緑色ノード</td><td>正しく推定/カウント済み</td></tr>
<tr><td style="color:#ff8080">赤色ノード</td><td>誤った推定（安定化）</td></tr>
<tr><td style="color:#00cfcf">シアンノード</td><td>初期レベルの推測（終了型）</td></tr>
<tr><td style="color:#ffc040">オレンジノード</td><td>中間推測（終了型）</td></tr>
</table>
<h3>表示オプション</h3>
<table>
<tr><td>O</td><td>エージェントの出次数認識を切替</td></tr>
<tr><td>A</td><td>エッジの矢印表示を切替</td></tr>
<tr><td>D</td><td>赤エッジ描画モードを切替：全表示 \u2192 選択ビューのみ \u2192 非表示</td></tr>
<tr><td>B</td><td>丸/四角ノードを切替</td></tr>
<tr><td>C</td><td>現在レベルのグレーハイライトバーを切替</td></tr>
<tr><td>H</td><td>この操作ガイドを表示</td></tr>
</table>
<div id="help-ref">理論的背景: <a href="https://arxiv.org/abs/2404.02673" target="_blank" rel="noopener">arxiv.org/abs/2404.02673</a></div>`,
        // 用語集モーダル
        glossaryTitle: 'Anonymous Dynamic Networks — 用語集',
        glossaryClose: '\u2715 閉じる',
        glossaryContent: `<div class="gl-entry"><div class="gl-term">エージェント（Agent）</div><div class="gl-def">ネットワーク内の計算主体。エージェントは固有識別子を持ちません—<em>匿名</em>です。各エージェントは整数の<em>入力</em>値から始まります（0 = リーダー）。</div></div>
<div class="gl-entry"><div class="gl-term">リーダー（Leader）</div><div class="gl-def">入力値0を持つ特別なエージェントで、シミュレータでは<strong>L</strong>と表示されます。カウントアルゴリズムは、エージェントの総数<em>n</em>を決定するための基準点としてリーダーを使用します。</div></div>
<div class="gl-entry"><div class="gl-term">動的ネットワーク（Dynamic Network）</div><div class="gl-def">通信リンクがラウンドごとに変化するネットワーク。静的ネットワークとは異なり、エッジは永続的ではありません—敵対者によって制御される可能性があります。エージェントが固定されたトポロジに依存できないことが主な課題です。</div></div>
<div class="gl-entry"><div class="gl-term">ラウンド（Round）</div><div class="gl-def">通信の1タイムステップ。各ラウンドでは一連の有向リンクが有効で、エージェントはそれらのリンクに沿ってのみメッセージを交換します。シミュレータでは<strong>\u2191 / \u2193</strong>またはスクロールホイールでラウンドを閲覧できます。</div></div>
<div class="gl-entry"><div class="gl-term">インタラクション/リンク（Interaction / Link）</div><div class="gl-def">あるラウンドにおけるエージェントAからエージェントBへの有向エッジ：AがBにメッセージを送ります。<em>多重度</em>は送られるメッセージの並行コピー数を表し、カウントアルゴリズムに影響します。</div></div>
<div class="gl-entry"><div class="gl-term">履歴木（History Tree）</div><div class="gl-def">エージェントがこれまでに観測したすべてを記録するツリー。ルートは初期状態で、各レベルは1ラウンド分の観測を追加します。2つのエージェントは、履歴木が同一の場合にのみ<em>区別不可能</em>です。シミュレータの右パネルでこのツリーを視覚化できます。</div></div>
<div class="gl-entry"><div class="gl-term">匿名性クラス（Anonymity Class）</div><div class="gl-def">同一の履歴木を持つエージェントのグループ—互いを区別できません。クラスのサイズがその<em>匿名性</em>です。匿名性1は、そのエージェントが履歴によって一意に識別可能であることを意味します。</div></div>
<div class="gl-entry"><div class="gl-term">カウント問題（Counting Problem）</div><div class="gl-def">エージェントの総数<em>n</em>を決定するタスク。エージェントは匿名であるため、単純に自分たちを数えることはできません—複数ラウンドにわたる受信メッセージのパターンから<em>n</em>を推論する必要があります。</div></div>
<div class="gl-entry"><div class="gl-term">安定化アルゴリズム（Stabilizing Algorithm）</div><div class="gl-def">最終的に正しい答えを出力し、<em>それ以降は変更しない</em>アルゴリズムですが、安定する前に誤った値を出力することがあります。シミュレータの安定化アルゴリズムはラウンド2n \u2212 2までに安定することが保証されています。<strong>安定化</strong>ボタンで選択できます。</div></div>
<div class="gl-entry"><div class="gl-term">終了型アルゴリズム（Terminating Algorithm）</div><div class="gl-def">正しい答えを出力して<em>停止</em>し、完了を明示的に通知するアルゴリズム。これは安定化よりも強い保証です：停止したとき、答えは証明可能に正しいです。<strong>終了型</strong>ボタンで選択できます。</div></div>
<div class="gl-entry"><div class="gl-term">出次数認識（Outdegree Awareness）</div><div class="gl-def">オプション機能（<strong>O</strong>で切替）で、各エージェントが前のラウンドで<em>送信した</em>メッセージ数も把握します。この追加情報により、アルゴリズムがより速くまたはより正確な推定ができる場合があります。</div></div>
<div class="gl-entry"><div class="gl-term">露出ペア（Exposed Pair）</div><div class="gl-def">エージェントAとBが互いを対応する多重度で観測し合うペア。露出ペアは、安定化アルゴリズムがネットワーク全体に匿名性推定値を伝播させるために使用する重要なメカニズムです。</div></div>
<div id="glossary-ref">理論的背景: <a href="https://arxiv.org/abs/2404.02673" target="_blank" rel="noopener">arxiv.org/abs/2404.02673</a></div>`,
    },
};
