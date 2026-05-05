#include "parser/parser_factory.h"

#include "parser/gcc_map_parser.h"
#include "parser/ghs_map_parser.h"
#include "parser/iar_map_parser.h"
#include "parser/keil_map_parser.h"
#include "parser/placeholder_map_parser.h"
#include "parser/renesas_cc_rh_map_parser.h"
#include "parser/renesas_cc_rl_map_parser.h"

std::unique_ptr<IMapParser> ParserFactory::Create(CompilerFamily family) {
    switch (family) {
    case CompilerFamily::Keil:
        return std::make_unique<KeilMapParser>();
    case CompilerFamily::Iar:
        return std::make_unique<IarMapParser>();
    case CompilerFamily::Gcc:
        return std::make_unique<GccMapParser>();
    case CompilerFamily::Rl78:
        return std::make_unique<RenesasCcRlMapParser>();
    case CompilerFamily::Rh850:
        return std::make_unique<RenesasCcRhMapParser>();
    case CompilerFamily::Ghs:
        return std::make_unique<GhsMapParser>();
    default:
        return nullptr;
    }
}
