#pragma once
#include <iterator>
#include <vector>

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end);
    Iterator begin() const;
    Iterator end() const;
    size_t size() const;
private:
    Iterator first_, last_;
    size_t size_;
};

template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, size_t page_size);
    auto begin() const;
    auto end() const;
    size_t size() const;
private:
    std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator>& range);

template <typename Container>
auto Paginate(const Container& c, size_t page_size);



