#ifndef SMC_SCRIPTING_SHOOT_EVENT_HPP
#define SMC_SCRIPTING_SHOOT_EVENT_HPP
#include "../scripting.hpp"
#include "event.hpp"

namespace SMC {
    namespace Scripting {
        class cShoot_Event: public cEvent {
        public:
            cShoot_Event(std::string ball_type);
            virtual std::string Event_Name();
            std::string Get_Ball_Type();
        protected:
            virtual void Run_MRuby_Callback(cMRuby_Interpreter* p_mruby, mrb_value callback);
        private:
            std::string m_ball_type;
        };
    }
}
#endif
