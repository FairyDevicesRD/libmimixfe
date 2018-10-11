/**
 * @file XFERecorder.h
 * @brief libmimiXFE 録音系 API
 * @author Copyright (C) 2017 Fairy Devices Inc. http://www.fairydevices.jp/
 * @author Masato Fujino, created on: 2017/07/09 
 */
#ifndef INCLUDE_XFERECORDER_H_
#define INCLUDE_XFERECORDER_H_

#include <vector>
#include <string>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <syslog.h>
#include <tumbler/ledring.h>
#include "XFETypedef.h"

namespace mimixfe
{
	/**
	 * @class XFERecorderError
	 * @brief 録音関係実行時エラー. エラーコード 1000 番台.
	 */
	class DLL_PUBLIC XFERecorderError : public std::runtime_error
	{
	public:
		explicit XFERecorderError(int errorno) : runtime_error(errorstr(errorno)), errorno_(errorno) {}
		int errorno() const { return errorno_; }
		XFERecorderError(int errorno, const std::string& errstr) : runtime_error(errstr), errorno_(errorno) {}
	private:
		const std::string errorstr(int errorno);
		int errorno_;
	};

	class DLL_LOCAL XFERecorderImpl;
	class DLL_PUBLIC XFERecorder
	{
	public:
		XFERecorder(
				const XFESourceConfig& sourceConfig,
				const XFEECConfig& ecConfig,
				const XFEVADConfig& vadConfig,
				const XFEBeamformerConfig& bfConfig,
				const XFELocalizerConfig& locConfig,
				const XFEOutputConfig& outConfig,
				recorderCallback_t recorderCallback,
				void *userdata);
		virtual ~XFERecorder();

		/**
		 * @brief ログレベルを設定する
		 * @param [in] mask ログマスク LOG_UPTO マクロを使うと簡単に設定できる
		 */
		void setLogLevel(int mask);

		/**
		 * @brief XFE 録音ストリームを開始する
		 */
		void start();

		/**
		 * @brief XFE 録音ストリームを終了する
		 * @return 終了ステータス。正常終了の場合 0 が返される。
		 */
		int stop();

		/**
		 * @brief XFE 録音ストリームが有効であるかどうかを確認する。
		 * @details XFE 録音ストリームは、XFERecorder クラスのインスタンス化直後、stop() 関数が呼び出された時、SIGINT（Ctrl+C）が発生したとき、
		 * 内部エラーによって例外が送出されたときに無効になる。
		 */
		bool isActive() const;

		/**
		 * @brief モニタリングコールバックを設定（追加）する
		 * @details 異なる MonitoringType の複数のモニタリングコールバックを設定することができる。モニタリングコールバック指定はオプション。
		 * @param [in] callback モニタリングコールバック関数
		 * @param [in] type モニタリングタイプ（モニタリングコールバックに与えられる音声の種類を指定する）
		 * @param [in] type モニタリングタイプ（モニタリングコールバックに与えられる音声の圧縮形式を指定する）
		 * @param [in] userdata 任意データ
		 * @return true if success
		 */
		bool addMonitoringCallback(monitoringCallback_t callback, MonitoringAudioType type, AudioCodec codec, void* userdata);

		/**
		 * @brief XFE に LED 制御権を与える。デフォルトでは XFE は LED 制御権を持ち、音源検出時に検出方向を光らせる。
		 * @param [in] enable true の場合 XFE に LED 制御権を保持させる。false の場合、制御権を放棄させる。
		 * @note LED 制御は外部ライブラリの libtumbler を用いて行われている。
		 */
		void controlLED(bool enable);

		/**
		 * @brief XFE が LED 制御権を保持しているかどうかを返す
		 * @return true if XFE has LED control, false if XFE doesn't have one
		 */
		bool controlLED() const;

		/**
		 * @brief XFE の音源検出時の LED リングの色を指定する
		 * @param [in] foreground 音源方向の点灯色（デフォルトでは LED(0,0,55)）
		 * @param [in] background それ以外の方向の点灯色（デフォルトでは LED(38,38,38)）
		 */
		void setLEDColor(tumbler::LED foreground, tumbler::LED background);

	private:
		std::unique_ptr<XFERecorderImpl> recorderImpl_;
		bool status_;
	};
}

#endif
