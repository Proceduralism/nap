#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#include <utility/fileutils.h>
#include <utility/datetimeutils.h>
#include <nap/logger.h>

using namespace nap;

TEST_CASE("File path transformations", "[fileutils]")
{
	// Extensions
	REQUIRE(utility::getFileExtension("/home/test/one.ext") != ".ext");
	REQUIRE(utility::getFileExtension("/home/test/one.ext") == "ext");
	REQUIRE(utility::getFileExtension("poekie.poes.png") == "png");
	REQUIRE(utility::getFileExtension("tommy.toedel.") == "");
	REQUIRE(utility::getFileExtension("file-name.longextension") == "longextension");

//	REQUIRE(utility::getFileDir("/home/someone//filename.ext") == "/home/someone");
	REQUIRE(utility::getFileDir("/home/someone/filename.ext") == "/home/someone");

	REQUIRE(utility::getFileName("/home/someone/filename.ext") == "filename.ext");
	REQUIRE(utility::stripFileExtension("/home/someone/filename.ext") == "/home/someone/filename");
	REQUIRE(utility::getFileNameWithoutExtension("/home/someone/filename.ext") == "filename");

	REQUIRE(utility::appendFileExtension("/cash/cow", "png") == "/cash/cow.png");
	REQUIRE(utility::hasExtension("foo.bar", "bar"));
	REQUIRE(!utility::hasExtension("foo.bar", "ar"));
	REQUIRE(utility::hasExtension("foo.foo.bar", "bar"));
	REQUIRE(!utility::hasExtension("foo.foo.bar", "foo.bar"));

	// TODO: Make more of this sweet stuff

}


TEST_CASE("String utilities", "[stringutils]")
{
	SECTION("splitString")
	{
		{
			auto split = utility::splitString("souffleur", '.');
			REQUIRE(split.size() == 1);
			REQUIRE(split[0] == "souffleur");
		}
		{
			auto split = utility::splitString("one.two.three", '.');
			REQUIRE(split.size() == 3);
			REQUIRE(split[0] == "one");
			REQUIRE(split[1] == "two");
			REQUIRE(split[2] == "three");
		}
		{
			auto split = utility::splitString("one/", '/');
			REQUIRE(split.size() == 1);
			REQUIRE(split[0] == "one");
		}
		{
			auto split = utility::splitString("/", '/');
			REQUIRE(split.size() == 1);
			REQUIRE(split[0] == "");
		}
		{
			auto split = utility::splitString("double//slash", '/');
			REQUIRE(split.size() == 3);
			REQUIRE(split[0] == "double");
			REQUIRE(split[1] == "");
			REQUIRE(split[2] == "slash");
		}
	}
	SECTION("joinString")
	{
		std::vector<std::string> vec;
		REQUIRE(utility::joinString(vec, "?") == "");

		vec = {"one"};
		REQUIRE(utility::joinString(vec, "!?!?") == "one");

		vec = {"the", "highway", "to", "hell"};
		REQUIRE(utility::joinString(vec, "/") == "the/highway/to/hell");
		REQUIRE(utility::joinString(vec, ", ") == "the, highway, to, hell");
		REQUIRE(utility::joinString(vec, "") == "thehighwaytohell");
	}
}

TEST_CASE("DateTime Utilities", "[datetime]")
{
	auto currenttime = utility::getCurrentTime();
	auto flc1_launch_str = "2006-03-24 22:30:01.123";
	auto flc1_launch = utility::createTimestamp(2006, 03, 24, 22, 30, 01, 123);

	utility::DateTime flc1_launch_date(flc1_launch);
	REQUIRE(flc1_launch_date.getYear() == 2006);
	REQUIRE(flc1_launch_date.getMonth() == utility::EMonth::March);
	REQUIRE(flc1_launch_date.getDayInTheMonth() == 24);
	REQUIRE(flc1_launch_date.getHour() == 22);
	REQUIRE(flc1_launch_date.getMinute() == 30);
	REQUIRE(flc1_launch_date.getSecond() == 01);
	REQUIRE(flc1_launch_date.getMilliSecond() == 123);

	REQUIRE(utility::timeFormat(flc1_launch) == flc1_launch_str);
}
