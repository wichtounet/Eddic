/*=============================================================================
    Copyright (c) 2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

//This is a "fork" of error_reporting.hpp from x3
//This adds the necessary features for eddic

#pragma once

#include <boost/locale/encoding_utf.hpp>
#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <ostream>
#include <sstream>

// Clang-style error handling utilities

namespace boost { namespace spirit { namespace x3 {

// tag used to get our error handler from the context
struct error_handler_tag;

template <typename Iterator>
struct error_handler {
    typedef Iterator iterator_type;

    error_handler(
        Iterator first, Iterator last, std::ostream& err_out
      , std::string file = "", int tabs = 4)
      : err_out(err_out)
      , file(file)
      , tabs(tabs)
      , pos_cache(first, last) {}

    typedef void result_type;

    void operator()(position_tagged pos, std::string const& message) const {
        auto where = pos_cache.position_of(pos);
        (*this)(err_out, where.begin(), where.end(), message);
    }

    void operator()(Iterator err_pos, std::string const& error_message) const {
        (*this)(err_out, err_pos, error_message);
    }

    void operator()(Iterator err_first, Iterator err_last, std::string const& error_message) const {
        (*this)(err_out, err_first, err_last, error_message);
    }

    std::string to_string(position_tagged pos, std::string const& message){
        std::stringstream ss;
        auto where = pos_cache.position_of(pos);
        (*this)(ss, where.begin(), where.end(), message);
        return ss.str();
    }

    void operator()(std::ostream& stream, Iterator err_pos, std::string const& error_message) const {
        Iterator first = pos_cache.first();
        Iterator last = pos_cache.last();

        // make sure err_pos does not point to white space
        skip_whitespace(err_pos, last);

        print_file_line(stream, position(err_pos));
        if(!error_message.empty()){
            stream << error_message << std::endl;
        }

        Iterator start = get_line_start(first, err_pos);
        if (start != first)
            ++start;
        print_line(stream, start, last);
        print_indicator(stream, start, err_pos, '_');
        stream << "^_" << std::endl;
    }

    void operator()(std::ostream& stream, Iterator err_first, Iterator err_last, std::string const& error_message) const {
        Iterator first = pos_cache.first();
        Iterator last = pos_cache.last();

        // make sure err_pos does not point to white space
        skip_whitespace(err_first, last);

        print_file_line(stream, position(err_first));
        if(!error_message.empty()){
            stream << error_message << std::endl;
        }

        Iterator start = get_line_start(first, err_first);
        if (start != first)
            ++start;
        print_line(stream, start, last);
        print_indicator(stream, start, err_first, ' ');
        print_indicator(stream, start, err_last, '~');
        stream << " <<-- Here" << std::endl;
    }

    template <typename AST>
    void tag(AST& ast, Iterator first, Iterator last){
        return pos_cache.annotate(ast, first, last);
    }

    boost::iterator_range<Iterator> position_of(position_tagged pos) const {
        return pos_cache.position_of(pos);
    }

private:

    void print_file_line(std::ostream& stream, std::size_t line) const {
        stream << "In file " << file << ", " << "line " << line << ':' << std::endl;
    }

    void print_line(std::ostream& stream, Iterator start, Iterator last) const {
        auto end = start;
        while (end != last) {
            auto c = *end;
            if (c == '\r' || c == '\n')
                break;
            else
                ++end;
        }

        typedef typename std::iterator_traits<Iterator>::value_type char_type;
        std::basic_string<char_type> line{start, end};
        stream << locale::conv::utf_to_utf<char>(line) << std::endl;
    }

    void print_indicator(std::ostream& stream, Iterator& start, Iterator last, char ind) const {
        for (; start != last; ++start){
            auto c = *start;
            if (c == '\r' || c == '\n')
                break;
            else if (c == '\t')
                for (int i = 0; i < tabs; ++i)
                    stream << ind;
            else
                stream << ind;
        }
    }

    void skip_whitespace(Iterator& err_pos, Iterator last) const {
        // make sure err_pos does not point to white space
        while (err_pos != last)
        {
            char c = *err_pos;
            if (std::isspace(c))
                ++err_pos;
            else
                break;
        }
    }

    void skip_non_whitespace(Iterator& err_pos, Iterator last) const {
    // make sure err_pos does not point to white space
        while (err_pos != last){
            char c = *err_pos;
            if (std::isspace(c))
                break;
            else
                ++err_pos;
        }
    }

    Iterator get_line_start(Iterator first, Iterator pos) const {
        Iterator latest = first;
        for (Iterator i = first; i != pos; ++i)
            if (*i == '\r' || *i == '\n')
                latest = i;
        return latest;
    }

    std::size_t position(Iterator i) const {
        std::size_t line { 1 };
        typename std::iterator_traits<Iterator>::value_type prev { 0 };

        for (Iterator pos = pos_cache.first(); pos != i; ++pos) {
            auto c = *pos;
            switch (c) {
            case '\n':
                if (prev != '\r') ++line;
                break;
            case '\r':
                if (prev != '\n') ++line;
                break;
            default:
                break;
            }
            prev = c;
        }

        return line;
    }

    std::ostream& err_out;
    std::string file;
    int tabs;
    position_cache<std::vector<Iterator>> pos_cache;
};

}}}
