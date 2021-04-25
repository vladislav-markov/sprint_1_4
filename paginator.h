#pragma once

#include <algorithm>
#include <iostream>
#include <utility>
#include <vector>

template < typename Iterator >
class IteratorRange
    {

        public:

            IteratorRange(
                    const Iterator begin,
                    const Iterator end )
                :
                    first_( begin ),
                    last_( end ),
                    size_( distance( first_, last_ ) )
            {

            }

            Iterator
            begin() const
                {
                    return first_;
                }

            Iterator
            end() const
                {
                    return last_;
                }

            std::size_t
            size() const
                {
                    return size_;
                }

        private:

            Iterator first_, last_;
            std::size_t size_;
    };

template < typename Iterator >
std::ostream &
operator<<(
        std::ostream & out,
        const IteratorRange< Iterator > & range )
    {
        for( Iterator it = range.begin(); it != range.end(); ++it )
            {
                out << *it;
            }

        return out;
    }

template < typename Iterator >
class Paginator
    {

        public:

            Paginator(
                    Iterator begin,
                    const Iterator end,
                    const std::size_t page_size )
                :
                    pages_( CreatePages( begin, end, page_size ) )
                {

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

            std::size_t
            size() const
                {
                    return pages_.size();
                }

        private:

            static std::vector< IteratorRange< Iterator > >
            CreatePages(
                    Iterator begin,
                    const Iterator end,
                    const std::size_t page_size )
                {
                    std::vector< IteratorRange< Iterator > > result;

                    for( std::size_t left = distance( begin, end ); left > 0; )
                        {
                            const std::size_t current_page_size = std::min( page_size, left );
                            const Iterator current_page_end = std::next( begin, current_page_size );
                            result.push_back( { begin, current_page_end } );

                            left -= current_page_size;
                            begin = current_page_end;
                        }

                    return result;
                }

            const std::vector< IteratorRange< Iterator > > pages_;
    };

template < typename Container >
auto
Paginate(
        const Container & c,
        const std::size_t page_size )
    {
        return Paginator( std::begin( c ), std::end(c), page_size );
    }

