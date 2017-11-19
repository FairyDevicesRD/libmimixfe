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
		DISCARD, //!< マイク入力を利用しない
		INPUT,   //!< マイク入力を利用する
		REFERENCE, //!< マイク入力を参照信号として利用する
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
		int channels_ = 18; // !< 出力チャネル数
		MicrophoneUsage microphoneUsage_[18]; // !< マイクの個別設定
	};

	class DLL_PUBLIC XFEVADConfig
	{
	public:
		bool enable_ = true;
		int timeToActive_ = 60;
		int timeToInactive_ = 800;
		int headPaddingTime_ = 600;
		int tailPaddingTime_ = 600;
		int rmsThreshold_ = 100;
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
		Preference pref_ = Preference::Fast;
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
			planar, //!< 本機を中心に置く水平面への射影に限定する。azimuth は 90 度に固定され、angle のみ 0,360 を取る。
			sphere, //!< 本機を中心に置く球。azimuth, angle 共に探索対象となる。planar_ と比較し、処理が重くなる。
		};
		virtual ~XFELocalizerConfig(){}
		bool enable_ = true; //!< ローカライザモジュールの有効無効の指定
		LocalizerType type_;
		SearchArea area_ = SearchArea::planar; //!< 音源探索領域の指定
	protected:
		XFELocalizerConfig(LocalizerType type) : type_(type){}
	};
	/**
	 * @class XFEStaticLocalizerConfig
	 * @brief 設定で固定された方向のみの音声を取得する（固定ビームフォーミング）
	 */
	class DLL_PUBLIC XFEStaticLocalizerConfig : public XFELocalizerConfig
	{
	public:
		XFEStaticLocalizerConfig(std::initializer_list<Direction> directions) :
			targetDirections_(directions.begin(), directions.end()),
			XFELocalizerConfig(LocalizerType::staticLocalizer){}
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
			maxSimultaneousSpeakers_(1) {}

		int maxSimultaneousSpeakers_; //!< 同時定位する最大音源数。推定音源数が最大音源数に満たない場合、推定音源のみ出力される。
		std::vector<Sector> sectorsForIgnore_; //!< 平面無視領域
	};

	/**
	 * @class XFEStreamInfo
	 * @brief 音声ストリームの解析結果。10msごとに結果が返される。
	 */
	class DLL_PUBLIC StreamInfo
	{
	public:
		int milliseconds_; // !< 経過時間[ms]
		float rms_; // !< 平均音量[rms]
		bool soundSourceDetected_; // !< 音源が定位されたかどうか
		Direction direction_; // !< 推定音源方向
		float speechProbability_; // !< 発話存在確率
		int extractedSoundSources_; // !< 抽出された同時発生音源数（実際にコールバック関数で出力される）
		int estimatedSoundSources_; // !< 推定された同時発生音源数
		float f0_; // !< F0
		float f1_; // !< F1
		float f2_; // !< F2
	};

	using recorderCallback_t = void (*)(
			short* buffer,
			size_t buflen,
			SpeechState state,
			int sourceId,
			StreamInfo* info,
			size_t infolen,
			void* userdata);
}

#endif /* LIBMIMIXFE_INCLUDE_TYPEDEF_H_ */
