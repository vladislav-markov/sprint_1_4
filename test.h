#pragma once

#include <iostream>
#include <vector>
#include <exception>
#include <string>
#include <iostream>
#include <map>
#include <set>
#include <utility>

#include "string_processing.h"

#include "search_server.h"
#include "request_queue.h"
#include "paginator.h"

using namespace std::string_literals;

// -------- Начало модульных тестов поисковой системы ----------

void
AssertImpl(
        bool t,
        const std::string & t_str,
        const std::string & file,
        const std::string & func,
        unsigned line,
        const std::string & hint )
    {
        using namespace std;

        if( !t )
            {
                std::cout << std::boolalpha;
                std::cout << file << "("s << line << "): "s << func << ": "s;
                std::cout << "ASSERT("s << t_str << ") failed. "s;

                if( !hint.empty() )
                    {
                        std::cout << "Hint: "s << hint;
                    }
                std::cout << std::endl;

                abort();
            }
    }

template < typename T, typename U >
void
AssertEqualImpl(
        const T & t,
        const U & u,
        const std::string & t_str,
        const std::string & u_str,
        const std::string & file,
        const std::string & func,
        unsigned line,
        const std::string & hint )
    {
        using namespace std;

        if( t != u )
            {
                std::cout << std::boolalpha;
                std::cout << file << "("s << line << "): "s << func << ": "s;
                std::cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
                std::cout << t << " != "s << u << "."s;

                if( !hint.empty() )
                    {
                        std::cout << " Hint: "s << hint;
                    }
                std::cout << std::endl;

                abort();
            }
    }

#define ASSERT(expr) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))
#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))
#define RUN_TEST(func) RunTestImpl((func), #func)

template< typename Functor >
void
RunTestImpl( Functor functor, const std::string & test_name )
    {
        using namespace std;

        functor();
        std::cerr << test_name << " OK"s << std::endl;
    }

std::ostream &
operator<<( std::ostream & out, DocumentStatus status )
    {
        using namespace std;

        const std::map< DocumentStatus, std::string > & document_status_to_str =
            {
                { DocumentStatus::ACTUAL,     "ACTUAL"s     },
                { DocumentStatus::IRRELEVANT, "IRRELEVANT"s },
                { DocumentStatus::REMOVED,    "REMOVED"s    },
                { DocumentStatus::BANNED,     "BANNED"s     },
            };

        return
                out << document_status_to_str.at( status );
    }

std::ostream &
operator<<( std::ostream & out, const std::vector< size_t > & vector_of_size_t )
    {
        using namespace std;

        out << '[';

        bool first = true;

        for( size_t elem : vector_of_size_t )
            {
                if( first )
                    {
                        out << elem;
                        first = false;
                    }
                else
                    {
                        out << ", "s << elem;
                    }
            }

        out << ']';

        return out;
    }

void
TestExcludeStopWordsFromAddedDocumentContent()
    {
        using namespace std;

        constexpr int doc_id = 42;
        const std::string & content = "cat in the city"s;
        const std::vector< int > & ratings = { 1, 2, 3 };

        {
            SearchServer server{ {} };
            server.AddDocument( doc_id, content, DocumentStatus::ACTUAL, ratings );

            const std::vector< Document > & found_docs = server.FindTopDocuments( "in"s );
            ASSERT_EQUAL( found_docs.size(), static_cast< size_t >( 1 ) );
            const Document & doc0 = found_docs[ 0 ];
            ASSERT_EQUAL( doc0.id, doc_id );
        }

        {
            SearchServer server { "in the"s };
            server.AddDocument( doc_id, content, DocumentStatus::ACTUAL, ratings );

            const std::vector< Document > & found_docs = server.FindTopDocuments( "in"s );
            ASSERT( found_docs.empty() );
        }

        {
            SearchServer server { "   in the   "s };
            server.AddDocument( doc_id, content, DocumentStatus::ACTUAL, ratings );

            {
                const std::vector< Document > & found_docs = server.FindTopDocuments( "in"s );
                ASSERT( found_docs.empty() );
            }

            {
                const std::vector< Document > & found_docs = server.FindTopDocuments( "cat"s );
                ASSERT( !found_docs.empty() );
            }
        }

        {
            const std::vector< std::string > & stop_words = { "in"s, "в"s, "на"s, ""s, "in"s };
            SearchServer server( stop_words );
            server.AddDocument( doc_id, content, DocumentStatus::ACTUAL, ratings );

            {
                const std::vector< Document > & found_docs = server.FindTopDocuments( "in"s );
                ASSERT( found_docs.empty() );
            }

            {
                const std::vector< Document > & found_docs = server.FindTopDocuments( "cat"s );
                ASSERT( !found_docs.empty() );
            }
        }

        {
            const std::set< std::string > & stop_words = { "in"s, "в"s, "на"s, ""s };
            SearchServer server( stop_words );
            server.AddDocument( doc_id, content, DocumentStatus::ACTUAL, ratings );

            {
                const std::vector< Document > & found_docs = server.FindTopDocuments( "in"s );
                ASSERT( found_docs.empty() );
            }

            {
                const std::vector< Document > & found_docs = server.FindTopDocuments( "cat"s );
                ASSERT( !found_docs.empty() );
            }
        }
    }

void
TestAddDocumentByDocumentCount()
    {
        using namespace std;

        SearchServer server{ {} };

        ASSERT_EQUAL( server.GetDocumentCount(), 0 );

        server.AddDocument( 42, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 } );

        ASSERT_EQUAL( server.GetDocumentCount(), 1 );
    }

void
TestAddDocumentWithHyphen()
    {
        using namespace std;

        const std::vector< int > & ratings = { 1, 2, 3 };

        SearchServer server{ {} };

        server.AddDocument( 1, "один иван-чай три"s, DocumentStatus::ACTUAL, ratings );

        ASSERT_EQUAL( server.GetDocumentCount(), 1 );

        {
            const std::vector< std::string > & queries =
                {
                    "иван-чай"s,
                };

            for( const std::string & query : queries )
                {
                    const std::vector< Document > & found_docs = server.FindTopDocuments( query );
                    ASSERT( !found_docs.empty() );
                }
        }

        {
            const std::vector< std::string > & queries =
                {
                    "иван"s,
                    "чай"s,
                };

            for( const std::string & query : queries )
                {
                    const std::vector< Document > & found_docs = server.FindTopDocuments( query );
                    ASSERT( found_docs.empty() );
                }
        }
    }

void
TestAddInvalidDocuments()
    {
        using namespace std;

        const std::string & content = "cat in the city"s;
        const std::vector< int > & ratings = { 1, 2, 3 };
        constexpr DocumentStatus status = DocumentStatus::ACTUAL;

        SearchServer server{ {} };

        {
            int document_id = 0;

            server.AddDocument( ++document_id, content,  status, ratings );
            server.AddDocument( ++document_id, "-"s,     status, ratings );
            server.AddDocument( ++document_id, "--cat"s, status, ratings );

            {
                bool exception_was_thrown = false;

                try
                    {
                        server.AddDocument( ++document_id, "скво\x12рец"s, status, ratings );
                    }
                catch( const std::invalid_argument & e )
                    {
                        exception_was_thrown = true;
                    }

                ASSERT( exception_was_thrown );
            }

            server.AddDocument( ++document_id, "cat-cat"s,  status, ratings );
            server.AddDocument( ++document_id, "cat--cat"s, status, ratings );
        }
    }

void
TestFindTopInvalidDocuments()
    {
        using namespace std;

        const std::string & content = "cat in the city"s;
        const std::vector< int > & ratings = { 1, 2, 3 };
        constexpr DocumentStatus status = DocumentStatus::ACTUAL;

        SearchServer server{ {} };

        server.AddDocument( 1, content, status, ratings );

        const std::vector< std::string > & expected_incorrect_queries =
            {
                "скво\x12рец"s,
                "--cat"s,
                "-"s,
            };

        for( const std::string & expected_incorrect_query
                : expected_incorrect_queries )
            {
                bool exception_was_thrown = false;

                try
                    {
                        (void) server.FindTopDocuments( expected_incorrect_query, status );
                    }
                catch( const std::invalid_argument & e )
                    {
                        exception_was_thrown = true;
                    }

                ASSERT( exception_was_thrown );
            }

        {
            (void) server.FindTopDocuments( "cat-cat"s,  status );
            (void) server.FindTopDocuments( "cat--cat"s, status );
        }
    }

void
TestAddDocumentByDocumentCountPerDocumentStatus()
    {
        using namespace std;

        const std::string & content = "cat in the city"s;
        const std::vector< int > & ratings = { 1, 2, 3 };

        SearchServer server{ {} };

        server.AddDocument( 1, content, DocumentStatus::ACTUAL,     ratings );
        server.AddDocument( 2, content, DocumentStatus::IRRELEVANT, ratings );
        server.AddDocument( 3, content, DocumentStatus::BANNED,     ratings );
        server.AddDocument( 4, content, DocumentStatus::REMOVED,    ratings );

        ASSERT_EQUAL( server.GetDocumentCount(), 4 );
    }

void
TestDocumentId()
    {
        using namespace std;

        const std::string & content = "cat in the city"s;
        const std::vector< int > & ratings = { 1, 2, 3 };
        const DocumentStatus status = DocumentStatus::ACTUAL;

        SearchServer server{ {} };

        {
            {
                bool exception_was_thrown = false;

                try
                    {
                        server.AddDocument( -2, content, status, ratings );
                        server.AddDocument( -1, content, status, ratings );
                    }
                catch( const std::invalid_argument & e )
                    {
                        exception_was_thrown = true;
                    }

                ASSERT( exception_was_thrown );
            }

            server.AddDocument( 0, content, DocumentStatus::ACTUAL, ratings );
            server.AddDocument( 1, content, DocumentStatus::ACTUAL, ratings );

            {
                bool exception_was_thrown = false;

                try
                    {
                        server.AddDocument( 1, content, DocumentStatus::ACTUAL, ratings );
                    }
                catch( const std::invalid_argument & e )
                    {
                        exception_was_thrown = true;
                    }

                ASSERT( exception_was_thrown );
            }

            server.AddDocument( 2, content, DocumentStatus::ACTUAL, ratings );

            ASSERT_EQUAL( server.GetDocumentCount(), 3 );
        }
    }

void
TestDocumentStatusMatchDocument()
    {
        using namespace std;

        const std::string & content = "cat in the city"s;
        const std::vector< int > & ratings = { 1, 2, 3 };

        SearchServer server{ {} };

        server.AddDocument( 1, content, DocumentStatus::ACTUAL,     ratings );
        server.AddDocument( 2, content, DocumentStatus::IRRELEVANT, ratings );
        server.AddDocument( 3, content, DocumentStatus::BANNED,     ratings );
        server.AddDocument( 4, content, DocumentStatus::REMOVED,    ratings );

        ASSERT_EQUAL( std::get< 1 >( server.MatchDocument( content, 1 ) ), DocumentStatus::ACTUAL     );
        ASSERT_EQUAL( std::get< 1 >( server.MatchDocument( content, 2 ) ), DocumentStatus::IRRELEVANT );
        ASSERT_EQUAL( std::get< 1 >( server.MatchDocument( content, 3 ) ), DocumentStatus::BANNED     );
        ASSERT_EQUAL( std::get< 1 >( server.MatchDocument( content, 4 ) ), DocumentStatus::REMOVED    );
    }

void
TestStatus()
    {
        using namespace std;

        const std::string & content = "cat in the city"s;
        const std::vector< int > & ratings = { 1, 2, 3 };
        const std::string & query = "in"s;

        const std::set< DocumentStatus > &
        document_status_values_for_iteratation =
            {
                DocumentStatus::ACTUAL    ,
                DocumentStatus::BANNED    ,
                DocumentStatus::IRRELEVANT,
                DocumentStatus::REMOVED   ,
            };

        const std::vector< std::map< DocumentStatus, int > > &
        test_parameters_document_status =
            {
                {
                    { DocumentStatus::ACTUAL,     0   },
                    { DocumentStatus::IRRELEVANT, 0   },
                    { DocumentStatus::BANNED,     0   },
                    { DocumentStatus::REMOVED,    0   },
                },

                {
                    { DocumentStatus::ACTUAL,     1   },
                    { DocumentStatus::IRRELEVANT, 2   },
                    { DocumentStatus::BANNED,     3   },
                    { DocumentStatus::REMOVED,    4   },
                },

                {
                    { DocumentStatus::ACTUAL,     100 },
                    { DocumentStatus::IRRELEVANT, 0   },
                    { DocumentStatus::BANNED,     0   },
                    { DocumentStatus::REMOVED,    0   },
                },

                {
                    { DocumentStatus::ACTUAL,     0   },
                    { DocumentStatus::IRRELEVANT, 100 },
                    { DocumentStatus::BANNED,     100 },
                    { DocumentStatus::REMOVED,    100 },
                },

                {
                    { DocumentStatus::ACTUAL,     100 },
                    { DocumentStatus::IRRELEVANT, 100 },
                    { DocumentStatus::BANNED,     100 },
                    { DocumentStatus::REMOVED,    100 },
                },

                {
                    { DocumentStatus::ACTUAL,     100 },
                    { DocumentStatus::IRRELEVANT, 0   },
                    { DocumentStatus::BANNED,     100 },
                    { DocumentStatus::REMOVED,    0   },
                },

                {
                    { DocumentStatus::ACTUAL,     0   },
                    { DocumentStatus::IRRELEVANT, 100 },
                    { DocumentStatus::BANNED,     0   },
                    { DocumentStatus::REMOVED,    100 },
                },
            };

        for( const std::map< DocumentStatus, int > & number_of_documents_to_add
                : test_parameters_document_status )
            {
                SearchServer server{ {} };

                {
                    int document_id = 0;

                    for( const DocumentStatus document_status : document_status_values_for_iteratation )
                        {
                            if( number_of_documents_to_add.count( document_status ) )
                                {
                                    for( int i = 0
                                            ; i < number_of_documents_to_add.at( document_status )
                                            ; ++i )
                                        {
                                            server.AddDocument( ++document_id, content, document_status, ratings );
                                        }
                                }
                        }
                }

                for( const auto document_status : document_status_values_for_iteratation )
                    {
                        const int expected_value
                                = std::min(
                                        number_of_documents_to_add.at( document_status ),
                                        MAX_RESULT_DOCUMENT_COUNT );

                        const std::vector< Document > & found_docs = server.FindTopDocuments( query, document_status );

                        const int actual_value
                                                = static_cast< int >(
                                                                        found_docs.size() );

                        ASSERT_EQUAL( expected_value, actual_value );
                    }
            }
    }

void
TestPredicate()
    {
        using namespace std;

        const std::string & content = "cat in the city"s;
        const std::string & query = "in"s;
        const std::vector< int > & ratings = { 1, 2, 3 };
        constexpr int ratings_avg = 2;

        const std::set< DocumentStatus > & document_status_values_for_iteratation =
            {
                DocumentStatus::ACTUAL,
                DocumentStatus::BANNED,
                DocumentStatus::IRRELEVANT,
                DocumentStatus::REMOVED
            };

        const std::vector< std::map< DocumentStatus, int > > &
        test_parameters_document_status =
            {
                {
                    { DocumentStatus::ACTUAL,     0   },
                    { DocumentStatus::IRRELEVANT, 0   },
                    { DocumentStatus::BANNED,     0   },
                    { DocumentStatus::REMOVED,    0   },
                },

                {
                    { DocumentStatus::ACTUAL,     1   },
                    { DocumentStatus::IRRELEVANT, 2   },
                    { DocumentStatus::BANNED,     3   },
                    { DocumentStatus::REMOVED,    4   },
                },

                {
                    { DocumentStatus::ACTUAL,     100 },
                    { DocumentStatus::IRRELEVANT, 0   },
                    { DocumentStatus::BANNED,     0   },
                    { DocumentStatus::REMOVED,    0   },
                },

                {
                    { DocumentStatus::ACTUAL,     0   },
                    { DocumentStatus::IRRELEVANT, 100 },
                    { DocumentStatus::BANNED,     100 },
                    { DocumentStatus::REMOVED,    100 },
                },

                {
                    { DocumentStatus::ACTUAL,     100 },
                    { DocumentStatus::IRRELEVANT, 100 },
                    { DocumentStatus::BANNED,     100 },
                    { DocumentStatus::REMOVED,    100 },
                },

                {
                    { DocumentStatus::ACTUAL,     100 },
                    { DocumentStatus::IRRELEVANT, 0   },
                    { DocumentStatus::BANNED,     100 },
                    { DocumentStatus::REMOVED,    0   },
                },

                {
                    { DocumentStatus::ACTUAL,     0   },
                    { DocumentStatus::IRRELEVANT, 100 },
                    { DocumentStatus::BANNED,     0   },
                    { DocumentStatus::REMOVED,    100 },
                },
            };

        const std::vector< std::function< bool( int, DocumentStatus, int ) > > &
        test_parameters_predicate =
            {
                []( int document_id, DocumentStatus status, int rating ) { return ( status == DocumentStatus::ACTUAL ); },
                []( int document_id, DocumentStatus status, int rating ) { return ( status == DocumentStatus::BANNED ); },
                []( int document_id, DocumentStatus status, int rating ) { return ( status == DocumentStatus::IRRELEVANT ); },
                []( int document_id, DocumentStatus status, int rating ) { return ( status == DocumentStatus::REMOVED ); },
                []( int document_id, DocumentStatus status, int rating )
                    {
                        return
                                (
                                        ( status == DocumentStatus::REMOVED )
                                    || ( status == DocumentStatus::BANNED )
                                );
                    },
                []( int document_id, DocumentStatus status, int rating )
                    {
                        return
                                (
                                        ( status == DocumentStatus::ACTUAL )
                                    || ( status == DocumentStatus::BANNED )
                                );
                    },
                []( int document_id, DocumentStatus status, int rating ) { return true; },
                []( int document_id, DocumentStatus status, int rating ) { return false; },
                []( int document_id, DocumentStatus status, int rating ) { return ( document_id % 2 == 0 ); },
                []( int document_id, DocumentStatus status, int rating )
                    {
                        return
                                (
                                        ( document_id > 4 )
                                    && ( document_id < 10 )
                                    && ( status != DocumentStatus::REMOVED )
                                    && ( rating > 0 )
                                );
                    },
                []( int document_id, DocumentStatus status, int rating )
                    {
                        return
                                (
                                        ( document_id > 4 )
                                    && ( document_id < 10 )
                                    && ( status != DocumentStatus::ACTUAL )
                                    && ( rating > 0 )
                                );
                    },
            };

        for( const std::map< DocumentStatus, int > & number_of_documents_to_add
                : test_parameters_document_status )
            {
                for( const auto & predicate : test_parameters_predicate )
                    {
                        SearchServer server{ {} };

                        int count_of_expected_documents_of_predicate = 0;
                        int document_id = -1;

                        for( const DocumentStatus document_status
                                : document_status_values_for_iteratation )
                            {
                                if( number_of_documents_to_add.count( document_status ) )
                                    {
                                        for( int i = 0
                                                ; i < number_of_documents_to_add.at( document_status )
                                                ; ++i )
                                            {
                                                ++document_id;

                                                server.AddDocument(
                                                        document_id,
                                                        content,
                                                        document_status,
                                                        ratings );

                                                if( predicate( document_id, document_status, ratings_avg ) )
                                                    {
                                                        ++count_of_expected_documents_of_predicate;
                                                    }
                                            }
                                    }
                            }

                        const int expected_value
                                                = std::min(
                                                        count_of_expected_documents_of_predicate,
                                                        MAX_RESULT_DOCUMENT_COUNT );

                        const std::vector< Document > & found_docs = server.FindTopDocuments( query, predicate );

                        const int actual_value
                                                = static_cast< int >(
                                                                        found_docs.size() );

                        ASSERT_EQUAL( expected_value, actual_value );
                    }
            }
    }

void
TestFindDocument()
    {
        using namespace std;

        const std::string & content = "cat in the city"s;
        const std::vector< int > & ratings = { 1, 2, 3 };
        constexpr DocumentStatus status = DocumentStatus::ACTUAL;

        const std::vector< std::vector< std::string > > &
        test_parameters_document_contents =
            {
                {},

                { ""s, },

                {
                    content,
                    content,
                    content,
                    ""s,
                },

                { ""s, ""s, ""s, ""s, },

                { content, content, content, content, },

                {
                    "in"s,
                    "in in"s,
                    "in in at"s,
                    " inn    "s,
                    " in    "s,
                },
            };

        const std::vector< std::string > & queries =
            {
                ""s,
                "in"s,
                "in at between above on"s,
                content
            };

        for( const std::vector< std::string > & document_contents : test_parameters_document_contents )
            {
                SearchServer server{ {} };

                const std::vector< std::set< std::string > > & documents
                        = [&document_contents, &server, &ratings]()
                    {
                        int document_id = -1;
                        std::vector< std::set< std::string > > result;

                        for( const std::string & text : document_contents )
                            {
                                server.AddDocument( ++document_id, text, status, ratings );

                                const std::set< std::string > & document_words = [&text]()
                                    {
                                        std::set< std::string > result;
                                        std::string word = ""s;
                                        for( const char c : text )
                                            {
                                                if( c == ' ' )
                                                    {
                                                        result.insert( word );
                                                        word.clear();
                                                    }
                                                else
                                                    {
                                                        word += c;
                                                    }
                                            }
                                        result.insert( word );

                                        return result;
                                    }();

                                result.push_back( document_words );
                            }

                        return result;
                    }();

                for( const std::string & query : queries )
                    {
                        const std::set< std::string > & query_words = [&query]()
                            {
                                std::set< std::string > result;
                                std::string word = ""s;

                                for( const char c : query )
                                    {
                                        if( c == ' ' )
                                            {
                                                result.insert( word );
                                                word.clear();
                                            }
                                        else
                                            {
                                                word += c;
                                            }
                                    }

                                if( !( word.size() == 0 && result.size() == 0 ) )
                                    {
                                        result.insert( word );
                                    }

                                return result;
                            }();

                        const int expected_value = [&documents, &query_words]()
                            {
                                int result = 0;
                                for( const std::set< std::string > & document : documents )
                                    {
                                        for( const std::string & query_word : query_words )
                                            {
                                                if( document.count( query_word ) )
                                                    {
                                                        ++result;
                                                        break;
                                                    }
                                            }
                                    }

                                return
                                        std::min(
                                                result,
                                                MAX_RESULT_DOCUMENT_COUNT );
                            }();

                        const std::vector< Document > & found_docs = server.FindTopDocuments( query );

                        const int actual_value
                                                = static_cast< int >(
                                                                        found_docs.size() );

                        ASSERT_EQUAL( expected_value, actual_value );
                    }
            }
    }

void
TestRatings()
    {
        using namespace std;

        constexpr DocumentStatus document_status = DocumentStatus::ACTUAL;
        const std::string & content = "cat in the city"s;
        const std::string & query = "in"s;

        const std::vector< std::vector< int > > & test_parameters_ratings =
            {
                {
                    // {},                 // vector of ratings should not be empty
                    { -1 },                // -1
                    { -1, -3, -1, -1, 0 }, // -6 / 5 = -1.2 ~ -1
                    { -1, -3, 1 },         // -3 / 3 = -1
                    { -4, -3, -4, 2 },     // -9 / 5 = -1.8 ~ -2
                    { 0 },                 //  0
                    { 0, 0, 0 },           //  0
                    { 0, 1, 1, 1, 1 },     //  4 / 5 = 0.8 ~ 0
                    { 1 },                 //  1
                    { 1, 1, 2 },           //  4 / 3 = 1.33
                    { 1, 2 },              //  1.5
                    { 2 },                 //  2
                    { 1, 2, 3 },           //  2
                    { 9, 0, 0, 0, 0 },     //  9 / 5 = 1.8
                }
            };

        for( const std::vector< int > & ratings : test_parameters_ratings )
            {
                const int rating_sum = accumulate( ratings.begin(), ratings.end(), 0 );

                const int expected_avg_rating
                                            = rating_sum
                                                            /
                                                                static_cast< int >( ratings.size() );

                SearchServer server{ {} };

                server.AddDocument( 0, content, document_status, ratings );

                const std::vector< Document > & found_docs = server.FindTopDocuments( query );

                const int actual_avg_rating = found_docs[ 0 ].rating;

                ASSERT_EQUAL( expected_avg_rating, actual_avg_rating );
            }
    }

void
TestRelevance()
    {
        using namespace std;

        constexpr DocumentStatus document_status = DocumentStatus::ACTUAL;
        const std::vector< int > & ratings = { 1, 2, 3 };

        {
            {
                SearchServer server{ {} };

                server.AddDocument( 0, "cat in"s,   document_status, ratings );
                server.AddDocument( 1, "the city"s, document_status, ratings );

                const std::vector< Document > & found_docs = server.FindTopDocuments( "cat in the city"s );

                ASSERT_EQUAL( found_docs[ 0 ].relevance, found_docs[ 1 ].relevance );
            }

            {
                SearchServer server{ {} };

                server.AddDocument( 0, " "s, document_status, ratings );
                server.AddDocument( 1, ""s,  document_status, ratings );

                const std::vector< Document > & found_docs = server.FindTopDocuments( " "s );

                ASSERT_EQUAL( found_docs.size(), static_cast< size_t >( 0 ) );
            }

            {
                SearchServer server{ {} };

                server.AddDocument( 0, ""s,    document_status, ratings );
                server.AddDocument( 1, " "s,   document_status, ratings );
                server.AddDocument( 2, "cat"s, document_status, ratings );
                server.AddDocument( 3, "cat  "s, document_status, ratings );
                server.AddDocument( 4, "cat cat "s, document_status, ratings );

                const std::vector< Document > & found_docs = server.FindTopDocuments( " "s );

                ASSERT_EQUAL( found_docs.size(), static_cast< size_t >( 0 ) );
            }

            {
                SearchServer server{ { " "s } };

                server.AddDocument( 0, ""s,    document_status, ratings );
                server.AddDocument( 1, " "s,   document_status, ratings );
                server.AddDocument( 2, "cat"s, document_status, ratings );
                server.AddDocument( 3, "cat  "s, document_status, ratings );
                server.AddDocument( 4, "cat cat "s, document_status, ratings );

                const std::vector< Document > & found_docs = server.FindTopDocuments( " "s );

                ASSERT_EQUAL( found_docs.size(), static_cast< size_t >( 0 ) );
            }

            {
                SearchServer server{ {} };

                server.AddDocument( 0, "белый кот модный ошейник"s,          document_status, ratings );
                server.AddDocument( 1, "пушистый кот пушистый хвост"s,       document_status, ratings );
                server.AddDocument( 2, "ухоженный пёс выразительные глаза"s, document_status, ratings );

                const std::vector< Document > & found_docs = server.FindTopDocuments( "пушистый ухоженный кот"s );

                const std::map< int, double > & calculated_relevance = [&found_docs]()
                    {
                        std::map< int, double > result;
                        for( const Document & document : found_docs )
                            {
                                result[ document.id ] = document.relevance;
                            }

                        return result;
                    }();

                const std::map< int, double > & document_to_expected_relevance =
                    {
                        { 0,     ( log(3./1)*(0./4) + log(3./1)*(0./4) + log(3./2)*(1./4) )      },
                        { 1,     ( log(3./1)*(2./4) + log(3./1)*(0./4) + log(3./2)*(1./4) )      },
                        { 2,     ( log(3./1)*(0./4) + log(3./1)*(1./4) + log(3./2)*(0./4) )      }
                    };

                ASSERT_EQUAL( document_to_expected_relevance.at( 0 ), calculated_relevance.at( 0 ) );
                ASSERT_EQUAL( document_to_expected_relevance.at( 1 ), calculated_relevance.at( 1 ) );
                ASSERT_EQUAL( document_to_expected_relevance.at( 2 ), calculated_relevance.at( 2 ) );
            }

            {
                SearchServer server{ {} };

                server.AddDocument( 0, "белый кот модный ошейник"s,          document_status, ratings );
                server.AddDocument( 1, "пушистый кот пушистый хвост"s,       document_status, ratings );
                server.AddDocument( 2, "ухоженный пёс выразительные глаза"s, document_status, ratings );

                const std::vector< Document > & found_docs = server.FindTopDocuments( "-пушистый ухоженный кот"s );

                const std::map< int, double > & calculated_relevance = [&found_docs]()
                    {
                        std::map< int, double > result;
                        for( const Document & document : found_docs )
                            {
                                result[ document.id ] = document.relevance;
                            }

                        return result;
                    }();

                const std::map< int, double > & document_to_expected_relevance =
                    {
                        { 0,     ( log(3./1)*(0./4) + log(3./1)*(0./4) + log(3./2)*(1./4) )      },
                        { 2,     ( log(3./1)*(0./4) + log(3./1)*(1./4) + log(3./2)*(0./4) )      }
                    };

                ASSERT_EQUAL( found_docs.size(), static_cast< size_t >( 2 ) );

                ASSERT_EQUAL( document_to_expected_relevance.at( 0 ), calculated_relevance.at( 0 ) );
                ASSERT_EQUAL( document_to_expected_relevance.at( 2 ), calculated_relevance.at( 2 ) );
            }
        }
    }

void
TestSorting()
    {
        using namespace std;

        const auto & Comparator = []( const Document & lhs, const Document & rhs )
            {
                if( abs( lhs.relevance - rhs.relevance ) < 1e-6 )
                    {
                        return
                                lhs.rating > rhs.rating;
                    }
                else
                    {
                        return
                                lhs.relevance > rhs.relevance;
                    }
            };

        const std::string & query = "белый красный оранжевый голубой синий"s;
        constexpr DocumentStatus document_status = DocumentStatus::ACTUAL;

        const std::vector< std::vector< int > > & test_parameters_ratings =
            {
                {
                    -10, 0, 20, 20, 9001, 100, 200, 200, 300, 300, 200,
                },

                {
                    -10, 0, 100, 20, 20, 200, 200, 300, 9001, 1000, 5,
                },

                {
                    9001, 300, 200, 20, 100, 200, 20, -10, 0, -25, 9,
                },

                {
                    9001, 300, 200, 200, 20, 20, 100, 0, -10, 200, 300,
                },
            };

        const std::vector< std::string > & test_parameters_document_contents =
            {
                "                         зеленый голубой       фиолетовый"s,
                "красный оранжевый желтый зеленый голубой синий фиолетовый"s,
                "красный оранжевый        зеленый         синий фиолетовый"s,
                "                                                         "s,
                "                                 голубой                 "s,
                "красный оранжевый желтый зеленый         синий фиолетовый"s,
                "красный оранжевый желтый                 синий фиолетовый"s,
                "красный оранжевый желтый"s,
                "красный оранжевый       "s,
                "красный оранжевый желтый"s,
                "красный           желтый"s,
            };

        for( const std::vector< int > & ratings : test_parameters_ratings )
            {
                SearchServer server{ {} };

                int document_id = -1;

                for( const int rating : ratings )
                    {
                        ++document_id;

                        server.AddDocument(
                                document_id,
                                test_parameters_document_contents.at( document_id ),
                                document_status,
                                { rating } );
                    }

                const std::vector< Document > & found_docs = server.FindTopDocuments( query );

                if( found_docs.size() > static_cast< size_t >( 1 ) )
                    {
                        Document lhs = found_docs.at( 0 );
                        Document rhs = found_docs.at( 1 );

                        for( size_t i = 0; ( i + 1 ) < found_docs.size(); ++i )
                            {
                                if( i > 0 )
                                    {
                                        lhs = rhs;
                                        rhs = found_docs.at( i + 1 );
                                    }

                                ASSERT( !Comparator( rhs, lhs ) );
                            }
                    }
            }
    }

void
TestMinusWords()
    {
        using namespace std;

        constexpr DocumentStatus document_status = DocumentStatus::ACTUAL;
        const std::vector< int > & ratings = { 1, 2, 3 };

        int counter = -1;

        const std::map< int, std::string > & test_parameters_documents_to_text =
            {
                { ++counter ,        "                         зеленый голубой       фиолетовый"s },
                { ++counter ,        ""s                                                          },
                { ++counter ,        "красный оранжевый желтый зеленый голубой синий фиолетовый"s },
                { ++counter ,        "красный оранжевый        зеленый         синий фиолетовый"s },
                { ++counter ,        "                                               фиолетовый"s },
                { ++counter ,        "                                 голубой синий фиолетовый"s },
                { ++counter ,        "красный оранжевый желтый зеленый зеленый синий фиолетовый"s },
                { ++counter ,        "                                                         "s },
                { ++counter ,        "красный оранжевый желтый                 синий фиолетовый"s },
                { ++counter ,        "красный оранжевый желтый          зеленый      фиолетовый"s },
                { ++counter ,        "красный оранжевый         зеленый              фиолетовый"s },
                { ++counter ,        "красный оранжевый желтый                       фиолетовый"s },
                { ++counter ,        "красный           желтый         зеленый синий фиолетовый"s },
            };

        const std::vector< std::string > & test_parameters_queries =
            {
                "    -красный оранжевый"s,
                "оранжевый -желтый     "s,
                "желтый -зеленый       "s,
                                      ""s,
                "                      "s,
                "   зеленый -голубой   "s,
                "голубой -синий        "s,
                "синий -фиолетовый     "s,
                "фиолетовый   красный  "s,
            };

        {
            SearchServer server{ {} };

            std::map< int, std::set< std::string > > test_parameters_document_words;

            for( const std::pair< int, std::string > & document_to_text : test_parameters_documents_to_text )
                {
                    server.AddDocument(
                            document_to_text.first,
                            document_to_text.second,
                            document_status,
                            ratings );

                    std::set< std::string > document_words;
                    std::string word = ""s;
                    for( const char c : document_to_text.second )
                        {
                            if( c == ' ' )
                                {
                                    document_words.insert( word );
                                    word.clear();
                                }
                            else
                                {
                                    word += c;
                                }
                        }
                    document_words.insert( word );

                    test_parameters_document_words[ document_to_text.first ] = document_words;
                }

            for( const std::string & query : test_parameters_queries )
                {
                    std::set< int > expected_document_id;
                    std::set< int > banned_document_id;

                    {
                        std::set< std::string > query_plus_words;
                        std::set< std::string > query_minus_words;

                        std::string word;

                        for( const char c : query )
                            {
                                if( c == ' ' )
                                    {
                                        if( word.size() > 0 && word[ 0 ] == '-' )
                                            {
                                                query_minus_words.insert( word );
                                            }
                                        else
                                            {
                                                query_plus_words.insert( word );
                                            }

                                        word.clear();
                                    }
                                else
                                    {
                                        word += c;
                                    }
                            }

                            if( word.size() > 0 && word[ 0 ] == '-' )
                                {
                                    query_minus_words.insert( word );
                                }
                            else
                                {
                                    query_plus_words.insert( word );
                                }

                        for( const std::pair< int, std::set< std::string > > & document_to_words
                                : test_parameters_document_words )
                            {
                                for( const std::string & plus_word : query_plus_words )
                                    {
                                        if( document_to_words.second.count( plus_word ) )
                                            {
                                                expected_document_id.insert( document_to_words.first );
                                                break;
                                            }
                                    }

                                for( const std::string & minus_word : query_minus_words )
                                    {
                                        if( document_to_words.second.count( minus_word ) > 0 )
                                            {
                                                banned_document_id.insert( document_to_words.first );
                                                expected_document_id.erase( document_to_words.first );
                                                break;
                                            }
                                    }
                            }

                        const std::vector< Document > & found_docs = server.FindTopDocuments( query );

                        const std::set< int > & actual_document_id = [&found_docs]()
                            {
                                std::set< int > result;
                                for( const Document & document : found_docs )
                                    {
                                        result.insert( document.id );
                                    }

                                return result;
                            }();


                        int documents_contained_from_expected_in_actual = 0;
                        int document_contained_from_banned_in_actual    = 0;

                        for( const int banned : banned_document_id )
                            {
                                if( actual_document_id.count( banned ) > 0 )
                                    {
                                        ++document_contained_from_banned_in_actual;
                                    }
                            }

                        for( const int expected : expected_document_id )
                            {
                                if( actual_document_id.count( expected ) )
                                    {
                                        ++documents_contained_from_expected_in_actual;
                                    }
                            }

                        const int expected_value
                                                = std::min(
                                                        documents_contained_from_expected_in_actual,
                                                        MAX_RESULT_DOCUMENT_COUNT );

                        ASSERT_EQUAL( expected_value, documents_contained_from_expected_in_actual );
                        ASSERT_EQUAL( document_contained_from_banned_in_actual, 0 );
                    }
                }
        }
    }

void
TestStructDocument()
    {
        using namespace std;

        {
            const Document doc { 1, 1.5, 2 };

            ASSERT_EQUAL( doc.id,        1   );
            ASSERT_EQUAL( doc.relevance, 1.5 );
            ASSERT_EQUAL( doc.rating,    2   );
        }

        {
            constexpr Document doc {};

            ASSERT_EQUAL( doc.id       , 0   );
            ASSERT_EQUAL( doc.relevance, 0.0 );
            ASSERT_EQUAL( doc.rating   , 0   );
        }
    }

void
TestGetDocumentId()
    {
        using namespace std;

        {
            SearchServer search_server { "и в на"s };

            {
                {
                    const std::vector< int > & expected_incorrect_document_ids =
                        {
                            -1, 0, 1, 2, 3,
                        };

                    for( const int expected_incorrect_document_id
                            : expected_incorrect_document_ids )
                        {
                            bool exception_was_thrown = false;

                            try
                                {
                                    (void) search_server.GetDocumentId( expected_incorrect_document_id );
                                }
                            catch( const std::out_of_range & e )
                                {
                                    exception_was_thrown = true;
                                }

                            ASSERT( exception_was_thrown );
                        }
                }
            }

            search_server.AddDocument( 42, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 } );

            {
                {
                    const std::vector< int > & expected_incorrect_document_ids =
                        {
                            -1,
                        };

                    for( const int expected_incorrect_document_id
                            : expected_incorrect_document_ids )
                        {
                            bool exception_was_thrown = false;

                            try
                                {
                                    (void) search_server.GetDocumentId( expected_incorrect_document_id );
                                }
                            catch( const std::out_of_range & e )
                                {
                                    exception_was_thrown = true;
                                }

                            ASSERT( exception_was_thrown );
                        }
                }

                ASSERT_EQUAL( search_server.GetDocumentId( 0 ),  42 );

                {
                    const std::vector< int > & expected_incorrect_document_ids =
                        {
                            1, 2, 3,
                        };

                    for( const int expected_incorrect_document_id
                            : expected_incorrect_document_ids )
                        {
                            bool exception_was_thrown = false;

                            try
                                {
                                    (void) search_server.GetDocumentId( expected_incorrect_document_id );
                                }
                            catch( const std::out_of_range & e )
                                {
                                    exception_was_thrown = true;
                                }

                            ASSERT( exception_was_thrown );
                        }
                }
            }

            search_server.AddDocument( 41, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 } );

            {
                {
                    const std::vector< int > & expected_incorrect_document_ids =
                        {
                            -1,
                        };

                    for( const int expected_incorrect_document_id
                            : expected_incorrect_document_ids )
                        {
                            bool exception_was_thrown = false;

                            try
                                {
                                    (void) search_server.GetDocumentId( expected_incorrect_document_id );
                                }
                            catch( const std::out_of_range & e )
                                {
                                    exception_was_thrown = true;
                                }

                            ASSERT( exception_was_thrown );
                        }
                }

                ASSERT_EQUAL( search_server.GetDocumentId( 0 ), 41 );
                ASSERT_EQUAL( search_server.GetDocumentId( 1 ), 42 );

                {
                    const std::vector< int > & expected_incorrect_document_ids =
                        {
                            2, 3,
                        };

                    for( const int expected_incorrect_document_id
                            : expected_incorrect_document_ids )
                        {
                            bool exception_was_thrown = false;

                            try
                                {
                                    (void) search_server.GetDocumentId( expected_incorrect_document_id );
                                }
                            catch( const std::out_of_range & e )
                                {
                                    exception_was_thrown = true;
                                }

                            ASSERT( exception_was_thrown );
                        }
                }
            }

            search_server.AddDocument( 43, "в"s, DocumentStatus::ACTUAL, { 1, 2, 3 } );

            {
                {
                    const std::vector< int > & expected_incorrect_document_ids =
                        {
                            -1,
                        };

                    for( const int expected_incorrect_document_id
                            : expected_incorrect_document_ids )
                        {
                            bool exception_was_thrown = false;

                            try
                                {
                                    (void) search_server.GetDocumentId( expected_incorrect_document_id );
                                }
                            catch( const std::out_of_range & e )
                                {
                                    exception_was_thrown = true;
                                }

                            ASSERT( exception_was_thrown );
                        }
                }

                ASSERT_EQUAL( search_server.GetDocumentId( 0 ),  41 );
                ASSERT_EQUAL( search_server.GetDocumentId( 1 ),  42 );
                ASSERT_EQUAL( search_server.GetDocumentId( 2 ),  43 );

                {
                    const std::vector< int > & expected_incorrect_document_ids =
                        {
                            3,
                        };

                    for( const int expected_incorrect_document_id
                            : expected_incorrect_document_ids )
                        {
                            bool exception_was_thrown = false;

                            try
                                {
                                    (void) search_server.GetDocumentId( expected_incorrect_document_id );
                                }
                            catch( const std::out_of_range & e )
                                {
                                    exception_was_thrown = true;
                                }

                            ASSERT( exception_was_thrown );
                        }
                }
            }
        }
    }

void
TestMatchInvalidDocuments()
    {
        using namespace std;

        SearchServer server{ {} };

        constexpr int document_id = 1;
        server.AddDocument( document_id, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 } );

        const std::vector< std::string > & expected_incorrect_queries =
            {
                "-"s,
                "--cat"s,
                "скво\x12рец"s,
            };

        for( const std::string & query : expected_incorrect_queries )
            {
                bool exception_was_thrown = false;

                try
                    {
                        (void) server.MatchDocument( query, document_id );
                    }
                catch( const std::invalid_argument & e )
                    {
                        exception_was_thrown = true;
                    }

                ASSERT( exception_was_thrown );
            }

        (void) server.MatchDocument( "cat-cat"s, document_id  );
        (void) server.MatchDocument( "cat--cat"s, document_id );
    }

void
TestConstructorInitialisation()
    {
        using namespace std;


        //         SearchServer doesn't have the default constructor
        //         {
        //             SearchServer server;
        //         }

        {
            SearchServer server{ {} };
        }

        {
            SearchServer server( "и в на"s );
        }

        {
            SearchServer server( "  и в  на  "s );
        }

        {
            const std::vector< std::string > & stop_words_vector = { "и"s, "в"s, "на"s, ""s, "в"s, " "s };
            SearchServer server( stop_words_vector );
        }

        {
            const std::set< std::string > & stop_words_set = { "и"s, "в"s, "на"s, ""s, " "s };
            SearchServer server( stop_words_set );
        }

        {
            {
                bool exception_was_thrown = false;

                try
                    {
                        SearchServer server( "скво\x12рец"s );
                    }
                catch( const std::invalid_argument & e )
                    {
                        exception_was_thrown = true;
                    }

                ASSERT( exception_was_thrown );
            }

            {
                bool exception_was_thrown = false;

                try
                    {
                        const std::vector< std::string > & stop_words_vector = { "скво\x12рец"s };
                        SearchServer server( stop_words_vector );
                    }
                catch( const std::invalid_argument & e )
                    {
                        exception_was_thrown = true;
                    }

                ASSERT( exception_was_thrown );
            }

            {
                bool exception_was_thrown = false;

                try
                    {
                        const std::set< std::string > & stop_words_set = { "скво\x12рец"s };
                        SearchServer server( stop_words_set );
                    }
                catch( const std::invalid_argument & e )
                    {
                        exception_was_thrown = true;
                    }

                ASSERT( exception_was_thrown );
            }
        }

        const std::vector< std::string > & test_parameters_stop_words =
            {
                "-"s,
                "--cat"s
                "cat--cat"s,
                "cat-cat"s,
            };

        for( const std::string & stop_words : test_parameters_stop_words )
            {
                {
                    SearchServer server( stop_words );
                }

                {
                    const std::vector< std::string > & stop_words_vector = { stop_words };
                    SearchServer server( stop_words_vector );
                }

                {
                    const std::set< std::string > & stop_words_set = { stop_words };
                    SearchServer server( stop_words_set );
                }
            }
    }

void
TestGetDocumentIdOverflow()
    {
        using namespace std;

        constexpr DocumentStatus document_status = DocumentStatus::ACTUAL;
        const std::vector< int > & ratings = { 1, 2, 3 };
        const std::string & content = "cat in the city"s;

        const std::vector< std::function< bool( int ) > > &
        test_parameters_predicate_which_document_id_add =
            {
                []( int document_id ) { return true; },
                []( int document_id ) { return document_id % 2 == 0; },
            };

        const std::vector< int > & test_parameters_max_document_id =
            {
                0, 1, 2, 10,
            };

        for( const auto predicate_of_document_id
                : test_parameters_predicate_which_document_id_add )
            {
                for( const int max_document_id : test_parameters_max_document_id )
                    {
                        SearchServer server{ {} };

                        bool some_documents_skipped = false;

                        for( int current_document_id = 0
                                ; current_document_id <= max_document_id
                                ; ++current_document_id )
                            {
                                {
                                    if( predicate_of_document_id( current_document_id ) )
                                        {
                                            server.AddDocument(
                                                    current_document_id,
                                                    content,
                                                    document_status,
                                                    ratings );
                                        }
                                    else
                                        {
                                            some_documents_skipped = true;
                                        }
                                }

                                {
                                    if( some_documents_skipped )
                                        {
                                            bool exception_was_thrown = false;

                                            try
                                                {
                                                    (void) server.GetDocumentId( current_document_id );
                                                }
                                            catch( const std::out_of_range & e )
                                                {
                                                    exception_was_thrown = true;
                                                }

                                            ASSERT( exception_was_thrown );
                                        }
                                    else
                                        {
                                            (void) server.GetDocumentId( current_document_id );
                                        }
                                }
                            }
                    }
            }
    }

void
TestPaginator()
    {
        using namespace std;

        constexpr DocumentStatus document_status = DocumentStatus::ACTUAL;
        const std::vector< int > & ratings = { 1, 2, 3 };
        const std::string & document_content = "cat and dog"s;
        const std::string & stop_words = "and"s;
        const std::string & query = "cat"s;

        constexpr int param_max_number_of_docs_to_add_for_verification_in_test = 20;
        constexpr int param_max_page_size_for_verification_in_test = 20;

        for( int param_number_of_docs_to_add = 0
                ; param_number_of_docs_to_add < param_max_number_of_docs_to_add_for_verification_in_test
                ; ++ param_number_of_docs_to_add )
            {
                for( int param_page_size = 0
                        ; param_page_size < param_max_page_size_for_verification_in_test
                        ; ++ param_page_size )
                    {
                        SearchServer search_server( stop_words );

                        for( int i = 0; i < param_number_of_docs_to_add; ++i )
                            {
                                search_server.AddDocument( i, document_content, document_status, ratings );
                            }

                        const auto & found_pages = [&search_server, &query, param_page_size]()
                            {
                                const auto & p = Paginate(
                                        search_server.FindTopDocuments( query ),
                                        param_page_size );

                                auto result = std::vector( 0, *( p.end() ) );

                                for( auto page = p.begin()
                                        ; page != p.end()
                                        ; ++page )
                                    {
                                        result.push_back( *page );
                                    }

                                return result;
                            }();

                        const std::vector< size_t > actual_pages_sizes = [&found_pages]()
                            {
                                std::vector< size_t > result;

                                for( size_t i = 0; i < found_pages.size(); ++i )
                                    {
                                        result.push_back(
                                                distance( found_pages[ i ].begin(),
                                                found_pages[ i ].end() ) );
                                    }

                                return result;
                            }();

                        const std::vector< size_t > expected_pages_sizes =
                        [param_number_of_docs_to_add, param_page_size]()
                            {
                                if( param_page_size == 0 )
                                    {
                                        return std::vector< size_t >( 1, 0 );
                                    }

                                const int expected_number_of_found_docs
                                        = std::min(
                                                param_number_of_docs_to_add,
                                                MAX_RESULT_DOCUMENT_COUNT );

                                std::vector< size_t > result = std::vector(
                                        expected_number_of_found_docs / param_page_size,
                                        static_cast< size_t >( param_page_size ) );

                                if( const int remainder =
                                            expected_number_of_found_docs
                                            % param_page_size
                                        ; remainder != 0 )
                                    {
                                        result.push_back( remainder );
                                    }

                                return result;
                            }();

                        ASSERT_EQUAL( actual_pages_sizes, expected_pages_sizes );
                    }
            }
    }

void
TestRequestQueue()
    {
        using namespace std;

        constexpr DocumentStatus document_status = DocumentStatus::ACTUAL;
        const std::vector< int > & ratings = { 1, 2, 3 };
        const std::string & document_content = "cat and dog"s;
        const std::string & stop_words = "and"s;
        const std::string & query = "cat"s;

        SearchServer search_server( stop_words );
        RequestQueue request_queue( search_server );

        for( int i = 1; i <= 5; ++i )
            {
                search_server.AddDocument( i, query, document_status, ratings );
            }
        ASSERT_EQUAL( request_queue.GetNoResultRequests(), 0 );

        for( int i = 0; i < 1439; ++i )
            {
                request_queue.AddFindRequest( "empty request"s );
            }
        ASSERT_EQUAL( request_queue.GetNoResultRequests(), 1439 );

        request_queue.AddFindRequest( "empty request"s );
        ASSERT_EQUAL( request_queue.GetNoResultRequests(), 1440 );

        for( int i = 0; i < 10; ++i )
            {
                request_queue.AddFindRequest( "empty request"s );
            }
        ASSERT_EQUAL( request_queue.GetNoResultRequests(), 1440 );

        request_queue.AddFindRequest( "cat"s );
        ASSERT_EQUAL( request_queue.GetNoResultRequests(), 1439 );

        request_queue.AddFindRequest( "big collar"s );
        ASSERT_EQUAL( request_queue.GetNoResultRequests(), 1439 );

        request_queue.AddFindRequest( "cat"s );
        ASSERT_EQUAL( request_queue.GetNoResultRequests(), 1438 );
    }

// Функция TestSearchServer является точкой входа для запуска тестов
void
TestSearchServer()
    {
        using namespace std;

        RUN_TEST( TestAddDocumentByDocumentCount                  );
        RUN_TEST( TestAddDocumentByDocumentCountPerDocumentStatus );
        RUN_TEST( TestAddDocumentWithHyphen                       );
        RUN_TEST( TestAddInvalidDocuments                         );
        RUN_TEST( TestConstructorInitialisation                   );
        RUN_TEST( TestDocumentId                                  );
        RUN_TEST( TestDocumentStatusMatchDocument                 );
        RUN_TEST( TestExcludeStopWordsFromAddedDocumentContent    );
        RUN_TEST( TestFindDocument                                );
        RUN_TEST( TestFindTopInvalidDocuments                     );
        RUN_TEST( TestGetDocumentId                               );
        RUN_TEST( TestGetDocumentIdOverflow                       );
        RUN_TEST( TestMatchInvalidDocuments                       );
        RUN_TEST( TestMinusWords                                  );
        RUN_TEST( TestPaginator                                   );
        RUN_TEST( TestPredicate                                   );
        RUN_TEST( TestRatings                                     );
        RUN_TEST( TestRelevance                                   );
        RUN_TEST( TestRequestQueue                                );
        RUN_TEST( TestSorting                                     );
        RUN_TEST( TestStatus                                      );
        RUN_TEST( TestStructDocument                              );
    }

// --------- Окончание модульных тестов поисковой системы -----------
