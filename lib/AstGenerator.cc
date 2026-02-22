#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace ccomp {
// source:
// https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
static std::vector<std::string> split(const std::string& str,
                                      const std::string& delim) {
  std::vector<std::string> tokens;
  size_t prev = 0, pos = 0;
  do {
    pos = str.find(delim, prev);
    if (pos == std::string::npos)
      pos = str.length();
    auto token = str.substr(prev, pos - prev);
    if (!token.empty()) {
      tokens.push_back(token);
    }
    prev = pos + delim.length();
  } while (pos < str.length() && prev < str.length());
  return tokens;
}

// Trim from the start (in place)
std::string left_trim(const std::string& str) {
  auto s = std::string(str);
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
      return !std::isspace(ch);
  }));
  return s;
}

std::string strip_whitespace(const std::string& str) {
  auto s = std::string(str);
  s.erase(remove_if(s.begin(), s.end(), isspace), s.end());
  return s;
}

struct AstSpecification {
 std::string name;
 std::vector<
  std::tuple<std::string, // name
  std::string, // required fields
  std::string>> // fields populated during semantic analysis or parsing
  types;
 // enums
 std::vector<
    std::tuple<std::string,
        std::vector<std::string>>>
    enums;
 // headers to include
 std::vector<std::string> headers;
};

class AstGen {
public:
  AstGen(const std::string &aDir, const AstSpecification aSpec)
      : outDir(aDir), astSpec(aSpec) {}
  void generate() {
    defineAst();
  }
  void defineAst() {
    auto baseName = astSpec.name;
    auto path = outDir + "/" + baseName + ".h";
    std::ofstream file(path);
    if (!file.is_open()) {
      std::cout << "Unable to open file." << std::endl;
      return;
    }

    /// #ifndef guard
    file << "#ifndef " + baseName + "_H_" << std::endl;
    file << "#define " + baseName + "_H_" << '\n' << '\n';

    // header includes
    for (auto& header : astSpec.headers) {
      file << "#include " << header << '\n';
    }

    // start namespace
    file << "\nnamespace ccomp {" << '\n';

    // forward declarations
    for (auto& type : astSpec.types) {
      auto className = std::get<0>(type);
      file << "class " << className << ";\n";
    }

    // variant specification
    file << "using " << baseName << " = std::variant<";
    for (auto type = astSpec.types.begin(); type != astSpec.types.end(); ++type) {
      auto className = std::get<0>(*type);
      file << strip_whitespace(className);
      if ((type + 1) == astSpec.types.end()) {
        file << ">;\n";
      } else {
        file << ", ";
      }
    }

    // enum definitions
    for (auto& anenum : astSpec.enums) {
      file << "enum " << baseName << std::get<0>(anenum) << " {\n";
      for (auto& field : std::get<1>(anenum)) {
        file << "  " << field << ",\n";
      }
      file << "};\n";
    }

    // Derived concrete classes
    for (auto type : astSpec.types) {
      auto className = std::get<0>(type);
      auto fields = std::get<1>(type);
      auto semaFields = std::get<2>(type);
      defineType(file, className, fields, semaFields);
    }

    /// } for namespace
    file << "} // end namespace\n\n";
    /// #endif for #ifndef
    file << "#endif\n";
    file.close();
  }

  void defineType(std::ofstream &file, const std::string &className,
                  const std::string& fields, const std::string& semaFields) {
    file << "class " << strip_whitespace(className) << " {\n";
    file << "public: " << '\n';
    file << "  " << strip_whitespace(className) << "(";
    auto fieldList = split(fields, ",");
    bool first = true;
    for (const auto& field : fieldList) {
      if (!first)
        file << ", ";
      if (first)
        first = false;
      file << left_trim(field);
    }
    file << ") :" << '\n' << "    ";
    first = true;
    for (const auto& field : fieldList) {
      if (!first)
        file << ", ";
      if (first)
        first = false;
      auto fieldName = split(field, " ")[1];
      // take ownership only if field is unique_ptr or a vector
      bool owner = (field.find("std::unique_ptr") != std::string::npos) ||
                   (field.find("std::vector") != std::string::npos);
      if (owner) {
        file << fieldName << "(std::move(" << strip_whitespace(fieldName) << "))";
      } else {
        file << fieldName << "(" << strip_whitespace(fieldName) << ")";
      }
    }
    file << " {}" << std::endl;
    file << "public: " << std::endl;
    for (auto field : fieldList) {
      file << "  " << left_trim(field) << ';' << '\n';
    }

    // fields poulated during semantic analysis
    auto semaFieldList = split(semaFields, ",");
    for (auto field : semaFieldList) {
      file << "  " << left_trim(field) << ';' << '\n';
    }
    file << "};" << '\n' << '\n';
  }

private:
  const std::string outDir;
  const AstSpecification astSpec;
};
} // namespace ccomp

using namespace ccomp;

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cout << "Usage: ast_generator <output directory>" << std::endl;
  } else {
    const std::string outDir = argv[1];
    std::cout << "ast_generator generating files in " << outDir << std::endl;
    const AstSpecification exprSpec = {
        "Expr",
        {{"Assign", "std::unique_ptr<Expr> lvalue, std::unique_ptr<Expr> value", ""},
         {"Conditional", "std::unique_ptr<Expr> condition, std::unique_ptr<Expr> thenExp, std::unique_ptr<Expr> elseExp", ""},
         {"BinaryExpr", "std::unique_ptr<Expr> left, Token Operator, std::unique_ptr<Expr> right", ""},
         {"LiteralExpr", "TokenType type, std::string value", ""},
         {"UnaryExpr", "Token Operator, std::unique_ptr<Expr> right", ""},
         {"Variable", "Token name", "std::shared_ptr<Symbol> sym, const Type* evalty=0"},
         {"Call", "std::unique_ptr<Expr> callee, std::vector<std::unique_ptr<Expr>> args", "const Function* fn=0"}},
        {},
        {"\"Token.h\"", "\"Scope.h\"", "\"Type.h\"", "<memory>", "<string>", "<vector>", "<variant>"}};
    AstGen exprGenerator(outDir, exprSpec);
    exprGenerator.generate();

    const AstSpecification stmtSpec = {
        "Stmt",
        {{"Block", "std::vector<std::unique_ptr<Stmt>> stmts", ""},
         {"Expression", "std::unique_ptr<Expr> expr", ""},
         {"FunctionParam", "Token type, Token name", "std::shared_ptr<Symbol> sym, const Type* evalty=0"},
         {"Function", "bool fileScope, Token returnty, Scope::StorageClass storage, Token name, std::vector<std::unique_ptr<Stmt>> params, std::unique_ptr<Stmt> body",
              "std::shared_ptr<Symbol> sym, std::unique_ptr<FunctionType> evalty"},
         {"If", "std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> thenBranch, std::unique_ptr<Stmt> elseBranch", ""},
         {"Return", "Token keyword, std::unique_ptr<Expr> value", ""},
         {"DoWhile", "std::unique_ptr<Stmt> body, std::unique_ptr<Expr> condition", "int loop_label"},
         {"While", "std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body", "int loop_label"},
         {"For", "std::unique_ptr<Stmt> init, std::unique_ptr<Expr> condition, std::unique_ptr<Expr> post, std::unique_ptr<Stmt> body", "int loop_label"},
         {"Decl", "bool fileScope, bool loopDecl, Token type, Scope::StorageClass storage, std::unique_ptr<Expr> name, std::unique_ptr<Expr> init", ""},
         {"Null", "Token loc", ""},
         {"Break", "Token loc", "int loop_label"},
         {"Continue", "Token loc", "int loop_label"}},
        {},
        {"\"Token.h\"", "\"Expr.h\"", "\"Scope.h\"", "\"Type.h\"", "<memory>", "<vector>", "<variant>"}};
    AstGen stmtGenerator(outDir, stmtSpec);
    stmtGenerator.generate();

    const AstSpecification tackySpec = {
        "Tacky",
        {{"TackyProgram", "std::vector<std::shared_ptr<Tacky>> functions, std::vector<std::shared_ptr<Tacky>> defs", ""},
         {"TackyFunction", "bool global, std::string name, std::vector<std::shared_ptr<Tacky>> params, std::vector<std::shared_ptr<Tacky>> instructions", ""},
         {"TackyStaticVar", "bool global, std::string name, int init", ""},
         {"TackyUnary", "Token op, std::shared_ptr<Tacky> src, std::shared_ptr<Tacky> dest", ""},
         {"TackyBinary", "Token op, std::shared_ptr<Tacky> src1, std::shared_ptr<Tacky> src2, std::shared_ptr<Tacky> dest", ""},
         {"TackyConstant", "int value", ""},
         {"TackyVar", "std::string identifier", ""},
         {"TackyReturn", "std::shared_ptr<Tacky> value", ""},
         {"TackyCopy", "std::shared_ptr<Tacky> src, std::shared_ptr<Tacky> dest", ""},
         {"TackyJump", "std::shared_ptr<Tacky> target", ""},
         {"TackyJumpIfZero", "std::shared_ptr<Tacky> condition, std::shared_ptr<Tacky> target", ""},
         {"TackyJumpIfNotZero", "std::shared_ptr<Tacky> condition, std::shared_ptr<Tacky> target", ""},
         {"TackyLabel", "std::string identifier", ""},
         {"TackyFunCall", "std::string fname, std::vector<std::shared_ptr<Tacky>> args, std::shared_ptr<Tacky> dest", ""}},
        {},
        {"\"Token.h\"", "<memory>", "<string>", "<vector>", "<variant>"}};
    AstGen tackyGenerator(outDir, tackySpec);
    tackyGenerator.generate();

    const AstSpecification asmSpec = {
        "Asm",
        {{"AsmProgram", "std::vector<std::shared_ptr<Asm>> functions", ""},
         {"AsmFunction", "bool global, std::string name, std::vector<std::shared_ptr<Asm>> instructions", ""},
         {"AsmStaticVar", "bool global, std::string name, int init", ""},
         {"AsmUnary", "Token op, std::shared_ptr<Asm> operand", ""},
         {"AsmBinary", "Token op, std::shared_ptr<Asm> operand1, std::shared_ptr<Asm> operand2", ""},
         {"AsmCmp", "std::shared_ptr<Asm> operand1, std::shared_ptr<Asm> operand2", ""},
         {"AsmIdiv", "std::shared_ptr<Asm> operand", ""},
         {"AsmCdq", "int dummy", ""},
         {"AsmJmp", "std::shared_ptr<Asm> target", ""},
         {"AsmJmpCC", "AsmCondCode cond_code, std::shared_ptr<Asm> target", ""},
         {"AsmSetCC", "AsmCondCode cond_code, std::shared_ptr<Asm> operand", ""},
         {"AsmLabel", "std::string identifier", ""},
         {"AsmMov", "std::shared_ptr<Asm> src, std::shared_ptr<Asm> dest", ""},
         {"AsmAllocateStack", "int size", ""},
         {"AsmDeallocateStack", "int size", ""},
         {"AsmPush", "std::shared_ptr<Asm> operand", ""},
         {"AsmCall", "std::string fname", ""},
         {"AsmReturn", "int dummy", ""},
         {"AsmImm", "int value", ""},
         {"AsmRegister", "AsmReg reg, AsmWordSize size", ""},
         {"AsmPseudo", "std::string identifier", ""},
         {"AsmStack", "int offset", ""},
         {"AsmData", "std::string identifier", ""}},
        {{"CondCode", {"E", "NE", "G", "GE", "L", "LE"}},
         {"Reg", {"AX", "CX", "DX", "DI", "SI", "R8", "R9", "R10", "R11"}},
         {"WordSize", {"QUAD", "LONG", "BYTE"}}},
        {"\"Token.h\"", "<memory>", "<vector>", "<string>", "<variant>"}};
    AstGen asmGenerator(outDir, asmSpec);
    asmGenerator.generate();
  }

  return 0;
}
