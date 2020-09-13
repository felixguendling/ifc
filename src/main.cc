#include <boost/foreach.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/variant/recursive_variant.hpp>

#include <fstream>
#include <iostream>
#include <string>
#include <variant>
#include <vector>

namespace fusion = boost::fusion;
namespace phoenix = boost::phoenix;
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

struct express_type {
  std::string name_;
  std::string type_;
  std::vector<std::string> details_;
};

// clang-format off
BOOST_FUSION_ADAPT_STRUCT(
    express_type,
    (std::string, name_)
    (std::string, type_)
    (std::vector<std::string>, details_)
);
// clang-format on

struct express_schema {
  std::string name_;
  std::vector<express_type> types_;
};

// clang-format off
BOOST_FUSION_ADAPT_STRUCT(
    express_schema,
    (std::string, name_)
    (std::vector<express_type>, types_)
);
// clang-format on

template <typename Iterator>
struct express_grammar : qi::grammar<Iterator, express_schema()> {
  express_grammar() : express_grammar::base_type{express_} {
    using phoenix::at_c;
    using phoenix::push_back;
    using qi::as_string;
    using qi::char_;
    using qi::lexeme;
    using qi::lit;
    using qi::space;
    using namespace qi::labels;

    enum_ = "TYPE "  //
            >> +(char_ - space)[at_c<0>(_val) += _1]  // type name
            >> " = ENUMERATION OF"  //
            >> *space >> '(' >> (as_string[*(char_ - char_(",)") - space)] %
                                 (*space >> ','))[at_c<2>(_val) = _1] >>
            ");"  //
            >> *(char_ - "END_TYPE;")  //
            >> "END_TYPE;";

    type_ = "TYPE "  //
            >> +(char_ - space)[at_c<0>(_val) += _1]  // type name
            >> " = "  //
            >> +(char_ - ';')[at_c<1>(_val) += _1]  // data type
            >> *(char_ - "END_TYPE;")  //
            >> "END_TYPE;";

    schema_ = "SCHEMA "  //
              >> +(char_ - ';')[at_c<0>(_val) += _1]  // schema name
              >> *((enum_[push_back(at_c<1>(_val), _1)] |
                    type_[push_back(at_c<1>(_val), _1)]  //
                    | char_) -  // types
                   "END_SCHEMA")  //
              >> "END_SCHEMA";

    comment_ = "(*" >> *(char_ - "*)") >> "*)";
    express_ = comment_ >> *space >> schema_[_val = _1] >> *space;
  }

  qi::rule<Iterator, express_type()> enum_;
  qi::rule<Iterator, express_type()> type_;
  qi::rule<Iterator, express_schema()> schema_;
  qi::rule<Iterator> comment_;
  qi::rule<Iterator, express_schema()> express_;
};

int main() {
  using boost::spirit::ascii::char_;
  using boost::spirit::ascii::space;
  using boost::spirit::qi::phrase_parse;
  typedef std::string::const_iterator iterator_type;

  std::string str =
      "(*\n"
      "    Comment\n"
      "*)\n\n"
      "SCHEMA IFC2X3;\n"
      "\n"
      "TYPE IfcAbsorbedDoseMeasure = REAL;\n"
      "END_TYPE;\n"
      "\n"
      "TYPE IfcAccelerationMeasure = REAL;\n"
      "END_TYPE;\n"
      "\n"
      "TYPE IfcNormalisedRatioMeasure = IfcRatioMeasure;\n"
      "  WHERE\n"
      "    WR1 : {0.0 <= SELF <= 1.0};\n"
      "END_TYPE;\n"
      "\n"
      "TYPE IfcActionTypeEnum = ENUMERATION OF\n"
      "  (PERMANENT_G\n"
      "  ,VARIABLE_Q\n"
      "  ,EXTRAORDINARY_A\n"
      "  ,USERDEFINED\n"
      "  ,NOTDEFINED);\n"
      "END_TYPE;\n"
      "\n"
      "END_SCHEMA\n";
  std::cout << str << "\n";
  auto iter = str.cbegin();
  auto end = str.cend();

  using boost::spirit::qi::copy;
  auto comment = express_grammar<decltype(iter)>{};

  express_schema schema;
  bool r =
      phrase_parse(iter, end, comment, boost::spirit::ascii::space, schema);

  if (r && iter == end) {
    std::cout << "-------------------------\n";
    std::cout << "Parsing succeeded\n";
    std::cout << "schema: \"" << schema.name_ << "\"\n";
    std::cout << "number of types: " << schema.types_.size() << "\n";
    for (auto const& t : schema.types_) {
      std::cout << "  name=" << t.name_ << ", type=" << t.type_ << "\n";
      for (auto const& d : t.details_) {
        std::cout << "    \"" << d << "\"\n";
      }
    }
    std::cout << "-------------------------\n";
  } else {
    std::string rest(iter, end);
    std::cout << "-------------------------\n";
    std::cout << "Parsing failed\n";
    std::cout << "stopped at: \": " << rest << "\"\n";
    std::cout << "-------------------------\n";
  }
}
