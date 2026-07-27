#include "src/ck3_world/Religions/Faith.h"
#include "src/ck3_world/Religions/Faiths.h"
#include "src/ck3_world/Religions/Religion.h"
#include "src/ck3_world/Religions/Religions.h"
#include "src/ck3_world/Titles/Titles.h"
#include "gtest/gtest.h"
#include <sstream>

TEST(CK3World_FaithsTests, FaithsDefaultToEmpty)
{
	std::stringstream input;
	const CK3::Faiths faiths(input);

	ASSERT_TRUE(faiths.getFaiths().empty());
}

TEST(CK3World_FaithsTests, FaithsCanBeLoaded)
{
	std::stringstream input;
	input << "13={tag=\"old_bon\"}\n";
	input << "15={tag=\"theravada\"}\n";

	const CK3::Faiths faiths(input);
	const auto& h1 = faiths.getFaiths().find(13);
	const auto& h2 = faiths.getFaiths().find(15);

	ASSERT_EQ("old_bon", h1->second->getName());
	ASSERT_EQ("theravada", h2->second->getName());
}

TEST(CK3World_FaithsTests, religionCanBeLinked)
{
	std::stringstream input;
	input << "13={tag=\"old_bon\"\n religion = 1}\n";
	input << "15={tag=\"theravada\"\n religion = 2}\n";
	CK3::Faiths faiths(input);

	std::stringstream input2;
	input2 << "1 = { tag=\"bon_religion\"}\n";
	input2 << "2 = { tag=\"buddhism_religion\"}\n";
	const CK3::Religions religions(input2);
	const CK3::Titles titles;
	faiths.linkReligions(religions, titles);

	const auto& f13 = faiths.getFaiths().find(13);
	const auto& f15 = faiths.getFaiths().find(15);

	ASSERT_EQ("bon_religion", f13->second->getReligion().second->getName());
	ASSERT_EQ("buddhism_religion", f15->second->getReligion().second->getName());
}

TEST(CK3World_FaithsTests, linkingMissingReligionThrowsException)
{
	std::stringstream input;
	input << "13={tag=\"old_bon\"\n religion = 1}\n";
	input << "15={tag=\"theravada\"\n religion = 2}\n";
	CK3::Faiths faiths(input);

	std::stringstream input2;
	input2 << "1 = { tag=\"bon_religion\"}\n";
	const CK3::Religions religions(input2);
	const CK3::Titles titles;

	ASSERT_THROW(faiths.linkReligions(religions, titles), std::runtime_error);
}
