%skeleton "lalr1.cc"
%name-prefix="BisonParser"
%defines
%define "parser_class_name" "ArithmeticExpressionParser"
%code requires {
#include <string>
#include <map>
#include <cctype>
#include <cassert>
#include <cmath>

#ifndef BISON_VERSION_1
#define BISON_NAMESPACE BisonParser
#define BISON_LOCATION location
#define BISON_LOCATION_TYPE BISON_NAMESPACE::ArithmeticExpressionParser::location_type
#define BISON_SEMANTIC_TYPE BISON_NAMESPACE::ArithmeticExpressionParser::semantic_type
#define BISON_TOKEN_TYPE    BISON_NAMESPACE::ArithmeticExpressionParser::token_type
#define BISON_TOKEN(t) BISON_NAMESPACE::ArithmeticExpressionParser::token::t
#else
#define BISON_NAMESPACE yy
#define BISON_LOCATION Location
#define BISON_LOCATION_TYPE BISON_NAMESPACE::Location
#define BISON_SEMANTIC_TYPE yystype
#define BISON_TOKEN_TYPE    yytokentype
#define BISON_TOKEN(t) t
#endif

namespace BISON_NAMESPACE {
class BISON_LOCATION;
}

namespace Core
{
    class ArithmeticExpressionParserDriver
    {
    public:
        ArithmeticExpressionParserDriver();

        bool parse(const std::string &s, double &result);
        void error(const BISON_NAMESPACE::BISON_LOCATION &l, const std::string &msg);
        std::string getLastError() const;

        class LexerInput
        {
        private:
            const std::string &str_;
            size_t pos_;

        public:
            LexerInput(const std::string &s);
            char get() const;
            const char *getString() const;
            LexerInput& operator++();
            LexerInput& operator+=(int p);
        };

        LexerInput* getLexerInput() const;
        void setResult(double);

        typedef double(*MathFunc)(double);
        MathFunc getFunction(const std::string &function);
    private:
        LexerInput *input_;
        double result_;
        std::string lastError_;
        std::map<std::string, MathFunc> functions_;
        void initializeFunctions();
    };
}
}

%parse-param { Core::ArithmeticExpressionParserDriver *driver }
%lex-param   { Core::ArithmeticExpressionParserDriver *driver }
%locations
%debug
%error-verbose

%union
{
    double fval;
    Core::ArithmeticExpressionParserDriver::MathFunc func;
};
%{

namespace {
#ifndef BISON_VERSION_1
    int yylex(BISON_SEMANTIC_TYPE *yylval,
              BISON_LOCATION_TYPE *yyloc,
              Core::ArithmeticExpressionParserDriver *driver);
#else
    int yylex(BISON_SEMANTIC_TYPE *yylval,
              BISON_LOCATION_TYPE *yyloc);
    Core::ArithmeticExpressionParserDriver *driver;
    // old bison parsers need global variables :(
#endif
}
%}

%token		END	0  "end of string"
%token <func>	FUNCTION   "function"
%token <fval>	NUMBER	   "number"
%type  <fval>   exp        "expression"

%printer { debug_stream() << $$;  } "number" "expression"

%%
%start unit;
unit: exp  { driver->setResult($1); };

%left '+' '-';
%left '*' '/';
%left NEG;
%right '^';

exp: NUMBER                 { $$ = $1; }
   | FUNCTION '(' exp ')'   { $$ = (*$1)($3); }
   | exp '+' exp            { $$ = $1 + $3; }
   | exp '-' exp            { $$ = $1 - $3; }
   | exp '*' exp            { $$ = $1 * $3; }
   | exp '/' exp            { $$ = $1 / $3; }
   | '-' exp %prec NEG      { $$ = -$2; }
   | exp '^' exp            { $$ = std::pow($1, $3); }
   | '(' exp ')'            { $$ = $2; }
;
%%

// ========================================================================

#ifndef BISON_VERSION_1
void BISON_NAMESPACE::ArithmeticExpressionParser::error(
  const BISON_LOCATION_TYPE &loc,
  const std::string &msg)
{
    driver->error(loc, msg);
}
#else
/* Bison 1.xx declares these functions but never defines them */
void BISON_NAMESPACE::ArithmeticExpressionParser::error_()
{}
void BISON_NAMESPACE::ArithmeticExpressionParser::print_()
{}
#endif


// ========================================================================

#include <cstdio>
#include "StringUtilities.hh"
using namespace Core;

ArithmeticExpressionParserDriver::ArithmeticExpressionParserDriver()
{
    initializeFunctions();
}

void ArithmeticExpressionParserDriver::initializeFunctions()
{
    functions_["log"] = std::log;
    functions_["exp"] = std::exp;
    functions_["sin"] = std::sin;
    functions_["cos"] = std::cos;
    functions_["sqrt"] = std::sqrt;
}

bool ArithmeticExpressionParserDriver::parse(
    const std::string &input, double &result)
{
    input_ = new LexerInput(input);
#ifndef BISON_VERSION_1
    BISON_NAMESPACE::ArithmeticExpressionParser parser(this);
#else
    BISON_LOCATION_TYPE l;
    driver = this;
    BISON_NAMESPACE::ArithmeticExpressionParser parser(false, l, this);
#endif
    bool error (parser.parse() != 0);
    delete input_;
    result = result_;
    return !error;
}

ArithmeticExpressionParserDriver::LexerInput*
ArithmeticExpressionParserDriver::getLexerInput() const {
    return input_;
}

void ArithmeticExpressionParserDriver::setResult(double r) {
    result_ = r;
}

void ArithmeticExpressionParserDriver::error(
    const BISON_NAMESPACE::BISON_LOCATION &loc, const std::string &msg) {
    lastError_ = form("%d-%d: %s", loc.begin.column, loc.end.column, msg.c_str());
}

std::string ArithmeticExpressionParserDriver::getLastError() const {
    return lastError_;
}

ArithmeticExpressionParserDriver::MathFunc
ArithmeticExpressionParserDriver::getFunction(const std::string &function) {
    std::map<std::string, MathFunc>::const_iterator i;
    i = functions_.find(function);
    if (i == functions_.end())
        return 0;
    else
        return i->second;
}


// ========================================================================


ArithmeticExpressionParserDriver::LexerInput::LexerInput(const std::string &s)
    : str_(s), pos_(0) {}

char ArithmeticExpressionParserDriver::LexerInput::get() const {
    return (pos_ < str_.size() ? str_[pos_] : '\0');
}

const char * ArithmeticExpressionParserDriver::LexerInput::getString() const {
    return (pos_ < str_.size() ? str_.c_str() + pos_ : 0);
}

ArithmeticExpressionParserDriver::LexerInput&
ArithmeticExpressionParserDriver::LexerInput::operator++() {
    ++pos_;
    return *this;
}

ArithmeticExpressionParserDriver::LexerInput&
ArithmeticExpressionParserDriver::LexerInput::operator+=(int p) {
    pos_ += p;
    return *this;
}


// ========================================================================

namespace {
#ifndef BISON_VERSION_1
int yylex(BISON_SEMANTIC_TYPE *yylval,
          BISON_LOCATION_TYPE *yyloc,
          ArithmeticExpressionParserDriver *driver)
#else
int yylex(BISON_SEMANTIC_TYPE *yylval,
          BISON_LOCATION_TYPE *yyloc)
#endif
{
    typedef BISON_TOKEN_TYPE token;
    char c;
    ArithmeticExpressionParserDriver::LexerInput *input = driver->getLexerInput();

    while((c = input->get()) == ' ' || c == '\t') {
        ++*input;
        yyloc->columns(1);
    }
    yyloc->step();
    if (c == '\0') {
        return BISON_TOKEN(END);
    }
    if (c == '.' || isdigit(c)) {
        int read;
        sscanf(input->getString(), "%lf%n", &yylval->fval, &read);
        *input += read;
        yyloc->columns(read);
        return BISON_TOKEN(NUMBER);
    }
    if (isalpha(c)) {
        std::string buffer;
        do {
            buffer += input->get();
            ++*input;
        } while (isalnum(input->get()));
        yyloc->columns(buffer.size());
        yylval->func = driver->getFunction(buffer);
        if (yylval->func == 0) {
            driver->error(*yyloc, std::string("unknown function: ") + buffer);
            yylval->fval = 0;
            return BISON_TOKEN(NUMBER);
        }
        return BISON_TOKEN(FUNCTION);
    }
    ++*input;
    yyloc->columns(1);
    return static_cast<BISON_TOKEN_TYPE>(c);
}
}
