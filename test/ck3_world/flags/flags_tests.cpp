#include <sstream>

#include "gtest/gtest.h"
#include "src/ck3_world/flags/flags.hpp"

TEST(CK3WorldFlagsTests, FlagsDefaultToBlank)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::Flags flags(input);

   ASSERT_TRUE(flags.GetFlags().empty());
   ASSERT_TRUE(flags.GetUnavailableDecisionFlags().empty());
}


TEST(CK3WorldFlagsTests, FlagsCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "data = {\n";
   input << "{ flag = seljuk_invasion_happened data = {type = boolean identity = 1} }\n";
   input << "{ flag = west_francia_renamed data = {type = boolean identity = 1} }\n";
   input << "}\n";

   const ck3::Flags flags(input);

   ASSERT_EQ(2, flags.GetFlags().size());
   ASSERT_EQ(1, flags.GetFlags().count("seljuk_invasion_happened"));
   const auto flag = flags.GetFlags().at("seljuk_invasion_happened");
   ASSERT_EQ("boolean", flag.GetType());
   ASSERT_EQ("1", flag.GetValue());
   ASSERT_EQ(1, flags.GetFlags().count("west_francia_renamed"));
}

TEST(CK3WorldFlagsTests, DecisionFlagsCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "list = {{\n";
   input << "name = \"unavailable_unique_decisions\"\n";
   input << " item = { type = flag flag = \"flag_formed_normandy_decision\"}\n";
   input << "item = {type = flag flag = \"flag_formed_kingdom_of_aragon\"}\n";
   input << "item = {type = flag flag = \"form_portugal_decision\"}\n";
   input << "item = {type = flag flag = \"flag_rebuked_roman_claim_to_sicily\"}\n";
   input << "duration = {5}\n";
   input << "}}\n";

   const ck3::Flags flags(input);

   ASSERT_EQ(4, flags.GetUnavailableDecisionFlags().size());
   ASSERT_EQ(1, flags.GetUnavailableDecisionFlags().count("flag_formed_normandy_decision"));
   ASSERT_EQ(1, flags.GetUnavailableDecisionFlags().count("form_portugal_decision"));
}

TEST(CK3WorldFlagsTests, OtherListsDontLoadDecisions)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "list = {{\n";
   input << "name = \"random_name\"\n";
   input << " item = { type = flag flag = \"flag_formed_normandy_decision\"}\n";
   input << "item = {type = flag flag = \"flag_formed_kingdom_of_aragon\"}\n";
   input << "item = {type = flag flag = \"form_portugal_decision\"}\n";
   input << "item = {type = flag flag = \"flag_rebuked_roman_claim_to_sicily\"}\n";
   input << "duration = {5}\n";
   input << "}}\n";

   const ck3::Flags flags(input);

   ASSERT_EQ(0, flags.GetUnavailableDecisionFlags().size());
}
