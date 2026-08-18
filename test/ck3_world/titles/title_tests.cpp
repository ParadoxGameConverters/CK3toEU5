#include <sstream>

#include "gtest/gtest.h"
#include "src/ck3_world/titles/title.hpp"

TEST(CK3WorldTitleTests, IdCanBeSet)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::Title title(input, 1);

   ASSERT_EQ(1, title.GetID());
}

TEST(CK3World_TitleTests, loadValuesDefaultToBlank)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::Title title(input, 1);

   ASSERT_TRUE(title.GetKey().empty());
   ASSERT_TRUE(title.GetName().empty());
   ASSERT_FALSE(title.GetCustomName());
   ASSERT_TRUE(title.GetAdjective().empty());
   ASSERT_TRUE(title.GetHistoryGovernment().empty());
   ASSERT_TRUE(title.GetLaws().empty());
   ASSERT_FALSE(title.GetHolder());
   ASSERT_FALSE(title.IsTheocraticLease());
   ASSERT_FALSE(title.IsCountyCapital());
   ASSERT_FALSE(title.IsDuchyCapital());
   ASSERT_FALSE(title.GetCapitalCounty());
   ASSERT_FALSE(title.GetDeFactoLiege());
   ASSERT_FALSE(title.GetDeJureLiege());
   ASSERT_TRUE(title.GetDeJureVassals().empty());
   ASSERT_TRUE(title.GetHeirs().empty());
   ASSERT_TRUE(title.GetClaimants().empty());
   ASSERT_TRUE(title.GetElectors().empty());
   ASSERT_FALSE(title.IsLandless());
}

TEST(CK3WorldTitleTests, PrimitivesCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "key=\"k_grenada\"\n";
   input << "title_name_data={\n";
   input << "name=\"Cuman Empire\"\n";
   input << "adj=\"the \"\n";
   input << "}\n";
   input << "history_government=\"theocratic_government\"\n";
   input << "laws={ law_1 second_law second_law second_law law_succession_3 }\n";
   input << "holder=123\n";
   input << "theocratic_lease=yes\n";
   input << "capital_barony=yes\n";
   input << "duchy_capital_barony=yes\n";
   input << "capital=123\n";
   input << "de_facto_liege=1234\n";
   input << "de_jure_liege=12345\n";
   input << "de_jure_vassals={ 1 2 3 4 5 }\n";
   input << "heir={ 3 4 5 }\n";
   input << "claim={ 7 8 }\n";
   input << "coat_of_arms_id=45\n";
   input << "color={ 1 2 3 }\n";
   input << "landless=yes\n";

   const ck3::Title title(input, 1);

   ASSERT_EQ("k_grenada", title.GetKey());
   ASSERT_EQ("Cuman Empire", title.GetName());
   ASSERT_EQ("the ", title.GetAdjective());
   ASSERT_EQ("theocratic_government", title.GetHistoryGovernment());
   ASSERT_EQ(3, title.GetLaws().size());
   ASSERT_EQ(1, title.GetLaws().count("second_law"));
   ASSERT_TRUE(title.GetHolder().has_value());
   if (title.GetHolder().has_value())
   {
      ASSERT_EQ(123, title.GetHolder()->GetID());
   }
   ASSERT_TRUE(title.IsTheocraticLease());
   ASSERT_TRUE(title.IsCountyCapital());
   ASSERT_TRUE(title.IsDuchyCapital());
   ASSERT_TRUE(title.GetCapitalCounty().has_value());
   if (title.GetCapitalCounty().has_value())
   {
      ASSERT_EQ(123, title.GetCapitalCounty()->GetID());
   }
   ASSERT_TRUE(title.GetDeFactoLiege().has_value());
   if (title.GetDeFactoLiege().has_value())
   {
      ASSERT_EQ(1234, title.GetDeFactoLiege()->GetID());
   }
   ASSERT_TRUE(title.GetDeJureLiege().has_value());
   if (title.GetDeJureLiege().has_value())
   {
      ASSERT_EQ(12345, title.GetDeJureLiege()->GetID());
   }
   ASSERT_EQ(5, title.GetDeJureVassals().size());
   ASSERT_EQ(title.GetDeJureVassals()[0].GetID(), 1);
   ASSERT_EQ(3, title.GetHeirs().size());
   ASSERT_EQ(3, title.GetHeirs()[0].GetID());
   ASSERT_EQ(4, title.GetHeirs()[1].GetID());
   ASSERT_EQ(5, title.GetHeirs()[2].GetID());
   ASSERT_EQ(2, title.GetClaimants().size());
   ASSERT_EQ(7, title.GetClaimants()[0].GetID());
   ASSERT_EQ(8, title.GetClaimants()[1].GetID());
   ASSERT_TRUE(title.IsLandless());
}

// TEST(CK3World_TitleTests, nameCanBeCleanedOfGUIJunk)
//{
//    std::stringstream input;
//    input << "key=\"k_grenada\"\n";
//    input << "name=\"Crusader \x15ONCLICK:TITLE,81 \x15TOOLTIP:LANDED_TITLE,81 \x15L England\x15!\x15!\x15!\"\n";
//    const ck3::Title title(input, 1);
//    ASSERT_EQ("Crusader England", title.GetDisplayName());
//
//    std::stringstream input2;
//    input2 << "key=\"k_grenada\"\n";
//    input2 << "name=\"\x15TOOLTIP:FAITH,catholic \x15L Catholic\x15!\x15! \x15ONCLICK:TITLE,733 "
//              "\x15TOOLTIP:LANDED_TITLE,733 \x15L Lotharingia\x15!\x15!\x15!\"\n";
//    const ck3::Title title2(input2, 1);
//    ASSERT_EQ("Catholic Lotharingia", title2.GetDisplayName());
// }

TEST(CK3WorldTitleTests, SuccessionElectionCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "succession_election={\n";
   input << "\telectors={ 17972 43082 16968 }\n";
   input << "}\n";

   const ck3::Title title(input, 1);

   ASSERT_EQ(3, title.GetElectors().size());
   ASSERT_EQ(17972, title.GetElectors()[0].GetID());
   ASSERT_EQ(43082, title.GetElectors()[1].GetID());
   ASSERT_EQ(16968, title.GetElectors()[2].GetID());
}
