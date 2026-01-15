cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

include_directories(${PROJECT_SOURCE_DIR}/include)

set(AST_GEN_FILES 
    ${PROJECT_SOURCE_DIR}/include/ast/Asm.h
    ${PROJECT_SOURCE_DIR}/include/ast/Expr.h
    ${PROJECT_SOURCE_DIR}/include/ast/Stmt.h)

add_library(ccomplib
            Token.cc
            Scanner.cc
            ErrorHandler.cc
            Parser.cc
            AsmGen.cc
            Codegen.cc
            ${AST_GEN_FILES})

add_custom_command(
  OUTPUT ${AST_GEN_FILES}
  DEPENDS ast_generator
  COMMAND ast_generator ${PROJECT_SOURCE_DIR}/include/ast)

add_executable(ast_generator ${PROJECT_SOURCE_DIR}/lib/AstGenerator.cc)
