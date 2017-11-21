# libmimixfe
mimi XFE module for Fairy I/O Tumbler

## はじめに

### 概要

T-01 用バイナリとして提供される libmimixfe.so とそのヘッダファイル、及び簡単な利用例を含むリポジトリです。 `include/` 以下にヘッダファイル、`lib/` 以下に libmimixfe.so 本体、`examples` 以下に利用例のソースコードが含まれます。利用例は、説明のための実装であり、プロダクション用の実装には好適ではない場合があります。

### 依存ライブラリ

LED リングの制御のために libtumbler.so が必要です（ https://github.com/FairyDevicesRD/tumbler )


### 利用例のビルド

T-01 実機上の Makefile が `examples/` 直下に用意されています。

``````````{.sh}
$ cd examples
$ make
``````````

適宜同梱の lixmimife.so 及び上記 libtumbler.so とリンクして実行してください。いくつかの利用例は `sudo` を必要とします。

## チュートリアル

未公開

