#include "avoidpragmaoncecheck.hpp"

#include <iostream>

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
                m_check(check), m_sm(sm)
        {}

        void PragmaDirective(SourceLocation loc, PragmaIntroducerKind introducer) override
        {
            static constexpr std::string_view replacementMsg = 
                "#ifndef MD1_FOO_BAR_BAZZ_HPP" ;
            static constexpr std::string_view onceStr = "once" ;

            SourceLocation endOfHashLoc = Lexer::getLocForEndOfToken(loc, 0, m_sm, LangOptions()) ;
            std::optional<Token> pragmaTok = Lexer::findNextToken(endOfHashLoc, m_sm, LangOptions()) ;
            if (pragmaTok.has_value() == false)
            {
                return ;
            }
            std::optional<Token> onceToken = Lexer::findNextToken(pragmaTok->getLastLoc(), m_sm, LangOptions()) ;
            if (onceToken.has_value() == false)
            {
                return ;
            }
            SourceLocation endLoc = onceToken->getLastLoc() ;
            StringRef pragmaText = Lexer::getSourceText
                (CharSourceRange::getCharRange(pragmaTok->getLastLoc(), endLoc), m_sm, LangOptions()).trim() ;

            // Check if the pragma is a #pragma once
            if (m_sm.isWrittenInMainFile(loc) && std::string_view(pragmaText) == onceStr)
            {
                // Emit diagnostic and suggest include guards
                m_check.diag(loc, "#pragma once is discouraged, use include guards instead")
                    << FixItHint::CreateReplacement(SourceRange(loc, endLoc), replacementMsg) ;
            }
        }

    private:
        ClangTidyCheck& m_check ;
        const SourceManager& m_sm ;
    } ;
}

void AvoidPragmaOnceCheck::registerPPCallbacks(const SourceManager &sm, Preprocessor *pp,
                                               Preprocessor* moduleExpanderPP)
{
    pp->addPPCallbacks(std::make_unique<PragmaOnceCallback>(*this, sm));
}


//------1-------2-------3-------4-------5-------6-------7-------8-------9-------0-------1-------2-------3-------//
}  // namespace clang::tidy::md1
