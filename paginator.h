#pragma once

#include "search_server.h"

using namespace std::string_literals;

template < typename Iterator >
class IteratorRange
    {

        public:
            IteratorRange( Iterator range_begin, Iterator range_end )
                :
                    range_begin_( range_begin ),
                    range_end_  ( range_end   )
                {
                    using namespace std;
                }

            Iterator
            begin() const
                {
                    using namespace std;

                    return range_begin_;
                }

            Iterator
            end() const
                {
                    using namespace std;

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
                    std::size_t page_size )
                : pages_(
                        ConstructPages( range_begin, range_end, page_size ) )
                {
                    using namespace std;
                }

            auto
            begin() const
                {
                    using namespace std;

                    return pages_.begin();
                }

            auto
            end() const
                {
                    using namespace std;

                    return pages_.end();
                }

        private:

            std::vector< IteratorRange< Iterator > >
            ConstructPages(
                    Iterator range_begin,
                    Iterator range_end,
                    std::size_t page_size )
                {
                    using namespace std;

                    std::vector< IteratorRange< Iterator > > result;

                    auto cur_it_range_begin = range_begin;

                    if( page_size == 0 )
                        {
                            result.push_back( IteratorRange( range_end, range_end ) );
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
                                    result.push_back( IteratorRange( cur_it_range_begin, cur_it_range_end ) );

                                    cur_it_range_begin = cur_it_range_end;
                                }
                        }

                    return result;
                }

            const std::vector< IteratorRange< Iterator > > pages_;
    };

template < typename Container >
auto Paginate(
        const Container & c,
        std::size_t page_size )
    {
        using namespace std;

        return
                Paginator( begin( c ), end( c ), page_size );
    }

std::ostream &
operator<<( std::ostream & out, const Document & document )
    {
        using namespace std;

        return
                out << "{ "s
                    << "document_id = "s << document.id        << ", "s
                    << "relevance = "s   << document.relevance << ", "s
                    << "rating = "s      << document.rating    << " }"s;
    }

template < typename Iterator >
std::ostream &
operator<<(
        std::ostream & out,
        const IteratorRange< Iterator > & iterator_range )
    {
        using namespace std;

        for( auto it = iterator_range.begin(); it != iterator_range.end(); ++it )
            {
                out << *it;
            }

        return out;
    }
