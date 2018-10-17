/**
 * @file XFETypedef.h
 * @brief libmimiXFE の設定の型定義
 * @author Copyright (C) 2017 Fairy Devices Inc. http://www.fairydevices.jp/
 * @author Masato Fujino, created on: 2017/07/09 
 */
#ifndef LIBMIMIXFE_INCLUDE_TYPEDEF_H_
#define LIBMIMIXFE_INCLUDE_TYPEDEF_H_

#define DLL_PUBLIC __attribute__ ((visibility ("default")))
#define DLL_LOCAL  __attribute__ ((visibility ("hidden")))

#include <vector>
#include <initializer_list>
#include <stddef.h>

namespace mimixfe
{
	/**
	 * @enum 発話検出状態
	 * @class SpeechState
	 */
	enum class DLL_PUBLIC SpeechState
	{
		SpeechStart, //!< 発話が開始された（発話の開始時点で 1 回出力される）
		InSpeech,    //!< 発話中
		SpeechEnd,   //!< 発話が終了した（発話の終了時点で 1 回出力される）
		NonSpeech,   //!< 発話が検出されていない
	};

	/**
	 * @enum マイクの利用形態
	 * @class MicrophoneUsage
	 */
	enum class DLL_PUBLIC MicrophoneUsage
	{
		DISCARD,   //!< マイク入力を利用しない
		INPUT,     //!< マイク入力を利用する
		REFERENCE, //!< マイク入力を参照信号として利用する
	};

	/**
	 * @enum 音声コーデック
	 * @class AudioCodec
	 */
	enum class DLL_PUBLIC AudioCodec
	{
		RAWPCM, //!< RAW PCM 16bit 無圧縮音声
		FLAC,   //!< FLAC 可逆圧縮音声
		SPEEX,  //!< SPEEX 不可逆圧縮音声
	};

	/**
	 * @class 音源の設定
	 * @brief 出力サンプリングレート、入力チャネルの設定。入力チャネルは 18 チャネル全てを設定しなければならない。
	 */
	class DLL_PUBLIC XFESourceConfig
	{
	public:
		XFESourceConfig()
		{
			for(int i=0;i<16;++i){
				microphoneUsage_[i] = MicrophoneUsage::INPUT;
			}
			microphoneUsage_[16] = MicrophoneUsage::REFERENCE;
			microphoneUsage_[17] = MicrophoneUsage::REFERENCE;
		}
		int samplingrate_ = 16000; // !< 出力サンプリングレート
		MicrophoneUsage microphoneUsage_[18]; // !< マイクの個別設定
	};

	class DLL_PUBLIC XFEVADConfig
	{
	public:
		bool enable_ = true;
		int timeToActive_ = 120;    //!< 発話開始判定に必要な長さ[ms]
		int timeToInactive_ = 800;  //!< 発話終了判定に必要な長さ[ms]
		int headPaddingTime_ = 600; //!< 切り出される発話区間先頭側を延長する長さ[ms]
		int tailPaddingTime_ = 600; //!< 切り出される発話区間末尾側を延長する長さ[ms]
		int dbfsThreshold_ = -96;   //!< 発話判定に必要な最低音量閾値[dbfs]
	};

	class DLL_PUBLIC XFEECConfig
	{
	public:
		enum class Preference{
			Accurate, //!< 最高精度だが処理が遅い
			Balanced, //!< EC 出力が後段認識器に与える影響を考慮した速度精度バランス調整
			Fast,     //!< 高速だが後段認識器に若干の悪影響を与える可能性がある
		};
		bool enable_ = true;
		Preference pref_ = Preference::Balanced;
		bool aesEnable_ = false;
		double aesRatio_ = 1.0;
	};

	class DLL_PUBLIC XFEBeamformerConfig
	{
	public:
		enum class type
		{
			MVDR_v1,
			MVDR_v2
		};
		bool enable_ = true;
		type type_ = type::MVDR_v2;
		float sensitibity_ = 1.0;
		bool postfilter_enable_ = true;
	};

	/**
	 * @class Direction
	 * @brief 方位角と迎え角で定められる方向ベクトル。
	 */
	class DLL_PUBLIC Direction
	{
	public:
		/*
		 * @brief コンストラクタ
		 * @attention `angle` は水平面上の場合に90となることに留意せよ。
		 * @param [in] azimuth 方位角（水平面上の角度。本機正面を0度として反時計回り）、単位は度[degree]（ラジアンではない）
		 * @param [in] angle 迎え角（本機真下を0度、水平面を90度、本機真上を180度とする）、単位は度[degree]（ラジアンではない）
		 */
		Direction(int azimuth, int angle) :
			azimuth_(azimuth), angle_(angle) {}

		Direction() :
			azimuth_(0), angle_(0) {}
		int azimuth_; // !< 方位角（水平面上の角度。本機正面を0度として反時計回り）
		int angle_;	  // !< 迎え角（本機真下を0度、水平面を90度、本機真上を180度とする）
	};

	/**
	 * @class Sector
	 * @brief 本機水平面上の扇型の定義。本機正面を0度とし反時計回りに`azimuth`角[degree] から、中心角`range`度[degree]の範囲の扇型。
	 */
	class DLL_PUBLIC Sector
	{
	public:
		Sector(int azimuth, int range) :
			azimuth_(azimuth),
			range_(range) {}
		int azimuth_;
		int range_;
	};

	/**
	 * @class XFELocalizer
	 * @brief 音源定位に関係する設定クラス。利用時にはこのクラスを継承する具体クラスを利用する
	 */
	class DLL_PUBLIC XFELocalizerConfig
	{
	public:
		/**
		 * @class LocalizerType
		 * @brief 音源定位器の種別。定位しない（固定方向）もしくは動的に定位する。
		 */
		enum class LocalizerType
		{
			staticLocalizer, //!< 音源を動的に定位しない。事前に設定した固定方向の音源を抽出する。
			dynamicLocalizer,//!< 音源を動的に定位する。
		};
		/**
		 * @class LocalizerArea
		 * @brief 音源探索領域. 平面のみの探索（処理が軽い）もしくは立体的な探索（処理が重い）
		 */
		enum class DLL_PUBLIC SearchArea
		{
			planar, //!< 本機を中心に置く水平面への射影に限定する。azimuth は 90 度に固定され、angle のみ [0,360] を取る。
			sphere, //!< 本機を中心に置く球。azimuth, angle 共に探索対象となる。planar_ と比較し、処理が重くなる。
		};
		virtual ~XFELocalizerConfig(){}
		bool enable_ = true; //!< ローカライザモジュールの有効無効の指定
		LocalizerType type_;
		SearchArea area_ = SearchArea::planar; //!< 音源探索領域の指定
		int maxSimultaneousSpeakers_ = 1; //!< 同時定位する最大音源数
	protected:
		XFELocalizerConfig(LocalizerType type) : type_(type){}
	};
	/**
	 * @class XFEStaticLocalizerConfig
	 * @brief 設定で固定された方向のみの音声を取得する
	 */
	class DLL_PUBLIC XFEStaticLocalizerConfig : public XFELocalizerConfig
	{
	public:
		XFEStaticLocalizerConfig(std::initializer_list<Direction> directions) :
			XFELocalizerConfig(LocalizerType::staticLocalizer),
			targetDirections_(directions.begin(), directions.end()){}
		std::vector<Direction> targetDirections_;
	};

	/**
	 * @class XFEDynamicLocalizerConfig
	 * @brief 音声到来方向を動的に推定する
	 */
	class DLL_PUBLIC XFEDynamicLocalizerConfig : public XFELocalizerConfig
	{
	public:
		XFEDynamicLocalizerConfig() :
			XFELocalizerConfig(LocalizerType::dynamicLocalizer),
			identicalRange_(30),
			sourceDetectionSensitibity_(0.4){}

		int identicalRange_; //!< 同時定位する場合に、指定角度以下を同一音源とみなす角度
		float sourceDetectionSensitibity_; //!< 同時定位する場合の、複数音源検出の感度[0,1]区間の浮動小数点数（0 のとき感度が低い、1 のとき感度が高い）、感度を上げると偽音源が検出される場合がある。
		std::vector<Sector> sectorsForIgnore_; //!< 平面無視領域
	};

	/**
	 * @class XFEOutputConfig
	 * @brief 出力の設定
	 */
	class DLL_PUBLIC XFEOutputConfig
	{
	public:
		enum class outputType{
			allFrames,   //!< 全ての入力フレームに対応する区間が出力される。出力対象音声がない場合も、ストリーム情報が出力される
			audioFrames, //!< 出力対象音声がある区間のみ出力される（互換）
		};
		outputType type_ = outputType::audioFrames; //!< コールバック関数が呼ばれるタイミング
		AudioCodec codec_; //!< 出力音声コーデック
	};

	/**
	 * @class XFEStreamInfo
	 * @brief 音声ストリームの解析結果。10msごとに結果が返される。
	 */
	class DLL_PUBLIC StreamInfo
	{
	public:
		unsigned long long milliseconds_; // !< 経過時間[ms]
		Direction direction_; // !< 10msec フレームの推定方向
		Direction utteranceDirection_; //!< 発話単位での推定方向（固定方向設定の場合は、設定方向の一）
		float speechProbability_; // !< 10msec フレームの発話存在確率[0,1]
		float rmsDbfs_; // !< 平均音量[dbfs]
		int numSoundSources_; //!< 抽出された音源数
		int totalNumSoundSources_; //!< 検出された音源数
		float spatialSpectralPeak_; //!< 空間スペクトル値[db]
		std::vector<float> spatialSpectrum_; //!< 平均空間スペクトル[db]
	};

	using recorderCallback_t = void (*)(
			short* buffer,
			size_t buflen,
			SpeechState state,
			int sourceId,
			StreamInfo* info,
			size_t infolen,
			void* userdata);

	enum class DLL_PUBLIC MonitoringAudioType
	{
		S48kC18,  //!< サンプリングレート 48k, 18ch 音声
		S48kC1,   //!< サンプリングレート 48k,  1ch 音声（16ch を 1ch にミックスダウン）
		S16kC18,  //!< サンプリングレート 16k, 18ch 音声
		S16kC16EC,//!< サンプリングレート 16k, 16ch, エコーキャンセル済音声
		S16kC1EC  //!< サンプリングレート 16k,  1ch, エコーキャンセル済音声（16ch を 1ch にミックスダウン）
	};

	using monitoringCallback_t = void (*)(
			const short* buffer,
			size_t buflen,
			void* userdata);
}

#endif /* LIBMIMIXFE_INCLUDE_TYPEDEF_H_ */
