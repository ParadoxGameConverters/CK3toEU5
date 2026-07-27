#include "src/ck3_world/VassalContracts/VassalContracts.h"
#include "gtest/gtest.h"
#include <sstream>

TEST(CK3World_VassalContractsTests, emptyInputYieldsNoContracts)
{
	std::stringstream input;
	const CK3::VassalContracts contracts(input);

	EXPECT_TRUE(contracts.getContractGroups().empty());
}

TEST(CK3World_VassalContractsTests, contractsAreKeyedByVassal)
{
	std::stringstream input;
	input << "database = {\n";
	input << "\t201327865 = {\n";
	input << "\t\tvassal = 50543344\n";
	input << "\t\tliege = 33776559\n";
	input << "\t\tlevels = { 5 0=2 1=2 2=2 }\n";
	input << "\t\tcontract_group = tributary_settled\n";
	input << "\t}\n";
	input << "\t117441786 = {\n";
	input << "\t\tvassal = 244351\n";
	input << "\t\tliege = 50508581\n";
	input << "\t\tcontract_group = celestial_vassal\n";
	input << "\t}\n";
	input << "}\n";
	const CK3::VassalContracts contracts(input);

	ASSERT_EQ(2u, contracts.getContractGroups().size());
	EXPECT_EQ("tributary_settled", contracts.getContractGroup(50543344));
	EXPECT_EQ("celestial_vassal", contracts.getContractGroup(244351));
}

TEST(CK3World_VassalContractsTests, cancelledContractsAndUnknownVassalsYieldNothing)
{
	std::stringstream input;
	input << "database = {\n";
	input << "\t318775681 = none\n";
	input << "}\n";
	const CK3::VassalContracts contracts(input);

	EXPECT_TRUE(contracts.getContractGroups().empty());
	EXPECT_TRUE(contracts.getContractGroup(12345).empty());
}
