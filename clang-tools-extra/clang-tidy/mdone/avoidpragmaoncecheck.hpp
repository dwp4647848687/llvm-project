#ifndef MD1_CLANG_TIDY_MDONE_AVOIDPRAGMAONCECHECK_HPP
#define MD1_CLANG_TIDY_MDONE_AVOIDPRAGMAONCECHECK_HPP

#include "../ClangTidyCheck.h"

namespace clang::tidy::md1 {
//------1-------2-------3-------4-------5-------6-------7-------8-------9-------0-------1-------2-------3-------//

class AvoidPragmaOnceCheck : public ClangTidyCheck
{
public:
    AvoidPragmaOnceCheck(StringRef Name, ClangTidyContext *Context) :
            ClangTidyCheck(Name, Context)
    {}

    void registerPPCallbacks(const SourceManager &SM, Preprocessor *PP, Preprocessor *ModuleExpanderPP) override ;
} ;

//------1-------2-------3-------4-------5-------6-------7-------8-------9-------0-------1-------2-------3-------//
}       // namespace clang::tidy::md1
#endif  // MD1_CLANG_TIDY_MDONE_AVOIDPRAGMAONCECHECK_HPP