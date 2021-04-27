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

void
RequestQueue::UpdateRequestsResultInfo(
        const std::vector< Document > & found_documents )
    {
        if( requests_.size() == sec_in_day_ )
            {
                requests_.pop_front();
            }

        requests_.push_back( { !found_documents.empty() } );
    }

std::vector< Document >
RequestQueue::AddFindRequest(
        const std::string & raw_query,
        const DocumentStatus status )
    {
        const std::vector< Document > found_documents
                        = server_.FindTopDocuments( raw_query, status );

        UpdateRequestsResultInfo( found_documents );

        return found_documents;
    }

std::vector< Document >
RequestQueue::AddFindRequest(
        const std::string & raw_query )
    {
        const std::vector< Document > found_documents
                                    = server_.FindTopDocuments( raw_query );

        UpdateRequestsResultInfo( found_documents );

        return found_documents;
    }

