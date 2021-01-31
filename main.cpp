#include <iostream>
#include <string>
#include <vector>
#include <exception>

#include "search_server.h"
#include "request_queue.h"
#include "paginator.h"


#include "test.h"


void
PrintDocument( const Document & document )
    {
        using namespace std;

        std::cout << "{ "s
            << "document_id = "s << document.id << ", "s
            << "relevance = "s << document.relevance << ", "s
            << "rating = "s << document.rating << " }"s << std::endl;
    }

void
PrintMatchDocumentResult( int document_id, const std::vector< std::string > & words, DocumentStatus status )
    {
        using namespace std;

        std::cout << "{ "s
            << "document_id = "s << document_id << ", "s
            << "status = "s << static_cast< int >( status ) << ", "s
            << "words ="s;

        for( const std::string & word : words )
            {
                std::cout << ' ' << word;
            }
        std::cout << "}"s << std::endl;
    }

void
AddDocument(
        SearchServer & search_server,
        int document_id,
        const std::string & document,
        DocumentStatus status,
        const std::vector< int > & ratings )
    {
        using namespace std;

        try
            {
                search_server.AddDocument(document_id, document, status, ratings);
            }
        catch( const std::exception & e )
            {
                std::cerr << "Ошибка добавления документа "s << document_id << ": "s << e.what() << std::endl;
            }
    }

void
FindTopDocuments( const SearchServer & search_server, const std::string & raw_query )
    {
        using namespace std;

        std::cout << "Результаты поиска по запросу: "s << raw_query << std::endl;
        try
            {
                for( const Document & document : search_server.FindTopDocuments( raw_query ) )
                    {
                        PrintDocument( document );
                    }
            }
        catch( const std::exception & e )
            {
                std::cerr << "Ошибка поиска: "s << e.what() << std::endl;
            }
    }

void
MatchDocuments( const SearchServer & search_server, const std::string & query )
    {
        using namespace std;

        try
            {
                std::cout << "Матчинг документов по запросу: "s << query << std::endl;
                const int document_count = search_server.GetDocumentCount();

                for( int index = 0; index < document_count; ++index )
                    {
                        const int document_id = search_server.GetDocumentId( index );
                        const auto & [ words, status ] = search_server.MatchDocument( query, document_id );
                        PrintMatchDocumentResult( document_id, words, status );
                    }
            }
        catch( const std::exception & e )
            {
                std::cerr << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << std::endl;
            }
    }

int
main()
    {
        using namespace std;

        TestSearchServer();

        {
            SearchServer search_server( "и в на"s );

            AddDocument( search_server,  1, "пушистый кот пушистый хвост"s,     DocumentStatus::ACTUAL, { 7, 2, 7 } );
            AddDocument( search_server,  1, "пушистый пёс и модный ошейник"s,   DocumentStatus::ACTUAL, { 1, 2 }    );
            AddDocument( search_server, -1, "пушистый пёс и модный ошейник"s,   DocumentStatus::ACTUAL, { 1, 2 }    );
            AddDocument( search_server,  3, "большой пёс скво\x12рец евгений"s, DocumentStatus::ACTUAL, { 1, 3, 2 } );
            AddDocument( search_server,  4, "большой пёс скворец евгений"s,     DocumentStatus::ACTUAL, { 1, 1, 1 } );

            FindTopDocuments( search_server, "пушистый -пёс"s  );
            FindTopDocuments( search_server, "пушистый --кот"s );
            FindTopDocuments( search_server, "пушистый -"s     );

            MatchDocuments( search_server, "пушистый пёс"s       );
            MatchDocuments( search_server, "модный -кот"s        );
            MatchDocuments( search_server, "модный --пёс"s       );
            MatchDocuments( search_server, "пушистый - хвост  "s );
        }

        {
            SearchServer search_server( "and with"s );

            search_server.AddDocument( 1, "funny cat and funny dog"s,   DocumentStatus::ACTUAL, { 7, 2, 7 } );
            search_server.AddDocument( 2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2, 3 } );
            search_server.AddDocument( 3, "big cat style hair"s,        DocumentStatus::ACTUAL, { 1, 2, 8 } );
            search_server.AddDocument( 4, "big dog cat hog"s,           DocumentStatus::ACTUAL, { 1, 3, 2 } );
            search_server.AddDocument( 5, "big dog hamster frog",       DocumentStatus::ACTUAL, { 1, 1, 1 } );

            constexpr int page_size = 2;
            const auto search_results = search_server.FindTopDocuments( "curly dog"s );
            const auto pages = Paginate( search_results, page_size );

            for( auto page = pages.begin(); page != pages.end(); ++page )
                {
                    std::cout << *page << std::endl;
                    std::cout << "Page break"s << std::endl;
                }
        }

        {
            SearchServer search_server( "and in at"s );
            RequestQueue request_queue( search_server );

            search_server.AddDocument( 1, "curly cat curly tail"s,       DocumentStatus::ACTUAL, { 7, 2, 7 } );
            search_server.AddDocument( 2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 } );
            search_server.AddDocument( 3, "big cat fancy collar "s,      DocumentStatus::ACTUAL, { 1, 2, 8 } );
            search_server.AddDocument( 4, "big dog sparrow Eugene"s,     DocumentStatus::ACTUAL, { 1, 3, 2 } );
            search_server.AddDocument( 5, "big dog sparrow Vasiliy"s,    DocumentStatus::ACTUAL, { 1, 1, 1 } );

            // 1439 запросов с нулевым результатом
            for( int i = 0; i < 1439; ++i )
                {
                    request_queue.AddFindRequest( "empty request"s );
                }

            // все еще 1439 запросов с нулевым результатом
            request_queue.AddFindRequest( "curly dog"s );

            // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
            request_queue.AddFindRequest( "big collar"s );

            // первый запрос удален, 1437 запросов с нулевым результатом
            request_queue.AddFindRequest( "sparrow"s );

            std::cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;
            return 0;
        }
    }

