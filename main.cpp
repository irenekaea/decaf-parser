#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <boost\spirit\home\x3.hpp>
#include <boost\spirit\include\support_istream_iterator.hpp>
#include <boost\spirit\include\support_multi_pass.hpp>
#include <boost\spirit\include\classic_position_iterator.hpp>
#include <boost\spirit\include\support_extended_variant.hpp>

namespace x3 = boost::spirit::x3;
namespace ascii = x3::ascii;

typedef std::istreambuf_iterator<char> base_iterator_type;
typedef boost::spirit::multi_pass<base_iterator_type> forward_iterator_type;
typedef boost::spirit::classic::position_iterator2<forward_iterator_type> pos_iterator_type;

namespace Variables
{
  std::stringstream ss;
}

struct Printer
{
  Printer(const std::string& str) : m_str(str) {}

  template<typename T>
  void operator()(T& t)
  {
    Variables::ss << x3::_where(t).begin().get_position().line << " ";
    Variables::ss << m_str << " \"" << x3::_attr(t) << "\"\n";
  }

  std::string m_str;
};

namespace Tokens
{
  // Skip white space | skip block comments | skip line comments
  auto Skip = ascii::space | "/*" >> *(x3::char_ - "*/") >> "*/" | "//" >> *(x3::char_ - x3::eol);
  auto AlphaNumUnderscore = x3::alnum | x3::char_('_');

  // Char ---------------------------------------------------------------------------------------------------------------------------------
  auto EscChar = (x3::lit('n') >> x3::attr('\n')) | x3::char_('\\') | (x3::lit('t') >> x3::attr('\t')) | x3::char_('\'') | x3::char_('\"');
  auto EscapeCharacter = x3::lexeme[x3::lit('\\') > EscChar];
  auto CharCharacter = x3::char_ - "\'" - "\\" - "\"" - "\n" - "\t";
  auto CharLiteral = x3::lexeme[x3::lit('\'') >> CharCharacter[Printer("CHARLITERAL")] > x3::lit('\'')]; // Will throw exception
  auto EscapeLiteral = x3::lexeme[x3::lit('\'') >> EscapeCharacter[Printer("CHARLITERAL")] > x3::lit('\'')];
  // --------------------------------------------------------------------------------------------------------------------------------------

  // Integers -----------------------------------------------------------------------------------------------------------------------------
  const x3::rule<class hexa_tag, std::string> HexLiteral = "HexLiteral";
  const auto HexLiteral_def = x3::lexeme[x3::string("0x") > +x3::xdigit];

  const x3::rule<class int_tag, std::string> IntLiteral = "IntLiteral";
  const auto IntLiteral_def = x3::lexeme[+x3::digit];
  // --------------------------------------------------------------------------------------------------------------------------------------

  // Operators
  auto Arithmetic = x3::string("+") | x3::string("-") | x3::string("*") | x3::string("/") | x3::string("%");
  auto Compare = x3::lexeme[x3::string("<=")] | x3::lexeme[x3::string(">=")] | x3::string("<") | x3::string(">");
  auto Equality = x3::lexeme[x3::string("==")] | x3::lexeme[x3::string("!=")];
  auto Conditional = x3::lexeme[x3::string("&&")] | x3::lexeme[x3::string("||")];
  auto OpenCurlyBrace = x3::char_('{')[Printer("OPENCURLYBRACE")];
  auto CloseCurlyBrace = x3::char_('}')[Printer("CLOSECURLYBRACE")];
  auto Comma = x3::char_(',')[Printer("COMMA")];
  auto Semicolon = x3::char_(';')[Printer("SEMICOLON")];;
  auto OpenSquare = x3::char_('[')[Printer("OPENSQUARE")];;
  auto CloseSquare = x3::char_(']')[Printer("CLOSESQUARE")];;
  auto Assign = x3::char_('=')[Printer("ASSIGNOP")];;
  auto OpenBrace = x3::char_('(')[Printer("OPENBRACE")];;
  auto CloseBrace = x3::char_(')')[Printer("CLOSEBRACE")];;


  auto Operators = Arithmetic[Printer("ARITHOP")] | Compare[Printer("RELOP")] | Equality[Printer("EQOP")] | Conditional[Printer("CONDOP")]
    | OpenCurlyBrace | CloseCurlyBrace | Comma | Semicolon | OpenSquare | CloseSquare | Assign | OpenBrace | CloseBrace;
  // --------------------------------------------------------------------------------------------------------------------------------------

  // Strings ------------------------------------------------------------------------------------------------------------------------------
  const x3::rule<class string_tag, std::string> StringLiteral = "StringLiteral";
  const auto StringLiteral_def = x3::lexeme['"' >> *(EscapeCharacter | CharCharacter) > '"'];
  // --------------------------------------------------------------------------------------------------------------------------------------

  // Reserved -----------------------------------------------------------------------------------------------------------------------------
  auto Boolean = x3::lexeme[x3::string("boolean")];
  auto Break = x3::lexeme[x3::string("break")];
  auto Callout = x3::lexeme[x3::string("callout")];
  auto Class = x3::lexeme[x3::string("class")];
  auto Continue = x3::lexeme[x3::string("continue")];
  auto Else = x3::lexeme[x3::string("else")];
  auto False = x3::lexeme[x3::lit("false") >> x3::attr('0')];
  auto For = x3::lexeme[x3::string("for")];
  auto If = x3::lexeme[x3::string("if")];
  auto Int = x3::lexeme[x3::string("int")];
  auto Return = x3::lexeme[x3::string("return")];
  auto True = x3::lexeme[x3::lit("true") >> x3::attr('1')];
  auto Void = x3::lexeme[x3::string("void")];

  auto Reserved = x3::lexeme[(Boolean | Break | Callout | Class | Continue | Else | For | False | If | Int | Return | True | Void) >> !AlphaNumUnderscore];
  const x3::rule<class bool_tag, std::string> BoolLiteral = "BoolLiteral";
  const auto BoolLiteral_def = x3::lexeme[(True | False) >> !AlphaNumUnderscore];
  // --------------------------------------------------------------------------------------------------------------------------------------

  // Identifier ---------------------------------------------------------------------------------------------------------------------------
  const x3::rule<class identifier_tag, std::string> Identifier = "identifier";
  const auto Identifier_def = x3::lexeme[x3::char_('_') >> *(AlphaNumUnderscore)] | x3::lexeme[x3::alpha >> *AlphaNumUnderscore];

  auto Iden = Identifier;
  // --------------------------------------------------------------------------------------------------------------------------------------

  BOOST_SPIRIT_DEFINE(Identifier, HexLiteral, IntLiteral, StringLiteral, BoolLiteral);

  auto Grammer = Operators | HexLiteral[Printer("HEXLITERAL")] | BoolLiteral[Printer("BOOLLITERAL")] | Reserved[Printer("RESERVED")] | Identifier[Printer("IDENTIFIER")] | StringLiteral[Printer("STRINGLITERAL")] |  EscapeLiteral | CharLiteral | StringLiteral | IntLiteral[Printer("INTLITERAL")];
}

template <typename Iterator>
bool Parse(Iterator first, Iterator last)
{
  bool r = x3::phrase_parse(first, last, *(Tokens::Grammer), Tokens::Skip);
  if (first != last)
  {
    throw(x3::expectation_failure<Iterator>(first, "Fail to parse"));
    return false;
  }
  return r;
}

void BeginParsing(base_iterator_type in_begin)
{
  // convert input iterator to forward iterator, usable by spirit parser
  forward_iterator_type fwd_begin =
    boost::spirit::make_default_multi_pass(in_begin);
  forward_iterator_type fwd_end;

  // wrap forward iterator with position iterator, to record the position

  pos_iterator_type position_begin(fwd_begin, fwd_end);
  pos_iterator_type position_end;

  try
  {
    if (Parse(position_begin, position_end))
    {
      Variables::ss.unsetf(std::ios::skipws);
      std::cout << Variables::ss.str() << std::endl;
    }
  }
  catch (const x3::expectation_failure<pos_iterator_type> &e)
  {
    const boost::spirit::classic::file_position_base<std::string>& pos = e.where().get_position();

    std::cout << "Parse Error at file " << pos.file << " line: " << pos.line << " column: " << pos.column << std::endl;
    std::cout << " " << e.where().get_currentline() << std::endl;
    std::cout << std::setw(pos.column) << " " << "^- here\n";
  }
  Variables::ss = std::stringstream(std::string());
}

int main(int argc, char** args)
{
  std::string string;
  std::ifstream in;
  std::stringstream ss;
  base_iterator_type in_begin;
  if (argc < 2)
  {
    while (std::getline(std::cin, string))
    {
      ss = std::stringstream(string);
      ss.unsetf(std::ios::skipws);
      in_begin = ss;

      BeginParsing(in_begin);
    }
  }
  else
  {
    string = args[1];
    // Open file
    in.open(string);
    // Disable skipping of whitespace
    in.unsetf(std::ios::skipws);
    in_begin = in;

    BeginParsing(in_begin);
  }
  

  return 0;
}