#include "src/ck3_world/Wars/Wars.h"
#include "gtest/gtest.h"
#include <sstream>

TEST(CK3World_WarsTests, emptyInputYieldsNoWars)
{
	std::stringstream input;
	const CK3::Wars wars(input);

	EXPECT_TRUE(wars.getWars().empty());
}

TEST(CK3World_WarsTests, activeWarsAreParsedWithSidesAndCasusBelli)
{
	std::stringstream input;
	input << "active_wars = {\n";
	input << "\t0 = {\n";
	input << "\t\tname = \"Conquest of Testland\"\n";
	input << "\t\tstart_date = 1355.3.2\n";
	input << "\t\tattacker = { participants = { { character = 101 } { character = 102 } } }\n";
	input << "\t\tdefender = { participants = { { character = 201 } } }\n";
	input << "\t\tcasus_belli = { type = conquest_cb attacker = 101 defender = 201 }\n";
	input << "\t}\n";
	input << "}\n";
	const CK3::Wars wars(input);

	ASSERT_EQ(1u, wars.getWars().size());
	const auto& war = wars.getWars()[0];
	EXPECT_EQ("Conquest of Testland", war.name);
	EXPECT_EQ(date("1355.3.2"), war.startDate);
	EXPECT_EQ("conquest_cb", war.cbType);
	EXPECT_EQ(101, war.attacker);
	EXPECT_EQ(201, war.defender);
	ASSERT_EQ(2u, war.attackerParticipants.size());
	EXPECT_EQ(102, war.attackerParticipants[1]);
	ASSERT_EQ(1u, war.defenderParticipants.size());
}

TEST(CK3World_WarsTests, warNamesShedTheirMarkupAndTargetedTitlesAreKept)
{
	std::stringstream input;
	input << "active_wars = {\n";
	input << "\t0 = {\n";
	input << "\t\tname = \"\x15ONCLICK:TITLE,4154 \x15TOOLTIP:LANDED_TITLE,4154 \x15L; Kazani\x15!\x15!\x15! Conquest of Elabuga\"\n";
	input << "\t\tcasus_belli = { type = conquest_cb targeted_titles = { 4168 4169 } attacker = 101 defender = 201 }\n";
	input << "\t}\n";
	input << "}\n";
	const CK3::Wars wars(input);

	ASSERT_EQ(1u, wars.getWars().size());
	const auto& war = wars.getWars()[0];
	EXPECT_EQ("Kazani Conquest of Elabuga", war.name);
	ASSERT_EQ(2u, war.targetedTitles.size());
	EXPECT_EQ(4168, war.targetedTitles[0]);
}

TEST(CK3World_WarsTests, concludedNoneSlotsAndPairlessWarsAreSkipped)
{
	std::stringstream input;
	input << "active_wars = {\n";
	input << "\t0 = none\n";
	input << "\t1 = {\n";
	input << "\t\tname = \"Headless War\"\n";
	input << "\t\tcasus_belli = { type = conquest_cb }\n"; // no attacker/defender pair
	input << "\t}\n";
	input << "}\n";
	const CK3::Wars wars(input);

	EXPECT_TRUE(wars.getWars().empty());
}
