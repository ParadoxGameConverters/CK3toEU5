#include "src/ck3_world/Cultures/Culture.h"
#include "src/ck3_world/Cultures/Cultures.h"
#include "gtest/gtest.h"

TEST(CK3World_CulturesTests, CulturesDefaultToEmpty)
{
	std::stringstream input;
	const CK3::Cultures cultures(input);

	EXPECT_TRUE(cultures.getCultures().empty());
}

TEST(CK3World_CulturesTests, UnbundledCulturesCanBeLoaded)
{
	std::stringstream input;
	input << "13={culture_template=\"akan\"}\n";
	input << "15={culture_template=\"kru\"}\n";

	const CK3::Cultures cultures(input);
	const auto& c1 = cultures.getCultures().find(13);
	const auto& c2 = cultures.getCultures().find(15);

	EXPECT_EQ(2, cultures.getCultures().size());
	EXPECT_EQ("akan", c1->second->getTemplate());
	EXPECT_EQ("kru", c2->second->getTemplate());
}

TEST(CK3World_CulturesTests, BundledCulturesCanBeLoaded)
{
	std::stringstream input;
	input << "cultures={\n";
	input << "\t13={culture_template=\"akan\"}\n";
	input << "\t15={culture_template=\"kru\"}\n";
	input << "}\n";

	const CK3::Cultures cultures(input);
	const auto& c1 = cultures.getCultures().find(13);
	const auto& c2 = cultures.getCultures().find(15);

	EXPECT_EQ(2, cultures.getCultures().size());
	EXPECT_EQ("akan", c1->second->getTemplate());
	EXPECT_EQ("kru", c2->second->getTemplate());
}

TEST(CK3World_CulturesTests, VanillaCultureNamesResolveFromTemplates)
{
	std::stringstream input;
	input << "cultures={\n";
	input << "\t13={culture_template=\"akan\"}\n";
	input << "\t15={culture_template=\"kru\"}\n";
	input << "}\n";

	const CK3::Cultures cultures(input);
	const auto& c1 = cultures.getCultures().find(13);
	const auto& c2 = cultures.getCultures().find(15);

	EXPECT_EQ("akan", c1->second->getName());
	EXPECT_EQ("kru", c2->second->getName());
}
