# サンプルプログラム

## ex1.cpp

### 概要

このサンプルプログラムは、事前に設定した 1 つの固定方向（目的方向）から到来する人間の声のみを抽出するサンプルプログラムです。抽出された音声は、ファイルに保存されます。

### 解説

#### XFE の準備

``````````.cpp
using namespace mimixfe;
XFESourceConfig s;
XFEECConfig e;
XFEVADConfig v;
XFEBeamformerConfig b;
XFEStaticLocalizerConfig c({Direction(270, 90)});
``````````

libmimixfe のネームスペースは mimixfe となります。まず最初に、libmimixfe を利用するための各種の設定クラスを構築します。設定内容の詳細は、libmimixfe の API ドキュメントを御覧ください。
このサンプルでは、多くの設定クラスをデフォルトで構築し、パラメータを調整していませんが、`XFEStaticLocalizerConfig` についてのみ、構築時にパラメータを与えています。
`XFEStaticLocalizerConfig` は音源定位器に与えられる設定クラスのひとつであり、固定方向の音源のみ抽出することを指示する設定クラスです。コンストラクタ引数で、`Direction(270,90)` を与えています。これは、当該方向の音源のみを抽出することを示しています。

ここで `Direction(X,Y)` とは、方位角 `X` 度、迎え角 `Y` 度で決定される 3 次元的な方向を示します。 Tumbler に対しての座標系の詳細は [マイク位置情報] (https://github.com/FairyDevicesRD/tumbler/tree/master/hardware_api/microphone/microphone_positions) を参照してください。このサンプルで利用されている、`Direction(270,90)` は、方位角 270 度、迎え角 90 度の方向であり、これは Tumbler の正面かつ水平面上を示します。水平面上とは、マイク設定高さに対する水平面であり、これは概ね Tumbler の高さ 3/4 程度の水平面となります。

### XFE の実行
