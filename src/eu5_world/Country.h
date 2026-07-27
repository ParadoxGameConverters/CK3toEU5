#ifndef EU5_COUNTRY_H
#define EU5_COUNTRY_H
#include "Color.h"
#include "Date.h"
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace CK3
{
class CoatOfArms;
}

namespace EU5
{
struct ConvertedDynasty
{
	std::string key;		// dynasty db key, e.g. conv_dynasty_123
	std::string rawName; // display name for localization
	std::string home;		// EU5 location
	std::shared_ptr<CK3::CoatOfArms> coa; // the CK3 dynasty arms, written as the EU5 dynasty coat of arms
};

struct ConvertedCharacter
{
	std::string key;		 // character db key, e.g. conv_char_123_TAG
	std::string nameKey;	 // localization key for the first name
	std::string rawName;	 // actual first name for localization
	bool female = false;
	date birthDate = date("1.1.1");
	std::optional<date> deathDate; // set for dead relatives so EU5 shows them as dynasty history
	std::string culture;	 // EU5 culture
	std::string religion; // EU5 religion
	std::string birthLocation;
	std::string dynastyKey;
	std::string fatherKey; // keys of relatives also exported for the same country
	std::string motherKey;
	std::string spouseKey;
	int adm = 50;
	int dip = 50;
	int mil = 50;
	std::vector<std::string> rulerTraits; // EU5 ruler traits mapped from CK3 personality traits
	std::string estate;						 // EU5 estate the character belongs to, from their CK3 station
	std::string generalTrait;				 // set for the ruler's knights, who arrive as army commanders
	std::string tag;
};

// A past holder of the CK3 title, written into the country's ruler_term history so the dynasty
// arrives in EU5 with the generations of rule it earned.
struct PastReign
{
	std::string characterKey;
	date startDate;
	date endDate;
	int regnalNumber = 0;
};

// A converted work of art (from a CK3 artifact) shown at the country's capital.
struct ConvertedArtwork
{
	std::string artType;			  // EU5 work-of-art type: painting/scripture/regalia/statue/poem/chronicle
	std::string key;				  // localization key
	std::string rawName;			  // display name for localization
	std::string rawDescription; // the tooltip text shown when the work is inspected
	std::string location;
	date creationDate;
	int quality = 40;
};

class Country
{
  public:
	Country() = default;

	std::string tag;
	std::string ck3Title;
	std::string displayName;
	std::string adjective;
	std::optional<commonItems::Color> color;
	std::string capital;							  // EU5 location
	std::vector<std::string> locations;			  // owned EU5 locations (capital included)
	std::set<std::string> nonDeJureLocations; // owned locations outside the realm's CK3 de jure land
	// Owned locations whose CK3 de jure crown is still worn by another independent realm, keyed to
	// that claimant's character ID. Land somebody else has not stopped calling theirs.
	std::map<std::string, long long> disputedLocations;
	// How firmly the realm holds each location. Everything not listed here converts as own_control_core;
	// these two sets carry the land it rules as a distinct possession or as a fresh conquest.
	std::set<std::string> integratedLocations;
	std::set<std::string> conqueredLocations;
	std::set<std::string> coreClaims; // foreign locations the ruler holds CK3 claims on, written as our_cores_conquered_by_others
	std::string religion;						  // EU5 religion
	std::string religiousSchool;				  // Muslim/dharmic theological school, where the faith has one
	std::string culture;							  // EU5 culture
	std::vector<std::string> acceptedCultures;  // cultures holding a real share of the realm's land
	std::vector<std::string> toleratedCultures; // minorities present but too small to be accepted
	std::string rank;								  // rank_county/rank_duchy/rank_kingdom/rank_empire
	std::string governmentType = "monarchy"; // EU5 government type: monarchy/republic/theocracy/tribe/steppe_horde
	std::string templateInclude;				  // setup template to include for sane government defaults
	std::string parliamentType;				  // parliament_type matching the template
	std::string heirSelection;					  // heir_selection, from CK3 succession laws or the template default
	std::string unitCategory = "feudal";	  // army composition category for the unit mapper
	std::map<std::string, int> maaUnits;	  // EU5 unit type -> men, from the realm's CK3 men-at-arms
	int techLevel = 3;							  // starting_technology_level
	std::string courtLanguage;					  // EU5 language of the primary culture
	std::string liturgicalLanguage;			  // EU5 language of the state religion
	std::map<std::string, int> societalValues; // societal value positions from CK3 laws, override the template
	std::optional<ConvertedCharacter> ruler;
	int treasury = 0;					 // starting gold from the CK3 ruler's personal treasury
	std::optional<date> reignStart; // when the ruler took the CK3 title, written as their ruler_term
	int regnalNumber = 0;			 // how many earlier holders of this title shared the ruler's name
	std::vector<PastReign> pastReigns;			  // earlier holders of the CK3 title, oldest first
	std::map<std::string, int> regnalNames;	  // EU5 name_* key -> reigns held under it, so the next Henry is numbered right
	std::string aiPersonality;			 // EU5 ai_personality picked from the ruler's CK3 traits
	std::optional<ConvertedCharacter> consort;
	std::optional<ConvertedCharacter> heir;
	std::optional<ConvertedDynasty> dynasty;
	std::vector<ConvertedCharacter> family;	 // the ruler's family tree (ruler included), parents always before children
	std::vector<ConvertedCharacter> courtiers; // knights and councillors serving the ruler, available for command and office
	std::vector<ConvertedArtwork> artworks; // CK3 artifacts owned by the court, displayed at the capital
	std::shared_ptr<CK3::CoatOfArms> coa; // the CK3 coat of arms, exported as the EU5 flag
};
} // namespace EU5

#endif // EU5_COUNTRY_H
