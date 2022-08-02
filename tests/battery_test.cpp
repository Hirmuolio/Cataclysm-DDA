#include "avatar.h"
#include "cata_catch.h"
#include "item.h"
#include "player_helpers.h"


TEST_CASE( "act as battery", "[battery]" )
{
    // Items that can hold battery power should act as batteries

    SECTION( "battery" ) {
        item tested_item( "medium_battery_cell" );
        CHECK( tested_item.act_as_battery() == true );
    }

    SECTION( "tool armor" ) {
        item tested_item( "powered_earplugs_on" );
        CHECK( tested_item.act_as_battery() == true );
    }

    SECTION( "tool" ) {
        item tested_item( "smart_phone" );
        CHECK( tested_item.act_as_battery() == true );
    }

    SECTION( "stethoscope is not a battery" ) {
        item tested_item( "stethoscope" );
        CHECK( tested_item.act_as_battery() == false );
    }
}

TEST_CASE( "ups power", "[battery]" )
{
    Character &they = get_player_character();
    clear_character( they );
    they.worn.wear_item( they, item( "debug_backpack" ), false, false );

    item_location bat_cell = they.i_add( item( "heavy_battery_cell" ) );
    item_location ups = they.i_add( item( "UPS_off" ) );



    SECTION( "Everything starts as empty" ) {
        REQUIRE( ups->put_in( *bat_cell, item_pocket::pocket_type::MAGAZINE_WELL ).success() );

        CHECK( ups->energy_remaining() == 0_kJ );
        CHECK( bat_cell->energy_remaining() == 0_kJ );
        CHECK( they.available_ups() == 0_kJ );
    }

    SECTION( "UPS gets energy from battery" ) {
        units::energy bat_charges = 13_kJ;
        bat_cell->mod_energy( bat_charges );
        REQUIRE( ups->put_in( *bat_cell, item_pocket::pocket_type::MAGAZINE_WELL ).success() );

        CHECK( ups->energy_remaining() == bat_charges );
        CHECK( they.available_ups() == bat_charges );
    }
}
