# nEdit - Win32用エディタエンジンライブラリ
nEdit は、C言語で実装された、軽量で高速なWindows（Win32 API）用テキストエディタエンジン（カスタムコントロール）です。
外部ライブラリへの依存が一切なく、Win32の標準的なウィンドウメッセージングモデルに統合できるように設計されています。

## 特徴
- Win32 APIのみで記述されており、ランタイムや外部DLLは不要です。
- メモリ消費が極めて少なく、大きなファイルも高速に処理します。
- スタティックライブラリとしてリンクするか、ソースコードをプロジェクトに直接含めることで、独自のWindowsアプリケーションにエディタ機能を組み込めます。

## 1. 組み込み方法（プロジェクトへの導入）
nEditを自作のWin32アプリケーションに組み込む最も簡単な手順は、ソースコードをプロジェクトに直接追加する方法です。

ステップ 1: ファイルの追加
リポジトリの以下の主要ファイルを、あなたのプロジェクトのソースツリーにコピーし、ビルド対象に含めてください。

- `nedit.h` (公開ヘッダー)
- `nedit.c` (メイン実装)
- (その他、内部で依存している `.c` / `.h` ファイル一式)

### ステップ 2: ライブラリの初期化
アプリケーションの起動時（`WinMain` 内など、ウィンドウを作成する前）に、nEditのウィンドウクラスを登録する必要があります。

```c
#include "nedit.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // nEditコントロールクラスの初期化と登録
    if (!nEdit_RegisterControl(hInstance)) {
        MessageBox(NULL, TEXT("nEditの初期化に失敗しました。"), TEXT("エラー"), MB_ICONERROR);
        return 0;
    }

    // ... 通常のウィンドウクラス登録やメッセージループ ...
}
```

### ステップ 3: エディタウィンドウの作成
親ウィンドウの `WM_CREATE` などのタイミングで、`CreateWindowEx` を使用して `NEDIT_CLASS_NAME`（"nEdit"）クラスのウィンドウを作成します。

```c
HWND hwndEditor;

case WM_CREATE:
    hwndEditor = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        NEDIT_CLASS_NAME,           // クラス名 ("nEdit")
        NULL,                       // ウィンドウタイトル（エディタでは通常NULL）
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE, // スタイル
        0, 0, 100, 100,             // 位置とサイズ（WM_SIZEで調整）
        hwnd,                       // 親ウィンドウのハンドル
        (HMENU)IDC_MY_EDITOR,       // コントロールID
        hInstance,
        NULL
    );
    break;

case WM_SIZE:
    // 親ウィンドウのサイズに合わせてエディタをリサイズ
    MoveWindow(hwndEditor, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
    break;
```

## 2. APIリファレンス
nEditの制御は、主に ウィンドウメッセージ（`SendMessage`） または 専用のC言語関数API を通じて行います。

### 2.1 初期化・管理関数
`BOOL nEdit_RegisterControl(HINSTANCE hInst)`
nEditのカスタムウィンドウクラスをシステムに登録します。

- 引数: `hInst` - アプリケーションのインスタンスハンドル。
- 戻り値: 登録に成功した場合は `TRUE`、失敗した場合は `FALSE`。

`void nEdit_UnregisterControl(HINSTANCE hInst)`
登録されたnEditのウィンドウクラスを解除します。アプリケーション終了時に呼び出します。

### 2.2 ウィンドウメッセージAPI
エディタの操作は、標準の `SendMessage` 関数を使用して、nEditのウィンドウハンドル（`hwndEditor`）に対してメッセージを送信することで行います。

テキストの設定と取得
- `WM_SETTEXT`
    - 説明: エディタにテキストを設定します。
    - lParam: 設定する文字列（Unicode/`WCHAR*`）へのポインタ。

- `WM_GETTEXT`
    - 説明: エディタからテキストを取得します。
    - wParam: バッファの最大文字数。
    - lParam: 文字列を受け取るバッファへのポインタ。

編集操作
- `WM_CUT` / `WM_COPY` / `WM_PASTE` / `WM_CLEAR`
    - 説明: クリップボードを用いた「切り取り」「コピー」「貼り付け」「削除」を実行します。
- `EM_UNDO` / `EM_REDO`
    - 説明: 直前の操作を取り消す（Undo）、またはやり直す（Redo）。

選択範囲とカーソル
- `EM_SETSEL`
    - 説明: テキストの選択範囲を設定します。
    - wParam: 選択開始位置（文字インデックス）。
    - lParam: 選択終了位置（文字インデックス）。
- `EM_GETSEL`
    - 説明: 現在の選択範囲の開始位置と終了位置を取得します。
    - wParam: 開始位置を受け取るポインタ（`DWORD*`）。
    - lParam: 終了位置を受け取るポインタ（`DWORD*`）。

### 2.3 通知メッセージ（親ウィンドウへの通知）
エディタ内で状態が変化すると、親ウィンドウに対して `WM_COMMAND` メッセージ（通知コードは `HIWORD(wParam)` に格納）が送られます。

- `EN_CHANGE`
    - 発生タイミング: テキストの内容がユーザーによって変更されたとき。
- `EN_SELCHANGE`
    - 発生タイミング: カーソル位置が移動した、または選択範囲が変更されたとき。

## 3. 最小構成のサンプルコード
以下は、nEditを組み込んだ最小限のテキストエディタウィンドウを表示する完全なサンプル（概要）です。

```c
#include <windows.h>
#include "nedit.h"

#define IDC_EDITOR 101
HWND g_hwndEditor;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            // エディタの作成
            g_hwndEditor = CreateWindowEx(WS_EX_CLIENTEDGE, NEDIT_CLASS_NAME, NULL,
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE,
                0, 0, 0, 0, hwnd, (HMENU)IDC_EDITOR, GetModuleHandle(NULL), NULL);
            
            // 初期テキストのセット
            SendMessage(g_hwndEditor, WM_SETTEXT, 0, (LPARAM)L"Welcome to nEdit!");
            return 0;

        case WM_SIZE:
            // 親ウィンドウのリサイズに追従
            MoveWindow(g_hwndEditor, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
            return 0;

        case WM_COMMAND:
            // エディタからの通知をキャッチ
            if (LOWORD(wParam) == IDC_EDITOR) {
                if (HIWORD(wParam) == EN_CHANGE) {
                    // テキストが変更された際の処理
                }
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show) {
    nEdit_RegisterControl(hInst);

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = TEXT("MainWin");
    RegisterClass(&wc);

    HWND hwnd = CreateWindow(TEXT("MainWin"), TEXT("nEdit Sample"), WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, NULL, NULL, hInst, NULL);
    ShowWindow(hwnd, show);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    nEdit_UnregisterControl(hInst);
    return (int)msg.wParam;
}
```

## ライセンス
このプロジェクトは MIT ライセンス のもとで公開されています。商用・非商用問わず、自由に変更および再配布が可能です。詳細は `LICENSE` ファイルを参照してください。
