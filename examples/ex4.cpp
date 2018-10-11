/*
 * @file ex4.cpp
 * @brief 動的方向複数音源抽出サンプル
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
		filename << "/tmp/ex4_";
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
	std::unordered_map<int, int> sourceId_to_dataId_;
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
		// 1. 発話検出時の推定音源方向を取得する
		std::vector<int> azms;
		for(size_t i=0;i<infolen;++i){
			if(info[i].direction_.azimuth_ != -1){
				azms.push_back(info[i].utteranceDirection_.azimuth_);
			}
		}
		int azimuth_mean = meanDegree(azms);
		std::cout << "Speech Start: " << azimuth_mean << " degree" << std::endl;

		// 2. 既存の音源群のうち、最も音源方向が近い既存音源に音声を追記する。既存の音源群が無い、もしくは全ての既存音源から推定音源方向が
		//    プラスマイナス10度以上だった場合に新規音源と判断することとし、新規音源として追加する
		if(p->sources_.size() == 0){
			p->sources_.emplace_back(new AudioSource(azimuth_mean));
			p->sourceId_to_dataId_[sourceId] = p->sources_.size()-1;
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
				p->sourceId_to_dataId_[sourceId] = min_diff_idx;
				std::cout << "Write sourceId=" << sourceId << " to existing source at " << p->sources_[min_diff_idx]->azimuth_ << ", min_diff=" << min_diff << std::endl;
			}else{
				// 新規音源として追加する
				p->sources_.emplace_back(new AudioSource(azimuth_mean));
				p->sourceId_to_dataId_[sourceId] = p->sources_.size()-1;
				std::cout << "Write sourceId=" << sourceId << " to new source " << ", min_diff=" << min_diff << std::endl;
				std::cout << "Current sources = " << p->sources_.size() << std::endl;
			}
		}
	}else if(state == mimixfe::SpeechState::InSpeech){
		s = "In Speech";
	}else if(state == mimixfe::SpeechState::SpeechEnd){
		s = "End of Speech";
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
		p->sources_[p->sourceId_to_dataId_[sourceId]]->addAudioData(buffer, buflen);
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
	XFEDynamicLocalizerConfig c;
	c.maxSimultaneousSpeakers_ = 2;
	XFEOutputConfig o;
	UserData data;
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
	if(return_status != 0){
		std::cerr << "Abort by error code = " << return_status << std::endl;
	}else{
		std::cout << "Normally finished" << std::endl;
	}
	return return_status;
}
