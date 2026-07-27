#include "src/mappers/LawMapper/LawMapper.h"
#include "gtest/gtest.h"
#include <sstream>

TEST(Mappers_LawMapperTests, valuePositionsComeFromMatchingLaws)
{
	std::stringstream input;
	input << "value = { ck3 = crown_authority_3 key = centralization_vs_decentralization offset = 60 }\n";
	input << "value = { ck3 = crown_authority_0 key = centralization_vs_decentralization offset = -60 }\n";
	const mappers::LawMapper mapper(input);

	const auto positions = mapper.getValuePositions({"crown_authority_3", "some_other_law"});
	ASSERT_EQ(1u, positions.size());
	EXPECT_EQ(60, positions.at("centralization_vs_decentralization"));
}

TEST(Mappers_LawMapperTests, noLawsMeansNoPositions)
{
	std::stringstream input;
	input << "value = { ck3 = crown_authority_3 key = centralization_vs_decentralization offset = 60 }\n";
	const mappers::LawMapper mapper(input);

	EXPECT_TRUE(mapper.getValuePositions({"unrelated_law"}).empty());
}

TEST(Mappers_LawMapperTests, heirSelectionMatchesByFragment)
{
	std::stringstream input;
	input << "heir = { ck3 = primogeniture eu5 = cognatic_primogeniture }\n";
	input << "heir = { ck3 = elective eu5 = oligarchic_elective }\n";
	const mappers::LawMapper mapper(input);

	// Substring match: the realm law contains the fragment.
	EXPECT_EQ("cognatic_primogeniture", *mapper.getHeirSelection({"single_heir_succession_law", "male_primogeniture_law"}));
	EXPECT_EQ("oligarchic_elective", *mapper.getHeirSelection({"feudal_elective_succession_law"}));
	EXPECT_FALSE(mapper.getHeirSelection({"partition_succession_law"}));
}

TEST(Mappers_LawMapperTests, firstFragmentInFileOrderWins)
{
	std::stringstream input;
	input << "heir = { ck3 = primogeniture eu5 = cognatic_primogeniture }\n";
	input << "heir = { ck3 = elective eu5 = oligarchic_elective }\n";
	const mappers::LawMapper mapper(input);

	// A realm with both matching laws follows the earlier link.
	EXPECT_EQ("cognatic_primogeniture", *mapper.getHeirSelection({"primogeniture_law", "elective_law"}));
}
