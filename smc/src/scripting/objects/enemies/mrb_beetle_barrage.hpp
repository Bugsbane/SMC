#ifndef SMC_SCRIPTING_BEETLE_BARRAGE_HPP
#define SMC_SCRIPTING_BEETLE_BARRAGE_HPP
#include "../../scripting.hpp"

namespace SMC {
	namespace Scripting {
		extern struct RClass* p_rcBeetleBarrage;
		void Init_BeetleBarrage(mrb_state* p_state);
	}
}

#endif
