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

ここで `Direction(X,Y)` とは、方位角 `X` 度、迎え角 `Y` 度で決定される 3 次元的な方向を示します。 Tumbler に対しての座標系の詳細は [マイク位置情報](https://github.com/FairyDevicesRD/tumbler/tree/master/hardware_api/microphone/microphone_positions) を参照してください。このサンプルで利用されている、`Direction(270,90)` は、方位角 270 度、迎え角 90 度の方向であり、これは Tumbler の正面かつ水平面上を示します。水平面上とは、マイク設定高さに対する水平面であり、これは概ね Tumbler の高さ 3/4 程度の水平面となります。

#### XFE の実行

``````````.cpp
XFERecorder rec(s,e,v,b,c,recorderCallback,reinterpret_cast<void*>(&data));
rec.setLogLevel(LOG_UPTO(LOG_DEBUG)); // デバッグレベルのログから出力する
rec.start();
while(rec.isActive()){
  std::cout << countup++  << " / " << timeout << std::endl;
	sleep(1);
	if(countup == timeout){
	  break;
	}
}
rec.stop();
``````````

先に準備した設定クラスを引数に与えて `XFERecorder` クラスを構築します。このとき、コールバック関数と、ユーザー定義データを合わせて引数に与えています。コールバック関数は、`libmimixfe` によって、音声が取得される度に呼び出されます。ユーザー定義データは、コールバック関数に与えられます。コールバック関数はメインスレッドとは異なる別スレッドで実行されることに留意してください。

次の行では、`setLogLevel()` 関数によって、`libmimixfe` が出力するログレベルを設定しています。ログは `/var/log/messages` 等のシステム標準ログに出力されます。`LOG_UPTO` マクロによって、簡単にログレベルを設定することができます。ログレベルを設定しない場合、`INFO`レベルからログ出力されます。

次の行では `start()` 関数によって、録音を開始すると同時に、設定に従って信号処理を開始します。

次の行では、`while(rec.isActive()){...}` というループに入ります。`isActive()` 関数は `libmimixfe` が正しく録音及び信号処理を実行している間は `true` が戻されます。ビジーループを防止するために、`sleep()` 関数等で適宜メインスレッドの実行を停止していますが、これは実際の利用方法に応じて適切に行なってください。メインスレッドは、このような形で終了しないようにします。このサンプルプログラムでは、簡易的に 30 秒で録音を終了するようにされています。

次の行では、`stop()` 関数を呼び出して、`libmimixfe` の動作を終了しています。

#### ユーザー定義コールバック関数の実装

``````````.cpp
void recorderCallback(
		short* buffer,
		size_t buflen,
		mimixfe::SpeechState state,
		int sourceId,
		mimixfe::StreamInfo* info,
		size_t infolen,
		void* userdata)
{
	std::string s = "";
	if(state == mimixfe::SpeechState::SpeechStart){
		s = "Speech Start";
	}else if(state == mimixfe::SpeechState::InSpeech){
		s = "In Speech";
	}else if(state == mimixfe::SpeechState::SpeechEnd){
		s = "End of Speech";
	}
  UserData *p = reinterpret_cast<UserData*>(userdata);
  fwrite(buffer, sizeof(short), buflen, p->file_);  // 取得した信号処理済音声をファイルに記録する・・・(1)

  // 画面表示で確認
  std::cout << "State: " << s << std::endl;
  for(size_t i=0;i<infolen;++i){
	  std::cout << info[i].milliseconds_ << "[ms] " << info[i].rms_ << "[rms] " << static_cast<int>(info[i].speechProbability_) << "[%] ";
	  std::cout << sourceId << " (" << info[i].extractedSoundSources_ << "/" << info[i].estimatedSoundSources_ << ")";
	  std::cout << " angle=" << info[i].direction_.angle_ << ", azimuth=" << info[i].direction_.azimuth_ << std::endl;
  }
}
``````````

この例では、上記(1)で示す行で、コールバック関数が受け取った libmimixfe の処理済音声を、ファイルに記録するだけの実装となっています。上記(1)以外のコードは、全てコールバック関数に与えられる引数の機能をデモンストレーションするためのコードであって不要なコードです。

実際の利用において、開発段階を除き、Tumbler 内でこのようにファイルに書き落とすということはないと想定されます。Tumbler のストレージ容量がすぐに一杯になってしまうだろうということと、処理済音声を mimi クラウドに送信した場合、クラウド側で音声ログとして一括管理することができるためです。このサンプルプログラムでは、録音音声をその場で確認することができるように、音声ファイルとしてローカルストレージに書き落とすという例を示していますので、御留意ください。



