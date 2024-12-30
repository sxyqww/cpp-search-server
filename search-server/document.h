#pragma once
#include <ostream>

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};
struct Document {
    Document();
    Document(int id_, double relevance_, int rating_);
    
    int id;
    double relevance;
    int rating;
};

std::ostream& operator<<(std::ostream& out, const Document& document);
