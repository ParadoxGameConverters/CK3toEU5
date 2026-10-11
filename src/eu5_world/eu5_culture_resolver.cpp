#include "eu5_culture_resolver.hpp"

#include <algorithm>
#include <cctype>

#include "src/ck3_world/cultures/culture.hpp"
#include "src/mappers/culture_group_mapper/culture_group_mapper.hpp"
#include "src/mappers/language_mapper/language_mapper.hpp"

namespace
{
// CK3 culture names reach us as save tokens or as player typed text, neither of which is
// necessarily a legal EU5 key.
std::string SanitizeName(const std::string& name)
{
   std::string clean;
   for (const char character: name)
   {
      const auto lowered = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
      if ((lowered >= 'a' && lowered <= 'z') || (lowered >= '0' && lowered <= '9'))
      {
         clean += lowered;
      }
      else if (!clean.empty() && clean.back() != '_')
      {
         clean += '_';
      }
   }
   while (!clean.empty() && clean.back() == '_')
   {
      clean.pop_back();
   }
   return clean;
}
}  // namespace

eu5::CultureResolver::CultureResolver(const GameDefinitions& game_definitions,
    const mappers::CultureGroupMapper& culture_group_mapper,
    const mappers::LanguageMapper& language_mapper):
    game_definitions_(&game_definitions),
    culture_group_mapper_(&culture_group_mapper),
    language_mapper_(&language_mapper)
{
   for (const auto& [name, definition]: game_definitions.GetCultureDefinitions())
   {
      if (definition.language.empty())
      {
         continue;
      }
      for (const auto& group: definition.groups)
      {
         group_language_.try_emplace(group, definition.language);
      }
   }
}

std::vector<std::string> eu5::CultureResolver::GroupsFor(const ck3::Culture& ck3_culture) const
{
   if (culture_group_mapper_ == nullptr)
   {
      return {};
   }

   std::vector<std::string> groups = culture_group_mapper_->GetEU5CultureGroups(ck3_culture.GetHeritage());
   if (groups.empty())
   {
      groups = culture_group_mapper_->GetEU5CultureGroupsForLanguage(ck3_culture.GetLanguage());
   }
   if (groups.empty())
   {
      for (const auto& name_list: ck3_culture.GetNameLists())
      {
         groups = culture_group_mapper_->GetEU5CultureGroupsForNameList(name_list);
         if (!groups.empty())
         {
            break;
         }
      }
   }

   // A group EU5 does not define would be rejected along with the culture that named it.
   if (game_definitions_ != nullptr && game_definitions_->IsLoaded())
   {
      std::erase_if(groups, [this](const std::string& group) {
         return !game_definitions_->HasCultureGroup(group);
      });
   }
   return groups;
}

std::optional<std::string> eu5::CultureResolver::LanguageFor(const ck3::Culture& ck3_culture) const
{
   if (language_mapper_ == nullptr)
   {
      return std::nullopt;
   }

   auto language = language_mapper_->GetEU5Language(ck3_culture.GetLanguage());
   if (!language.has_value())
   {
      for (const auto& name_list: ck3_culture.GetNameLists())
      {
         language = language_mapper_->GetEU5LanguageForNameList(name_list);
         if (language.has_value())
         {
            break;
         }
      }
   }
   if (language.has_value() && game_definitions_ != nullptr && game_definitions_->IsLoaded() &&
       !game_definitions_->HasLanguage(*language))
   {
      return std::nullopt;
   }
   return language;
}

bool eu5::CultureResolver::IsCompatible(const ck3::Culture& ck3_culture, const std::string& eu5_culture) const
{
   if (game_definitions_ == nullptr)
   {
      return true;
   }
   const auto* definition = game_definitions_->GetCultureDefinition(eu5_culture);
   if (definition == nullptr)
   {
      // Not a culture we can reason about. Leave whatever EU5 put there.
      return true;
   }

   const auto groups = GroupsFor(ck3_culture);
   const auto language = LanguageFor(ck3_culture);
   if (groups.empty() && !language.has_value())
   {
      // Nothing maps. Silence is not disagreement, so EU5's own data stands.
      return true;
   }

   if (std::ranges::any_of(groups, [definition](const std::string& group) {
          return std::ranges::find(definition->groups, group) != definition->groups.end();
       }))
   {
      return true;
   }
   return language.has_value() && *language == definition->language;
}

std::string eu5::CultureResolver::Resolve(const ck3::Culture& ck3_culture)
{
   const auto& key = ck3_culture.GetName();
   if (const auto cached = resolved_.find(key); cached != resolved_.end())
   {
      return cached->second;
   }

   // A vanilla CK3 culture keeps its template key; a dynamic one has only its generated name.
   const auto& source_name = ck3_culture.GetTemplate().has_value() ? *ck3_culture.GetTemplate() : key;
   const auto name = SanitizeName(source_name);
   if (name.empty())
   {
      unresolved_.insert(key);
      resolved_.emplace(key, "");
      return {};
   }

   // EU5 already has this culture. Its keys are inconsistent about the suffix - english and
   // armenian_culture are both top level culture keys - so both spellings have to be tried.
   if (game_definitions_ != nullptr)
   {
      for (const auto& candidate: {name, name + "_culture"})
      {
         if (game_definitions_->HasCulture(candidate))
         {
            resolved_.emplace(key, candidate);
            return candidate;
         }
      }
   }

   const auto groups = GroupsFor(ck3_culture);
   if (groups.empty())
   {
      // Without a group the culture cannot be placed, and a definition naming no group would be
      // rejected. EU5's own culture stays.
      unresolved_.insert(key);
      resolved_.emplace(key, "");
      return {};
   }

   auto language = LanguageFor(ck3_culture);
   if (!language.has_value())
   {
      // Borrow the language of an EU5 culture already in the group. Wrong in detail, but it keeps
      // the culture on the map instead of discarding it.
      const auto borrowed = group_language_.find(groups.front());
      if (borrowed == group_language_.end())
      {
         unresolved_.insert(key);
         resolved_.emplace(key, "");
         return {};
      }
      language = borrowed->second;
   }

   // Two CK3 cultures can sanitize to the same key - a campaign may name a hybrid after a culture
   // that already exists elsewhere in the save.
   std::string generated_name = name;
   int suffix = 2;
   while (generated_cultures_.contains(generated_name))
   {
      generated_name = name + "_" + std::to_string(suffix++);
   }

   generated_cultures_.insert_or_assign(generated_name, CultureDefinition{*language, groups});
   resolved_.emplace(key, generated_name);
   return generated_name;
}

void eu5::CultureResolver::MarkUsed(const std::string& eu5_culture)
{
   used_cultures_.insert(eu5_culture);
}

std::map<std::string, eu5::CultureDefinition> eu5::CultureResolver::GetUsedGeneratedCultures() const
{
   std::map<std::string, CultureDefinition> used;
   for (const auto& [name, definition]: generated_cultures_)
   {
      if (used_cultures_.contains(name))
      {
         used.emplace(name, definition);
      }
   }
   return used;
}
