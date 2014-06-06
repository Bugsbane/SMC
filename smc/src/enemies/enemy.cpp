/***************************************************************************
 * enemy.cpp  -  base class for all enemies
 *
 * Copyright © 2003 - 2011 The SMC Contributors
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../enemies/enemy.hpp"
#include "../core/camera.hpp"
#include "../video/animation.hpp"
#include "../user/savegame.hpp"
#include "../core/game_core.hpp"
#include "../core/i18n.hpp"
#include "../level/level_player.hpp"
#include "../level/level_manager.hpp"
#include "../scripting/events/die_event.hpp"

namespace SMC
{

/* *** *** *** *** *** *** cEnemy *** *** *** *** *** *** *** *** *** *** *** */

cEnemy :: cEnemy( cSprite_Manager *sprite_manager )
: cAnimated_Sprite( sprite_manager, "enemy" )
{
	m_sprite_array = ARRAY_ENEMY;
	m_type = TYPE_ENEMY;

	m_camera_range = 1500;

	m_massive_type = MASS_MASSIVE;
	m_state = STA_FALL;
	m_can_be_ground = 1;
	m_dead = 0;
	m_counter = 0.0f;
	m_dying_counter = 0.0f;
	m_color = COL_DEFAULT;

	m_kill_sound = "enemy/furball/die.ogg";
	m_kill_points = 10;

	m_velx_max = 0.0f;
	m_velx_gain = 0.0f;

	m_fire_resistant = 0;
	m_can_be_hit_from_shell = 1;
}

cEnemy :: ~cEnemy( void )
{

}

void cEnemy :: Load_From_Savegame( cSave_Level_Object *save_object )
{
	// state
	if( save_object->exists( "state" ) )
	{
		m_state = static_cast<Moving_state>(string_to_int( save_object->Get_Value( "state" ) ));
	}

	// new position x
	if( save_object->exists( "new_posx" ) )
	{
		Set_Pos_X( string_to_float( save_object->Get_Value( "new_posx" ) ) );
	}

	// new position y
	if( save_object->exists( "new_posy" ) )
	{
		Set_Pos_Y( string_to_float( save_object->Get_Value( "new_posy" ) ) );
	}

	// direction
	if( save_object->exists( "direction" ) )
	{
		m_direction = static_cast<ObjectDirection>(string_to_int( save_object->Get_Value( "direction" ) ));
	}

	// velocity x
	if( save_object->exists( "velx" ) )
	{
		m_velx = string_to_float( save_object->Get_Value( "velx" ) );
	}

	// velocity y
	if( save_object->exists( "vely" ) )
	{
		m_vely = string_to_float( save_object->Get_Value( "vely" ) );
	}

	// active
	if( save_object->exists( "active" ) )
	{
		Set_Active( string_to_int( save_object->Get_Value( "active" ) ) > 0 );
	}

	// dead
	if( save_object->exists( "dead" ) )
	{
		Set_Dead( string_to_int( save_object->Get_Value( "dead" ) ) > 0 );
	}
}

cSave_Level_Object *cEnemy :: Save_To_Savegame( void )
{
	cSave_Level_Object *save_object = new cSave_Level_Object();

	// default values
	save_object->m_type = m_type;
	save_object->m_properties.push_back( cSave_Level_Object_Property( "posx", int_to_string( static_cast<int>(m_start_pos_x) ) ) );
	save_object->m_properties.push_back( cSave_Level_Object_Property( "posy", int_to_string( static_cast<int>(m_start_pos_y) ) ) );



	// state
	save_object->m_properties.push_back( cSave_Level_Object_Property( "state", int_to_string( m_state ) ) );

	// new position ( only save if needed )
	if( !Is_Float_Equal( m_start_pos_x, m_pos_x ) || !Is_Float_Equal( m_start_pos_y, m_pos_y ) )
	{
		save_object->m_properties.push_back( cSave_Level_Object_Property( "new_posx", int_to_string( static_cast<int>(m_pos_x) ) ) );
		save_object->m_properties.push_back( cSave_Level_Object_Property( "new_posy", int_to_string( static_cast<int>(m_pos_y) ) ) );
	}

	// direction
	save_object->m_properties.push_back( cSave_Level_Object_Property( "direction", int_to_string( m_direction ) ) );

	// velocity
	save_object->m_properties.push_back( cSave_Level_Object_Property( "velx", float_to_string( m_velx ) ) );
	save_object->m_properties.push_back( cSave_Level_Object_Property( "vely", float_to_string( m_vely ) ) );

	// active ( only save if needed )
	if( !m_active )
	{
		save_object->m_properties.push_back( cSave_Level_Object_Property( "active", int_to_string( m_active ) ) );
	}

	// dead ( only save if needed )
	if( m_dead )
	{
		save_object->m_properties.push_back( cSave_Level_Object_Property( "dead", int_to_string( m_dead ) ) );
	}

	return save_object;
}

void cEnemy :: Set_Dead( bool enable /* = 1 */ )
{
	m_dead = enable;

	if (m_dead) {
		// Issue the die event
		Scripting::cDie_Event evt;
		evt.Fire(pActive_Level->m_mruby, this);
	}

	Update_Valid_Update();
}

void cEnemy :: Update( void )
{
	cAnimated_Sprite::Update();

	// dying animation
	if( m_dead && m_active )
	{
		Update_Dying();
	}
}

void cEnemy :: Update_Dying()
{
	// Increase dying animation counter
	m_dying_counter += pFramerate->m_speed_factor;

	// By convention, enemies’ DownGrade(true) implementation
	// turns them upside down.
	// FIXME: Use a proper boolean for this.
	if (Is_Float_Equal(m_rot_z, 180.0f))
		Update_Instant_Dying();
	else
		Update_Normal_Dying();
}

void cEnemy :: Update_Normal_Dying()
{
	float speed = pFramerate->m_speed_factor * 0.05f;

	Add_Scale_X( -speed * 0.5f );
	Add_Scale_Y( -speed );

	if( m_scale_y < 0.01f ) {
		Set_Scale(1.0f);
		Set_Active(false);
	}
}

void cEnemy :: Update_Instant_Dying()
{
	// a little bit upwards first
	if( m_dying_counter < 5.0f )
		Move( 0.0f, -5.0f );
	// if not below the ground : fall
	else if( m_col_rect.m_y < pActive_Camera->m_limit_rect.m_y )
		Move( 0.0f, 20.0f );
	// if below disable
	else {
		m_rot_z = 0.0f;
		Set_Active( false );
	}
}

void cEnemy :: Update_Late( void )
{
	// another object controls me
	if( m_state == STA_OBJ_LINKED )
	{
		// todo: have a parent pointer and use that instead of always the player
		Move( pLevel_Player->m_velx, pLevel_Player->m_vely );

		// handle collisions manually
		m_massive_type = MASS_MASSIVE;
		cObjectCollisionType *col_list = Collision_Check( &m_col_rect );
		Add_Collisions( col_list, 1 );
		delete col_list;
		Handle_Collisions();
		m_massive_type = MASS_PASSIVE;
	}
}

void cEnemy :: Update_Velocity( void )
{
	// note: this is currently only useful for walker enemy types
	if( m_direction == DIR_RIGHT )
	{
		if( m_velx < m_velx_max )
		{
			Add_Velocity_X_Max( m_velx_gain, m_velx_max );
			Set_Animation_Speed( m_velx / m_velx_max );
		}
		else if( m_velx > m_velx_max )
		{
			Add_Velocity_X_Min( -m_velx_gain, m_velx_max );
			Set_Animation_Speed( m_velx / m_velx_max );
		}
	}
	else if( m_direction == DIR_LEFT )
	{
		if( m_velx > -m_velx_max )
		{
			Add_Velocity_X_Min( -m_velx_gain, -m_velx_max );
			Set_Animation_Speed( m_velx / -m_velx_max );
		}
		else if( m_velx < -m_velx_max )
		{
			Add_Velocity_X_Max( m_velx_gain, -m_velx_max );
			Set_Animation_Speed( m_velx / -m_velx_max );
		}
	}
}

void cEnemy :: Generate_Hit_Animation( cParticle_Emitter *anim /* = NULL */ ) const
{
	bool create_anim = 0;

	if( !anim )
	{
		create_anim = 1;
		// create animation
		anim = new cParticle_Emitter( m_sprite_manager );
	}

	anim->Set_Emitter_Rect( m_col_rect.m_x, m_pos_y + ( m_col_rect.m_h / 3 ), m_col_rect.m_w );
	anim->Set_Image( pVideo->Get_Surface( "animation/particles/light.png" ) );
	anim->Set_Quota( 4 );
	anim->Set_Pos_Z( m_pos_z - 0.000001f );
	anim->Set_Time_to_Live( 0.3f );
	Color col_rand = Color( static_cast<Uint8>( rand() % 5 ), rand() % 5, rand() % 100, 0 );
	// not bright enough
	/*if( col_rand.red + col_rand.green + col_rand.blue < 250 )
	{
		// boost a random color
		unsigned int rand_color = ( rand() % 3 );

		// yellow
		if( rand_color == 0 )
		{
			col_rand.red = 175;
			col_rand.green = 175;
		}
		// green
		else if( rand_color == 1 )
		{
			col_rand.green = 175;
		}
		// blue
		else
		{
			col_rand.blue = 175;
		}
	}*/
	anim->Set_Color( Color( static_cast<Uint8>(250), 250, 150, 255 ), col_rand );
	anim->Set_Speed( 0.5f, 2.6f );
	anim->Set_Scale( 0.2f, 0.6f );
	anim->Set_Direction_Range( 220, 100 );
	anim->Set_Fading_Alpha( 1 );
	//anim->Set_Fading_Size( 1 );
	anim->Set_Blending( BLEND_ADD );
	
	if( create_anim )
	{
		anim->Emit();
		pActive_Animation_Manager->Add( anim );
	}
}

void cEnemy :: Handle_Collision( cObjectCollision *collision )
{
	if( m_dead )
	{
		return;
	}

	cAnimated_Sprite::Handle_Collision( collision );
}

void cEnemy :: Handle_Collision_Lava( cObjectCollision *p_collision )
{
	if (p_collision->m_direction == DIR_UNDEFINED)
		return;

	// Delegate to cLava::Handle_Collision_Enemy()
	Send_Collision(p_collision);
}

void cEnemy :: Handle_Collision_Massive (cObjectCollision *p_collision )
{
	if (p_collision->m_obj->m_type == TYPE_CRATE)
		Send_Collision(p_collision);
	else
		cAnimated_Sprite::Handle_Collision_Massive(p_collision);
}

void cEnemy :: Handle_out_of_Level( ObjectDirection dir )
{
	// abyss
	if ( dir == DIR_BOTTOM )
	{
		DownGrade(true);
	}

	if( dir == DIR_LEFT )
	{
		Set_Pos_X( pActive_Camera->m_limit_rect.m_x - m_col_pos.m_x );
	}
	else if( dir == DIR_RIGHT )
	{
		Set_Pos_X( pActive_Camera->m_limit_rect.m_x + pActive_Camera->m_limit_rect.m_w - m_col_pos.m_x - m_col_rect.m_w - 0.01f );
	}

	if( dir == DIR_LEFT || dir == DIR_RIGHT )
	{
		Turn_Around( dir );
	}
}

xmlpp::Element* cEnemy :: Save_To_XML_Node( xmlpp::Element* p_element )
{
	return cAnimated_Sprite::Save_To_XML_Node(p_element);
}

std::string cEnemy :: Create_Name() const
{
	std::stringstream ss;
	ss << m_name
	   << " " << _(Get_Direction_Name(m_start_direction).c_str());

	return ss.str();
}

bool cEnemy :: Is_Update_Valid()
{
	if (m_dead || m_freeze_counter)
		return false;

	return true;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC
