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
		 * @brief モニタリングコールバックを設定する
		 */
		bool setMonitoringCallback(monitoringCallback_t monitoringCallback, MonitoringType t);

	private:
		std::unique_ptr<XFERecorderImpl> recorderImpl_;
		bool status_;
	};
}

#endif
