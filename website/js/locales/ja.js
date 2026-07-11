// ── Japanese locale ────────────────────────────────────────────────────────
// すべてのユーザー向け日本語テキスト。新しいキーを追加する場合は en.js にも追加してください。

const LOCALE_JA = {

    algoInfo: [
        null,
        {
            title:      '安定化カウントアルゴリズム',
            desc:       '各ラウンドでエージェントは Vista を交換します。アルゴリズムは最初の non-branching level を見つけ、赤辺の多重度の比を使ってリーダーから匿名性の推定を広げます。途中の推定は間違うことがありますが、ラウンド 2n \u2212 2 までには正しい n に安定することが保証されます。',
            boundLabel: '安定するラウンド',
            bound:      n => n > 0 ? `2n \u2212 2 = ${2 * n - 2}` : '\u2014',
        },
        {
            title:      '終了型カウントアルゴリズム',
            desc:       'より強い保証です。guesser・heavy node・検証済みの isle/cut といった正しさの証明書が揃うまで停止しません。誤った答えを確定することはありません。その代わり上界は遅く、ラウンド 3n \u2212 3 までに終了します。',
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
        { title: '安定化と終了型の違い',                          body: '学習では、2つのカウントアルゴリズムの性質の違いに焦点を当てます。まず安定化：途中では仮の（ときには誤った）推定を出し、やがて正しい n に落ち着きます。ただし「完了した」とは言いません。UIの説明はツアーを見てください。',                                                          action: null,                                               highlight: null            },
        { title: 'このデモネットワーク',                          body: 'エージェント6体、動的ラウンド5つです。Lは唯一のリーダー（入力0）、他は最初は区別できません。情報が足りないあいだ、サイズの推定はあくまで推測にすぎません。',                                                                                              action: null,                                               highlight: null            },
        { title: '安定化を有効にする',                            body: '「安定化」をクリックしてください。バナーに保証が表示されます：出力はラウンド 2n \u2212 2 までに安定します（ここでは最大10）。安定とは値が変わらなくなることであり、アルゴリズムが停止することではありません。',                                                              action: '「安定化」ボタンをクリック',                         highlight: '.algo-group'   },
        { title: 'エージェントを選ぶ',                            body: '左パネルのエージェントを左クリックしてください。アルゴリズムはそのエージェントの Vista（部分履歴木）の上で動きます。',                                                                                                                                              action: '左パネルで任意のエージェントを左クリック',            highlight: null            },
        { title: '1ステップ進める',                               body: '\u25b6ステップ（またはSpace）を押してください。アルゴリズムは non-branching level を探し、赤辺の比を使ってリーダーから匿名性の推定を広げます。履歴木の色の変化に注目してください。',                                                                                    action: '\u25b6ステップまたはSpaceを押す',                    highlight: '#btn-step'     },
        { title: '仮の推定を見る',                                body: 'Lは緑になります（匿名性1）。対称なグループは黄から始まることが多く、これは証明済みではなく推測です。赤は誤った推定です。安定化は落ち着く前に誤ってよい—それがこのアルゴリズムの本質的なリスクです。',                                                          action: null,                                               highlight: null            },
        { title: 'ここで止めてはいけない理由',                     body: 'Vista は「もう十分」に見えても、まだ欠けているエージェントやリンクがあることがあります。それらしい n が出た瞬間に停止すると、誤答を確定してしまう恐れがあります。だから安定化は証明書なしに止めず、値が落ち着くまで走り続けます。',                                      action: null,                                               highlight: null            },
        { title: '落ち着くまで進める',                             body: '\u25b6ステップを押し続けてください。履歴木のルートが緑で 6 を示したとき、推定は n = 6 に安定しています。アルゴリズムは停止しませんが、（上界の範囲では）その答えはもう変わりません。',                                                                              action: 'ルートが緑で 6 になるまで\u25b6ステップを押し続ける', highlight: '#btn-step'     },
        { title: '安定した — でも「完了」ではない',                body: '安定化は正しいカウントに到達しましたが、「止めてよい」ことの証明はありません。上の「終了型」タブでは、証明書が揃ってから停止する、より強い保証を見られます（その代わり上界は 3n \u2212 3）。',                                                                  action: null,                                               highlight: null            },
    ],

    guidedTerm: [
        { title: '終了型：証明してから停止する',                   body: '終了型は、カウントが正しいと証明できる証拠が揃ったときだけ n を出力して停止します。安定化と違い、誤った答えを確定しません。代償として、より遅い上界（2n \u2212 2 ではなく 3n \u2212 3）が必要です。同じ6エージェントのネットワークです。',                              action: null,                                               highlight: null            },
        { title: '終了型を有効にする',                            body: '「終了型」をクリックしてください。バナーが 3n \u2212 3 の上界に更新されます（n = 6 なら最大15ラウンド）。',                                                                                                                                                          action: '「終了型」ボタンをクリック',                         highlight: '.algo-group'   },
        { title: 'エージェントを選ぶ',                            body: '任意のエージェントを左クリックしてください。アルゴリズムは、それらしい数字だけでなく、検証可能な構造を Vista の中に探します。',                                                                                                                                    action: '任意のエージェントを左クリック',                      highlight: null            },
        { title: '未確定の色に注目',                              body: '\u25b6ステップを押してください。シアンは初期の未確定な推測、オレンジは途中の絞り込みです。ノードが赤になることはありません—誤答を確定しないためです。',                                                                                                          action: '\u25b6ステップまたはSpaceを押す',                    highlight: '#btn-step'     },
        { title: '推測ではなく証明書',                            body: '終了型は証拠を段階的に積み上げます。guesser が推定の種になり、heavy node が整合性を確かめ、検証済みの isle や cut が証明書になります。そのあと初めてルートが緑になり、停止できます。',                                                                              action: null,                                               highlight: null            },
        { title: '停止するまで進める',                             body: '\u25b6ステップを押し続けてください。ルートが緑で 6 になったとき、アルゴリズムは終了しています。答えは証明済みで、これ以上変わりません。',                                                                                                                            action: 'ルートが n = 6 で緑になるまで\u25b6ステップを押す',   highlight: '#btn-step'     },
        { title: '2つの保証を比べる',                             body: '安定化：途中は誤り得る／2n \u2212 2 までに落ち着く／停止信号なし。終了型：誤答を確定しない／証明書付きで停止／上界は 3n \u2212 3。同じカウント問題でも、約束が違います。タブを切り替えて見比べてください。',                                                          action: null,                                               highlight: null            },
    ],

    commentary: {
        uniqueIdentified: (du, unique, n) => `+${du} エージェントを一意に特定  (${unique} / ${n})`,
        classesChanged:   (from, to)       => `匿名性クラス: ${from} \u2192 ${to}`,
        rootGuessChanged: (was, now, ok)   => `ルート推定値: ${was} \u2192 ${now}${ok ? '  \u2713 正解！' : ''}`,
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
<div class="gl-entry"><div class="gl-term">安定化アルゴリズム（Stabilizing Algorithm）</div><div class="gl-def">やがて正しい答えを出し、<em>それ以降は変えません</em>。ただし安定する前は誤った値を出してよいアルゴリズムです。停止信号はありません。ラウンド 2n \u2212 2 までに安定することが保証されます。<strong>安定化</strong>で選択します。</div></div>
<div class="gl-entry"><div class="gl-term">終了型アルゴリズム（Terminating Algorithm）</div><div class="gl-def">正しい答えを出してから<em>停止</em>し、正しさの証明書付きで完了を示します。安定化より強い保証で、止まった時点の答えは証明済みです。上界は 3n \u2212 3。<strong>終了型</strong>で選択します。</div></div>
<div class="gl-entry"><div class="gl-term">出次数認識（Outdegree Awareness）</div><div class="gl-def">オプション機能（<strong>O</strong>で切替）で、各エージェントが前のラウンドで<em>送信した</em>メッセージ数も把握します。この追加情報により、アルゴリズムがより速くまたはより正確な推定ができる場合があります。</div></div>
<div class="gl-entry"><div class="gl-term">Non-Branching Level（分岐なしレベル）</div><div class="gl-def">履歴木で、見えるノードがどれも子をちょうど1つだけ持つレベル。ここでは赤辺の多重度の比が匿名性の比と一致するため、安定化アルゴリズムはリーダーから推定を広げられます。</div></div>
<div class="gl-entry"><div class="gl-term">Isle / Cut（終了型）</div><div class="gl-def">Vista 内で検証された構造で、正しさの証明書として使います。終了型は guesser と heavy node で推測を積み、isle や cut がカウントを確認してから初めて確定します。</div></div>
<div id="glossary-ref">理論的背景: <a href="https://arxiv.org/abs/2404.02673" target="_blank" rel="noopener">arxiv.org/abs/2404.02673</a></div>`,
    },
};
