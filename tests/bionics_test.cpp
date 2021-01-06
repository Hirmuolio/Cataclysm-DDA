#include "catch/catch.hpp"
#include "bionics.h"

#include <climits>
#include <list>
#include <memory>
#include <string>

#include "avatar.h"
#include "item.h"
#include "item_pocket.h"
#include "pimpl.h"
#include "player.h"
#include "player_helpers.h"
#include "ret_val.h"
#include "type_id.h"
#include "units.h"
#include "item_location.h"

static void clear_bionics( player &p )
{
    p.my_bionics->clear();
    p.set_power_level( 0_kJ );
    p.set_max_power_level( 0_kJ );
}

static void test_consumable_charges( player &p, std::string &itemname, bool when_none,
                                     bool when_max )
{
    item it = item( itemname, calendar::turn_zero, 0 );

    INFO( "\'" + it.tname() + "\' is count-by-charges" );
    CHECK( it.count_by_charges() );

    it.charges = 0;
    INFO( "consume \'" + it.tname() + "\' with " + std::to_string( it.charges ) + " charges" );
    REQUIRE( p.can_consume( it ) == when_none );

    it.charges = INT_MAX;
    INFO( "consume \'" + it.tname() + "\' with " + std::to_string( it.charges ) + " charges" );
    REQUIRE( p.can_consume( it ) == when_max );
}

static void test_consumable_ammo( player &p, std::string &itemname, bool when_empty,
                                  bool when_full )
{
    item it = item( itemname, calendar::turn_zero, 0 );

    it.ammo_unset();
    INFO( "consume \'" + it.tname() + "\' with " + std::to_string( it.ammo_remaining() ) + " charges" );
    REQUIRE( p.can_consume( it ) == when_empty );

    if( !it.magazine_default().is_null() ) {
        item mag( it.magazine_default() );
        mag.ammo_set( mag.ammo_default() );
        it.put_in( mag, item_pocket::pocket_type::MAGAZINE_WELL );
    } else if( !it.ammo_default().is_null() ) {
        it.ammo_set( it.ammo_default() ); // fill
    }

    INFO( "consume \'" + it.tname() + "\' with " + std::to_string( it.ammo_remaining() ) + " charges" );
    REQUIRE( p.can_consume( it ) == when_full );
}

TEST_CASE( "bionics", "[bionics] [item]" )
{
    avatar &dummy = get_avatar();
    clear_avatar();

    // one section failing shouldn't affect the rest
    clear_bionics( dummy );

    // Could be a SECTION, but prerequisite for many tests.
    INFO( "no power capacity at first" );
    CHECK( !dummy.has_max_power() );

    dummy.add_bionic( bionic_id( "bio_power_storage" ) );

    INFO( "adding Power Storage CBM only increases capacity" );
    CHECK( !dummy.has_power() );
    REQUIRE( dummy.has_max_power() );

    SECTION( "bio_fuel_cell_gasoline" ) {
        dummy.add_bionic( bionic_id( "bio_fuel_cell_gasoline" ) );

        static const std::list<std::string> always = {
            "gasoline"
        };
        for( std::string it : always ) {
            test_consumable_charges( dummy, it, true, true );
        }

        static const std::list<std::string> never = {
            "light_atomic_battery_cell", // TOOLMOD, no ammo actually
            "rm13_armor"      // TOOL_ARMOR
        };
        for( std::string it : never ) {
            test_consumable_ammo( dummy, it, false, false );
        }
    }

    clear_bionics( dummy );

    SECTION( "bio_batteries" ) {
        dummy.add_bionic( bionic_id( "bio_batteries" ) );

        static const std::list<std::string> always = {
            "battery" // old-school
        };
        for( auto it : always ) {
            test_consumable_charges( dummy, it, true, true );
        }

        static const std::list<std::string> ammo_when_full = {
            "light_battery_cell", // MAGAZINE, NO_UNLOAD
        };
        for( auto it : ammo_when_full ) {
            test_consumable_ammo( dummy, it, false, true );
        }

        static const std::list<std::string> never = {
            "flashlight",  // !is_magazine()
            "laser_rifle", // NO_UNLOAD, uses ups_charges
            "UPS_off"     // NO_UNLOAD, !is_magazine()
        };
        for( auto it : never ) {
            test_consumable_ammo( dummy, it, false, false );
        }
    }

    clear_bionics( dummy );
    // TODO: bio_cable bio_reactor
    // TODO: (pick from stuff with power_source)
}

TEST_CASE( "bionic_power", "[bionics] [item]" )
{

    avatar &dummy = get_avatar();
    clear_avatar();


    SECTION( "bio_power_drain" ) {
        clear_bionics( dummy );
        // Start with power storage
        // Set bionic power to 50 kJ
        // Drain 10.5 kJ power
        // Check that there is now 10.5 kJ less power.

        dummy.add_bionic( bionic_id( "bio_power_storage" ) );
        dummy.set_power_level( 50_kJ );

        // Should have 50 kJ / 100 kJ power.
        CHECK( dummy.get_whole_power_level() == 50_kJ );
        REQUIRE( dummy.get_whole_max_power_level() == 100_kJ );

        dummy.mod_power_level( -10500_J );

        // Should have 89.5 kJ / 100 kJ power.
        CHECK( dummy.get_whole_power_level() == 39500_J );
    }

    SECTION( "bio_power_charge" ) {
        clear_bionics( dummy );
        // Start with power storage
        // Charge 10.5 kJ power
        // Check that there is now 10.5 kJ more power.

        dummy.add_bionic( bionic_id( "bio_power_storage" ) );
        dummy.mod_power_level( 10500_J );

        // Should have 10.5 kJ / 100 kJ power.
        CHECK( dummy.get_whole_power_level() == 10500_J );
    }

    // Battery powered tests done in succesion with same bionics. Just reset the power as needed.

    SECTION( "bio_power_no_battery" ) {
        // Start with power storage
        // Add battery compartment without battery
        // Fill bionic power
        // Should have 50 kJ / 100 kJ power

        clear_bionics( dummy );
        dummy.add_bionic( bionic_id( "bio_power_storage" ) );
        dummy.add_bionic( bionic_id( "bio_battery_compartment" ) );
        dummy.set_power_level( 50_kJ );

        // Should have 100 kJ / 100 kJ power.
        CHECK( dummy.get_whole_power_level() == 50_kJ );
        CHECK( dummy.get_whole_max_power_level() == 100_kJ );
    }

    SECTION( "bio_power_battery" ) {
        clear_bionics( dummy );
        dummy.add_bionic( bionic_id( "bio_power_storage" ) );
        dummy.set_power_level( 50_kJ );
        dummy.add_bionic( bionic_id( "bio_battery_compartment" ) );

        // Just reload the battery in the first item that player is wearing. This works as long as player was naked in the beginning.
        // The battery should be empty.
        item &battery_compartment = dummy.worn.front();
        item &light_battery = dummy.i_add( item( "light_battery_cell" ) );
        battery_compartment.reload( dummy, item_location( dummy, &light_battery ), 1 );

        // Adding the battery increases power capacity but power and bionic power stay.
        CHECK( dummy.get_whole_power_level() == 50_kJ );
        CHECK( dummy.get_power_level() == 50_kJ );
        CHECK( dummy.get_whole_max_power_level() == 200_kJ );
    }
    SECTION( "bio_power_battery_recharge" ) {
        clear_bionics( dummy );
        dummy.add_bionic( bionic_id( "bio_battery_compartment" ) );
        dummy.add_bionic( bionic_id( "bio_power_storage" ) );
        item &battery_compartment = dummy.worn.front();
        item &light_battery = dummy.i_add( item( "light_battery_cell" ) );
        battery_compartment.reload( dummy, item_location( dummy, &light_battery ), 1 );

        dummy.set_power_level( 50_kJ );

        // With not-full bionic the recharge goes into bionic and not battery
        dummy.mod_power_level( 10500_J );
        CHECK( dummy.get_whole_power_level() == 60500_J );
        CHECK( dummy.get_power_level() == 60500_J );

        // Energy levels match with almost full bionic
        dummy.set_power_level( 99_kJ );
        dummy.mod_power_level( 10_kJ );
        CHECK( dummy.get_whole_power_level() == 109_kJ );

        // Energy levels match with full bionic
        dummy.mod_power_level( -1000_kJ ); //set battery and bionic empty
        dummy.set_power_level( 100_kJ );
        dummy.mod_power_level( 10500_J );
        CHECK( dummy.get_whole_power_level() == 110500_J );

        // Filling completely fills completely
        dummy.mod_power_level( 100000_J );
        CHECK( dummy.get_whole_power_level() == 200_kJ );
    }
    SECTION( "bio_power_battery_drain" ) {
        clear_bionics( dummy );
        dummy.add_bionic( bionic_id( "bio_battery_compartment" ) );
        dummy.add_bionic( bionic_id( "bio_power_storage" ) );
        item &battery_compartment = dummy.worn.front();
        item &light_battery = dummy.i_add( item( "light_battery_cell" ) );
        battery_compartment.reload( dummy, item_location( dummy, &light_battery ), 1 );

        dummy.set_power_level( 50_kJ );

        // Fill up bionic and battery
        dummy.mod_power_level( 1000_kJ );

        // Battery should get drained before bionic
        dummy.mod_power_level( -10_kJ );
        CHECK( dummy.get_whole_power_level() == 190_kJ );
        CHECK( dummy.get_power_level() == 100_kJ );

        // Not integer kJ drain
        dummy.mod_power_level( -10500_J );
        CHECK( dummy.get_whole_power_level() == 179500_J );
        CHECK( dummy.get_power_level() == 99500_J );

        // Full battery should work with empty bionic power
        // The bionic power is shuffled around a bit to fit the 0.5 kJ but not more than 2 kJ
        dummy.mod_power_level( 1000_kJ );
        dummy.set_power_level( 0_kJ );
        REQUIRE( dummy.get_whole_power_level() == 100_kJ );

        dummy.mod_power_level( -10500_J );
        CHECK( dummy.get_whole_power_level() == 89500_J );
        CHECK( dummy.get_power_level() < 2_kJ );
    }
}
