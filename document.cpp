#include <string>

#include "document.h"

Document::Document(
        const int id_,
        const double relevance_,
        const int rating_ )
    :
        id( id_ )
        , relevance( relevance_ )
        , rating( rating_ )
    {}

void
PrintDocument( const Document & document )
    {
        using namespace std::string_literals;

        std::cout
                    << "{ "s
                        << "document_id = "s << document.id        << ", "s
                        << "relevance = "s   << document.relevance << ", "s
                        << "rating = "s      << document.rating
                    << " }"s
                    << std::endl;
    }

std::ostream &
operator<<( std::ostream & out, const Document & document )
    {
        using namespace std::string_literals;

        return
                out
                    << "{ "s
                        << "document_id = "s << document.id        << ", "s
                        << "relevance = "s   << document.relevance << ", "s
                        << "rating = "s      << document.rating
                    << " }"s;
    }

