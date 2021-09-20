#pragma once

#include "Logger.h"

#include <functional>
#include <optional>

namespace dpl {

	template<typename T, typename S>
	class PipelineStage {
	public:

		PipelineStage() = default;
		PipelineStage(std::function<void(const T&)>& fn) : output(fn) { }

		virtual void operator<<(const T&) = 0;
		
		template<typename Func> requires std::invocable<Func, S>
		void setOutputCallback(const Func& fn) {
			#ifdef DPL_LOG
			output_fn = [&fn](const auto& out_sym) -> void {
				std::cout << dpl::log::streamer{ out_sym } << '\n';
				std::invoke(fn, out_sym);
			};
			#else
			output_fn = fn;
			#endif //DPL_LOG
		}

		template<typename InT, typename OutT>
		void setOutputStage(const PipelineStage<InT, OutT>& nxt) {
			#ifdef DPL_LOG
			output_fn = [&nxt](const auto& out_sym) -> void {
				std::cout << dpl::log::streamer{ out_sym } << '\n';
				nxt << out_sym;
			};
			#else
			output_fn = [&nxt](const auto& out_sym) { nxt << out_sym; };
			#endif //DPL_LOG
		}

	protected:

		void output(const S& arg) {
			try {
				output_fn(arg);
			} catch (const std::bad_function_call& exc) { /* nothing */ }
		}

	private:

		std::function<void(const S&)> output_fn;

	};

}