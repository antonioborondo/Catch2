
#include "catch.hpp"

#include <iostream> // !DBG
#include <numeric>
#include <map>
#include <set>

#include "internal/catch_generators.hpp"

#include "internal/catch_suppress_warnings.h"

namespace Catch {
namespace generators {


} // namespace generators
} // namespace Catch


TEST_CASE("Generators impl") {
    using namespace Catch::generators;

    SECTION( "range" ) {
        auto gen = range(1,3);

        CHECK( gen.size() == 3 );

        CHECK( gen[0] == 1 );
        CHECK( gen[1] == 2 );
        CHECK( gen[2] == 3 );
    }
    SECTION( "fixed values" ) {
        auto gen = values( { 3, 1, 4, 1 } );

        CHECK( gen.size() == 4 );
        CHECK( gen[0] == 3 );
        CHECK( gen[1] == 1 );
        CHECK( gen[2] == 4 );
        CHECK( gen[3] == 1 );
    }
    SECTION( "random range" ) {
        auto gen = random( 3, 8 );

        CHECK( gen.size() == 6 );
        for( size_t i = 0; i < 6; ++i ) {
            CHECK( gen[i] >= 3 );
            CHECK( gen[i] <= 8 );
            if( i > 0 )
                CHECK( gen[i] != gen[i-1] );
        }
    }
    SECTION( "random selection" ) {
        auto gen = random<int>( 10 );

        CHECK( gen.size() == 10 );
        for( size_t i = 0; i < 10; ++i ) {
            if( i > 0 )
                CHECK( gen[i] != gen[i-1] );
        }
    }
    SECTION( "combined" ) {
        auto gen = range( 1, 2 ) << values( { 9, 7 } );

        CHECK( gen.size() == 4 );
        CHECK( gen[0] == 1 );
        CHECK( gen[1] == 2 );
        CHECK( gen[2] == 9 );
        CHECK( gen[3] == 7 );
    }

    SECTION( "values" ) {
        auto gen = NullGenerator() << 3 << 1;

        CHECK( gen.size() == 2 );
        CHECK( gen[0] == 3 );
        CHECK( gen[1] == 1 );
    }

    SECTION( "values first" ) {
        auto gen = 7 << Generator<int>();

        CHECK( gen.size() == 1 );
        CHECK( gen[0] == 7 );
    }

    SECTION( "type erasure" ) {
        auto gen = range( 7, 9 ) << 11;

        // Make type erased version
        auto dynCopy = pf::make_unique<Generator<int>>( std::move( gen ) );
        std::unique_ptr<GeneratorBase const> base = std::move( dynCopy );

        // Only thing we can do is ask for the size
        CHECK( base->size() == 4 );

        // Restore typed version
        auto typed = dynamic_cast<Generator<int> const*>( base.get() );
        REQUIRE( typed );
        CHECK( typed->size() == 4 );
        CHECK( (*typed)[0] == 7 );
        CHECK( (*typed)[3] == 11 );
    }

    SECTION( "memoized" ) {
        GeneratorCache cache;

        auto lineInfo = CATCH_INTERNAL_LINEINFO;

        int created = 0;
        auto fun = [&]{
            created++;
            return values({42, 7});
        };

        // generator is only created on first call
        CHECK( created == 0 );
        CHECK( memoize( cache, lineInfo, fun )[0] == 42 );
        CHECK( created == 1 );
        CHECK( memoize( cache, lineInfo, fun )[0] == 42 );
        CHECK( created == 1 );
        CHECK( memoize( cache, lineInfo, fun )[1] == 7 );
        CHECK( created == 1 );
    }

    SECTION( "strings" ) {
        GeneratorCache cache;
        auto const& gen = memoize( cache, CATCH_INTERNAL_LINEINFO, []{ return values({ "one", "two", "three", "four" } ); }  );

        REQUIRE( gen.size() == 4 );
        CHECK( gen[0] == "one" );
        CHECK( gen[1] == "two" );
        CHECK( gen[2] == "three" );
        CHECK( gen[3] == "four" );
    }
}

#define GENERATE( ... ) Catch::generators::generate( CATCH_INTERNAL_LINEINFO, []{ using namespace Catch::generators; return NullGenerator() << __VA_ARGS__; } )

TEST_CASE("Generators") {

    auto i = GENERATE( values( { "a", "b", "c" } ) );

    SECTION( "one" ) {
        auto j = GENERATE( range( 8, 11 ) << 2 );
        std::cout << "one: " << i << ", " << j << std::endl;
    }
    SECTION( "two" ) {
        auto j = GENERATE( 3.141 << 1.379 );
        std::cout << "two: " << i << ", " << j << std::endl;
    }
}

TEST_CASE( "200 ints" ) {
    auto x = GENERATE( range( 0,100 ) );
    auto y = GENERATE( range( 200,300 ) );

    CHECK( x < y );
}

#ifdef __cpp_structured_bindings
TEST_CASE( "strlen" ) {
    auto [test_input, expected] = GENERATE( values<std::pair<std::string_view, int>>({
            {"one", 3},
            {"two", 3},
            {"three", 5},
            {"four", 4}
        }));

    REQUIRE( test_input.size() == expected );
}

TEST_CASE( "strlen2" ) {
    auto [test_input, expected] = GENERATE( table<std::string, int>({
            {"one", 3},
            {"two", 3},
            {"three", 5},
            {"four", 4}
        }));

    REQUIRE( test_input.size() == expected );
}
#endif

TEST_CASE( "strlen3" ) {
    struct Data { std::string str; int len; };
    auto data = GENERATE( values<Data>({
            {"one", 3},
            {"two", 3},
            {"three", 5},
            {"four", 4}
        }));

    REQUIRE( data.str.size() == data.len );
}


auto square( int i ) -> int { return i*i; }

TEST_CASE( "sqr" ) {
    auto x = GENERATE( random( -10000, 10000 ) );
    CAPTURE( x );
    REQUIRE( square(x) >= 0 );
}

#ifdef __cpp_structured_bindings
// Based on example from https://docs.cucumber.io/gherkin/reference/#scenario-outline
// (thanks to https://github.com/catchorg/Catch2/issues/850#issuecomment-399504851)
auto eatCucumbers( int start, int eat ) -> int { return start-eat; }

SCENARIO("Eating cucumbers") {

    auto [start, eat, left] = GENERATE( table<int,int,int> ({
            { 12, 5, 7 },
            { 20, 5, 15 }
        }));

    GIVEN( "there are " << start << " cucumbers" )
    WHEN( "I eat " << eat << " cucumbers" )
    THEN( "I should have " << left << " cucumbers" ) {
        REQUIRE( eatCucumbers( start, eat ) == left );
    }
}
#endif
