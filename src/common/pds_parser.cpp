#include "common/pds_parser.h"

#include "common/string_utils.h"

#include <cctype>
#include <sstream>
#include <stdexcept>

namespace ck3eu5::common {

PdsParser::Lexer::Lexer(std::string_view input): input_(input) {}

char PdsParser::Lexer::peek(const size_t offset) const
{
	if (index_ + offset >= input_.size())
	{
		return '\0';
	}
	return input_[index_ + offset];
}

char PdsParser::Lexer::advance()
{
	const char current = peek();
	if (current == '\0')
	{
		return '\0';
	}
	++index_;
	if (current == '\n')
	{
		++line_;
		column_ = 1;
	}
	else
	{
		++column_;
	}
	return current;
}

void PdsParser::Lexer::skipWhitespaceAndComments()
{
	while (true)
	{
		while (std::isspace(static_cast<unsigned char>(peek())))
		{
			advance();
		}
		if (peek() == '#')
		{
			while (peek() != '\0' && peek() != '\n')
			{
				advance();
			}
			continue;
		}
		break;
	}
}

PdsParser::Token PdsParser::Lexer::next()
{
	skipWhitespaceAndComments();
	Token token;
	token.line = line_;
	token.column = column_;

	const char current = peek();
	if (current == '\0')
	{
		token.type = TokenType::End;
		return token;
	}

	if (current == '{')
	{
		token.type = TokenType::LBrace;
		token.text = "{";
		advance();
		return token;
	}
	if (current == '}')
	{
		token.type = TokenType::RBrace;
		token.text = "}";
		advance();
		return token;
	}
	if (current == '=')
	{
		token.type = TokenType::Equals;
		token.text = "=";
		advance();
		return token;
	}
	if (current == '"')
	{
		token.type = TokenType::String;
		advance();
		while (true)
		{
			const char c = peek();
			if (c == '\0')
			{
				throw std::runtime_error("Unterminated quoted string in PDS input");
			}
			if (c == '"')
			{
				advance();
				break;
			}
			if (c == '\\')
			{
				advance();
				const char escaped = peek();
				if (escaped == '\0')
				{
					throw std::runtime_error("Unterminated escape sequence in PDS input");
				}
				token.text.push_back(advance());
				continue;
			}
			token.text.push_back(advance());
		}
		return token;
	}

	token.type = TokenType::Identifier;
	while (true)
	{
		const char c = peek();
		if (c == '\0' || std::isspace(static_cast<unsigned char>(c)) || c == '=' || c == '{' || c == '}' || c == '#')
		{
			break;
		}
		token.text.push_back(advance());
	}
	return token;
}

PdsParser::Token PdsParser::peek()
{
	if (!has_buffered_)
	{
		buffered_ = lexer_->next();
		has_buffered_ = true;
	}
	return buffered_;
}

PdsParser::Token PdsParser::next()
{
	const auto token = peek();
	has_buffered_ = false;
	return token;
}

PdsParser::Token PdsParser::consume(const TokenType expected, std::string_view message)
{
	const auto token = next();
	if (token.type != expected)
	{
		std::ostringstream error;
		error << message << " at line " << token.line << ", column " << token.column;
		throw std::runtime_error(error.str());
	}
	return token;
}

PdsNode PdsParser::parse(std::string_view text)
{
	Lexer lexer(text);
	lexer_ = &lexer;
	has_buffered_ = false;
	return parseBlockBody(false);
}

PdsNode PdsParser::parseValue()
{
	const auto token = peek();
	if (token.type == TokenType::LBrace)
	{
		next();
		return parseBlockBody(true);
	}
	if (token.type == TokenType::Identifier || token.type == TokenType::String)
	{
		return PdsNode::makeScalar(next().text);
	}

	std::ostringstream error;
	error << "Unexpected token while parsing value at line " << token.line << ", column " << token.column;
	throw std::runtime_error(error.str());
}

PdsNode PdsParser::parseBlockBody(const bool stop_on_rbrace)
{
	PdsNode node = PdsNode::makeBlock();

	while (true)
	{
		const auto token = peek();
		if (token.type == TokenType::End)
		{
			if (stop_on_rbrace)
			{
				throw std::runtime_error("Unexpected end of PDS input; missing closing brace");
			}
			break;
		}
		if (token.type == TokenType::RBrace)
		{
			if (!stop_on_rbrace)
			{
				throw std::runtime_error("Unexpected closing brace in PDS input");
			}
			next();
			break;
		}
		if (token.type == TokenType::LBrace)
		{
			next();
			node.addItem(parseBlockBody(true));
			continue;
		}
		if (token.type == TokenType::Identifier || token.type == TokenType::String)
		{
			const auto key_or_value = next();
			if (peek().type == TokenType::Equals)
			{
				next();
				node.addProperty(key_or_value.text, parseValue());
			}
			else
			{
				node.addItem(PdsNode::makeScalar(key_or_value.text));
			}
			continue;
		}

		std::ostringstream error;
		error << "Unexpected token in PDS input at line " << token.line << ", column " << token.column;
		throw std::runtime_error(error.str());
	}

	return node;
}

}  // namespace ck3eu5::common
