#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace ccomp {
// source:
// https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
static std::vector<std::string> split(const std::string &str,
                                      const std::string &delim) {
  std::vector<std::string> tokens;
  size_t prev = 0, pos = 0;
  do {
    pos = str.find(delim, prev);
    if (pos == std::string::npos)
      pos = str.length();
    auto token = str.substr(prev, pos - prev);
    if (!token.empty())
      tokens.push_back(token);
    prev = pos + delim.length();
  } while (pos < str.length() && prev < str.length());
  return tokens;
}
} // namespace ccomp

struct AstSpecification {
 std::string name;
 std::vector<std::string> types;
 // enums
 std::vector<
    std::tuple<std::string,
        std::vector<std::string>>>
    enums;
 // Return type, and visitor name
 std::vector<
    std::tuple<
        std::string, // return type of visitor (encapsulated in shared_ptr)
        std::string, // visitor name
        std::string>> // visitor argument
    visitors;
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

    // Expr base abstract interface
    file << "#include \"Token.h\"" << '\n';
    file << "#include <memory>" << '\n';
    file << "#include <any>" << '\n';
    file << "#include <vector>" << '\n';
    file << "namespace ccomp {" << '\n';

    // forward declarations
    file << "class " << baseName << ";" << std::endl;
    for (auto &type : astSpec.types) {
      auto className = type.substr(0, type.find(":"));
      // remove spaces from className
      className.erase(std::remove(className.begin(), className.end(), ' '),
                      className.end());
      file << "class " << className << ";\n";
    }
    file << "class Expr;\n";

    file << '\n';
    defineVisitor(file, baseName);
    file << '\n';

    file << "class " << baseName << " {" << std::endl;
    file << "public:" << std::endl;
    for (auto& anenum : astSpec.enums) {
      file << "  enum " << std::get<0>(anenum) << " {\n";
      for (auto& field : std::get<1>(anenum)) {
        file << "    " << field << ",\n";
      }
      file << "  };\n";
    }
    file << "  virtual ~" << baseName << "() {}" << std::endl;
    file << "  virtual std::any accept("
         << baseName + "Visitor& visitor) = 0;" << '\n';
    file << "};" << '\n' << '\n';

    // Derived concrete classes
    for (auto type : astSpec.types) {
      auto className = type.substr(0, type.find(":"));
      auto fields = type.substr(type.find(":") + 1, type.size());
      defineType(file, baseName, className, fields);
    }

    /// } for namespace
    file << "} // end namespace\n\n";
    /// #endif for #ifndef
    file << "#endif\n";
    file.close();
  }

  void defineType(std::ofstream &file, const std::string &baseName,
                  const std::string &className, const std::string fields) {
    file << "class " + className + " : "
         << "public std::enable_shared_from_this<" << className << ">,"
         << " public " << baseName << " { " << std::endl;
    file << "public: " << '\n';
    file << "  " << className << "(";
    auto fieldList = ccomp::split(fields, ",");
    bool first = true;
    for (auto field : fieldList) {
      if (!first)
        file << ", ";
      if (first)
        first = false;
      file << "  " << field;
    }
    file << ")  :" << '\n' << "    ";
    first = true;
    for (auto field : fieldList) {
      if (!first)
        file << ", ";
      if (first)
        first = false;
      auto fieldName = ccomp::split(field, " ")[1];
      file << fieldName + "(" + fieldName + ")";
    }
    file << " {}" << std::endl;
    for (auto& visitor : astSpec.visitors) {
        defineVisitor(file, className,
            std::get<0>(visitor), std::get<1>(visitor), std::get<2>(visitor));
    }
    file << "public: " << std::endl;
    for (auto field : fieldList) {
      file << "  " << field << ';' << '\n';
    }
    file << "};" << '\n' << '\n';
  }

  void defineVisitor(std::ofstream &file,
    const std::string &className, const std::string& retType __attribute_maybe_unused__,
    const std::string& visitorName, const std::string& visitorArg) {
    file << " std::any " << visitorName << '('
         << visitorArg << "& visitor) override {\n";
    file << "    std::shared_ptr<" << className << "> p{shared_from_this()};"
         << '\n';
    file << "    return visitor.visit" << className << "(p);" << std::endl;
    file << "  }" << std::endl;
  }

  void defineVisitor(std::ofstream &file, const std::string &baseName) {
    auto visitorClassName = baseName + "Visitor";
    file << "class " << visitorClassName << " {" << std::endl;
    file << "public:" << std::endl;
    file << "  virtual ~" << visitorClassName << "() {}" << std::endl;
    for (auto type : astSpec.types) {
      auto className = type.substr(0, type.find(":"));
      file << "  virtual std::any "
           << "    visit" + className << "(std::shared_ptr<" << className
           << "> " << baseName << " __attribute_maybe_unused__) { return nullptr; }" << std::endl;
    }
    file << "};" << std::endl;
  }

private:
  const std::string outDir;
  const AstSpecification astSpec;
};

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cout << "Usage: ast_generator <output directory>" << std::endl;
  } else {
    const std::string outDir = argv[1];
    std::cout << "ast_generator generating files in " << outDir << std::endl;
    const AstSpecification exprSpec = {
        "Expr",
        {"Assign   : std::shared_ptr<Expr> lvalue, std::shared_ptr<Expr> value",
         "BinaryExpr      : std::shared_ptr<Expr> left, Token Operator, std::shared_ptr<Expr> right",
         "LiteralExpr     : TokenType type, std::string value",
         "UnaryExpr       : Token Operator, std::shared_ptr<Expr> right",
         "Variable        : Token name"},
        {},
        {{"CObject", "accept", "ExprVisitor"}}};
    AstGen exprGenerator(outDir, exprSpec);
    exprGenerator.generate();

    const AstSpecification stmtSpec = {
        "Stmt",
        {"Block  : std::vector<std::shared_ptr<Stmt>> stmts",
            "Expression : std::shared_ptr<Expr> expr",
            "Function   : Token name, std::vector<Token> params, std::vector<std::shared_ptr<Stmt>> body",
            "If         : std::shared_ptr<Expr> condition, std::shared_ptr<Stmt> thenBranch, std::shared_ptr<Stmt> elseBranch",
            "Print      : std::shared_ptr<Expr> expr",
            "Return     : Token keyword, std::shared_ptr<Expr> value",
            "While      : std::shared_ptr<Expr> condition, std::shared_ptr<Stmt> body",
            "Decl       : Token name, std::shared_ptr<Expr> init",
            "Null       : Token loc"},
        {},
        {{"CObject", "accept", "StmtVisitor"}}};
    AstGen stmtGenerator(outDir, stmtSpec);
    stmtGenerator.generate();

    const AstSpecification tackySpec = {
        "Tacky",
        {"TackyProgram     : std::vector<std::shared_ptr<Tacky>> functions",
            "TackyFunction    : Token name, std::vector<std::shared_ptr<Tacky>> instructions",
            "TackyUnary  : Token op, std::shared_ptr<Tacky> src, std::shared_ptr<Tacky> dest",
            "TackyBinary  : Token op, std::shared_ptr<Tacky> src1, std::shared_ptr<Tacky> src2, std::shared_ptr<Tacky> dest",
            "TackyConstant : int value",
            "TackyVar : std::string identifier",
            "TackyReturn : std::shared_ptr<Tacky> value",
            "TackyCopy : std::shared_ptr<Tacky> src, std::shared_ptr<Tacky> dest",
            "TackyJump : std::shared_ptr<TackyLabel> target",
            "TackyJumpIfZero : std::shared_ptr<Tacky> condition, std::shared_ptr<TackyLabel> target",
            "TackyJumpIfNotZero : std::shared_ptr<Tacky> condition, std::shared_ptr<TackyLabel> target",
            "TackyLabel : std::string identifier"},
        {},
        {{"Tacky", "accept", "TackyVisitor"}}};
    AstGen tackyGenerator(outDir, tackySpec);
    tackyGenerator.generate();

    const AstSpecification asmSpec = {
        "Asm",
        {"AsmProgram     : std::vector<std::shared_ptr<Asm>> functions",
                "AsmFunction    : Token name, std::vector<std::shared_ptr<Asm>> instructions",
                "AsmUnary       : Token op, std::shared_ptr<Asm> operand",
                "AsmBinary      : Token op, std::shared_ptr<Asm> operand1, std::shared_ptr<Asm> operand2",
                "AsmCmp         : std::shared_ptr<Asm> operand1, std::shared_ptr<Asm> operand2",
                "AsmIdiv        : std::shared_ptr<Asm> operand",
                "AsmCdq         : int dummy",
                "AsmJmp         : std::shared_ptr<AsmLabel> target",
                "AsmJmpCC       : Asm::CondCode cond_code, std::shared_ptr<AsmLabel> target",
                "AsmSetCC       : Asm::CondCode cond_code, std::shared_ptr<Asm> operand",
                "AsmLabel      : std::string identifier",
                "AsmMov         : std::shared_ptr<Asm> src, std::shared_ptr<Asm> dest",
                "AsmAllocateStack : int size",
                "AsmReturn      : int dummy",
                "AsmImm         : int value",
                "AsmRegister    : Asm::Reg reg",
                "AsmPseudo      : std::string identifier",
                "AsmStack       : int offset"},
        {{"CondCode", {"E", "NE", "G", "GE", "L", "LE"}},
                {"Reg", {"AX", "DX", "R10", "R11"}}},
        {{"Asm", "accept", "AsmVisitor"}}};
    AstGen asmGenerator(outDir, asmSpec);
    asmGenerator.generate();
  }

  return 0;
}
