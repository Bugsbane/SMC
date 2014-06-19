#include "../../../level/level.hpp"
#include "../../../user/savegame.hpp"
#include "../../../gui/hud.hpp"
#include "../../../core/property_helper.hpp"
#include "../../events/event.hpp"
#include "../mrb_eventable.hpp"
#include "mrb_level.hpp"

/**
 * Class: LevelClass
 *
 * `LevelClass` exposes it’s sole instance through the `Level` singleton,
 * which always points to the currently active level. It is a mostly
 * informational object allowing you to access a level’s global settings,
 * but does not permit you to change them, because this either wouldn’t
 * make much sense in the first place (why change the author name from
 * within the script?) or could even cause severe confusion for the game
 * (such as changing the filename).
 *
 * This class allows you to register handlers for two very special
 * events: The **save** and the **load** event. These events are not
 * fired during regular gameplay, but instead when the player creates a
 * new savegame (**save**) or restores an existing one (**load**). By
 * returning an MRuby hash from the **save** event handler, you can
 * advertise SMC to store it in the savegame; later, when the user loads
 * this savegame again, the hash is deserialised from the savegame and
 * passed back as an argument to the even thandler of the **load**
 * event. This way you can store information on your level from within
 * the scripting API that will persist between saves and loads of a
 * level.
 *
 * Consider this example:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ruby
 * # Say, you have a number of switches in your
 * # level. Their state is stored inside this
 * # global table.
 * switches = {
 *   :blue  => false,
 *   :red   => false,
 *   :green => false
 * }
 *
 * # The player may activate your switches,
 * # causing the respective entry in the
 * # global `switches' table to change.
 * UIDS[114].on_touch do |collidor|
 *   switches[:red] = true if collidor.player?
 * end
 *
 * # Now, if the player jumps on your switch and
 * # then saves and reloads, the switch’s state
 * # gets lost. To prevent this, we define handlers
 * # for the `save' and `load' events that persist
 * # the state of the global `switches' table.
 * # See below to see why we don’t dump the symbols
 * # into the savegame.
 * Level.on_save do |store|
 *   store["blue"]  = switches[:blue]
 *   store["red"]   = switches[:red]
 *   store["green"] = switches[:green]
 * end
 *
 * Level.on_load do |store|
 *   switches[:blue]  = store["blue"]
 *   switches[:red]   = store["red"]
 *   switches[:green] = store["green"]
 * end
 *
 * # This way the switches will remain in their
 * # respective state even after saving/reloading
 * # a game. If you change graphics for pressed
 * # switches, you still have to do this manually
 * # in your event handlers, though.
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * Please note that the hash yielded to the block of the `save` event
 * gets converted to JSON for persistency. This comes with a major
 * limitation: You can’t store arbitrary MRuby objects in this hash,
 * and if you do, they will be autoconverted to strings, which is
 * most likely not what you want. So please stick with the primitive
 * types JSON supports, especially with regard to symbols (as keys
 * and values), which are converted to strings and therefore will
 * show up as strings in the parameter of the `load` event’s callback.
 *
 * You are advised to not register more than one event handler for
 * the `save` and `load` events, respectively. While this is possible,
 * it has several drawbacks:
 *
 * * For the `save` event, the lastly called event handler decides
 *   which data to store. The other’s data gets skipped.
 * * For the `load` event, the JSON data gets parsed once per callback,
 *   putting unnecessary strain on the game and delaying level loading.
 *
 * Internal note
 * -------------
 *
 * You will most likely neither notice nor need it, but the Lua `Level`
 * singleton actually doesn’t wrap SMC’s notion of the currently running
 * level, `pActive_Level`, but rather the pointer to the savegame
 * mechanism, `pSavegame`. This facilitates the handling of the event
 * table for levels. Also, it is more intuitively to have the `Save`
 * and `Load` events defined on the Level rather than on a separate
 * Savegame object.
 *
 * Events
 * ------
 *
 * Load
 * : Called when the user loads a savegame containing this level. The
 *   event handler gets passed a hash containing any values
 *   requested in the **save** event’s handler, but note it was
 *   deserialised from a JSON representation and hence subject to its
 *   limits. Do not assume your level is active when this is called,
 *   the player may be in a sublevel (however, usually
 *   this has no impact on what you want to restore, but don’t try to
 *   warp the player or things like that, it will result in undefined
 *   behaviour probably leading SMC to crash).
 *
 * Save
 * : Called when the users saves a game. The event handler should store
 *   all values you want to preserve between level loading in saving
 *   in the hash it receives as a parameter, but please see the explanations
 *   further above regarding the limitations of this hash. Do not assume your
 *   level is active when this is called, because the player may be in a
 *   sublevel (however, usually this has no impact on what you want to save).
 */

using namespace SMC;
using namespace SMC::Scripting;


/***************************************
 * Events
 ***************************************/

MRUBY_IMPLEMENT_EVENT(load);
MRUBY_IMPLEMENT_EVENT(save);

/***************************************
 * Methods
 ***************************************/

static mrb_value Initialize(mrb_state* p_state,	 mrb_value self)
{
	mrb_raise(p_state, MRB_NOTIMP_ERROR(p_state), "Cannot create instances of this class.");
	return self; // Not reached
}

/**
 * Method: LevelClass#author
 *
 *   author() → a_string
 *
 * Returns the content of the level’s *Author* info field.
 */
static mrb_value Get_Author(mrb_state* p_state,	 mrb_value self)
{
	return mrb_str_new_cstr(p_state, pActive_Level->m_author.c_str());
}

/**
 * Method: LevelClass#description
 *
 *   description() → a_string
 *
 * Returns the content of the level’s *Description* info field.
 */
static mrb_value Get_Description(mrb_state* p_state, mrb_value self)
{
	return mrb_str_new_cstr(p_state, pActive_Level->m_description.c_str());
}

/**
 * Method: LevelClass#difficulty
 *
 *   difficulty() → an_integer
 *
 * Returns the content of the level’s *Difficulty* info field.
 * This reaches from 0 (undefined) over 1 (very easy) to 100
 * ((mostly) uncompletable),
 */
static mrb_value Get_Difficulty(mrb_state* p_state, mrb_value self)
{
	return mrb_fixnum_value(pActive_Level->m_difficulty);
}

/**
 * Method: LevelClass#engine_version
 *
 *   engine_version() → an_integer
 *
 * Returns the SMC engine version used to create the level.
 */
static mrb_value Get_Engine_Version(mrb_state* p_state, mrb_value self)
{
	return mrb_fixnum_value(pActive_Level->m_engine_version);
}

/**
 * Method: LevelClass#filename
 *
 *   filename() → a_string
 *
 * Returns the level’s filename.
 */
static mrb_value Get_Filename(mrb_state* p_state, mrb_value self)
{
	return mrb_str_new_cstr(p_state, path_to_utf8(pActive_Level->m_level_filename).c_str());
}

/**
 * Method: LevelClass#music_filename
 *
 *   music_filename( [ format [, with_ext ] ] ) → a_string
 *
 * Returns the default level music’s filename, relative to
 * the `music/` directory.
 */
static mrb_value Get_Music_Filename(mrb_state* p_state, mrb_value self)
{
	return mrb_str_new_cstr(p_state, path_to_utf8(pActive_Level->Get_Music_Filename()).c_str());
}

/**
 * Method: LevelClass#script
 *
 *   script() → a_string
 *
 * Returns the MRuby code associated with this level.
 */
static mrb_value Get_Script(mrb_state* p_state, mrb_value self)
{
	return mrb_str_new_cstr(p_state, pActive_Level->m_script.c_str());
}

/**
 * Method: LevelClass#next_level_filename
 *
 *   next_level_filename() → a_string
 *
 * If a new level shall automatically be loaded when this level
 * completes, this returns the filename of the target level. Otherwise
 * the return value is undefined, but most likely an empty string.
 */
static mrb_value Get_Next_Level_Filename(mrb_state* p_state, mrb_value self)
{
	return mrb_str_new_cstr(p_state, path_to_utf8(pActive_Level->m_next_level_filename).c_str());
}

/**
 * Method: LevelClass#finish
 *
 *   finish( direction, [ win_music ] )
 *
 * The player immediately wins the level and the game resumes to the
 * world overview, advancing to the next level point. If the level was
 * loaded using the level menu directly (and hence there is no
 * overworld), returns to the level menu.
 *
 * #### Parameters
 *
 * direction
 * : A symbol denoting the direction in which to advance
 *   on the overworld. One of `:up`, `:down`, `:right`,
 *   or `:left`.
 *
 * win_music (false)
 * : If set, plays the level win music.
 */
static mrb_value Finish(mrb_state* p_state,  mrb_value self)
{
	mrb_value obj;
	mrb_sym dirsym;
	mrb_get_args(p_state, "n|o", &dirsym, &obj);

	std::string dirstr = mrb_sym2name(p_state, dirsym);
	ObjectDirection dir = Get_Direction_Id(dirstr);

	if (dir != DIR_UP && dir != DIR_DOWN && dir != DIR_RIGHT && dir != DIR_LEFT) {
		mrb_raisef(p_state, MRB_ARGUMENT_ERROR(p_state), "Invalid level exit overworld direction '%s'!", dirstr.c_str());
		return mrb_nil_value(); // Not reached
	}

	pLevel_Manager->Finish_Level(dir, mrb_test(obj));
	return mrb_nil_value();
}

/**
 * Method: LevelClass#display_info_message
 *
 *   display_info_message( message )
 *
 * Shows a **short**, informative message on the screen. This is achieved
 * by displaying a prominent sprite covering the full width of the
 * game window containing your message for a few seconds, before the
 * entire construction (i.e. sprite plus message) is then slowly faded
 * out to invisibility.
 *
 * This method is not meant to display larger passages of text to the
 * user; use the `Message` class from the SSL for that. No line breaking
 * is done (and only a single line of text is supported).
 *
 * This method is intended for displaying merely optional pieces of
 * information; for instance, if you built a large tower level, you
 * may use this method to display the floor the player just entered to
 * give him more orientation.
 *
 * Do not overuse this method. If you use it, stick to one usage scheme;
 * don’t use it for too many different kinds of information, that would
 * confuse the player probably.
 *
 * #### Parameters
 * message
 * : The message to display. A short oneliner.
 *
 * #### Example
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~ ruby
 * # Say the object with UID 14 is a warp point that
 * # warps you to the tower’s 3rd floor when touched.
 * # To make the player aware, write your code like this:
 * UIDS[14].on_touch do |collidor|
 *   next unless collidor.player? # Only react on the player
 *
 *   Level.display_info_message("3rd floor")
 *   collidor.warp(400, -620)
 * end
 * ~~~~~~~~~~~~~~~~~~~~~~~
 */
static mrb_value Display_Info_Message(mrb_state* p_state, mrb_value self)
{
	char* message = NULL;
	mrb_get_args(p_state, "z", &message);

	pHud_Infomessage->Set_Text(message);
	return mrb_nil_value();
}

void SMC::Scripting::Init_Level(mrb_state* p_state)
{
	struct RClass* p_rcLevel = mrb_define_class(p_state, "LevelClass", p_state->object_class);
	mrb_include_module(p_state, p_rcLevel, mrb_class_get(p_state, "Eventable"));
	MRB_SET_INSTANCE_TT(p_rcLevel, MRB_TT_DATA);

	// Make the Level constant the only instance of LevelClass
	mrb_define_const(p_state, p_state->object_class, "Level", pSavegame->Create_MRuby_Object(p_state));

	mrb_define_method(p_state, p_rcLevel, "initialize", Initialize, MRB_ARGS_NONE());
	mrb_define_method(p_state, p_rcLevel, "author", Get_Author, MRB_ARGS_NONE());
	mrb_define_method(p_state, p_rcLevel, "description", Get_Description, MRB_ARGS_NONE());
	mrb_define_method(p_state, p_rcLevel, "difficulty", Get_Difficulty, MRB_ARGS_NONE());
	mrb_define_method(p_state, p_rcLevel, "engine_version", Get_Engine_Version, MRB_ARGS_NONE());
	mrb_define_method(p_state, p_rcLevel, "filename", Get_Filename, MRB_ARGS_NONE());
	mrb_define_method(p_state, p_rcLevel, "music_filename", Get_Music_Filename, MRB_ARGS_NONE());
	mrb_define_method(p_state, p_rcLevel, "script", Get_Script, MRB_ARGS_NONE());
	mrb_define_method(p_state, p_rcLevel, "next_level_filename", Get_Next_Level_Filename, MRB_ARGS_NONE());
	mrb_define_method(p_state, p_rcLevel, "finish", Finish, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1));
	mrb_define_method(p_state, p_rcLevel, "display_info_message", Display_Info_Message, MRB_ARGS_REQ(1));

	mrb_define_method(p_state, p_rcLevel, "on_load", MRUBY_EVENT_HANDLER(load), MRB_ARGS_NONE());
	mrb_define_method(p_state, p_rcLevel, "on_save", MRUBY_EVENT_HANDLER(save), MRB_ARGS_NONE());
}
