#ifndef MD1_CLANG_TIDY_MDONE_MDONEMODULE_HPP
#define MD1_CLANG_TIDY_MDONE_MDONEMODULE_HPP

#include "../ClangTidyModule.h"
#include "../ClangTidyModuleRegistry.h"

namespace clang::tidy::md1 {
//------1-------2-------3-------4-------5-------6-------7-------8-------9-------0-------1-------2-------3-------//

class MDOneModule : public ClangTidyModule
{
public:
    void addCheckFactories(ClangTidyCheckFactories &checkFactories) override ;
} ;

//------1-------2-------3-------4-------5-------6-------7-------8-------9-------0-------1-------2-------3-------//
}       // namespace clang::tidy::md1
#endif  // MD1_CLANG_TIDY_MDONE_MDONEMODULE_HPP