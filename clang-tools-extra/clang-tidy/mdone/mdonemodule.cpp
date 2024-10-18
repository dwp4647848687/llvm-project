#include "mdonemodule.hpp"
#include "avoidpragmaoncecheck.hpp"

namespace clang::tidy::md1 {
//------1-------2-------3-------4-------5-------6-------7-------8-------9-------0-------1-------2-------3-------//

void MDOneModule::addCheckFactories(ClangTidyCheckFactories &CheckFactories)
{
    CheckFactories.registerCheck<AvoidPragmaOnceCheck>("mdone-avoid-pragma-once") ;
}

// Register the MyOrgModule using Clang's registry system.
static ClangTidyModuleRegistry::Add<MDOneModule> X("mdone-module", "Adds md1-specific clang-tidy checks.") ;

// This anchor is used to force the linker to link in the generated object file

//------1-------2-------3-------4-------5-------6-------7-------8-------9-------0-------1-------2-------3-------//
}  // namespace clang::tidy::md1

namespace clang::tidy
{
    volatile int MDOneModuleAnchorSource = 0 ;
}