# μT-Kernel3.0 ARMv7-Mマイコン向け実装仕様書 <!-- omit in toc -->
## Version.02.00.00 <!-- omit in toc -->
## 2023.12.01 <!-- omit in toc -->

<div class="page"/>

# 目次 <!-- omit in toc -->
- [1. 概要](#1-概要)
  - [1.1. 目的](#11-目的)
  - [1.2. 関連ドキュメント](#12-関連ドキュメント)
  - [1.3. ソースコード構成](#13-ソースコード構成)
- [2. 基本実装仕様](#2-基本実装仕様)
  - [2.1. マイコン関連](#21-マイコン関連)
    - [2.1.1. 実行モードと保護レベル](#211-実行モードと保護レベル)
    - [2.1.2. CPUレジスタ](#212-cpuレジスタ)
    - [2.1.3. コプロセッサ対応](#213-コプロセッサ対応)
  - [2.2. メモリ関連](#22-メモリ関連)
    - [2.2.1. メモリモデル](#221-メモリモデル)
    - [2.2.4. スタック](#224-スタック)
    - [2.2.5. 動的メモリ管理](#225-動的メモリ管理)
  - [2.3. 割込みおよび例外関係](#23-割込みおよび例外関係)
    - [2.3.1. マイコンの割込みおよび例外](#231-マイコンの割込みおよび例外)
    - [2.3.2. 割込み関連定義](#232-割込み関連定義)
    - [2.3.3. ベクタテーブル](#233-ベクタテーブル)
    - [2.3.4. 割込み優先度](#234-割込み優先度)
    - [2.3.5. OS内部で使用する割込み](#235-os内部で使用する割込み)
    - [2.3.6. 多重割込み対応](#236-多重割込み対応)
    - [2.3.7. クリティカルセクション](#237-クリティカルセクション)
    - [2.3.8. μT-Kenrel/OSの割込み管理機能](#238-μt-kenrelosの割込み管理機能)
    - [2.3.9. μT-Kernel/SMの割込み管理機能](#239-μt-kernelsmの割込み管理機能)
    - [2.3.10. OS管理外割込み](#2310-os管理外割込み)
    - [2.3.11. その他の例外処理](#2311-その他の例外処理)
- [3. システムの起動](#3-システムの起動)
  - [3.1. 起動処理](#31-起動処理)
    - [3.1.1. リセットハンドラ](#311-リセットハンドラ)
    - [3.1.2. OS初期化処理](#312-os初期化処理)
- [4. システムの終了と再起動](#4-システムの終了と再起動)
  - [4.1. 終了処理と再起動処理](#41-終了処理と再起動処理)
- [5. タスク](#5-タスク)
  - [5.1. タスクの処理ルーチン](#51-タスクの処理ルーチン)
  - [5.2. タスクのコンテキスト情報](#52-タスクのコンテキスト情報)
- [6. 時間管理機能](#6-時間管理機能)
  - [6.1. システムタイマ](#61-システムタイマ)
- [7. 変更履歴](#7-変更履歴)

<div class="page"/>

# 1. 概要
## 1.1. 目的
本書はARMv7-Mコアを使用したマイコン向けのμT-Kernel3.0の実装仕様を記す。
対象は、トロンフォーラムから公開されているμT-Kernel 3.0(V3.00.07)のうち、ARMv7-Mコアを使用したマイコン向けの実装部分である。
ハードウェアに依存しない共通の実装仕様は「μT-Kernel3.0共通実装仕様書」を参照、個々のマイコンの実装仕様は各マイコン向けの実装仕様書を参照のこと。

以降、単にOSと称する場合はμT-Kenrel3.0を示し、本実装と称する場合、前述のソースコードの実装を示す。

## 1.2. 関連ドキュメント
以下に関連するドキュメントを記す。この他にARMv7-Mコアを使用した各種マイコンの実装仕様書がある。

| 分類      | 名称                                                          | 発行             |
| --------- | ------------------------------------------------------------- | ---------------- |
| OS        | μT-Kernel 3.0仕様書(Ver.3.00.01)<br>TEF020-S004-03.00.01      | トロンフォーラム |
| OS        | μT-Kernel3.0共通実装仕様書(Ver.2.00.00)<br>TEF033-W002-211115 | トロンフォーラム |
| T-Monitor | T-Monitor仕様書<br>TEF020-S002-01.00.01                       | トロンフォーラム |

## 1.3. ソースコード構成
機種依存定義sysdependディレクトリ下の本実装のディレクトリ構成を以下に示す。名称に(*)の点いたディレクトリが本実装の対象である。

```
─ sysdepend                   ハードウェア依存部
     ├ <ターゲット名1>             ターゲット名1のシステム依存部
     ├       ：
     ├ <ターゲット名n>             ターゲット名nのシステム依存部
     └ cpu                         CPU依存部
          ├ <CPU1>                      CPU1依存部	
          ├     ：
          └ <CPUn>                      CPUn依存部
          └ core                        CPUコア依存部
               ├ armv7m                     ARMv7-Mコア依存部(*)
               ├    ：
               └ <core n>                   コアn依存部
```

<div class="page"/>

# 2. 基本実装仕様
## 2.1. マイコン関連
### 2.1.1. 実行モードと保護レベル
ARMv7-Mは、プログラムの動作モードとして、スレッドモードとハンドラモードの二つのモードをもち、それぞれに特権モードまたはユーザモードの実行モードを割り当てることができる。
本実装では、マイコンの実行モードは、特権モードのみを使用する。

### 2.1.2. CPUレジスタ
ARMv7-Mは内部レジスタとして、汎用レジスタ(R0~R12)、SP(R13)、LR(R14)、PC(R15) 、xPSRを有する。

OSのAPI (tk_set_reg、 tk_get_reg) を用いて実行中のタスクのコンテキストのレジスタ値を操作できる。
APIで使用するマイコンのレジスタのセットは 以下のファイルにて定義される。

    include/tk/sysdepend/cpu/core/armv7m/cpudef.h 

(1) 汎用レジスタ
```
typedef struct t_regs {
    VW      r[13];      /* General purpose register R0-R12 */
    void    *lr;        /* Link register R14 */
} T_REGS;
```

(2) 例外時に保存されるレジスタ T_EIT
```
typedef struct t_eit {
    void    *pc;        /* Program counter R15 */
    UW      xpsr;       /* Program status register */
    UW      taskmode;   /* Task mode flag */
} T_EIT;
```

(3) 制御レジスタ T_CREGS
```
typedef struct t_cregs {
    void    *ssp;       /* System stack pointer R13_svc */
} T_CREGS;
```

OSのAPIによって操作されるのは、実際にはスタック上に退避されたレジスタの値である。よって、実行中のタスクに操作することはできない（OS仕様上、自タスクへの操作、またはタスク独立部からのAPI呼出しは禁止されている）。

taskmodeレジスタは、マイコンの物理的なレジスタを割り当てず、OS内の仮想的なレジスタとして実装する。本レジスタは、メモリへのアクセス権限（保護レベル）を保持する仮想レジスタである。ただし、本実装ではプログラムは特権モードでのみ実行されるので、常に値は0となる。

### 2.1.3. コプロセッサ対応
ARMv7-Mでは、Cortex-M4など一部のCPUコアがFPUを搭載する。
FPUを搭載したマイコンでは、コンフィギュレーションUSE_FPUをTRUEに指定すると、OSでFPU対応の機能が有効となり、FPUにコプロセッサ番号0が割り当てられる。
FPUが有効の場合、以下の機能が有効となる。

(1) FPUの初期化
OS起動時にFPUを有効化するとともに、Lazystackingを有効にする。
具体的には以下のようにFPUのレジスタが設定される。

| レジスタ | 設定値     | 意味                                                                     |
| -------- | ---------- | ------------------------------------------------------------------------ |
| CPACR    | 0x00F00000 | CP10およびCP11のアクセス許可                                             |
| FPCCR    | 0xC0000000 | ASPEN =1　自動状態保持を有効 <br> LSPEN =1　レイジーな自動状態保持を有効 |

Lazystackingにより、タスクにてFPUを使用中に例外が発生した場合、FPUレジスタ（S0～S15およびFPSCR）の退避領域がスタックに確保される。その後、例外処理中にFPUが使用されたときに実際のレジスタの値が保存される。
本実装では、タスクのディスパッチの際のFPUレジスタの保存に本機能を使用している。Lazystackingを無効にした場合の動作は保証しない。

(2) TA_FPU属性のタスク
タスク属性にTA_FPUまたはTA_COP0が指定可能となる。本属性のタスクは、ディスパッチ時にFPUレジスタの値を保存し、FPUの仕様が可能となる。TA_FPUとTA_COP0は同じ意味である。

(3) FPU関連API
FPU関連のAPI (tk_set_cop、 tk_get_cop) が使用可能となる。本APIを用いて実行中のタスクのコンテキストのFPUレジスタ値を操作できる。
FPUレジスタは以下のファイルで定義される。

    include/tk/sysdepend/cpu/core/armv7m/cpudef.h 

```
typedef struct t_copregs {
    VW  s[32];      /* FPU General purpose register S0-S31 */
    UW  fpscr;      /* Floating-point Status and Control Register */
} T_COPREGS;

```

APIによって操作されるのは、実際にはスタック上に退避されたレジスタの値である。よって、実行中のタスクに操作することはできない（OS仕様上、自タスクへの操作、またはタスク独立部からのAPI呼出しは禁止されている）。

(4) FPU使用の際の注意点
ユーザのプログラム中においてFPUを使用する場合（プログラムコード中にFPU命令が含まれる場合）は以下の点に注意する必要がある。

* タスク中にてFPUを使用する場合は、タスク属性にTA_FPU属性を指定しなければなければならない。TA_FPU属性のタスクは、ディスパッチの際にスタックにFPUレジスタの値を退避する。
TA_FPU属性以外のタスクは、ディスパッチの際にFPUレジスタの値を退避しないため、FPUレジスタの内容が破壊される可能性がある。
なお、TA_FPU属性のタスクは他の属性のタスクより、ディスパッチ時のスタック使用量は退避するFPUレジスタの分だけ増加する（タスクのスタックについては「タスクのコンテキスト情報」を参照のこと）

* 割込み発生時にFPUが使用されている場合は、マイコンの機能としてFPUレジスタ（S0～S15およびFPSCR）がスタック上に退避される。退避していないFPUレジスタ(S16～S31)が、割込みハンドラ中において使用される場合は、プログラム中で対応しなければならない。
なお、割込み発生時のスタック使用量は退避するFPUレジスタの分だけ増加する。

* 本実装ではOSのAPIは関数呼び出しである。よってAPIはタスクのコンテキスト（スタック）で実行されるため、特にFPUの使用を考慮する必要はない。なお、APIの呼び出しが例外など他の実装の場合にはFPU使用に際し考慮する必要がある場合もありうる。

以上より、FPUを使用したプログラムを作成する際は、必ずTA_FPU属性以外のタスクにおけるFPUの使用が禁止しなければならない（タスクのコード中にFPU命令があってはならない）。
ただし、GCCなどのC言語処理系において、プログラムの部分的にFPUを使用、未使用を指定することは難しい。もっとも簡単な実現方法は、すべてのタスクをTA_FPU属性とすることである。

## 2.2. メモリ関連
### 2.2.1. メモリモデル
ARMv7-Mは32bitのアドレス空間を有する。MMU(Memory Management Unit：メモリ管理ユニット)は有さず、単一の物理アドレス空間のみである。
本実装では、プログラムは一つの実行オブジェクトに静的にリンクされていることを前提とする。OSとユーザプログラム（アプリケーションなど）は静的にリンクされており、関数呼び出しが可能とする。

### 2.2.4. スタック
ARMv7-Mには、MSP(Main Stack Pointer)とPSP(Process Stack Pointer)の二種類のスタックが存在する。ただし、本実装ではMSPのみを使用し、PSPは使用しない。
本実装で使用するスタックには共通仕様に従い以下の種類がある。

(1) タスクスタック
(2) 例外スタック
(3) テンポラリスタック

なお、本実装ではOS初期化処理のみ例外スタックを使用する。初期化終了後は、割込みハンドラなどのタスク独立部もタスクスタックを使用し、例外スタックは使用しない。
よって、タスクスタックのサイズには、タスク実行中に発生した割込みによるスタックの使用サイズも加味しなくてはならない。

### 2.2.5. 動的メモリ管理
コンフィギュレーションのUSE_IMALLOCが1の場合、動的メモリ管理に使用するシステムメモリが確保される。USE_IMALLOCが0の場合、動的メモリ管理は無効となる。USE_IMALLOCの初期値は1である。
システムメモリの領域は以下のように定められる。

(1) システムメモリの開始アドレス
コンフィギュレーションCNF_SYSTEMAREA_TOPの値が0の場合、RAM上のプログラムのデータ領域(BSS領域)の最終アドレスの次のアドレスが、開始アドレスとなる。
値が0以外の場合は、その値が開始アドレスとなる。ただし、その値がRAM上のプログラムのデータ領域(BSS領域)の最終アドレス以下の場合は、BSS領域の最終アドレスの次のアドレスが開始アドレスとなる。つまり、BSS領域と重複することはない。

(2) システムメモリの終了アドレス
コンフィギュレーションCNF_SYSTEMAREA_ENDの値が0の場合、RAM上の例外スタックの開始アドレスの前のアドレスが、終了アドレスとなる。
値が0以外の場合は、その値が終了アドレスとなる。ただし、その値が例外スタックの開始アドレス以上の場合は、例外スタックの開始アドレスの前のアドレスが開始アドレスとなる。つまり、例外スタックの領域と重複することはない。

## 2.3. 割込みおよび例外関係
### 2.3.1. マイコンの割込みおよび例外
ARMv7-Mには以下の例外が存在する。なお、OS仕様上は例外、割込みをまとめて、割込みと称する。

| 例外番号 | 例外の種別                | 備考                     |
| -------- | ------------------------- | ------------------------ |
| 1        | リセット                  |                          |
| 2        | NMI(ノンマスカブル割込み) |                          |
| 3        | ハードフォールト          |                          |
| 4        | メモリ管理                |                          |
| 5        | バスフォールト            |                          |
| 6        | 用法フォールト            |                          |
| 7～10    | 予約                      |                          |
| 11       | SVC(スーパーバイザコール) | OSで使用                 |
| 12       | デバッグモニタ            |                          |
| 13       | 予約                      |                          |
| 14       | PendSV                    | OSで使用                 |
| 15       | SysTick                   | OSで使用                 |
| 16～255  | 外部割込み#0 ~ #239 (※)  | OSの割込み管理機能で管理 |

(※)ARMv7-Mの外部割込みの本数は最大240である。ただし、実際の本数はマイコンの実装により異なる。外部割込みの本数は各マイコンの実装において N_INTVEC として定義される。

### 2.3.2. 割込み関連定義
各マイコンについて、以下のハードウェア仕様に関わる割込み関連の定義を行う。

| 名称                  | 意味                       |
| --------------------- | -------------------------- |
| N_INTVEC              | 外部割込み数               |
| N_SYSVEC              | 例外数                     |
| INTPRI_BITWIDTH       | 割込み優先度の幅(ビット数) |
| INTPRI_MAX_EXTINT_PRI | 外部割込みの最高優先度     |
| INTPRI_SVC            | SVC例外の優先度            |
| INTPRI_SYSTICK        | システムタイマ例外の優先度 |
| INTPRI_PENDSV         | PendSV例外の優先度         |

本実装ではSVCには対応せず、システムコールは関数呼び出しのみである。よってSVC割込みは使用しない。SVC割込みは、将来の拡張に備えて優先度の定義のみが存在する。
各定義の具体的な値は、各マイコンの実装仕様書を参照のこと。

### 2.3.3. ベクタテーブル
ARMv7-Mは、前述の各種例外に対応する例外ハンドラのアドレスを設定したベクタテーブルを有する。リセット時のベクタテーブルはマイコン毎にROM上に定義される。具体的には各マイコンの実装仕様書を参照のこと。
OSの初期化処理において、ROM上のベクターテーブルはRAM上にコピーされ、以降はそちらが使用される。RAM上のベクタテーブルは、以下のファイルにexchdr_tblとして定義される。

     kernel/sysdepend/cpu/core/armv7m/interrupt.c

ただし、コンフィグレーションUSE_STATIC_IVTが1に設定されている場合は、ROM上のベクターテーブルが使用され続ける。USE_STATIC_IVTの初期値は0である。

### 2.3.4. 割込み優先度
ARMv7-Mは、8bit(256段階)の割込み優先度を持ち、この8bitを横取り優先度とサブ優先度に分ける。
本実装ではAIRCR(アプリケーション割り込みおよびリセット制御レジスタ)のPRIGROUPを3に設定し、横取り優先度を4ビット、サブ優先度を4ビットに設定している。
OSのAPIで設定可能な割込み優先度は、4bitの横取り優先度である。よって16段階の割込み優先度が設定可能である（優先度の数字の小さい方が優先度は高い）。
ただし、実際に有効な割込み優先度のビット幅(INTPRI_BITWIDTH)は、各マイコンのハードウェアの仕様により異なる。もし、マイコンの優先度のビット幅が4bit未満であれば、実際に設定可能な優先度は16段階より少なくなる。各マイコンの実装仕様書を参照のこと。

### 2.3.5. OS内部で使用する割込み
OSの内部で使用する割込みには、以下のようにARMv7-Mの割込みまたは例外が割り当てられる。該当する割込みまたは例外は、OS以外で使用してはならない。

| 種類                 | 割込み番号 | 割り当てられる割込み・例外 | 優先度 |
| -------------------- | ---------- | -------------------------- | ------ |
| システムタイマ割込み | 15         | SysTick                    | 最高   |
| ディスパッチ要求     | 14         | PendSV                     | 最低   |
| 強制ディスパッチ要求 | 14         | PendSV                     | 最低   |

システムタイマ割込みは最高優先度(INTPRI_MAX_EXTINT_PRI)、ディスパッチ要求は最低優先度が設定される。他の割込みは、この割込み優先度の間の優先度を使用しなければならない。
なお、具体的な割込み優先度はマイコンのハードウェアの仕様により異なる。各マイコンの実装仕様書を参照のこと。

ディスパッチ要求は、本実装ではクリティカルセクションの最後に、タスクのディスパッチが必要な場合に発行される。
また、強制ディスパッチ要求が、OSの起動時とタスクの終了時に発行される。本実装では、ディスパッチ要求と強制ディスパッチ要求は同一の割込み（PendSV）を使用する。

### 2.3.6. 多重割込み対応
ARMv7-Mの割込みコントローラNVIC（Nested Vector Intrrupt Controler）は、ハードウェアの機能として、多重割込みに対応している。割込みハンドラの実行中に、より優先度の高い割込みが発生した場合は、実行中の割込みハンドラに割り込んで優先度の高い割込みハンドラが実行される。

### 2.3.7. クリティカルセクション
OSのAPI実行中などのクリティカルセクションでは、原則としてすべての優先度の割込みはマスクされる。
本実装では、クリティカルセクションはBASEPRIレジスタに最高外部割込み優先度INTPRI_MAX_EXTINT_PRIを設定することにより実現する。
INTPRI_MAX_EXTINT_PRIは、本OSが管理する割込みの最高の割込み優先度であり、各マイコンにて定義される。各マイコンの実装仕様書を参照のこと。
クリティカルセクション中は、INTPRI_MAX_EXTINT_PRI以下の優先度の割込みはマスクされる。
PRIMASKおよびFAULTMASKレジスタは使用しない。よって、リセット、NMI、ハードフォルトはクリティカルセクション中でもマスクされない。

### 2.3.8. μT-Kenrel/OSの割込み管理機能
μT-Kernel/OSの割込み管理機能は、割込みハンドラの管理を行う。
本実装では、割込み管理機能はARMv7-Mの外部割込みを対象とし、割込みハンドラの管理を行う。その他の例外については対応しない。
割込みハンドラに関して以下のように定める。

- 割込み番号
OSの割込み管理機能が使用する割込み番号は、マイコンの外部割込みの番号と同一とする。例えば、IRQ0は割込み番号0となる。

- 割込みハンドラ属性
ARMv7-Mでは、C言語の関数による割込みハンドラの記述がハードウェアの仕様として可能となっている。そのため、TA_HLNG属性とTA_ASM属性のハンドラで大きな差異はなくなっている。

TA_HLNG属性の割込みハンドラは、割込みの発生後、OSの高級言語対応ルーチンを経由して呼び出される。
高級言語対応ルーチンは以下のファイルにknl_hll_inthdr関数として記述されている。

    kernel/sysdepend/cpu/core/armv7m/interrupt.c

高級言語対応ルーチンでは以下の処理が実行される。

(1) タスク独立部の設定
処理開始時にシステム変数knl_taskindpをインクリメントし、終了時にデクリメントする。本変数が0以上の値のとき、タスク独立部であることを示す。

(2) 割込みハンドラの実行
割込みPSR(IPSR)レジスタの値を参照し、テーブルknl_hll_inthdrに登録されている割込みハンドラを実行する。

TA_ASM属性の割込みハンドラは、マイコンの割込みベクタテーブルに直接登録される。よって、割込み発生時には、OSの処理を介さずに直接ハンドラが実行される。よって、TA_ASM属性の割込みハンドラからは、原則としてAPIなどのOS機能の使用が禁止される。ただし、前述のOS割込み処理ルーチンと同様の処理を行うことにより、OS機能の使用が可能となる。

- デフォルトハンドラ
割込みハンドラが未登録の状態で、割込みが発生した場合はデフォルトハンドラが実行される。デフォルトハンドラは、以下のファイルにDefault_Handler関数として実装されている。

     kernel/sysdepend/cpu/core/armv7m/exc_hdr.c

デフォルトハンドラは、プロファイルUSE_EXCEPTION_DBG_MSGを1に設定することにより、デバッグ用の情報を出力する。デバッグ情報は、T-Monitor互換ライブラリのコンソール出力に出力される。初期設定は1(有効)である。

必要に応じてユーザがデフォルトハンドラを記述することにより、未定義割込み発生時の処理を行うことができる。デフォルトハンドラはweak宣言がされているので、ユーザが同名の関数を作成しリンクすることにより、上書きすることができる。

### 2.3.9. μT-Kernel/SMの割込み管理機能
μT-Kernel/SMの割込み管理機能は、CPUの割込み管理機能および割込みコントローラ(NVIC)の制御を行う。
本機能の各APIの実装について以下に記す。

- CPU割込み制御
CPU割込み制御は、マイコンのBASEPRIレジスタのみを制御して実現する。PRIMASKおよびFAULTMASKレジスタは使用しない。

① CPU割込みの禁止（DI）
CPU割込みの禁止(DI)は、BASEPRIレジスタに外部割込みの最高優先度INTPRI_MAX_EXTINT_PRIを設定し、割込みを禁止する。DIはdisint関数を呼ぶ出すマクロである。

② CPU割込みの許可（EI）
割込みの許可(EI)は、BASEPRIレジスタの値をDI実行前に戻す。EIはset_basepri関数を呼び出すマクロである。

③ CPU内割込みマスクレベルの設定(SetCpuIntLevel)
BASEPRIレジスタに指定した割込みマスクレベル+1の値を設定する。よって、指定したマスクレベルより低い優先度の割込みはマスクされる。
0を指定した場合はすべての割込みがマスクされる。また、最低優先度を指定した場合はすべての割込みが許可される。
μT-Kernel3.0仕様の基づき、INTLEVEL_DIを指定した場合はすべての割込みがマスクされる。また、INTLEVEL_EIを指定した場合はすべての割込みが許可される。

④ CPU内割込みマスクレベルの参照(GetCpuIntLevel)
BASEPRIレジスタの設定値を参照し、設定値-1の値を返す。この値は割込みマスクレベルである。
なお、すべての優先度の割込みが許可されている場合は、INTLEVEL_EIの値を返すものとする。

上記のAPIおよび関数は以下のファイルに記述される。

    lib/libtk/sysdepend/cpu/core/armv7m/int_armv7m.c


- 割込みコントローラ制御
ARMv7-Mの標準割込みコントローラNVICの制御を行う。なお、各マイコンはNVICの他にも割込み制御機能を有する場合がある。よって、割込みコントローラ制御については、各マイコンの実装仕様書を参照のこと。
以下にはNVICの制御についての共通事項のみを記す。

① 割込みコントローラの割込み許可(EnableInt)
割込みコントローラ(NVIC)の割込みイネーブルセットレジスタ(ISER)を設定し、指定された割込みを許可する。同時に割込み優先度レジスタ(IPR)に指定された割込み優先度を設定する。

② 割込みコントローラの割込み禁止(DisableInt)
割込みコントローラ(NVIC)の割込みイネーブルクリアレジスタ(ICER)を設定し、指定された割込みを禁止する。

③ 割込み発生のクリア(ClearInt)
割込みコントローラ(NVIC)の割込み保留クリアレジスタ(ICPR)を設定し、指定された割込みが保留されていればクリアする。

④ 割込みコントローラのEOI発行(EndOfInt)
ARMv7-MではEOIの発行は不要である。よって、EOI発行(EndOfInt)は何も実行しないマクロとして定義される。

⑤ 割込み発生の検査(CheckInt)
割込み発生の検査(CheckInt)は、割込みコントローラ(NVIC)の割込み保留クリアレジスタ(ICPR)を参照することにより実現する。

⑥ 割込みモード設定(SetIntMode)
割込みコントローラ(NVIC)に本機能はないため、未実装である。

⑦ 割込みコントローラの割込みマスクレベル設定(SetCtrlIntLevel)
割込みコントローラ(NVIC)に本機能はないため、未実装である。

⑧ 割込みコントローラの割込みマスクレベル取得(GetCtrlIntLevel)
割込みコントローラ(NVIC)に本機能はないため、未実装である。

NVICの制御は以下のファイルに記述される。

    lib/libtk/sysdepend/cpu/core/armv7m/int_armv7m.c
    lib/libtk/sysdepend/cpu/core/armv7m/int_armv7m.h

### 2.3.10. OS管理外割込み
最高外部割込み優先度INTPRI_MAX_EXTINT_PRIより優先度の高い外部割込みは、OS管理外割込みとなる。
管理外割込みはOS自体の動作よりも優先して実行される。よって、OSから制御することはできない。また、管理外割込みの中でOSのAPIなどの機能を使用することも原則としてできない。
本実装ではINTPRI_MAX_EXTINT_PRIは優先度1と定義されている。よって、優先度0以上の例外が管理外割込みとなる。マイコンのデフォルトの設定では、リセット、NMI、ハードフォルト、メモリ管理の例外がこれに該当する。

### 2.3.11. その他の例外処理
本実装では、OSは外部割込み以外の例外を管理しない。
これらの例外には、暫定的な例外ハンドラを定義している。暫定的な例外ハンドラは、kernel/sysdepend/cpu/core/armv7m/exc_hdr.cの以下の関数として実装されている。

| 例外番号 | 例外の種別                | 関数名             |
| -------- | ------------------------- | ------------------ |
| 2        | NMI(ノンマスカブル割込み) | NMI_Handler        |
| 3        | ハードフォルト            | HardFault_Handler  |
| 4        | メモリ管理                | MemManage_Handler  |
| 5        | バスフォールト            | BusFault_Handler   |
| 6        | 用法フォールト            | UsageFault_Handler |
| 11       | SVC(スーパーバイザコール) | Svcall_Handler     |
| 12       | デバッグモニタ            | DebugMon_Handler   |

暫定的の例外ハンドラは、プロファイルUSE_EXCEPTION_DBG_MSGを1に設定にすることにより、デバッグ用の情報を出力する。初期設定は1(有効)である。

必要に応じてユーザが例外ハンドラを記述することにより、各例外に応じた処理を行うことができる。暫定的の例外ハンドラはweak宣言がされているので、ユーザが同名の関数を作成しリンクすることにより、上書きすることができる。

各例外ハンドラはベクタテーブルに登録されているので、例外発生時にOSを介さず直接実行される。例外の優先度が、優先度0以上の場合、その例外はOS管理外例外となる。それ以外の優先度の場合は、ASM属性の割込みハンドラと同等である。

<div class="page"/>

# 3. システムの起動
## 3.1. 起動処理
### 3.1.1. リセットハンドラ
電源投入またはリセットなどにより、リセットハンドラが実行される。
リセットハンドラは、以下のファイルにReset_Handler関数として実装される。

     kernel/sysdepend/cpu/core/armv7m/reset_hdl.c

Reset_Handler関数の処理手順を以下に示す。

(1) ハードウェア初期化
knl_startup_hw関数を呼出し、リセット時の最小限のハードウェアの初期化を行う。
knl_startup_hwは対象システムごとに実装される。処理内容は各マイコン向けの実装仕様書を参照のこと。

(2) 例外ベクタテーブルの作成
共通実装仕様書を参照のこと。

(3) 変数領域(data, bss)の初期化
共通実装仕様書を参照のこと。

(4) システムメモリ領域の確保
共通実装仕様書を参照のこと。

(5) 割込みの基本設定
割込みコントローラNVICの基本設定を行う。また、OSが使用する例外の優先度を設定する。

(6) FPUの有効化
コンフィギュレーションUSE_FPUがFPU使用(1)に設定され、かつ、マイコンがFPUを使用できる場合、FPUを有効化する。

(7) OS初期化処理(main)の実行
リセット処理を終了するOSの初期化処理(main)を実行し、リセット処理は終了する。

### 3.1.2. OS初期化処理
OS初期化処理は共通部のmain関数で実行される。共通実装仕様書を参照のこと。
main関数から以下のARMv7-Mコアに依存する処理が実行される。

(1) 割込み初期化 (knl_init_interrupt)
OSが使用する割込みの初期化を行う。
以下のファイルにknl_init_interrupt関数として定義される。ただし、本実装では何も処理を行っていない。ユーザは必要に応じて処理を記述する。

    kernel/sysdepend/cpu/core/armv7m/interrupt.c

(2) システムタイマ初期化 (knl_start_hw_timer)
システムタイマのハードウェアに依存した初期化を行う。以下のファイルにknl_start_hw_timerインライン関数として定義される。

     kernel/sysdepend/cpu/core/armv7m/sys_timer.h

本インライン関数では、SysTickタイマをコンフィギュレーションCNF_TIMER_PERIODで指定したシステムタイマの割込み周期で割込みが発生するように設定を行っている。

<div class="page"/>

# 4. システムの終了と再起動
## 4.1. 終了処理と再起動処理
終了処理と再起動処理は共通部のshutdown_system関数で実行される。shutdown_system関数からハードウェアに依存する処理が呼び出される。
対象ハードウェアに依存する処理のうち、ARMv7-Mコアに共通の処理を記す。各マイコンに依存する処理は、各マイコン向けの実装仕様書を参照のこと。

(1) システムタイマ終了処理 (knl_terminate_hw_timer)
システムタイマのハードウェアに依存した終了化を行う。以下のファイルにknl_terminate_hw_timerインライン関数として定義される。

     kernel/sysdepend/cpu/core/armv7m/sys_timer.h

本インライン関数では、SysTickタイマを停止し、タイマ割込みも発生しないようにする。

<div class="page"/>

# 5. タスク
ARMv7-Mに依存するタスクの仕様を以下に記す。各マイコン向け実装仕様書も参照のこと。

## 5.1. タスクの処理ルーチン
タスクの処理ルーチンの実行開始時の各レジスタの状態は以下である。これ以外のレジスタの値は不定である。

| レジスタ | 値                           | 補足             |
| -------- | ---------------------------- | ---------------- |
| PRIMASK  | 0                            | 割込み許可       |
| R0       | 第一引数 stacd               | タスク起動コード |
| R1       | 第二引数 *exinf              | タスク拡張情報   |
| R13(MSP) | タスクスタックの先頭アドレス |                  |

## 5.2. タスクのコンテキスト情報
スタック上に保存されるタスクのコンテキスト情報を以下に示す。

(1) FPUレジスタを保存しない場合
xPSRからR0までのレジスタはディスパッチ時の例外により保存される。R4からLR(EXC_RETUN値)はディスパッチャにより保存される。
```
High Address +---------------+
             | xPSR          |
             | PC(R15)       | Return address
             | LR(R14)       |
             | R12           |
             | R0-R3         |
             +---------------+ Save by Exception entry process.
             | R4 - R11      |
      ssp -> | LR(EXC_RETURN)|
 Low Address +---------------+
```

(2) FPUレジスタを保存する場合
前述のFPUレジスタを保存しない場合に加えて、FPUのレジスタの値が保存される。
FPSRCおよびS0～S15までのレジスタの値はディスパッチ時の例外により保存される。S16～S32のレジスタの値およびufpuはディスパッチャにより保存される。
ufpuはEXC_RETURNのbit4以外をマスクした値であり、タスクのコンテキストでのFPU命令の実行を示す（CONTROLレジスタのFPCAビットの反転）。
```
High Address +---------------+
             | (FPSCR)       |
             | (S0 - S15)    |
             +---------------+
             | xPSR          |
             | PC(R15)       | Return address
             | LR(R14)       |
             | R12           |
             | R0-R3         |
             +---------------+ Save by Exception entry process.
             | R4 - R11      |
      ssp -> | LR(EXC_RETURN)|
             +---------------+
             | (S16 - S31)   |
      ssp -> | (ufpu)        |
 Low Address +---------------+
 ```

<div class="page"/>

# 6. 時間管理機能
時間管理機能のARMv7-Mコアに依存する仕様を以下に記す。各マイコン向け実装仕様書も参照のこと。

## 6.1. システムタイマ
本実装では、マイコン内蔵のSysTickタイマをシステムタイマとして使用する。
システムタイマのティック時間は、1ミリ秒から50ミリ秒の間で設定できる。
ティック時間の標準の設定値は10ミリ秒である。

<div class="page"/>

# 7. 変更履歴
| 版数    | 日付       | 内容                                                    |
| ------- | ---------- | ------------------------------------------------------- |
| 2.00.00 | 2023.12.01 | 新規作成<br>版数は他の仕様書にあわせて2.00.00からとする |
