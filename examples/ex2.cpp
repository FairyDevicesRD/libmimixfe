/*
 * @file ex2.cpp
 * @brief 固定方向同時複数音源抽出サンプル
 * @author Copyright (C) 2017 Fairy Devices Inc. http://www.fairydevices.jp/
 * @author Masato Fujino, created on: 2017/07/14
 */

#include <iostream>
#include <unistd.h>
#include <syslog.h>
#include <string>
#include <sched.h>
#include <signal.h>
#include <iomanip>
#include <unordered_map>
#include "XFERecorder.h"
#include "XFETypedef.h"

volatile sig_atomic_t xfe_flag_ = 0;
void xfe_sig_handler_(int signum){ xfe_flag_ = 1; }

class UserData
{
public:
	FILE *file1_;
	FILE *file2_;
	FILE *file3_;
	FILE *file4_;
	std::unordered_map<int, FILE*> sourceIdToFile_;
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
	if(state == mimixfe::SpeechState::SpeechStart){
		s = "Speech Start";
		// 発話検出の際に、この sourceId がどの方向に該当するかを確認しテーブルに記録する
		if(info[0].utteranceDirection_.azimuth_ == 0){
			p->sourceIdToFile_[sourceId] = p->file1_;
		}else if(info[0].utteranceDirection_.azimuth_ == 90){
			p->sourceIdToFile_[sourceId] = p->file2_;
		}else if(info[0].utteranceDirection_.azimuth_ == 180){
			p->sourceIdToFile_[sourceId] = p->file3_;
		}else if(info[0].utteranceDirection_.azimuth_ == 270){
			p->sourceIdToFile_[sourceId] = p->file4_;
		}
	}else if(state == mimixfe::SpeechState::InSpeech){
		s = "In Speech";
	}else if(state == mimixfe::SpeechState::SpeechEnd){
		s = "End of Speech";
	}
	fwrite(buffer, sizeof(short), buflen, p->sourceIdToFile_[sourceId]);

	// 画面表示で確認
	std::cout << "State: " << s << " ( ID = " << sourceId << " )" << std::endl;
	for(size_t i=0;i<infolen;++i){
		std::cout << info[i].milliseconds_ << "[ms] " <<
				std::fixed << std::setprecision(3)
			  	  << info[i].rmsDbfs_ << "[dbFS] " << info[i].speechProbability_*100.0F << "[%] ";
		std::cout << sourceId << " ( " << info[i].numSoundSources_ << "/" << info[i].totalNumSoundSources_ << " )";
		std::cout << " angle=" << info[i].direction_.angle_ << ", azimuth=" <<
			  info[i].direction_.azimuth_ << ", peak=" << info[i].spatialSpectralPeak_ << " ( matchedDir: azimuth = " <<
			  info[i].utteranceDirection_.azimuth_ << ", angle = " << info[i].utteranceDirection_.angle_ << std::endl;
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
	XFEStaticLocalizerConfig c({Direction(0, 90), Direction(90,90), Direction(180,90), Direction(270,90)});
	XFEOutputConfig o;
	c.maxSimultaneousSpeakers_ = 2;
	UserData data;
	data.file1_ = fopen("/tmp/ex2_000.raw","w");
	data.file2_ = fopen("/tmp/ex2_090.raw","w");
	data.file3_ = fopen("/tmp/ex2_180.raw","w");
	data.file4_ = fopen("/tmp/ex2_270.raw","w");
	int return_status = 0;
	try{
		XFERecorder rec(s,e,v,b,c,o,recorderCallback,reinterpret_cast<void*>(&data));
		rec.setLogLevel(LOG_UPTO(LOG_DEBUG)); // デバッグレベルのログから出力する
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
	fclose(data.file1_);
	fclose(data.file2_);
	fclose(data.file3_);
	fclose(data.file4_);
	if(return_status != 0){
		 std::cerr << "Abort by error code = " << return_status << std::endl;
	}else{
		 std::cout << "Normally finished" << std::endl;
	}
	return return_status;
}
