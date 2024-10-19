#include "avoidpragmaoncecheck.hpp"

#include <iostream>
#include <optional>

#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/PPCallbacks.h"

namespace clang::tidy::md1 {
//------1-------2-------3-------4-------5-------6-------7-------8-------9-------0-------1-------2-------3-------//

namespace
{
    class PragmaOnceCallback : public PPCallbacks
    {
    public:
        PragmaOnceCallback(ClangTidyCheck& check, const SourceManager& sm) :
                m_check(check),
                m_sm(sm)
        {}

        std::optional<std::string> getSuggestedIncludeGuard(SourceLocation loc)
        {
            std::string fileName = m_sm.getFilename(loc).str() ;
            const char* workspaceRoot = std::getenv("WORKSPACE_ROOT") ;
            if (workspaceRoot == nullptr)  // If the environment variable is not set
            {
                return std::nullopt ;
            }
            size_t pos = fileName.find(workspaceRoot) ; // If the file is not in the workspace
            if (pos == std::string::npos)
            {
                return std::nullopt ;
            }
            // Remove the workspace root from the of the file name string
            fileName.erase(pos, strlen(workspaceRoot)) ;
            // Replace all '/' and '.' with '_'
            std::replace(fileName.begin(), fileName.end(), '/', '_') ;
            std::replace(fileName.begin(), fileName.end(), '.', '_') ;
            // Capitalize the file name
            std::transform(fileName.begin(), fileName.end(), fileName.begin(), ::toupper) ;
            // Add the include guard prefix
            return "MD1" + fileName ;
        }

        void PragmaDirective(SourceLocation loc, PragmaIntroducerKind introducer) override
        {
            static constexpr std::string_view expectedText = "#pragma once" ;
            SourceLocation endLoc = loc.getLocWithOffset(expectedText.size()) ;

            // Check if the pragma is a #pragma once
            StringRef pragmaText = Lexer::getSourceText
                (CharSourceRange::getCharRange(loc, endLoc), m_sm, m_longOpts).trim() ;
            if (m_sm.isWrittenInMainFile(loc) == false || std::string_view(pragmaText) != expectedText)
            {
                return ;
            }

            // Emit diagnostic and suggest include guards
            std::optional<std::string> includeGuard = getSuggestedIncludeGuard(loc) ;
            static constexpr std::string_view checkMsg = "#pragma once is discouraged, use include guards instead" ;
            if (includeGuard == std::nullopt)
            {
                m_check.diag(loc, checkMsg) ;
                return ;
            }
            // If the include guard could be generated, suggest the include guards
            SourceLocation fileEndLoc = m_sm.getLocForEndOfFile(m_sm.getMainFileID()) ; 
            m_check.diag(loc, checkMsg)
                << FixItHint::CreateReplacement(SourceRange(loc, endLoc), "#ifndef " + includeGuard.value())
                << FixItHint::CreateInsertion(endLoc, "\n#define " + includeGuard.value())
                << FixItHint::CreateInsertion(fileEndLoc, "\n#endif  // " + includeGuard.value()) ; ;
        }

    private:
        ClangTidyCheck& m_check ;
        const SourceManager& m_sm ;
        const LangOptions m_longOpts ;
    } ;
}

void AvoidPragmaOnceCheck::registerPPCallbacks(const SourceManager &sm, Preprocessor *pp,
                                               Preprocessor* moduleExpanderPP)
{
    pp->addPPCallbacks(std::make_unique<PragmaOnceCallback>(*this, sm));
}


//------1-------2-------3-------4-------5-------6-------7-------8-------9-------0-------1-------2-------3-------//
}  // namespace clang::tidy::md1
