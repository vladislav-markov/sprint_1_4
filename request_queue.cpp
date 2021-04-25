#include <algorithm>
#include <string>
#include <vector>

#include "document.h"
#include "request_queue.h"
#include "search_server.h"

RequestQueue::RequestQueue( const SearchServer & search_server )
    :
        server_( search_server )
    {

    }

std::vector< Document >
RequestQueue::AddFindRequest( const std::string & raw_query )
    {
        if( requests_.size() == sec_in_day_ )
            {
                requests_.pop_front();
            }

        const std::vector< Document > found_docs = server_.FindTopDocuments( raw_query );

        requests_.push_back( { !found_docs.empty() } );

        return found_docs;
    }

std::vector< Document >
RequestQueue::AddFindRequest( const std::string & raw_query, DocumentStatus status )
    {
        if( requests_.size() == sec_in_day_ )
            {
                requests_.pop_front();
            }

        const std::vector< Document > found_docs = server_.FindTopDocuments( raw_query, status );

        requests_.push_back( { !found_docs.empty() } );

        return found_docs;
    }

int
RequestQueue::GetNoResultRequests() const
    {
        const int number_of_no_result_requests = std::count_if(
                requests_.cbegin(),
                requests_.cend(),
                []( const QueryResult query ){ return !query.has_result; } );

        return number_of_no_result_requests;
    }

