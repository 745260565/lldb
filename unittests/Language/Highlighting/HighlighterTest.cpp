//===-- HighlighterTest.cpp -------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "gtest/gtest.h"

#include "lldb/Core/Highlighter.h"

#include "Plugins/Language/CPlusPlus/CPlusPlusLanguage.h"
#include "Plugins/Language/Go/GoLanguage.h"
#include "Plugins/Language/Java/JavaLanguage.h"
#include "Plugins/Language/OCaml/OCamlLanguage.h"
#include "Plugins/Language/ObjC/ObjCLanguage.h"
#include "Plugins/Language/ObjCPlusPlus/ObjCPlusPlusLanguage.h"

using namespace lldb_private;

namespace {
class HighlighterTest : public testing::Test {
public:
  static void SetUpTestCase();
  static void TearDownTestCase();
};
} // namespace

void HighlighterTest::SetUpTestCase() {
  // The HighlighterManager uses the language plugins under the hood, so we
  // have to initialize them here for our test process.
  CPlusPlusLanguage::Initialize();
  GoLanguage::Initialize();
  JavaLanguage::Initialize();
  ObjCLanguage::Initialize();
  ObjCPlusPlusLanguage::Initialize();
  OCamlLanguage::Initialize();
}

void HighlighterTest::TearDownTestCase() {
  CPlusPlusLanguage::Terminate();
  GoLanguage::Terminate();
  JavaLanguage::Terminate();
  ObjCLanguage::Terminate();
  ObjCPlusPlusLanguage::Terminate();
  OCamlLanguage::Terminate();
}

static std::string getName(lldb::LanguageType type) {
  HighlighterManager m;
  return m.getHighlighterFor(type, "").GetName().str();
}

static std::string getName(llvm::StringRef path) {
  HighlighterManager m;
  return m.getHighlighterFor(lldb::eLanguageTypeUnknown, path).GetName().str();
}

TEST_F(HighlighterTest, HighlighterSelectionType) {
  EXPECT_EQ(getName(lldb::eLanguageTypeC_plus_plus), "clang");
  EXPECT_EQ(getName(lldb::eLanguageTypeC_plus_plus_03), "clang");
  EXPECT_EQ(getName(lldb::eLanguageTypeC_plus_plus_11), "clang");
  EXPECT_EQ(getName(lldb::eLanguageTypeC_plus_plus_14), "clang");
  EXPECT_EQ(getName(lldb::eLanguageTypeObjC), "clang");
  EXPECT_EQ(getName(lldb::eLanguageTypeObjC_plus_plus), "clang");

  EXPECT_EQ(getName(lldb::eLanguageTypeUnknown), "none");
  EXPECT_EQ(getName(lldb::eLanguageTypeJulia), "none");
  EXPECT_EQ(getName(lldb::eLanguageTypeJava), "none");
  EXPECT_EQ(getName(lldb::eLanguageTypeHaskell), "none");
}

TEST_F(HighlighterTest, HighlighterSelectionPath) {
  EXPECT_EQ(getName("myfile.cc"), "clang");
  EXPECT_EQ(getName("moo.cpp"), "clang");
  EXPECT_EQ(getName("mar.cxx"), "clang");
  EXPECT_EQ(getName("foo.C"), "clang");
  EXPECT_EQ(getName("bar.CC"), "clang");
  EXPECT_EQ(getName("a/dir.CC"), "clang");
  EXPECT_EQ(getName("/a/dir.hpp"), "clang");
  EXPECT_EQ(getName("header.h"), "clang");

  EXPECT_EQ(getName(""), "none");
  EXPECT_EQ(getName("/dev/null"), "none");
  EXPECT_EQ(getName("Factory.java"), "none");
  EXPECT_EQ(getName("poll.py"), "none");
  EXPECT_EQ(getName("reducer.hs"), "none");
}

TEST_F(HighlighterTest, FallbackHighlighter) {
  HighlighterManager mgr;
  const Highlighter &h =
      mgr.getHighlighterFor(lldb::eLanguageTypePascal83, "foo.pas");

  HighlightStyle style;
  style.identifier.Set("[", "]");
  style.semicolons.Set("<", ">");

  const char *code = "program Hello;";
  std::string output = h.Highlight(style, code);

  EXPECT_STREQ(output.c_str(), code);
}

TEST_F(HighlighterTest, DefaultHighlighter) {
  HighlighterManager mgr;
  const Highlighter &h = mgr.getHighlighterFor(lldb::eLanguageTypeC, "main.c");

  HighlightStyle style;

  const char *code = "int my_main() { return 22; } \n";
  std::string output = h.Highlight(style, code);

  EXPECT_STREQ(output.c_str(), code);
}

//------------------------------------------------------------------------------
// Tests highlighting with the Clang highlighter.
//------------------------------------------------------------------------------

static std::string highlightC(llvm::StringRef code, HighlightStyle style) {
  HighlighterManager mgr;
  const Highlighter &h = mgr.getHighlighterFor(lldb::eLanguageTypeC, "main.c");
  return h.Highlight(style, code);
}

TEST_F(HighlighterTest, ClangEmptyInput) {
  HighlightStyle s;
  EXPECT_EQ("", highlightC("", s));
}

TEST_F(HighlighterTest, ClangScalarLiterals) {
  HighlightStyle s;
  s.scalar_literal.Set("<scalar>", "</scalar>");

  EXPECT_EQ(" int i = <scalar>22</scalar>;", highlightC(" int i = 22;", s));
}

TEST_F(HighlighterTest, ClangStringLiterals) {
  HighlightStyle s;
  s.string_literal.Set("<str>", "</str>");

  EXPECT_EQ("const char *f = 22 + <str>\"foo\"</str>;",
            highlightC("const char *f = 22 + \"foo\";", s));
}

TEST_F(HighlighterTest, ClangUnterminatedString) {
  HighlightStyle s;
  s.string_literal.Set("<str>", "</str>");

  EXPECT_EQ(" f = \"", highlightC(" f = \"", s));
}

TEST_F(HighlighterTest, Keywords) {
  HighlightStyle s;
  s.keyword.Set("<k>", "</k>");

  EXPECT_EQ(" <k>return</k> 1; ", highlightC(" return 1; ", s));
}

TEST_F(HighlighterTest, Colons) {
  HighlightStyle s;
  s.colon.Set("<c>", "</c>");

  EXPECT_EQ("foo<c>:</c><c>:</c>bar<c>:</c>", highlightC("foo::bar:", s));
}

TEST_F(HighlighterTest, ClangBraces) {
  HighlightStyle s;
  s.braces.Set("<b>", "</b>");

  EXPECT_EQ("a<b>{</b><b>}</b>", highlightC("a{}", s));
}

TEST_F(HighlighterTest, ClangSquareBrackets) {
  HighlightStyle s;
  s.square_brackets.Set("<sb>", "</sb>");

  EXPECT_EQ("a<sb>[</sb><sb>]</sb>", highlightC("a[]", s));
}

TEST_F(HighlighterTest, ClangCommas) {
  HighlightStyle s;
  s.comma.Set("<comma>", "</comma>");

  EXPECT_EQ(" bool f = foo()<comma>,</comma> 1;",
            highlightC(" bool f = foo(), 1;", s));
}

TEST_F(HighlighterTest, ClangPPDirectives) {
  HighlightStyle s;
  s.pp_directive.Set("<pp>", "</pp>");

  EXPECT_EQ("<pp>#</pp><pp>include</pp><pp> </pp><pp>\"foo\"</pp><pp> </pp>//c",
            highlightC("#include \"foo\" //c", s));
}

TEST_F(HighlighterTest, ClangComments) {
  HighlightStyle s;
  s.comment.Set("<cc>", "</cc>");

  EXPECT_EQ(" <cc>/*com */</cc> <cc>// com /*n*/</cc>",
            highlightC(" /*com */ // com /*n*/", s));
}

TEST_F(HighlighterTest, ClangOperators) {
  HighlightStyle s;
  s.operators.Set("[", "]");

  EXPECT_EQ(" 1[+]2[/]a[*]f[&]x[|][~]l", highlightC(" 1+2/a*f&x|~l", s));
}

TEST_F(HighlighterTest, ClangIdentifiers) {
  HighlightStyle s;
  s.identifier.Set("<id>", "</id>");

  EXPECT_EQ(" <id>foo</id> <id>c</id> = <id>bar</id>(); return 1;",
            highlightC(" foo c = bar(); return 1;", s));
}
