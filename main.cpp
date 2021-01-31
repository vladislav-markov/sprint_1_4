#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <set>

using namespace std;

constexpr int MAX_RESULT_DOCUMENT_COUNT = 5;

string
ReadLine()
    {
        string s;
        getline( cin, s );
        return s;
    }

int
ReadLineWithNumber()
    {
        int result;
        cin >> result;
        ReadLine();
        return result;
    }

[[nodiscard]]
vector< string >
SplitIntoWords( const string & text )
    {
        const vector< string > & words = [&text]()
            {
                string word;
                vector< string > v;
                for_each( text.begin(), text.end(), [&word, &v]( char c )
                    {
                        if( c == ' ' )
                            {
                                v.push_back( word );
                                word.clear();
                            }
                        else
                            {
                                word += c;
                            }
                    } );
                v.push_back( word );

                return v;
            }();

        return words;
    }

struct Document
    {
        Document() = default;

        Document( int id_, double relevance_, int rating_ )
            :
                id( id_ ),
                relevance( relevance_ ),
                rating( rating_ )
            {

            }

        int id = 0;
        double relevance = 0.0;
        int rating = 0;
    };

enum class DocumentStatus
    {
        ACTUAL,
        IRRELEVANT,
        BANNED,
        REMOVED,
    };

class SearchServer
    {
        public:

            template < typename StopWordsCollection >
            explicit SearchServer( const StopWordsCollection & input_stop_words )
                : stop_words_( ParseStopWords( input_stop_words ) )
                {

                }

            explicit SearchServer( const string & stop_words_text )
                : SearchServer( SplitIntoWords( stop_words_text ) )
                {

                }

            void
            AddDocument(
                    int document_id,
                    const string & document,
                    DocumentStatus status,
                    const vector< int > & ratings )
                {
                    if( document_id < 0 )
                        {
                            const string & message = "Попытка добавить документ с отрицательным id"s;
                            throw invalid_argument( message );
                        }
                    else if( documents_.count( document_id ) )
                        {
                            const string & message = "Попытка добавить документ c id ранее добавленного документа"s;
                            throw invalid_argument( message );
                        }
                    else if( const vector< string > & document_words = SplitIntoWords( document )
                            ; any_of(
                                    document_words.begin(),
                                    document_words.end(),
                                    []( const string & document_word ){ return !IsValidWord( document_word ); } ) )
                        {
                            const string & message
                                    = "Наличие недопустимых символов (с кодами от 0 до 31) "s
                                        + "в тексте добавляемого документа"s;

                            throw invalid_argument( message );
                        }

                    const vector< string > & words = SplitIntoWordsNoStop( document );
                    const double inv_word_count = 1.0 / words.size();

                    for( const string & word : words )
                        {
                            word_to_document_freqs_[ word ][ document_id ] += inv_word_count;
                        }

                    documents_.emplace(
                            document_id,
                            DocumentData
                                {
                                    ComputeAverageRating( ratings ),
                                    status,
                                } );
                }

            template < typename DocumentPredicate >
            vector< Document >
            FindTopDocuments(
                    const string & raw_query,
                    DocumentPredicate document_predicate ) const
                {
                    for( const string & raw_query_word : SplitIntoWords( raw_query ) )
                        {
                            if( !IsValidWord( raw_query_word ) )
                                {
                                    const string & message
                                            = "В словах поискового запроса "s
                                                + "есть недопустимые символы с кодами от 0 до 31"s;

                                    throw invalid_argument( message );
                                }
                            else if( !IsValidHyphenWordSingleMinus( raw_query_word ) )
                                {
                                    const string & message
                                            = "Отсутствие текста после символа 'минус', например, 'пушистый -'."s;

                                    throw invalid_argument( message );
                                }
                            else if( !IsValidHyphenWordMultiMinus( raw_query_word ) )
                                {
                                    const string & message
                                            = "Наличие более чем одного минуса перед словами, "s
                                                + "которых не должно быть в искомых документах, например, "s
                                                + "'пушистый --кот'. В середине слов минусы разрешаются, "s
                                                + "например: 'иван-чай.'"s;

                                    throw invalid_argument( message );
                                }
                        }

                    const Query & query = ParseQuery( raw_query );
                    auto matched_documents = FindAllDocuments( query, document_predicate );

                    sort(
                            matched_documents.begin(),
                            matched_documents.end(),
                            []( const Document & lhs, const Document & rhs )
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
                                } );

                    if( matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT )
                        {
                            matched_documents.resize( MAX_RESULT_DOCUMENT_COUNT );
                        }

                    return matched_documents;
                }

            vector< Document >
            FindTopDocuments(
                    const string & raw_query,
                    DocumentStatus status = DocumentStatus::ACTUAL ) const
                {
                    return FindTopDocuments(
                            raw_query,
                            [status]( int document_id, DocumentStatus document_status, int rating )
                                {
                                    return
                                            document_status == status;
                                } );
                }

            int
            GetDocumentCount() const
                {
                    return documents_.size();
                }

            int
            GetDocumentId( const int index ) const
                {
                    const string & error_message
                            = "Индекс переданного документа выходит за "s
                                + "пределы допустимого диапазона (0; количество документов)"s;

                    if( index < 0 )
                        {
                            throw out_of_range( error_message );
                        }

                    /*                                                                                          *
                     *      Допустим, максимальное значение типа < int > = 2.                                   *
                     *      И документы = { 0, 1, 2 };                                                          *
                     *      значит size_t = 3                                                                   *
                     *      Тип аргумента-индекса - < int >. То есть, мы можем ожидать максимум значение 2.     *
                     *      Получается, считаем корректной проверку ( index '<' size_t ), т.к. ( 2 '<' 3 )      *
                     *      соответственно, на проверку '=>' бросаем исключение,                                *
                     *      чтобы не вызывать advance() ниже для больших значений входного аргумента            *
                     */

                    else if( const size_t input_index = static_cast< size_t >( index )
                            ; input_index >= documents_.size() )
                        {
                            throw out_of_range( error_message );
                        }

                    auto documents_iterator = documents_.begin();
                    advance( documents_iterator, index );

                    return
                            documents_iterator->first;
                }

            tuple< vector< string >, DocumentStatus >
            MatchDocument(
                    const string & raw_query,
                    int document_id ) const
                {
                    for( const string & raw_query_word : SplitIntoWords( raw_query ) )
                        {
                            if( !IsValidWord( raw_query_word ) )
                                {
                                    const string & message
                                            = "В словах поискового запроса есть недопустимые "s
                                                + "символы с кодами от 0 до 31"s;

                                    throw invalid_argument( message );
                                }
                            else if( !IsValidHyphenWordSingleMinus( raw_query_word ) )
                                {
                                    const string & message
                                            = "Отсутствие текста после символа 'минус', например, 'пушистый -'."s;

                                    throw invalid_argument( message );
                                }
                            else if( !IsValidHyphenWordMultiMinus( raw_query_word ) )
                                {
                                    const string & message
                                            = "Наличие более чем одного минуса перед словами, "s
                                                + "которых не должно быть в искомых документах, например, "s
                                                + "'пушистый --кот'. В середине слов минусы разрешаются, "s
                                                + "например: 'иван-чай.'"s;

                                    throw invalid_argument( message );
                                }
                        }

                    const Query & query = ParseQuery( raw_query );

                    vector< string > matched_words;
                    for( const string & word : query.plus_words )
                        {
                            if( word_to_document_freqs_.count( word ) == 0)
                                {
                                    continue;
                                }
                            if( word_to_document_freqs_.at( word ).count( document_id ) )
                                {
                                    matched_words.push_back( word );
                                }
                        }
                    for( const string & word : query.minus_words )
                        {
                            if( word_to_document_freqs_.count( word ) == 0 )
                                {
                                    continue;
                                }
                            if( word_to_document_freqs_.at( word ).count( document_id ) )
                                {
                                    matched_words.clear();
                                    break;
                                }
                        }

                    return
                            { matched_words, documents_.at( document_id ).status };
                }

        private:
            struct DocumentData
                {
                    int rating;
                    DocumentStatus status;
                };

            const set< string > stop_words_;
            map< string, map< int, double > > word_to_document_freqs_;
            map< int, DocumentData > documents_;

            template < typename StopWordsCollection >
            [[nodiscard]]
            set< string >
            ParseStopWords( const StopWordsCollection & input_stop_words )
                {
                    if( input_stop_words.empty() )
                        {
                            return {};
                        }

                    if( any_of(
                            input_stop_words.begin(),
                            input_stop_words.end(),
                            []( const string & word ){ return !IsValidWord( word ); } ) )
                        {
                            const string & message
                                    = "Любое из переданных стоп-слов содержит недопустимые "s
                                        + "символы, то есть символы с кодами от 0 до 31"s;

                            throw invalid_argument( message );
                        }

                    const string & string_stop_words
                            = accumulate(
                                    input_stop_words.begin(),
                                    input_stop_words.end(),
                                    ""s,
                                    []( string & lhs, const string & rhs )
                                        {
                                            return
                                                    lhs += ( rhs + ' ' );
                                        } );

                    const vector< string > & stop_words = SplitIntoWords( string_stop_words );

                    const set< string > & result = [&stop_words]()
                        {
                            set< string > r;

                            for_each(
                                    stop_words.begin(),
                                    stop_words.end(),
                                    [&r]( const string & word ){ r.insert( word ); } );

                            return r;
                        }();

                    return result;
                }

            static bool
            IsValidHyphenWordMultiMinus( const string & word )
                {
                    if( ( word.size() > 1 ) && ( word[ 0 ] == '-' ) && ( word[ 1 ] == '-' ) )
                        {
                            return false;
                        }

                    return true;
                }

            static bool
            IsValidHyphenWordSingleMinus( const string & word )
                {
                    if( ( word.size() == 1 ) && ( word[ 0 ] == '-' ) )
                        {
                            return false;
                        }

                    return true;
                }

            static bool
            IsValidWord( const string & word )
                {
                    return none_of(
                            word.begin(),
                            word.end(),
                            []( char c )
                                {
                                    return
                                            ( c >= '\0' ) && ( c < ' ' );
                                } );
                }

            bool
            IsStopWord( const string & word ) const
                {
                    return
                            stop_words_.count( word ) > 0;
                }

            [[nodiscard]]
            vector< string >
            SplitIntoWordsNoStop( const string & text ) const
                {
                    const vector< string > & all_words = SplitIntoWords( text );

                    const vector< string > & words = [this, &all_words]()
                        {
                            vector< string > v;

                            copy_if(
                                    all_words.begin(),
                                    all_words.end(),
                                    back_inserter( v ),
                                    [this]( const string & word )
                                        {
                                            return !IsStopWord( word );
                                        } );

                            return v;
                        }();

                    return words;
                }

            [[nodiscard]]
            static int
            ComputeAverageRating( const vector< int > & ratings )
                {
                    if( ratings.empty() )
                        {
                            return 0;
                        }

                    const int ratings_sum
                                        = accumulate(
                                                ratings.begin(),
                                                ratings.end(),
                                                0,
                                                []( int lhs, const int rhs )
                                                    {
                                                        return
                                                                lhs += rhs;
                                                    } );

                    const int ratings_size
                                            = static_cast< int >( ratings.size() );

                    return
                            ratings_sum / ratings_size;
                }

            struct QueryWord
                {
                    string data;
                    bool is_minus;
                    bool is_stop;
                };

            [[nodiscard]]
            QueryWord
            ParseQueryWord( string text ) const
                {
                    bool is_minus = false;

                    // Word shouldn't be empty
                    if( text[ 0 ] == '-' )
                        {
                            is_minus = true;
                            text = text.substr( 1 );
                        }

                    return
                            { text, is_minus, IsStopWord( text ) };
                }

            struct Query
                {
                    set< string > plus_words;
                    set< string > minus_words;
                };

            [[nodiscard]]
            Query
            ParseQuery( const string & text ) const
                {
                    const Query & query = [&text, this]()
                        {
                            Query result;
                            for( const string & word : SplitIntoWords( text ) )
                                {
                                    const QueryWord & query_word = ParseQueryWord( word );

                                    if( !query_word.is_stop )
                                        {
                                            if( query_word.is_minus )
                                                {
                                                    result.minus_words.insert( query_word.data );
                                                }
                                            else
                                                {
                                                    result.plus_words.insert( query_word.data );
                                                }
                                        }
                                }

                            return result;
                        }();

                    return query;
                }

            [[nodiscard]]
            double
            ComputeWordInverseDocumentFreq(
                    const string & word ) const
                {
                    return log(
                                1.0
                                    * GetDocumentCount()
                                                        / word_to_document_freqs_.at( word ).size() );
                }

            template < typename DocumentPredicate >
            vector< Document >
            FindAllDocuments(
                    const Query & query,
                    DocumentPredicate document_predicate ) const
                {
                    const map< int, double > &
                    document_to_relevance = [this, &query, document_predicate]()
                        {
                            map< int, double > result;

                            for( const string & word : query.plus_words )
                                {
                                    if( word_to_document_freqs_.count( word ) == 0 )
                                        {
                                            continue;
                                        }

                                    const double inverse_document_freq
                                            = ComputeWordInverseDocumentFreq( word );

                                    for( const auto & [ document_id, term_freq ]
                                            : word_to_document_freqs_.at( word ) )
                                        {
                                            const auto & document_data = documents_.at( document_id );

                                            if( document_predicate(
                                                    document_id,
                                                    document_data.status,
                                                    document_data.rating ) )
                                                {
                                                    result[ document_id ]
                                                                            += term_freq * inverse_document_freq;
                                                }
                                        }
                                }

                            for( const string & word : query.minus_words )
                                {
                                    if( word_to_document_freqs_.count( word ) == 0 )
                                        {
                                            continue;
                                        }

                                    for( const auto & [ document_id, _ ]
                                            : word_to_document_freqs_.at( word ) )
                                        {
                                            result.erase( document_id );
                                        }
                                }

                            return result;
                        }();


                    const vector< Document > & matched_documents
                            = [&document_to_relevance, this]()
                        {
                            vector< Document > result;
                            for( const auto & [ document_id, relevance ] : document_to_relevance )
                                {
                                    result.push_back(
                                            Document
                                                {
                                                    document_id,
                                                    relevance,
                                                    documents_.at( document_id ).rating
                                                } );
                                }

                            return result;
                        }();

                    return matched_documents;
                }
    };

template < typename Iterator >
class IteratorRange
    {
        public:
            IteratorRange( Iterator range_begin, Iterator range_end )
                :
                    range_begin_( range_begin ),
                    range_end_  ( range_end   )
                {

                }

            Iterator
            begin() const
                {
                    return range_begin_;
                }

            Iterator
            end() const
                {
                    return range_end_;
                }

        private:
            Iterator range_begin_;
            Iterator range_end_;
    };

template < typename Iterator >
class Paginator
    {
        public:

            Paginator(
                    Iterator range_begin,
                    Iterator range_end,
                    size_t page_size )
                {
                    auto cur_it_range_begin = range_begin;

                    if( page_size == 0 )
                        {
                            pages_.push_back( IteratorRange( range_end, range_end ) );
                        }

                    else
                        {
                            while( distance( cur_it_range_begin, range_end ) > 0 )
                                {
                                    auto cur_it_range_end = cur_it_range_begin;

                                    int advance_distance = 0;

                                    if( const int dis1 = distance( cur_it_range_begin, range_end )
                                            ; dis1 < static_cast< int >( page_size ) )
                                        {
                                            advance_distance = dis1;
                                        }
                                    else
                                        {
                                            advance_distance = page_size;
                                        }

                                    advance( cur_it_range_end, advance_distance );
                                    pages_.push_back( IteratorRange( cur_it_range_begin, cur_it_range_end ) );

                                    cur_it_range_begin = cur_it_range_end;
                                }
                        }

                }

            auto
            begin() const
                {
                    return pages_.begin();
                }

            auto
            end() const
                {
                    return pages_.end();
                }

        private:

            vector< IteratorRange< Iterator > > pages_;
    };

template < typename Container >
auto Paginate(
        const Container & c,
        size_t page_size )
    {
        return
                Paginator( begin( c ), end( c ), page_size );
    }

ostream &
operator<<( ostream & out, const Document & document )
    {
        return
                out << "{ "s
                    << "document_id = "s << document.id        << ", "s
                    << "relevance = "s   << document.relevance << ", "s
                    << "rating = "s      << document.rating    << " }"s;
    }


template < typename Iterator >
ostream &
operator<<(
        ostream & out,
        const IteratorRange< Iterator > & iterator_range )
    {
        for( auto it = iterator_range.begin(); it != iterator_range.end(); ++it )
            {
                out << *it;
            }

        return out;
    }

// -------- Начало модульных тестов поисковой системы ----------

void
AssertImpl(
        bool t,
        const string & t_str,
        const string & file,
        const string & func,
        unsigned line,
        const string & hint )
    {
        if( !t )
            {
                cout << boolalpha;
                cout << file << "("s << line << "): "s << func << ": "s;
                cout << "ASSERT("s << t_str << ") failed. "s;

                if( !hint.empty() )
                    {
                        cout << "Hint: "s << hint;
                    }
                cout << endl;

                abort();
            }
    }

template < typename T, typename U >
void
AssertEqualImpl(
        const T & t,
        const U & u,
        const string & t_str,
        const string & u_str,
        const string & file,
        const string & func,
        unsigned line,
        const string & hint )
    {
        if( t != u )
            {
                cout << boolalpha;
                cout << file << "("s << line << "): "s << func << ": "s;
                cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
                cout << t << " != "s << u << "."s;

                if( !hint.empty() )
                    {
                        cout << " Hint: "s << hint;
                    }
                cout << endl;

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
RunTestImpl( Functor functor, const string & test_name )
    {
        functor();
        cerr << test_name << " OK"s << endl;
    }

ostream &
operator<<( ostream & out, DocumentStatus status )
    {
        const map< DocumentStatus, string > & document_status_to_str =
            {
                { DocumentStatus::ACTUAL,     "ACTUAL"s     },
                { DocumentStatus::IRRELEVANT, "IRRELEVANT"s },
                { DocumentStatus::REMOVED,    "REMOVED"s    },
                { DocumentStatus::BANNED,     "BANNED"s     },
            };

        return
                out << document_status_to_str.at( status );
    }

ostream &
operator<<( ostream & out, const vector< size_t > & vector_of_size_t )
    {
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
        constexpr int doc_id = 42;
        const string & content = "cat in the city"s;
        const vector< int > & ratings = { 1, 2, 3 };

        {
            SearchServer server{ {} };
            server.AddDocument( doc_id, content, DocumentStatus::ACTUAL, ratings );

            const vector< Document > & found_docs = server.FindTopDocuments( "in"s );
            ASSERT_EQUAL( found_docs.size(), static_cast< size_t >( 1 ) );
            const Document & doc0 = found_docs[ 0 ];
            ASSERT_EQUAL( doc0.id, doc_id );
        }

        {
            SearchServer server { "in the"s };
            server.AddDocument( doc_id, content, DocumentStatus::ACTUAL, ratings );

            const vector< Document > & found_docs = server.FindTopDocuments( "in"s );
            ASSERT( found_docs.empty() );
        }

        {
            SearchServer server { "   in the   "s };
            server.AddDocument( doc_id, content, DocumentStatus::ACTUAL, ratings );

            {
                const vector< Document > & found_docs = server.FindTopDocuments( "in"s );
                ASSERT( found_docs.empty() );
            }

            {
                const vector< Document > & found_docs = server.FindTopDocuments( "cat"s );
                ASSERT( !found_docs.empty() );
            }
        }

        {
            const vector< string > & stop_words = { "in"s, "в"s, "на"s, ""s, "in"s };
            SearchServer server( stop_words );
            server.AddDocument( doc_id, content, DocumentStatus::ACTUAL, ratings );

            {
                const vector< Document > & found_docs = server.FindTopDocuments( "in"s );
                ASSERT( found_docs.empty() );
            }

            {
                const vector< Document > & found_docs = server.FindTopDocuments( "cat"s );
                ASSERT( !found_docs.empty() );
            }
        }

        {
            const set< string > & stop_words = { "in"s, "в"s, "на"s, ""s };
            SearchServer server( stop_words );
            server.AddDocument( doc_id, content, DocumentStatus::ACTUAL, ratings );

            {
                const vector< Document > & found_docs = server.FindTopDocuments( "in"s );
                ASSERT( found_docs.empty() );
            }

            {
                const vector< Document > & found_docs = server.FindTopDocuments( "cat"s );
                ASSERT( !found_docs.empty() );
            }
        }
    }

void
TestAddDocumentByDocumentCount()
    {
        SearchServer server{ {} };

        ASSERT_EQUAL( server.GetDocumentCount(), 0 );

        server.AddDocument( 42, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 } );

        ASSERT_EQUAL( server.GetDocumentCount(), 1 );
    }

void
TestAddDocumentWithHyphen()
    {
        const vector< int > & ratings = { 1, 2, 3 };

        SearchServer server{ {} };

        server.AddDocument( 1, "один иван-чай три"s, DocumentStatus::ACTUAL, ratings );

        ASSERT_EQUAL( server.GetDocumentCount(), 1 );

        {
            const vector< string > & queries =
                {
                    "иван-чай"s,
                };

            for( const string & query : queries )
                {
                    const vector< Document > & found_docs = server.FindTopDocuments( query );
                    ASSERT( !found_docs.empty() );
                }
        }

        {
            const vector< string > & queries =
                {
                    "иван"s,
                    "чай"s,
                };

            for( const string & query : queries )
                {
                    const vector< Document > & found_docs = server.FindTopDocuments( query );
                    ASSERT( found_docs.empty() );
                }
        }
    }

void
TestAddInvalidDocuments()
    {
        const string & content = "cat in the city"s;
        const vector< int > & ratings = { 1, 2, 3 };
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
                catch( const invalid_argument & e )
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
        const string & content = "cat in the city"s;
        const vector< int > & ratings = { 1, 2, 3 };
        constexpr DocumentStatus status = DocumentStatus::ACTUAL;

        SearchServer server{ {} };

        server.AddDocument( 1, content, status, ratings );

        const vector< string > & expected_incorrect_queries =
            {
                "скво\x12рец"s,
                "--cat"s,
                "-"s,
            };

        for( const string & expected_incorrect_query
                : expected_incorrect_queries )
            {
                bool exception_was_thrown = false;

                try
                    {
                        (void) server.FindTopDocuments( expected_incorrect_query, status );
                    }
                catch( const invalid_argument & e )
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
        const string & content = "cat in the city"s;
        const vector< int > & ratings = { 1, 2, 3 };

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
        const string & content = "cat in the city"s;
        const vector< int > & ratings = { 1, 2, 3 };
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
                catch( const invalid_argument & e )
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
                catch( const invalid_argument & e )
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
        const string & content = "cat in the city"s;
        const vector< int > & ratings = { 1, 2, 3 };

        SearchServer server{ {} };

        server.AddDocument( 1, content, DocumentStatus::ACTUAL,     ratings );
        server.AddDocument( 2, content, DocumentStatus::IRRELEVANT, ratings );
        server.AddDocument( 3, content, DocumentStatus::BANNED,     ratings );
        server.AddDocument( 4, content, DocumentStatus::REMOVED,    ratings );

        ASSERT_EQUAL( get< 1 >( server.MatchDocument( content, 1 ) ), DocumentStatus::ACTUAL     );
        ASSERT_EQUAL( get< 1 >( server.MatchDocument( content, 2 ) ), DocumentStatus::IRRELEVANT );
        ASSERT_EQUAL( get< 1 >( server.MatchDocument( content, 3 ) ), DocumentStatus::BANNED     );
        ASSERT_EQUAL( get< 1 >( server.MatchDocument( content, 4 ) ), DocumentStatus::REMOVED    );
    }

void
TestStatus()
    {
        const string & content = "cat in the city"s;
        const vector< int > & ratings = { 1, 2, 3 };
        const string & query = "in"s;

        const set< DocumentStatus > &
        document_status_values_for_iteratation =
            {
                DocumentStatus::ACTUAL    ,
                DocumentStatus::BANNED    ,
                DocumentStatus::IRRELEVANT,
                DocumentStatus::REMOVED   ,
            };

        const vector< map< DocumentStatus, int > > &
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

        for( const map< DocumentStatus, int > & number_of_documents_to_add
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

                        const vector< Document > & found_docs = server.FindTopDocuments( query, document_status );

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
        const string & content = "cat in the city"s;
        const string & query = "in"s;
        const vector< int > & ratings = { 1, 2, 3 };
        constexpr int ratings_avg = 2;

        const set< DocumentStatus > & document_status_values_for_iteratation =
            {
                DocumentStatus::ACTUAL,
                DocumentStatus::BANNED,
                DocumentStatus::IRRELEVANT,
                DocumentStatus::REMOVED
            };

        const vector< map< DocumentStatus, int > > &
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

        const vector< function< bool( int, DocumentStatus, int ) > > &
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

        for( const map< DocumentStatus, int > & number_of_documents_to_add
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

                        const vector< Document > & found_docs = server.FindTopDocuments( query, predicate );

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
        const string & content = "cat in the city"s;
        const vector< int > & ratings = { 1, 2, 3 };
        constexpr DocumentStatus status = DocumentStatus::ACTUAL;

        const vector< vector< string > > &
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

        const vector< string > & queries =
            {
                ""s,
                "in"s,
                "in at between above on"s,
                content
            };

        for( const vector< string > & document_contents : test_parameters_document_contents )
            {
                SearchServer server{ {} };

                const vector< set< string > > & documents
                        = [&document_contents, &server, &ratings]()
                    {
                        int document_id = -1;
                        vector< set< string > > result;

                        for( const string & text : document_contents )
                            {
                                server.AddDocument( ++document_id, text, status, ratings );

                                const set< string > & document_words = [&text]()
                                    {
                                        set< string > result;
                                        string word = ""s;
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

                for( const string & query : queries )
                    {
                        const set< string > & query_words = [&query]()
                            {
                                set< string > result;
                                string word = ""s;

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
                                for( const set< string > & document : documents )
                                    {
                                        for( const string & query_word : query_words )
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

                        const vector< Document > & found_docs = server.FindTopDocuments( query );

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
        constexpr DocumentStatus document_status = DocumentStatus::ACTUAL;
        const string & content = "cat in the city"s;
        const string & query = "in"s;

        const vector< vector< int > > & test_parameters_ratings =
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

        for( const vector< int > & ratings : test_parameters_ratings )
            {
                const int rating_sum = accumulate( ratings.begin(), ratings.end(), 0 );

                const int expected_avg_rating
                                            = rating_sum
                                                            /
                                                                static_cast< int >( ratings.size() );

                SearchServer server{ {} };

                server.AddDocument( 0, content, document_status, ratings );

                const vector< Document > & found_docs = server.FindTopDocuments( query );

                const int actual_avg_rating = found_docs[ 0 ].rating;

                ASSERT_EQUAL( expected_avg_rating, actual_avg_rating );
            }
    }

void
TestRelevance()
    {
        constexpr DocumentStatus document_status = DocumentStatus::ACTUAL;
        const vector< int > & ratings = { 1, 2, 3 };

        {
            {
                SearchServer server{ {} };

                server.AddDocument( 0, "cat in"s,   document_status, ratings );
                server.AddDocument( 1, "the city"s, document_status, ratings );

                const vector< Document > & found_docs = server.FindTopDocuments( "cat in the city"s );

                ASSERT_EQUAL( found_docs[ 0 ].relevance, found_docs[ 1 ].relevance );
            }

            {
                SearchServer server{ {} };

                server.AddDocument( 0, " "s, document_status, ratings );
                server.AddDocument( 1, ""s,  document_status, ratings );

                const vector< Document > & found_docs = server.FindTopDocuments( " "s );

                ASSERT_EQUAL( found_docs.size(), static_cast< size_t >( 0 ) );
            }

            {
                SearchServer server{ {} };

                server.AddDocument( 0, ""s,    document_status, ratings );
                server.AddDocument( 1, " "s,   document_status, ratings );
                server.AddDocument( 2, "cat"s, document_status, ratings );
                server.AddDocument( 3, "cat  "s, document_status, ratings );
                server.AddDocument( 4, "cat cat "s, document_status, ratings );

                const vector< Document > & found_docs = server.FindTopDocuments( " "s );

                ASSERT_EQUAL( found_docs.size(), static_cast< size_t >( 0 ) );
            }

            {
                SearchServer server{ { " "s } };

                server.AddDocument( 0, ""s,    document_status, ratings );
                server.AddDocument( 1, " "s,   document_status, ratings );
                server.AddDocument( 2, "cat"s, document_status, ratings );
                server.AddDocument( 3, "cat  "s, document_status, ratings );
                server.AddDocument( 4, "cat cat "s, document_status, ratings );

                const vector< Document > & found_docs = server.FindTopDocuments( " "s );

                ASSERT_EQUAL( found_docs.size(), static_cast< size_t >( 0 ) );
            }

            {
                SearchServer server{ {} };

                server.AddDocument( 0, "белый кот модный ошейник"s,          document_status, ratings );
                server.AddDocument( 1, "пушистый кот пушистый хвост"s,       document_status, ratings );
                server.AddDocument( 2, "ухоженный пёс выразительные глаза"s, document_status, ratings );

                const vector< Document > & found_docs = server.FindTopDocuments( "пушистый ухоженный кот"s );

                const map< int, double > & calculated_relevance = [&found_docs]()
                    {
                        map< int, double > result;
                        for( const Document & document : found_docs )
                            {
                                result[ document.id ] = document.relevance;
                            }

                        return result;
                    }();

                const map< int, double > & document_to_expected_relevance =
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

                const vector< Document > & found_docs = server.FindTopDocuments( "-пушистый ухоженный кот"s );

                const map< int, double > & calculated_relevance = [&found_docs]()
                    {
                        map< int, double > result;
                        for( const Document & document : found_docs )
                            {
                                result[ document.id ] = document.relevance;
                            }

                        return result;
                    }();

                const map< int, double > & document_to_expected_relevance =
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

        const string & query = "белый красный оранжевый голубой синий"s;
        constexpr DocumentStatus document_status = DocumentStatus::ACTUAL;

        const vector< vector< int > > & test_parameters_ratings =
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

        const vector< string > & test_parameters_document_contents =
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

        for( const vector< int > & ratings : test_parameters_ratings )
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

                const vector< Document > & found_docs = server.FindTopDocuments( query );

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
        constexpr DocumentStatus document_status = DocumentStatus::ACTUAL;
        const vector< int > & ratings = { 1, 2, 3 };

        int counter = -1;

        const map< int, string > & test_parameters_documents_to_text =
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

        const vector< string > & test_parameters_queries =
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

            map< int, set< string > > test_parameters_document_words;

            for( const pair< int, string > & document_to_text : test_parameters_documents_to_text )
                {
                    server.AddDocument(
                            document_to_text.first,
                            document_to_text.second,
                            document_status,
                            ratings );

                    set< string > document_words;
                    string word = ""s;
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

            for( const string & query : test_parameters_queries )
                {
                    set< int > expected_document_id;
                    set< int > banned_document_id;

                    {
                        set< string > query_plus_words;
                        set< string > query_minus_words;

                        string word;

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

                        for( const pair< int, set< string > > & document_to_words
                                : test_parameters_document_words )
                            {
                                for( const string & plus_word : query_plus_words )
                                    {
                                        if( document_to_words.second.count( plus_word ) )
                                            {
                                                expected_document_id.insert( document_to_words.first );
                                                break;
                                            }
                                    }

                                for( const string & minus_word : query_minus_words )
                                    {
                                        if( document_to_words.second.count( minus_word ) > 0 )
                                            {
                                                banned_document_id.insert( document_to_words.first );
                                                expected_document_id.erase( document_to_words.first );
                                                break;
                                            }
                                    }
                            }

                        const vector< Document > & found_docs = server.FindTopDocuments( query );

                        const set< int > & actual_document_id = [&found_docs]()
                            {
                                set< int > result;
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
        {
            SearchServer search_server { "и в на"s };

            {
                {
                    const vector< int > & expected_incorrect_document_ids =
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
                            catch( const out_of_range & e )
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
                    const vector< int > & expected_incorrect_document_ids =
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
                            catch( const out_of_range & e )
                                {
                                    exception_was_thrown = true;
                                }

                            ASSERT( exception_was_thrown );
                        }
                }

                ASSERT_EQUAL( search_server.GetDocumentId( 0 ),  42 );

                {
                    const vector< int > & expected_incorrect_document_ids =
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
                            catch( const out_of_range & e )
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
                    const vector< int > & expected_incorrect_document_ids =
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
                            catch( const out_of_range & e )
                                {
                                    exception_was_thrown = true;
                                }

                            ASSERT( exception_was_thrown );
                        }
                }

                ASSERT_EQUAL( search_server.GetDocumentId( 0 ), 41 );
                ASSERT_EQUAL( search_server.GetDocumentId( 1 ), 42 );

                {
                    const vector< int > & expected_incorrect_document_ids =
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
                            catch( const out_of_range & e )
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
                    const vector< int > & expected_incorrect_document_ids =
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
                            catch( const out_of_range & e )
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
                    const vector< int > & expected_incorrect_document_ids =
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
                            catch( const out_of_range & e )
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
        SearchServer server{ {} };

        constexpr int document_id = 1;
        server.AddDocument( document_id, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 } );

        const vector< string > & expected_incorrect_queries =
            {
                "-"s,
                "--cat"s,
                "скво\x12рец"s,
            };

        for( const string & query : expected_incorrect_queries )
            {
                bool exception_was_thrown = false;

                try
                    {
                        (void) server.MatchDocument( query, document_id );
                    }
                catch( const invalid_argument & e )
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
            const vector< string > & stop_words_vector = { "и"s, "в"s, "на"s, ""s, "в"s, " "s };
            SearchServer server( stop_words_vector );
        }

        {
            const set< string > & stop_words_set = { "и"s, "в"s, "на"s, ""s, " "s };
            SearchServer server( stop_words_set );
        }

        {
            {
                bool exception_was_thrown = false;

                try
                    {
                        SearchServer server( "скво\x12рец"s );
                    }
                catch( const invalid_argument & e )
                    {
                        exception_was_thrown = true;
                    }

                ASSERT( exception_was_thrown );
            }

            {
                bool exception_was_thrown = false;

                try
                    {
                        const vector< string > & stop_words_vector = { "скво\x12рец"s };
                        SearchServer server( stop_words_vector );
                    }
                catch( const invalid_argument & e )
                    {
                        exception_was_thrown = true;
                    }

                ASSERT( exception_was_thrown );
            }

            {
                bool exception_was_thrown = false;

                try
                    {
                        const set< string > & stop_words_set = { "скво\x12рец"s };
                        SearchServer server( stop_words_set );
                    }
                catch( const invalid_argument & e )
                    {
                        exception_was_thrown = true;
                    }

                ASSERT( exception_was_thrown );
            }
        }

        const vector< string > & test_parameters_stop_words =
            {
                "-"s,
                "--cat"s
                "cat--cat"s,
                "cat-cat"s,
            };

        for( const string & stop_words : test_parameters_stop_words )
            {
                {
                    SearchServer server( stop_words );
                }

                {
                    const vector< string > & stop_words_vector = { stop_words };
                    SearchServer server( stop_words_vector );
                }

                {
                    const set< string > & stop_words_set = { stop_words };
                    SearchServer server( stop_words_set );
                }
            }
    }

void
TestGetDocumentIdOverflow()
    {
        constexpr DocumentStatus document_status = DocumentStatus::ACTUAL;
        const vector< int > & ratings = { 1, 2, 3 };
        const string & content = "cat in the city"s;

        const vector< function< bool( int ) > > &
        test_parameters_predicate_which_document_id_add =
            {
                []( int document_id ) { return true; },
                []( int document_id ) { return document_id % 2 == 0; },
            };

        const vector< int > & test_parameters_max_document_id =
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
                                            catch( const out_of_range & e )
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
        constexpr DocumentStatus document_status = DocumentStatus::ACTUAL;
        const vector< int > & ratings = { 1, 2, 3 };
        const string & document_content = "cat and dog"s;
        const string & stop_words = "and"s;
        const string & query = "cat"s;

        constexpr int param_max_number_of_docs_to_add = 20;
        constexpr int param_max_page_size = 20;

        for( int param_number_of_docs_to_add = 0
                ; param_number_of_docs_to_add < param_max_number_of_docs_to_add
                ; ++ param_number_of_docs_to_add )
            {
                for( int param_page_size = 0
                        ; param_page_size < param_max_page_size
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

                                auto result = vector( 0, *( p.end() ) );

                                for( auto page = p.begin()
                                        ; page != p.end()
                                        ; ++page )
                                    {
                                        result.push_back( *page );
                                    }

                                return result;
                            }();


                        const vector< size_t > actual_pages_sizes = [&found_pages]()
                            {
                                vector< size_t > result;

                                for( size_t i = 0; i < found_pages.size(); ++i )
                                    {
                                        result.push_back(
                                                distance( found_pages[ i ].begin(),
                                                found_pages[ i ].end() ) );
                                    }

                                return result;
                            }();

                        const vector< size_t > expected_pages_sizes =
                        [param_number_of_docs_to_add, param_page_size]()
                            {
                                if( param_page_size == 0 )
                                    {
                                        return vector< size_t >( 1, 0 );
                                    }

                                const int expected_number_of_found_docs
                                        = std::min(
                                                param_number_of_docs_to_add,
                                                MAX_RESULT_DOCUMENT_COUNT );

                                vector< size_t > result = vector(
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

// Функция TestSearchServer является точкой входа для запуска тестов
void
TestSearchServer()
    {
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
        RUN_TEST( TestSorting                                     );
        RUN_TEST( TestStatus                                      );
        RUN_TEST( TestStructDocument                              );
    }

// --------- Окончание модульных тестов поисковой системы -----------

// ------------ Пример использования ----------------

void
PrintDocument( const Document & document )
    {
        cout << "{ "s
            << "document_id = "s << document.id << ", "s
            << "relevance = "s << document.relevance << ", "s
            << "rating = "s << document.rating << " }"s << endl;
    }

void
PrintMatchDocumentResult( int document_id, const vector< string > & words, DocumentStatus status )
    {
        cout << "{ "s
            << "document_id = "s << document_id << ", "s
            << "status = "s << static_cast< int >( status ) << ", "s
            << "words ="s;

        for( const string & word : words )
            {
                cout << ' ' << word;
            }
        cout << "}"s << endl;
    }

void
AddDocument(
        SearchServer & search_server,
        int document_id,
        const string & document,
        DocumentStatus status,
        const vector< int > & ratings )
    {
        try
            {
                search_server.AddDocument(document_id, document, status, ratings);
            }
        catch( const exception & e )
            {
                cerr << "Ошибка добавления документа "s << document_id << ": "s << e.what() << endl;
            }
    }

void
FindTopDocuments( const SearchServer & search_server, const string & raw_query )
    {
        cout << "Результаты поиска по запросу: "s << raw_query << endl;
        try
            {
                for( const Document & document : search_server.FindTopDocuments( raw_query ) )
                    {
                        PrintDocument( document );
                    }
            }
        catch( const exception & e )
            {
                cerr << "Ошибка поиска: "s << e.what() << endl;
            }
    }

void
MatchDocuments( const SearchServer & search_server, const string & query )
    {
        try
            {
                cout << "Матчинг документов по запросу: "s << query << endl;
                const int document_count = search_server.GetDocumentCount();

                for( int index = 0; index < document_count; ++index )
                    {
                        const int document_id = search_server.GetDocumentId( index );
                        const auto & [ words, status ] = search_server.MatchDocument( query, document_id );
                        PrintMatchDocumentResult( document_id, words, status );
                    }
            }
        catch( const exception & e )
            {
                cerr << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << endl;
            }
    }

int
main()
    {
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
                    cout << *page << endl;
                    cout << "Page break"s << endl;
                }
        }
    }

