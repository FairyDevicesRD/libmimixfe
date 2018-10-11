/*
 * @file ex1.cpp
 * @brief 固定方向単一音源サンプル（モニタリングコールバック例を含む）
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

void monitoringCallback(const short* buffer, size_t buflen, void* userdata)
{
	  UserData *p = reinterpret_cast<UserData*>(userdata);
	  fwrite(buffer, sizeof(short), buflen, p->file_);
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
	v.timeToInactive_ = 1000;
	XFEBeamformerConfig b;
	XFEStaticLocalizerConfig c({Direction(270, 90)});
	XFEOutputConfig o;

	UserData data1;
	data1.file_ = fopen("/tmp/ex1.raw","w");
	UserData data2;
	data2.file_ = fopen("/tmp/monitor_ex1.raw","w");

	int return_status = 0;
	try{
		XFERecorder rec(s,e,v,b,c,o,recorderCallback,reinterpret_cast<void*>(&data1));
		//rec.setLEDColor(tumbler::LED(255,0,0), tumbler::LED(100,100,100));
		//rec.controlLED(false);
		rec.setLogLevel(LOG_UPTO(LOG_DEBUG)); // デバッグレベルのログから出力する
		rec.addMonitoringCallback(monitoringCallback, MonitoringAudioType::S16kC1EC, AudioCodec::RAWPCM, reinterpret_cast<void*>(&data2));
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
	 }catch(const XFERecorderError& e){
		std::cerr << "XFE Recorder Exception: " << e.what() << "(" << e.errorno() << ")" << std::endl;
	 }catch(const std::exception& e){
		std::cerr << "Exception: " << e.what() << std::endl;
	 }
	 fclose(data1.file_);
	 fclose(data2.file_);
	 if(return_status != 0){
		 std::cerr << "Abort by error code = " << return_status << std::endl;
	 }else{
		 std::cout << "Normally finished" << std::endl;
	 }
	 return return_status;
}
