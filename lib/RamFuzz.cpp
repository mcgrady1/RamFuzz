#include "RamFuzz.hpp"

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;
using namespace ast_matchers;

using clang::tooling::ClangTool;
using clang::tooling::FrontendActionFactory;
using clang::tooling::newFrontendActionFactory;
using std::ostream;
using std::ostringstream;
using std::shared_ptr;
using std::string;
using std::unordered_map;
using std::vector;

namespace {

bool skip(CXXMethodDecl *M) { return isa<CXXDestructorDecl>(M); }

auto ClassMatcher =
    cxxRecordDecl(isExpansionInMainFile(),
                  unless(hasAncestor(namespaceDecl(isAnonymous()))),
                  hasDescendant(cxxMethodDecl(isPublic())))
        .bind("class");

/// Generates ramfuzz code into an ostream.  The user can feed a RamFuzz
/// instance to a custom MatchFinder, or simply getActionFactory() and run it in
/// a ClangTool.
class RamFuzz : public MatchFinder::MatchCallback {
public:
  RamFuzz(std::ostream &out = std::cout) : out(out) {
    MF.addMatcher(ClassMatcher, this);
    AF = newFrontendActionFactory(&MF);
  }

  /// Match callback.
  void run(const MatchFinder::MatchResult &Result) override;

  FrontendActionFactory &getActionFactory() { return *AF; }

private:
  /// Where to output the generated code.
  ostream &out;

  /// A FrontendActionFactory to run MF.  Owned by *this because it
  /// requires live MF to remain valid.
  shared_ptr<FrontendActionFactory> AF;

  /// A MatchFinder to run *this on ClassMatcher.  Owned by *this
  /// because it's only valid while *this is alive.
  MatchFinder MF;
};

/// Valid identifier from a CXXMethodDecl name.
string valident(const string &mname) {
  static const unordered_map<char, char> table = {
      {' ', '_'}, {'=', 'e'}, {'+', 'p'}, {'-', 'm'}, {'*', 's'},
      {'/', 'd'}, {'%', 'c'}, {'&', 'a'}, {'|', 'f'}, {'^', 'r'},
      {'<', 'l'}, {'>', 'g'}, {'~', 't'}, {'!', 'b'}, {'[', 'h'},
      {']', 'i'}, {'(', 'j'}, {')', 'k'}, {'.', 'n'},
  };
  string transf = mname;
  for (char &c : transf) {
    auto found = table.find(c);
    if (found != table.end())
      c = found->second;
  }
  return transf;
}

} // anonymous namespace

void RamFuzz::run(const MatchFinder::MatchResult &Result) {
  if (const auto *C = Result.Nodes.getNodeAs<CXXRecordDecl>("class")) {
    unordered_map<string, unsigned> namecount;
    unsigned mcount = 0;
    out << "namespace ramfuzz {\n";
    const string rfcls = string("RF__") + C->getNameAsString();
    const string arglist = string("(") + C->getQualifiedNameAsString() + "&)";
    out << "class " << rfcls << " {\n";
    out << " public:\n";
    for (auto M : C->methods()) {
      if (skip(M))
        continue;
      const string name = valident(M->getNameAsString());
      out << "  void " << name << namecount[name]++ << arglist << ";\n";
      mcount++;
    }
    out << "  using mptr = void (" << rfcls << "::*)" << arglist << ";\n";
    out << "  static mptr roulette[" << mcount << "];\n";
    out << "};\n";
    out << "} // namespace ramfuzz\n";
  }
}

string ramfuzz(const string &code) {
  ostringstream str;
  bool success = clang::tooling::runToolOnCode(
      RamFuzz(str).getActionFactory().create(), code);
  return success ? str.str() : "fail";
}

int ramfuzz(ClangTool &tool, const vector<string> &sources, ostream &out) {
  for (const auto &f : sources)
    out << "#include \"" << f << "\"\n";
  return tool.run(&RamFuzz(out).getActionFactory());
}