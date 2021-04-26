#include <algorithm>

#include "request_queue.h"

RequestQueue::RequestQueue( const SearchServer & search_server )
    :
        server_( search_server )
    {}

int
RequestQueue::GetNoResultRequests() const
    {
        const int number_of_no_result_requests = std::count_if(
                requests_.cbegin(),
                requests_.cend(),
                []( const QueryResult & query ){ return !query.has_result; } );

        return number_of_no_result_requests;
    }


