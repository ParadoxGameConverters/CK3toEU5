#include "src/ck3_world/Relations/Opinions.h"
#include "gtest/gtest.h"
#include <sstream>

TEST(CK3World_OpinionsTests, emptyInputYieldsNoRivals)
{
	std::stringstream input;
	const CK3::Opinions opinions(input);

	EXPECT_TRUE(opinions.getRivalPairs().empty());
}

TEST(CK3World_OpinionsTests, rivalAndNemesisRelationsAreCollected)
{
	std::stringstream input;
	input << "active_opinions = {\n";
	input << "\t{ owner = 1 target = 2 scripted_relations = { rival = { } } }\n";
	input << "\t{ owner = 3 target = 4 scripted_relations = { nemesis = { } } }\n";
	input << "\t{ owner = 5 target = 6 scripted_relations = { friend = { } } }\n";
	input << "}\n";
	const CK3::Opinions opinions(input);

	ASSERT_EQ(2u, opinions.getRivalPairs().size());
	EXPECT_EQ(std::make_pair(1LL, 2LL), opinions.getRivalPairs()[0]);
	EXPECT_EQ(std::make_pair(3LL, 4LL), opinions.getRivalPairs()[1]);
}

TEST(CK3World_OpinionsTests, incompleteEntriesAreSkipped)
{
	std::stringstream input;
	input << "active_opinions = {\n";
	input << "\t{ owner = 1 scripted_relations = { rival = { } } }\n"; // no target
	input << "\t{ target = 2 scripted_relations = { rival = { } } }\n"; // no owner
	input << "}\n";
	const CK3::Opinions opinions(input);

	EXPECT_TRUE(opinions.getRivalPairs().empty());
}
