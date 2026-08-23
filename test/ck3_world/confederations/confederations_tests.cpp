#include <sstream>

#include "gtest/gtest.h"
#include "src/ck3_world/confederations/confederations.hpp"


TEST(CK3WorldConfederationsTests, ConfederationsDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::Confederations confederations(input);

   EXPECT_TRUE(confederations.GetConfederations().empty());
}

TEST(CK3WorldConfederationsTests, ConfederationsCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;

   input << "database={\n";
   input << "		0={\n";
   input << "			members={ 16787777 9525 9944 33566183 16795849 16818520 }\n";
   input << "			name=\"Irish Confederation\"\n";
   input << "			color=rgb { 49 249 72 }\n";
   input << "			coat_of_arms=17763\n";
   input << "			variables={\n";
   input << "				data={ {\n";
   input << "						flag=confederation_culture\n";
   input << "						data={\n";
   input << "							type=culture\n";
   input << "							identity=65\n";
   input << "						}\n";
   input << "					}\n";
   input << "}\n";
   input << "			}\n";
   input << "		}\n";
   input << "		16777217={\n";
   input << "			members={ 39303 26333 33589596 10062 40945 16807097 46100 50346833 }\n";
   input << "			name=\"Khazar Confederation\"\n";
   input << "			color=rgb { 210 160 69 }\n";
   input << "			coat_of_arms=31944\n";
   input << "			variables={\n";
   input << "				data={ {\n";
   input << "						flag=confederation_culture\n";
   input << "						data={\n";
   input << "							type=culture\n";
   input << "							identity=157\n";
   input << "						}\n";
   input << "					}\n";
   input << " }\n";
   input << "			}\n";
   input << "}\n";
   input << "		16777218=none\n";
   input << "}\n";

   const ck3::Confederations confederations(input);
   const auto& confederation1 = confederations.GetConfederations().find(0);
   const auto& confederation2 =
       confederations.GetConfederations().find(16777217);  // NOLINT(readability-magic-numbers) : "magic number"

   EXPECT_EQ(2, confederations.GetConfederations().size());
   EXPECT_EQ("Irish Confederation", confederation1->second->GetName());
   EXPECT_EQ("Khazar Confederation", confederation2->second->GetName());
}
