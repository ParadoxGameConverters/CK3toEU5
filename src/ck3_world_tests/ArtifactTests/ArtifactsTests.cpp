#include "src/ck3_world/Artifacts/Artifacts.h"
#include "gtest/gtest.h"
#include <sstream>

TEST(CK3World_ArtifactsTests, emptyInputYieldsNoArtifacts)
{
	std::stringstream input;
	const CK3::Artifacts artifacts(input);

	EXPECT_TRUE(artifacts.getArtifacts().empty());
}

TEST(CK3World_ArtifactsTests, artifactsAreParsedWithVisualsAndOwner)
{
	std::stringstream input;
	input << "artifacts = {\n";
	input << "\t42 = {\n";
	input << "\t\tname = \"The River is Red\"\n";
	input << "\t\trarity = famed\n";
	input << "\t\towner = 999\n";
	input << "\t\tquality = 78\n";
	input << "\t\twealth = 55\n";
	input << "\t\tvisuals = { type = sword }\n";
	input << "\t}\n";
	input << "}\n";
	const CK3::Artifacts artifacts(input);

	ASSERT_EQ(1u, artifacts.getArtifacts().size());
	const auto& artifact = artifacts.getArtifacts()[0];
	EXPECT_EQ(42, artifact.ID);
	EXPECT_EQ("The River is Red", artifact.name);
	EXPECT_EQ("famed", artifact.rarity);
	EXPECT_EQ(999, artifact.owner);
	EXPECT_EQ(78, artifact.quality);
	EXPECT_EQ(55, artifact.wealth);
	EXPECT_EQ("sword", artifact.visualType);
}

TEST(CK3World_ArtifactsTests, descriptionsKeepTheirTextAndLoseTheirMarkup)
{
	std::stringstream input;
	input << "artifacts = {\n";
	input << "\t7 = {\n";
	input << "\t\tname = \"Crown of \x15ONCLICK:TITLE,81 \x15TOOLTIP:LANDED_TITLE,81 \x15L England\x15!\x15!\x15!\"\n";
	input << "\t\tdescription = \"A warbanner of the armies of \x15ONCLICK:TITLE,80 \x15TOOLTIP:LANDED_TITLE,80 \x15L; the Emirate\x15!\x15!\x15!, taken at "
				 "\x15ONCLICK:PROVINCE,4 \x15TOOLTIP:PROVINCE,4 \x15L; Tasagdah\x15!\x15!\x15!.\"\n";
	input << "\t\towner = 999\n";
	input << "\t\tvisuals = { type = tapestry }\n";
	input << "\t}\n";
	input << "}\n";
	const CK3::Artifacts artifacts(input);

	ASSERT_EQ(1u, artifacts.getArtifacts().size());
	const auto& artifact = artifacts.getArtifacts()[0];
	EXPECT_EQ("Crown of England", artifact.name);
	EXPECT_EQ("A warbanner of the armies of the Emirate, taken at Tasagdah.", artifact.description);
}

TEST(CK3World_ArtifactsTests, markupStripperClosesGapsLeftByMissingReferences)
{
	EXPECT_EQ("An elegant shaft inlaid with.", CK3::stripSaveMarkup("An elegant  shaft inlaid with ."));
	EXPECT_EQ("Dedicated \"To My Beloved, Golshan\".", CK3::stripSaveMarkup("Dedicated \\\"To My Beloved, \x15L Golshan\x15!\\\"."));
}

TEST(CK3World_ArtifactsTests, creationDateComesFromTheArtifactHistory)
{
	std::stringstream input;
	input << "artifacts = {\n";
	input << "\t7 = {\n";
	input << "\t\tname = \"Old Seal\"\n";
	input << "\t\towner = 999\n";
	input << "\t\thistory = {\n";
	input << "\t\t\tentries = { { type = inherited date = 1377.1.1 actor = 1 } { type = created date = 1346.8.20 actor = 2 } }\n";
	input << "\t\t}\n";
	input << "\t}\n";
	input << "}\n";
	const CK3::Artifacts artifacts(input);

	ASSERT_EQ(1u, artifacts.getArtifacts().size());
	ASSERT_TRUE(artifacts.getArtifacts()[0].creationDate.has_value());
	EXPECT_EQ(date("1346.8.20"), *artifacts.getArtifacts()[0].creationDate);
}

TEST(CK3World_ArtifactsTests, historyWithoutACreationFallsBackToItsOldestEntry)
{
	std::stringstream input;
	input << "artifacts = {\n";
	input << "\t7 = {\n";
	input << "\t\tname = \"Looted Crown\"\n";
	input << "\t\towner = 999\n";
	input << "\t\thistory = { entries = { { type = given date = 1377.1.1 } { type = claimed date = 1350.2.2 } } }\n";
	input << "\t}\n";
	input << "}\n";
	const CK3::Artifacts artifacts(input);

	ASSERT_EQ(1u, artifacts.getArtifacts().size());
	ASSERT_TRUE(artifacts.getArtifacts()[0].creationDate.has_value());
	EXPECT_EQ(date("1350.2.2"), *artifacts.getArtifacts()[0].creationDate);
}

TEST(CK3World_ArtifactsTests, ownerlessOrNamelessArtifactsAreSkipped)
{
	std::stringstream input;
	input << "artifacts = {\n";
	input << "\t1 = { name = \"Orphan Crown\" }\n";	  // no owner
	input << "\t2 = { owner = 999 quality = 10 }\n"; // no name
	input << "}\n";
	const CK3::Artifacts artifacts(input);

	EXPECT_TRUE(artifacts.getArtifacts().empty());
}
