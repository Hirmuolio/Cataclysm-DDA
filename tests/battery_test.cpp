#include "cata_catch.h"
#include "item.h"


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
