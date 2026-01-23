#include "AsmGen.h"
#include "Codegen.h"
#include "ErrorHandler.h"
#include "Scanner.h"
#include "Parser.h"
#include "AstPrinter.h"
#include "TackyGen.h"
#include "AsmGen.h"
#include "Codegen.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <format>
#include <vector>
#include <fstream>
#include <sstream>

const int PHASE_LEX = 0x1;
const int PHASE_PARSE = 0x2;
const int PHASE_TACKY = 0x4;
const int PHASE_CODEGEN = 0x8;

#define SETBIT(val, mask) ((val) |= (1<<(mask)))
#define ISBITSET(val, mask) (((val) & (1<<(mask)))!=0)

static int compile(const std::string& source, const char* outputpath, ccomp::ErrorHandler& errorHandler, int compiler_phases) {
  if (!ISBITSET(compiler_phases, PHASE_LEX)) {
    printf("no lex\n");
    return 0;
  }

  /// scanner
  ccomp::Scanner scanner(source, errorHandler);
  std::vector<ccomp::Token> tokens = scanner.scanAndGetTokens();

#if 0
  // print tokens
  for (auto token : tokens) {
    printf("token: %s\n", token.toString().c_str());
  }
#endif

  // if found error during scanning, report
  if (errorHandler.foundError) {
    errorHandler.report();
    return 65;
  }

  if (!ISBITSET(compiler_phases, PHASE_PARSE)) {
    printf("no parse\n");
    return 0;
  }

  /// parser
  ccomp::Parser parser(tokens, errorHandler);
  auto stmts = parser.parse();
  // if found error during parsing, report
  if (errorHandler.foundError) {
    errorHandler.report();
    return 65;
  }

#if 0
  /// print ast
	ccomp::AstPrinter pp;
	pp.print(stmts);
#endif

  if (!ISBITSET(compiler_phases, PHASE_TACKY)) {
    printf("no tackygen\n");
    return 0;
  }

  /// tackygen
  ccomp::TackyGen tackygen(stmts, errorHandler);
  auto tackyasm = tackygen.gen();
  // if found error during parsing, report
  if (errorHandler.foundError) {
    errorHandler.report();
    return 65;
  }

  if (!ISBITSET(compiler_phases, PHASE_CODEGEN)) {
    printf("no codegen\n");
    return 0;
  }

  /// asmgen
  ccomp::AsmGen asmgen(tackyasm, errorHandler);
  auto progasm = asmgen.gen();
  // if found error during parsing, report
  if (errorHandler.foundError) {
    errorHandler.report();
    return 65;
  }

  /// codegen
  ccomp::Codegen codegen(progasm, errorHandler);
  auto codeasm = codegen.code();
  // if found error during parsing, report
  if (errorHandler.foundError) {
    errorHandler.report();
    return 65;
  }

  // write to file or stdout
  if (outputpath) {
    std::ofstream outputasm(outputpath);
    outputasm << codeasm << '\n';
  } else {
    printf("%s\n", codeasm.c_str());
  }

#if 0
  Resolver resolver(interpreter, errorHandler);
  resolver.resolve(stmts);
  if (errorHandler.foundError) {
    errorHandler.report();
    return 65;
  }

  try {
    interpreter->interpret(stmts);
  } catch (const RuntimeException &e) {
    std::cerr << e.what() << '\n' << "[line " << e.tok.line << "]" << '\n';
    return 70;
  }
  #endif
  return 0;
}

static int compileFile(const std::string& path, const char* outputpath, ccomp::ErrorHandler& errorHandler, int compiler_phases) {
  // preprocess file with gcc
  std::filesystem::path filepath(path);
  std::filesystem::path filestem = filepath.filename().stem();
  std::filesystem::path preprocesedpath = filepath.parent_path() / (filestem.string() + ".pre");
  std::string gcc_args = std::format("gcc -E -P {} -o {}", path, preprocesedpath.c_str());
  int retCode = std::system(gcc_args.c_str());

  std::ifstream file(preprocesedpath.c_str());
  // can't open file
  if (!file.is_open()) {
    printf("can't open file %s\n", preprocesedpath.c_str());
    retCode = 70;
  } else {
    std::ostringstream stream;
    stream << file.rdbuf();
    file.close();
    retCode = compile(stream.str(), outputpath, errorHandler, compiler_phases);
  }

  return retCode;
}

int main(int argc, char** argv) {
  int retCode = 0;
  if (argc < 2) {
    printf("Usage: ccomp [filename]\n");
    retCode = 1;
  } else if (argc == 2) {
    int compiler_phases = 0;
    SETBIT(compiler_phases, PHASE_LEX);
    SETBIT(compiler_phases, PHASE_PARSE);
    SETBIT(compiler_phases, PHASE_TACKY);
    SETBIT(compiler_phases, PHASE_CODEGEN);

    std::filesystem::path filepath(argv[1]);
    std::filesystem::path filestem = filepath.filename().stem();
    std::filesystem::path asmoutputpath = filepath.parent_path() / (filestem.string() + ".s");
    printf ("compiling %s\n", filepath.c_str());
    printf("output filename %s\n", asmoutputpath.c_str());

    ccomp::ErrorHandler errorHandler;
    retCode = compileFile(filepath, asmoutputpath.c_str(), errorHandler,
                          compiler_phases);
    if (retCode == 0) {
      // produced an asm file, compile with gcc.
      std::filesystem::path binoutputpath = filepath.parent_path() / filestem;
      std::string gcc_args = std::format("gcc {} -o {}", asmoutputpath.c_str(), binoutputpath.c_str());
      retCode = std::system(gcc_args.c_str());
    }
  } else if (argc == 3) {
    const char* opt = argv[1];
    int compiler_phases = 0;
    if (strcmp(opt, "--lex") == 0) {
      SETBIT(compiler_phases, PHASE_LEX);
    } else if (strcmp(opt, "--parse") == 0) {
      SETBIT(compiler_phases, PHASE_LEX);
      SETBIT(compiler_phases, PHASE_PARSE);
    } else if (strcmp(opt, "--tacky") == 0) {
      SETBIT(compiler_phases, PHASE_LEX);
      SETBIT(compiler_phases, PHASE_PARSE);
      SETBIT(compiler_phases, PHASE_TACKY);
    } else if (strcmp(opt, "--codegen") == 0) {
      SETBIT(compiler_phases, PHASE_LEX);
      SETBIT(compiler_phases, PHASE_PARSE);
      SETBIT(compiler_phases, PHASE_TACKY);
      SETBIT(compiler_phases, PHASE_CODEGEN);
    }

    const char* filepath = argv[2];
    ccomp::ErrorHandler errorHandler;
    retCode = compileFile(filepath, nullptr, errorHandler, compiler_phases);
  }

  return retCode;
}
