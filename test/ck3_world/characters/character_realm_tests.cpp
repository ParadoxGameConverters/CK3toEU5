#include <optional>
#include <sstream>

#include "gtest/gtest.h"
#include "src/ck3_world/characters/character_realm.hpp"

namespace ck3
{

TEST(CK3WorldCharactersRealmTests, CharacterRealmDefaultsToEmpty)  // NOLINT - readability-function-cognitive-complexity
{
   std::stringstream input;
   const CharacterRealm character_realm(input);

   ASSERT_EQ(character_realm.GetVassalPower(), std::nullopt);
   ASSERT_EQ(character_realm.GetRealmCapital().GetID(), -1);
   ASSERT_TRUE(character_realm.GetCourtLanguage().empty());
   ASSERT_EQ(character_realm.GetCouncil().size(), 0);
   ASSERT_EQ(character_realm.GetDomain().size(), 0);
   ASSERT_TRUE(character_realm.GetGovernmentType().empty());
   ASSERT_EQ(character_realm.GetLaws().size(), 0);
}

TEST(CK3WorldCharactersRealmTests, CharacterRealmParsed)  // NOLINT - readability-function-cognitive-complexity
{
   std::stringstream input;
   input << "vassal_power_value = 1337\n";
   input << "laws={ japanese_bureaucracy_2 single_heir_succession_law }\n";
   input << "realm_capital = 13414\n";
   input << "domain={ 13176 13177 13297}\n";
   input << "government=japan_feudal_government\n";
   input << "council={ 16795546 16795548 16795549 33556428 33572763 33574740 }\n";
   input << "royal_court={ language=language_japonic }\n";
   const CharacterRealm character_realm(input);

   ASSERT_EQ(character_realm.GetVassalPower(), 1337);
   ASSERT_EQ(character_realm.GetRealmCapital().GetID(), 13414);
   ASSERT_EQ(character_realm.GetCourtLanguage(), "language_japonic");
   ASSERT_EQ(character_realm.GetCouncil().size(), 6);
   ASSERT_EQ(character_realm.GetDomain().size(), 3);
   ASSERT_EQ(character_realm.GetGovernmentType(), "japan_feudal_government");
   ASSERT_EQ(character_realm.GetLaws().size(), 2);
}

}  // namespace ck3