#include <cmath>

#include "string_processing.h"
#include "search_server.h"

using namespace std::string_literals;

struct SearchServer::QueryWord
    {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

std::vector< Document >
SearchServer::FindTopDocuments(
        const std::string & raw_query,
        DocumentStatus status ) const
    {
        using namespace std;

        return FindTopDocuments(
                raw_query,
                [status]( int document_id, DocumentStatus document_status, int rating )
                    {
                        return
                                document_status == status;
                    } );
    }

void
SearchServer::AddDocument(
        int document_id,
        const std::string & document,
        DocumentStatus status,
        const std::vector< int > & ratings )
    {
        using namespace std;

        if( document_id < 0 )
            {
                const std::string & message = "Попытка добавить документ с отрицательным id"s;
                throw std::invalid_argument( message );
            }
        else if( documents_.count( document_id ) )
            {
                const std::string & message = "Попытка добавить документ c id ранее добавленного документа"s;
                throw std::invalid_argument( message );
            }
        else if( const std::vector< std::string > & document_words = SplitIntoWords( document )
                ; any_of(
                        document_words.begin(),
                        document_words.end(),
                        []( const std::string & document_word ){ return !IsValidWord( document_word ); } ) )
            {
                const std::string & message
                        = "Наличие недопустимых символов (с кодами от 0 до 31) "s
                            + "в тексте добавляемого документа"s;

                throw std::invalid_argument( message );
            }

        const std::vector< std::string > & words = SplitIntoWordsNoStop( document );
        const double inv_word_count = 1.0 / words.size();

        for( const std::string & word : words )
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

int
SearchServer::GetDocumentCount() const
    {
        using namespace std;

        return documents_.size();
    }

int
SearchServer::GetDocumentId( const int index ) const
    {
        using namespace std;

        const std::string & error_message
                = "Индекс переданного документа выходит за "s
                    + "пределы допустимого диапазона (0; количество документов)"s;

        if( index < 0 )
            {
                throw std::out_of_range( error_message );
            }

        /*                                                                                          *
            *      Допустим, максимальное значение типа < int > = 2.                                   *
            *      И документы = { 0, 1, 2 };                                                          *
            *      значит std::size_t = 3                                                                   *
            *      Тип аргумента-индекса - < int >. То есть, мы можем ожидать максимум значение 2.     *
            *      Получается, считаем корректной проверку ( index '<' std::size_t ), т.к. ( 2 '<' 3 )      *
            *      соответственно, на проверку '=>' бросаем исключение,                                *
            *      чтобы не вызывать advance() ниже для больших значений входного аргумента            *
            */

        else if( const std::size_t input_index = static_cast< std::size_t >( index )
                ; input_index >= documents_.size() )
            {
                throw std::out_of_range( error_message );
            }

        auto documents_iterator = documents_.begin();
        std::advance( documents_iterator, index );

        return
                documents_iterator->first;
    }

std::tuple< std::vector< std::string >, DocumentStatus >
SearchServer::MatchDocument(
        const std::string & raw_query,
        int document_id ) const
    {
        using namespace std;

        for( const std::string & raw_query_word : SplitIntoWords( raw_query ) )
            {
                if( !IsValidWord( raw_query_word ) )
                    {
                        const std::string & message
                                = "В словах поискового запроса есть недопустимые "s
                                    + "символы с кодами от 0 до 31"s;

                        throw std::invalid_argument( message );
                    }
                else if( !IsValidHyphenWordSingleMinus( raw_query_word ) )
                    {
                        const std::string & message
                                = "Отсутствие текста после символа 'минус', например, 'пушистый -'."s;

                        throw std::invalid_argument( message );
                    }
                else if( !IsValidHyphenWordMultiMinus( raw_query_word ) )
                    {
                        const std::string & message
                                = "Наличие более чем одного минуса перед словами, "s
                                    + "которых не должно быть в искомых документах, например, "s
                                    + "'пушистый --кот'. В середине слов минусы разрешаются, "s
                                    + "например: 'иван-чай.'"s;

                        throw std::invalid_argument( message );
                    }
            }

        const Query & query = ParseQuery( raw_query );

        std::vector< std::string > matched_words;
        for( const std::string & word : query.plus_words )
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
        for( const std::string & word : query.minus_words )
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

bool
SearchServer::IsValidHyphenWordMultiMinus( const std::string & word )
    {
        using namespace std;

        if( ( word.size() > 1 ) && ( word[ 0 ] == '-' ) && ( word[ 1 ] == '-' ) )
            {
                return false;
            }

        return true;
    }

bool
SearchServer::IsValidHyphenWordSingleMinus( const std::string & word )
    {
        using namespace std;

        if( ( word.size() == 1 ) && ( word[ 0 ] == '-' ) )
            {
                return false;
            }

        return true;
    }

bool
SearchServer::IsValidWord( const std::string & word )
    {
        using namespace std;

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
SearchServer::IsStopWord( const std::string & word ) const
    {
        using namespace std;

        return
                stop_words_.count( word ) > 0;
    }

std::vector< std::string >
SearchServer::SplitIntoWordsNoStop( const std::string & text ) const
    {
        using namespace std;

        const std::vector< std::string > & all_words = SplitIntoWords( text );

        const std::vector< std::string > & words = [this, &all_words]()
            {
                std::vector< std::string > v;

                copy_if(
                        all_words.begin(),
                        all_words.end(),
                        back_inserter( v ),
                        [this]( const std::string & word )
                            {
                                return !IsStopWord( word );
                            } );

                return v;
            }();

        return words;
    }

int
SearchServer::ComputeAverageRating( const std::vector< int > & ratings )
    {
        using namespace std;

        if( ratings.empty() )
            {
                return 0;
            }

        const int ratings_sum
                            = std::accumulate(
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

SearchServer::QueryWord
SearchServer::ParseQueryWord( std::string text ) const
    {
        using namespace std;

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

SearchServer::Query
SearchServer::ParseQuery( const std::string & text ) const
    {
        using namespace std;

        const Query & query = [&text, this]()
            {
                Query result;
                for( const std::string & word : SplitIntoWords( text ) )
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

double
SearchServer::ComputeWordInverseDocumentFreq(
        const std::string & word ) const
    {
        using namespace std;

        return std::log(
                        1.0
                            * GetDocumentCount()
                                                / word_to_document_freqs_.at( word ).size() );
    }

