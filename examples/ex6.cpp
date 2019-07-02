/*
 * @file ex6.cpp
 * @brief ex3.cpp 動的方向単一音源抽出サンプルを一部変更し、libmimixfe 内での LED 制御を行わなず、ユーザー側で LED 制御を行う例。音源方向にアニメーション点灯させる。
 * @author Copyright (C) 2017 Fairy Devices Inc. http://www.fairydevices.jp/
 * @author Masato Fujino, created on: 2017/07/14
 */

#include <unistd.h>
#include <syslog.h>
#include <sched.h>
#include <signal.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <iomanip>
#include <numeric>
#include <memory>

#include "XFERecorder.h"
#include "XFETypedef.h"

#include "utils.h"

volatile sig_atomic_t xfe_flag_ = 0;
void xfe_sig_handler_(int signum){ xfe_flag_ = 1; }


/**
 * @class AzimuthFrame
 * @brief Tumbler 座標系における方位角（向かって右が 0 度、正面が 270 度）を指定した時、指定方位の LED を点灯させる機能を持つ。libtumbler が提供している Frame クラスを継承している。
 * @details LED は 18 個しかないため、ちょうど 18 で割り切れる方位以外は、2 つの LED の明度のバランスを取ることで表現している
 */
class AzimuthFrame : public tumbler::Frame
{

public:
	/**
	 * @brief コンストラクタ
	 * @param [in] 点灯させたい方向の方位角[0,360]
	 * @param [in] foreground 点灯させたい方向の LED 色
	 * @param [in] background その他の LED 色
	 */
	AzimuthFrame(
			const std::vector<int>& azimuths,
			const tumbler::LED& foreground,
			const tumbler::LED& background) :
		tumbler::Frame(background),
		azimuths_(azimuths.begin(), azimuths.end()),
		foreground_(foreground),
		background_(background)
	{
		// LED は 18 個あり、20 度ごとの角度で配置されている
		int node0Idx, node1Idx; // 反時計回りの始点と終点
		float node0, node1;     // 同明度
		for(size_t i=0;i<azimuths_.size();++i){
			// 値域対応
			int caz = azimuths_[i];
			if(360 <= caz){
				caz = caz - 360;
			}else if(caz < 0){
				caz = 360 + caz;
			}

			// 角度からセグメントを計算
			if(caz < 10 || 350 <= caz){
				// 端点処理
				node0Idx = 17;
				node1Idx = 0;
			}else{ // 10 -> 349
				// [10,29]->[20,39]=>1
				// [30,49]->[40,59]=>2
				// [330,349]->[340,359]=>17
				int segment = (caz+10) / 20;
				node0Idx = segment-1;
				node1Idx = segment;
			}

			// 明度バランス
			// 角度 = 0  -> ida = 10
			// 角度 = 5  -> ida = 15
			// 角度 = 9  -> ida = 19
			// 角度 = 10 -> ida =  0
			// 角度 = 15 -> ida =  5
			// 角度 = 17 -> ida =  7
			// 角度 = 20 -> ida = 10
			// ....
			int ida =  (caz+10) % 20;
			CalcBrightnessPair(ida, node0, node1);
			setLED(V2PMap(node0Idx), tumbler::LED(foreground.r_*node0, foreground.g_*node0, foreground.b_*node0));
			setLED(V2PMap(node1Idx), tumbler::LED(foreground.r_*node1, foreground.g_*node1, foreground.b_*node1));
		}
	}

private:

	/**
	 * @brief 所与の内分角に対して 2 つの LED ペアの明るさバランスを計算する
	 * @param [in] degree 内分角[0,19](度)
	 * @param [out] node0 片側の明度（反時計回りの始点側）
	 * @param [out] node1 片側の明度（反時計回りの終点側）
	 */
	void CalcBrightnessPair(int degree, float& node0, float& node1)
	{
		if(degree < 0 || 19 < degree){ // [0,19]度
			throw std::runtime_error("CalcBrightnessPair(), degree out of range.");
		}
		node0 = 1.0 - degree*0.05;
		node1 = degree*0.05;
	}

	/**
	 * @brief 基準座標系における仮想LEDIDと物理LED番号の対応
	 * @param [in] VID 仮想LEDID
	 * @return 物理LEDID
	 */
	int V2PMap(int VID)
	{
		switch (VID){
		case 0: return 4;
		case 1: return 3;
		case 2: return 2;
		case 3: return 1;
		case 4: return 0;
		case 5: return 17;
		case 6: return 16;
		case 7: return 15;
		case 8: return 14;
		case 9: return 13;
		case 10: return 12;
		case 11: return 11;
		case 12: return 10;
		case 13: return 9;
		case 14: return 8;
		case 15: return 7;
		case 16: return 6;
		case 17: return 5;
		default: return -1;
		}
	}

	std::vector<int> azimuths_;
	const tumbler::LED foreground_;
	const tumbler::LED background_;
};

/**
 * @brief 角度計算
 * @param [in] base 基準となる角度
 * @param [in] diff 基準となる角度からの差
 * @return 基準となる角度に対して、第二引数で指定した差を持つ角度
 */
int calcDiffDegree(int base, int diff)
{
	int d = base-diff;
	if(d < 0){
		return d+360;
	}else if(360 < d){
		return d-360;
	}else{
		return d;
	}
}

/**
 * @brief アニメーション定義の実装
 * @param [in] degree LED を点灯させたい方位角
 * @return アニメーションのためのフレーム群
 * @note AzimuthFrame クラスは単一のフレームを返す。libmimixfe では AzimuthFrame クラスの直接利用による単一フレームを用いているが
 * 本サンプルプログラムでは、それらを複数用いてアニメーションの例を定義してみた。
 */
std::vector<tumbler::Frame> myAnimation(int degree)
{
	tumbler::LED foregroundColor(0,0,255);
	AzimuthFrame af2({calcDiffDegree(degree,70), calcDiffDegree(degree,-70)}, foregroundColor, tumbler::LED(40,40,40));
	AzimuthFrame af3({calcDiffDegree(degree,50), calcDiffDegree(degree,-50)}, foregroundColor, tumbler::LED(30,30,30));
	AzimuthFrame af4({calcDiffDegree(degree,20), calcDiffDegree(degree,-20)}, foregroundColor, tumbler::LED(20,20,20));
	AzimuthFrame af6({degree}, foregroundColor, tumbler::LED(10,10,10));
	return {af2,af3,af4,af6};
}

/**
 * @brief デフォルトパターンを返す関数、典型的に利用される関数です。
 * @details このパターンは Arduino Sketch にも内蔵されており、電源オフからオンへの切り替わり時点で OS が起動する前の時点で点灯されるパターン。reset 関数により消灯されるため、
 * デフォルトパターンでの再点灯は、内部制御点灯により libtumbler 側から改めて命令する必要があります。内蔵されたパターンの定義は、以下にあります。
 * @see https://github.com/FairyDevicesRD/tumbler/blob/be436908177daf22df32427c245034223300d102/arduino/sketch/LEDRing.h#L198
 * @return フレーム
 */
tumbler::Frame defaultPattern(){
	tumbler::Frame f;
	int r = 255;
	int g = 255;
	int b = 255;
	float a = 0.4;
	float c = 0.6;
	f.setLED(4, tumbler::LED(r, g, b));
	f.setLED(3, tumbler::LED(r, g, b));
    f.setLED(2, tumbler::LED(r*a*a,g*a*a,b*c));
    f.setLED(1, tumbler::LED(r*a*a*a,g*a*a*a,b*c*c));
    f.setLED(0, tumbler::LED(r*a*a*a*a,g*a*a*a*a,b*c*c*c));
    return f;
}

/**
 * @class AudioSource
 * @brief 音源のデータクラス
 */
class AudioSource
{
public:
	using Ptr = std::unique_ptr<AudioSource>;
	AudioSource(int azimuth) : azimuth_(azimuth)
	{
		std::stringstream filename;
		filename << "/tmp/ex3_";
		filename << azimuth_ << ".raw";
		file_ = fopen(filename.str().c_str(), "w");
	}
	~AudioSource(){ fclose(file_); }
	void addAudioData(short* audioData, size_t audioDataLen) { fwrite(audioData, sizeof(short), audioDataLen, file_); }

	FILE* file_;  //!< 音声ファイル
	int azimuth_; //!< 代表音源方向
};

class UserData
{
public:
	std::vector<AudioSource::Ptr> sources_;
	int currentId_;
};

void recorderCallback(
		short* buffer,
		size_t buflen,
		mimixfe::SpeechState state,
		int sourceId,
		mimixfe::StreamInfo* info,
		size_t infolen,
		void* userdata)
{
	UserData *p = reinterpret_cast<UserData*>(userdata);
	std::string s = "";
	tumbler::LEDRing& ring = tumbler::LEDRing::getInstance();
	if(state == mimixfe::SpeechState::SpeechStart){
		s = "Speech Start";
		// 1. 発話検出時の推定音源方向を取得する
		std::vector<int> azms;
		for(size_t i=0;i<infolen;++i){
			if(info[i].direction_.azimuth_ != -1){
				azms.push_back(info[i].utteranceDirection_.azimuth_);
			}
		}
		int azimuth_mean = meanDegree(azms);
		std::cout << "Speech Start: " << azimuth_mean << " degree" << std::endl;

		// 2. 推定音源方向に LED リングをアニメーション点灯させる
		ring.setFrames(myAnimation(azimuth_mean));
		ring.show(true); // 非同期で点灯させる

		// 3. 既存の音源群のうち、最も音源方向が近い既存音源に音声を追記する。既存の音源群が無い、もしくは全ての既存音源から推定音源方向が
		//    プラスマイナス10度以上だった場合に新規音源と判断することとし、新規音源として追加する
		if(p->sources_.size() == 0){
			p->sources_.emplace_back(new AudioSource(azimuth_mean));
			p->currentId_ = p->sources_.size()-1;
			std::cout << "Write sourceId=" << sourceId << " to new source."<< std::endl;
			std::cout << "Current sources = " << p->sources_.size() << std::endl;
		}else{
			// 今回検出された推定音源方向と、最も方向が近い既存音源を探す
			int min_diff = 360;
			int min_diff_idx = 0;
			for(size_t i=0;i<p->sources_.size();++i){
				int diff = diffDegree(p->sources_[i]->azimuth_, azimuth_mean);
				if(diff < min_diff){
					min_diff = diff;
					min_diff_idx = i;
				}
			}
			if(min_diff < 20){
				// 既存音源に追記する
				p->currentId_ = min_diff_idx;
				std::cout << "Write sourceId=" << sourceId << " to existing source at " << p->sources_[min_diff_idx]->azimuth_ << ", min_diff=" << min_diff << std::endl;
			}else{
				// 新規音源として追加する
				p->sources_.emplace_back(new AudioSource(azimuth_mean));
				p->currentId_ = p->sources_.size()-1;
				std::cout << "Write sourceId=" << sourceId << " to new source " << ", min_diff=" << min_diff << std::endl;
				std::cout << "Current sources = " << p->sources_.size() << std::endl;
			}
		}
	}else if(state == mimixfe::SpeechState::InSpeech){
		s = "In Speech";
	}else if(state == mimixfe::SpeechState::SpeechEnd){
		s = "End of Speech";
		ring.motion(true, 1, defaultPattern()); // デフォルトパターン点灯に戻す
	}

	// 画面表示で確認
	std::cout << "State: " << s << " ( ID = " << sourceId << " )" << std::endl;
	for(size_t i=0;i<infolen;++i){
		std::cout << info[i].milliseconds_ << "[ms] " <<
				std::fixed << std::setprecision(3)
			  	  << info[i].rmsDbfs_ << "[dbFS] " << info[i].speechProbability_*100.0F << "[%] ";
		std::cout << sourceId << " ( " << info[i].numSoundSources_ << "/" << info[i].totalNumSoundSources_ << " )";
		std::cout << " angle=" << info[i].direction_.angle_ << ", azimuth=" <<
			  info[i].direction_.azimuth_ << ", peak=" << info[i].spatialSpectralPeak_;
		std::cout << " utterance_azimuth=" << info[i].utteranceDirection_.azimuth_ << std::endl;
	}

	// 音声データを記録する
	if(buflen != 0){
		p->sources_[p->currentId_]->addAudioData(buffer, buflen);
	}
}

int main(int argc, char** argv)
{
	if(signal(SIGINT, xfe_sig_handler_) == SIG_ERR){
		return 1;
	}
	// LED リングの外部制御点灯 FPS を 30 に設定しておく
	tumbler::LEDRing& ring = tumbler::LEDRing::getInstance();
	ring.setFPS(30);

	using namespace mimixfe;
	XFESourceConfig s;
	XFEECConfig e;
	XFEVADConfig v;
	XFEBeamformerConfig b;
	XFEDynamicLocalizerConfig c;
	c.area_ = XFELocalizerConfig::SearchArea::planar;
	XFEOutputConfig o;
	UserData data;
	int return_status = 0;
	try{
		XFERecorder rec(s,e,v,b,c,o,recorderCallback,reinterpret_cast<void*>(&data));
		rec.setLogLevel(LOG_UPTO(LOG_DEBUG)); // デバッグレベルのログから出力する
		rec.controlLED(false); // libmimixfe 内部での LED 制御を行わない
		rec.start();
		int countup = 0;
		int timeout = 120;
		while(rec.isActive()){
			std::cout << countup++  << " / " << timeout << std::endl;
			if(countup == timeout){
				rec.stop();
				break;
			}
			if(xfe_flag_ == 1){
				rec.stop();
				break;
			}
			sleep(1);
		}
		return_status = rec.stop();
	}catch(const XFERecorderError& e){
		std::cerr << "XFE Recorder Exception: " << e.what() << "(" << e.errorno() << ")" << std::endl;
	}catch(const std::exception& e){
		std::cerr << "Exception: " << e.what() << std::endl;
	}
	if(return_status != 0){
		std::cerr << "Abort by error code = " << return_status << std::endl;
	}else{
		std::cout << "Normally finished" << std::endl;
	}
	return return_status;
}
