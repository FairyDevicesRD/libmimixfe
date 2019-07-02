/*
 * @file ex5.cpp
 * @brief 固定方向単一音源で、LED リングの光り方を変えたサンプル
 * @author Copyright (C) 2017 Fairy Devices Inc. http://www.fairydevices.jp/
 * @author Masato Fujino, created on: 2017/07/14
 */

#include <iostream>
#include <unistd.h>
#include <syslog.h>
#include <sched.h>
#include <signal.h>
#include <string>
#include <iomanip>
#include "XFERecorder.h"
#include "XFETypedef.h"

volatile sig_atomic_t xfe_flag_ = 0;
void xfe_sig_handler_(int signum){ xfe_flag_ = 1; }

/**
 * @brief サンプルプログラムで用いる虹色を再現したフレームを返す関数、18 個の tumbler::LED で 7 色のグラデーションとなるように手で色指定した。
 * @return フレーム（18 個の tumbler::LED の色指定が為されたもの）
 */
tumbler::Frame rainbowPattern()
{
	  tumbler::Frame f;
	  f.setLED(0, tumbler::LED(255,0,0));
	  f.setLED(1, tumbler::LED(255,0,0));
	  f.setLED(2, tumbler::LED(255,0,0));
	  f.setLED(3, tumbler::LED(255,165,0));
	  f.setLED(4, tumbler::LED(255,165,0));
	  f.setLED(5, tumbler::LED(255,165,0));
	  f.setLED(6, tumbler::LED(0,255,0));
	  f.setLED(7, tumbler::LED(0,255,0));
	  f.setLED(8, tumbler::LED(0,255,0));
	  f.setLED(9, tumbler::LED(0,255,255));
	  f.setLED(10, tumbler::LED(0,255,255));
	  f.setLED(11, tumbler::LED(0,255,255));
	  f.setLED(12, tumbler::LED(0,0,255));
	  f.setLED(13, tumbler::LED(0,0,255));
	  f.setLED(14, tumbler::LED(0,0,255));
	  f.setLED(15, tumbler::LED(128,0,128));
	  f.setLED(16, tumbler::LED(128,0,128));
	  f.setLED(17, tumbler::LED(128,0,128));
	  return f;
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

class UserData
{
public:
	FILE *file_;
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
	std::string s = "";
	if(state == mimixfe::SpeechState::SpeechStart){
		s = "Speech Start";
	}else if(state == mimixfe::SpeechState::InSpeech){
		s = "In Speech";
	}else if(state == mimixfe::SpeechState::SpeechEnd){
		s = "End of Speech";
	}else if(state == mimixfe::SpeechState::NonSpeech){
		s = "Non Speech";
	}
  UserData *p = reinterpret_cast<UserData*>(userdata);
  if(buflen != 0){
	  fwrite(buffer, sizeof(short), buflen, p->file_);
  }

  // 画面表示で確認
  std::cout << "State: " << s << " / sourceId=" << sourceId << std::endl;
  for(size_t i=0;i<infolen;++i){
	  std::cout << info[i].milliseconds_ << "[ms] " <<
	  std::fixed << std::setprecision(3)
			  << info[i].rmsDbfs_ << "[dbFS] " << info[i].speechProbability_*100.0F << "[%] ";
	  std::cout << sourceId << " ( " << info[i].numSoundSources_ << "/" << info[i].totalNumSoundSources_ << " )";
	  std::cout << " angle=" << info[i].direction_.angle_ << ", azimuth=" <<
			  info[i].direction_.azimuth_ << ", peak=" << info[i].spatialSpectralPeak_ << std::endl;
  }
}

int main(int argc, char** argv)
{
	if(signal(SIGINT, xfe_sig_handler_) == SIG_ERR){
		return 1;
	}
	using namespace mimixfe;
	XFESourceConfig s;
	XFEECConfig e;
	XFEVADConfig v;
	XFEBeamformerConfig b;
	XFEStaticLocalizerConfig c({Direction(270, 90)});
	XFEOutputConfig o;

	UserData data1;
	data1.file_ = fopen("/tmp/ex1.raw","w");

	int return_status = 0;
	try{
		XFERecorder rec(s,e,v,b,c,o,recorderCallback,reinterpret_cast<void*>(&data1));
		rec.setLogLevel(LOG_UPTO(LOG_DEBUG)); // デバッグレベルのログから出力する

		// 音源検出時の LED リングの光り方
		rec.setLEDColor(tumbler::LED(255,0,0), tumbler::LED(0,80,0)); // 音源定位方向を赤、それ以外を緑（明るさを小さめ）に

		// 音源未検出時の LED リングの光り方
		rec.setDefaultFrame(tumbler::Frame(tumbler::LED(0,0,0)), 0);   // 消灯（消灯で静止）
		//rec.setDefaultFrame(defaultPattern(), 1); // デフォルトパターンで時計回り回転
		//rec.setDefaultFrame(rainbowPattern(), 2); // 虹色パターンで反時計回り回転

		rec.start();
		int countup = 0;
		int timeout = 60;
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
	 fclose(data1.file_);
	 return return_status;
}
