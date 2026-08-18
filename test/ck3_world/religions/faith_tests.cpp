#include <sstream>

#include "gtest/gtest.h"
#include "src/ck3_world/religions/faith.hpp"

TEST(CK3WorldFaithTests, FaithIDLoads)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::Faith faith(input, 42);

   ASSERT_EQ(42, faith.GetID());
}

TEST(CK3WorldFaithTests, LoadValuesDefaultToBlank)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::Faith faith(input, 1);

   ASSERT_TRUE(faith.GetTag().empty());
   ASSERT_TRUE(faith.GetDoctrines().empty());
   ASSERT_EQ(faith.GetReligion().GetID(), -1);
   ASSERT_EQ(faith.GetReligionHead().GetID(), -1);
   ASSERT_TRUE(faith.GetCustomName().empty());
   ASSERT_TRUE(faith.GetCustomAdjective().empty());
   ASSERT_TRUE(faith.GetDescription().empty());
   ASSERT_TRUE(faith.GetFaithType().empty());
   ASSERT_TRUE(faith.GetIconPath().empty());
   ASSERT_TRUE(faith.IsReformed());
}

TEST(CK3WorldFaithTests, FaithPrimitivesCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "tag=\"akom_pagan\"\n";
   input << "color=rgb { 229 178 76 }icon=\"i\\c\\on_path.dds\"\n";
   input << "doctrine=\"tenet_adorcism\"\n";
   input << "doctrine=\"doctrine_monogamy\"\n";
   input << "doctrine=\"doctrine_deviancy_shunned\"\n";
   input << "religion = 7\n";
   input << "name = \"Custom Name\"\n";
   input << "adjective = \"Custom Adj\"\n";
   input << "desc = \"Custom Desc\"\n";
   input << "icon = \"gfx/icon.dds\"\n";
   input << "faith_type = daoxue\n";
   input << "religious_head=42\n";
   input << "variables = { \"a bunch of nonsense \nreally\"}";

   const ck3::Faith faith(input, 42);

   ASSERT_EQ("akom_pagan", faith.GetTag());
   ASSERT_EQ(2, faith.GetDoctrines().size());
   ASSERT_EQ(1, faith.GetTenets().size());
   ASSERT_TRUE(faith.GetTenets().contains("tenet_adorcism"));
   ASSERT_TRUE(faith.GetDoctrines().contains("doctrine_monogamy"));
   ASSERT_TRUE(faith.GetDoctrines().contains("doctrine_deviancy_shunned"));
   ASSERT_EQ(7, faith.GetReligion().GetID());
   ASSERT_EQ("Custom Name", faith.GetCustomName());
   ASSERT_EQ("Custom Adj", faith.GetCustomAdjective());
   ASSERT_EQ("Custom Desc", faith.GetDescription());
   ASSERT_EQ("gfx/icon.dds", faith.GetIconPath());
   ASSERT_EQ("daoxue", faith.GetFaithType());
   ASSERT_TRUE(faith.IsReformed());
   ASSERT_EQ(faith.GetReligionHead().GetID(), 42);
}


TEST(CK3WorldFaithTests, UnreformedFaithLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "tag=\"akom_pagan\"\n";
   input << "color=rgb { 229 178 76 }icon=\"i\\c\\on_path.dds\"\n";
   input << "doctrine=\"tenet_adorcism\"\n";
   input << "doctrine=\"doctrine_monogamy\"\n";
   input << "doctrine=\"doctrine_deviancy_shunned\"\n";
   input << "doctrine=west_african_unreformed_faith_doctrine\n";
   input << "religion = 7\n";
   input << "name = \"Custom Name\"\n";
   input << "adjective = \"Custom Adj\"\n";
   input << "desc = \"Custom Desc\"\n";
   input << "icon = \"gfx/icon.dds\"\n";
   input << "faith_type = daoxue\n";
   input << "variables = { \"a bunch of nonsense \nreally\"}";

   const ck3::Faith faith(input, 42);

   ASSERT_EQ("akom_pagan", faith.GetTag());
   ASSERT_FALSE(faith.IsReformed());
}
