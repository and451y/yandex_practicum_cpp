#pragma once
#include <execution>
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <list>

#include "search_server.h"

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server,
                                                  const std::vector<std::string>& queries); 

std::vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries);
