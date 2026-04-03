#pragma once

#include "common/pds_node.h"

#include <string_view>

namespace ck3eu5::common {

class PdsParser
{
  public:
	PdsNode parse(std::string_view text);

  private:
	enum class TokenType
	{
		Identifier,
		String,
		Equals,
		LBrace,
		RBrace,
		End
	};

	struct Token
	{
		TokenType type = TokenType::End;
		std::string text;
		size_t line = 1;
		size_t column = 1;
	};

	class Lexer
	{
	  public:
		explicit Lexer(std::string_view input);

		Token next();

	  private:
		void skipWhitespaceAndComments();
		char peek(size_t offset = 0) const;
		char advance();

		std::string_view input_;
		size_t index_ = 0;
		size_t line_ = 1;
		size_t column_ = 1;
	};

	PdsNode parseBlockBody(bool stop_on_rbrace);
	PdsNode parseValue();
	Token consume(TokenType expected, std::string_view message);
	Token peek();
	Token next();

	Lexer* lexer_ = nullptr;
	Token buffered_;
	bool has_buffered_ = false;
};

}  // namespace ck3eu5::common
