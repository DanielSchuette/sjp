/* The parser follows the nomenclature at ``https://www.json.org/json-en.html''.
 * The data wrappers that derive from JSONVALUE are mostly defined in-line in
 * ``sjp.hh''.
 *
 * Simple-JSON-Parser (SJP) Copyright (C) 2021 Daniel Schuette
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include <algorithm>
#include <cmath>

#include "common.hh"
#include "sjp.hh"

[[noreturn]] static void fail(const char* msg, int code)
{
    fprintf(stderr, "error: %s\n", msg);
    exit(code);
}

static std::string padding(size_t size)
{
    return std::string(size*2, ' ');
}

sjp::Parser::Parser(FILE* is, const io::Logger* log)
    : in_stream(is), logger(log), unget_queue()
{
    // We heavily rely on this pointer _not_ being NULL. Be sure to check that.
    if (!logger)           fail("logger must not be NULL", 1);
    if (!in_stream)        logger->error("input stream is NULL");
    if (feof(in_stream))   logger->error("input stream is empty");
    if (ferror(in_stream)) logger->error("input stream is in bad state");
}

sjp::Parser::Parser(const Parser& other) noexcept
    : in_stream(other.in_stream), logger(other.logger),
      unget_queue(other.unget_queue), cursor(other.cursor)
{
}

sjp::Parser::Parser(Parser&& other) noexcept
    : Parser()
{
    swap(*this, other);
}

sjp::Parser& sjp::Parser::operator=(Parser rhs) noexcept
{
    swap(*this, rhs);
    return *this;
}

// Since JSON is so easy, we don't lex the input first.
sjp::Json sjp::Parser::parse(void)
{
    JsonValue* root = json();

    char c;
    if ((c = get_char()) != EOF)
        logger->warn("expected EOF after top-level JSON object, got `%c' "
                     "at %s", c, cursor.to_string().c_str());
    else
        logger->log("parser ran successfully (%ld line%s read)",
                    cursor.line_no, cursor.line_no > 1 ? "s" : "");

    return sjp::Json(root);
}

sjp::JsonValue* sjp::Parser::json(void)
{
    JsonValue* elem = element();
    return elem;
}

sjp::JsonValue* sjp::Parser::element(void)
{
    JsonValue* val = value();
    return val;
}

sjp::JsonValue* sjp::Parser::value(void)
{
    ws();

    auto valid_in_number = [](char c) -> bool
    { return (c >= '0' && c <= '9') || c == '-'; };
    char c = peek_char();
    JsonValue* val = nullptr;

    switch (c) {
    case '{': val = object(); break;
    case '[': val = array();  break;
    case '"': val = string(); break;
    case 't': val = true_();  break;
    case 'f': val = false_(); break;
    case 'n': val = null();   break;
    default:
        if (valid_in_number(c)) val = number();
        else {
            eat_char(); // so in case of EOF, we're at the correct LINE_NO
            logger->error("expected value at %s", cursor.to_string().c_str());
        }
    }

    ws();
    return val;
}

sjp::JsonValue* sjp::Parser::object(void)
{
    match_char('{');
    JsonObject* obj = new JsonObject(cursor.line_no, cursor.char_no);

    ws();
    if (peek_char() == '}') { eat_char(); return obj; }

    bool done = false;
    while (!done) {
        // We must be careful about whitespace.
        ws();
        JsonString* str = static_cast<JsonString*>(string());

        ws();
        match_char(':');
        JsonValue* val = value(); // skips whitespace for us
        obj->add_value(str->get_string_copy(), val, *logger);
        delete str;

        if (peek_char() == ',') eat_char();
        else                    done = true;
    }
    match_char('}');

    return obj;
}

sjp::JsonValue* sjp::Parser::array(void)
{
    match_char('[');
    JsonArray* arr = new JsonArray(cursor.line_no, cursor.char_no);

    ws();
    if (peek_char() == ']') { eat_char(); return arr; }

    bool done = false;
    while (!done) {
        JsonValue* val = value(); // skips whitespace already
        arr->add_value(val);

        if (peek_char() == ',') eat_char();
        else                    done = true;
    }
    match_char(']');

    return arr;
}

// @TODO: For now, we just skip the 4 characters making up the code point.
char sjp::Parser::get_unicode_from_hex(void)
{
    eat_char();
    eat_char();
    eat_char();
    eat_char();
    return ' ';
}

// @TODO: We handle escapes in a very limited way, `\uXXXX' not supported.
sjp::JsonValue* sjp::Parser::string(void)
{
    match_char('"');
    JsonString* str = new JsonString(cursor.line_no, cursor.char_no);
    std::string str_val;

    char c = peek_char();
    while (c != '"' && c != EOF && c != '\n') {
        if (c == '\\') {
            eat_char();
            c = get_char();
            switch (c) {
            case '\\': case '/': case '"':
                str_val += c;
                break;
            case 'b': str_val += '\b'; break;
            case 'f': str_val += '\f'; break;
            case 'n': str_val += '\n'; break;
            case 'r': str_val += '\r'; break;
            case 't': str_val += '\t'; break;
            case 'u':
                get_unicode_from_hex();
                break;
            default:
                logger->warn("invalid escape sequence \\%c", c);
            }
        } else {
            // If this is no escape sequence, we simply add C to the ouput.
            str_val += get_char();
        }
        c = peek_char();
    }
    match_char('"');

    str->add_value(str_val);

    return str;
}

/* NOTE: This routine is messy and might profit from cleanup. Also, some of the
 * error reporting is slightly off (again, getting a new line character some-
 * times throws things up; thus, we call CORRECT_FOR_REPORTING on cursor).
 */
sjp::JsonValue* sjp::Parser::number(void)
{
    // We're still one char before this number.
    JsonNumber* num = new JsonNumber(cursor.line_no, cursor.char_no+1);

    auto   is_digit = [](char c) { return '0' <= c && c <= '9'; };
    bool   negative = false;
    double d_val    = 0.0;
    char   c;

    c = get_char();
    if (c == '-') { negative = true; c = get_char(); }

    if ('1' <= c && c <= '9') {
        d_val += c - '0';
        while (is_digit(peek_char())) {
            c = get_char();
            d_val = d_val * 10 + (c - '0');
        }
    } else if (c == '0') {
        // Go straight to exponent and fractional parts.
    } else {
        cursor.correct_for_reporting(c);
        logger->error("expected a digit at %s", cursor.to_string().c_str());
    }

    size_t frac_size = 1;
    if (peek_char() == '.') {
        eat_char();
        while (is_digit(peek_char())) {
            c = get_char();
            d_val = d_val * 10 + (c - '0');
            frac_size *= 10;
        }
        if (frac_size == 1) {
            cursor.correct_for_reporting(c);
            logger->error("expected a digit after decimal point at %s",
                          cursor.to_string().c_str());
        }
    }
    d_val = d_val / static_cast<double>(frac_size);

    if ((c = peek_char()) == 'E' || c == 'e') {
        eat_char();

        c = get_char();
        bool neg_expo = false;
        if (c == '+' || c == '-') {
            if (c == '-') neg_expo = true;
            c = get_char();
        }

        if (c < '0' || c > '9') {
            cursor.correct_for_reporting(c);
            logger->error("expected a digit in exponent at %s",
                          cursor.to_string().c_str());
        }
        double expo = c - '0';
        while (is_digit(peek_char())) {
            c = get_char();
            expo = expo * 10 + (c - '0');
        }
        if (neg_expo) expo *= -1;
        d_val *= exp10(expo);
    }

    if (negative) d_val *= -1;
    num->add_value(d_val);

    return num;
}

sjp::JsonValue* sjp::Parser::true_(void)
{
    JsonTrue* true_ = new JsonTrue(cursor.line_no, cursor.char_no+1);
    match_string("true");
    return true_;
}

sjp::JsonValue* sjp::Parser::false_(void)
{
    JsonFalse* false_ = new JsonFalse(cursor.line_no, cursor.char_no+1);
    match_string("false");
    return false_;
}

sjp::JsonValue* sjp::Parser::null(void)
{
    JsonNull* null_ = new JsonNull(cursor.line_no, cursor.char_no+1);
    match_string("null");
    return null_;
}

/* Since <stdio.h> buffers input, we won't do that (for now). At some point, we
 * might want to get rid of that level of indirection and read directly from a
 * file descriptor.
 * Some algorithmic subtlety comes from the fact that EOF on a line by itself
 * doesn't count as a line. Thus, we have to specifically handle that case.
 */
[[nodiscard]] char sjp::Parser::get_char(void)
{
    char c;

    if (unget_queue.size() > 0) {
        c = unget_queue.front();
        unget_queue.pop();
    } else {
        size_t cnt = fread(&c, 1, 1, in_stream);
        if (cnt != 1) {
            if (feof(in_stream))   c = EOF;
            if (ferror(in_stream)) logger->error("unable to read from stream");
        }
    }

    if (c == '\n') {
        cursor.increment_line();
    } else if (c == EOF && cursor.char_no == 0) {
        // If this is the first "char" on a new line, we don't count that line.
        cursor.line_no = cursor.line_no > 1 ? cursor.line_no-1 : 1;
    } else {
        cursor.char_no++;
    }

    return c;
}

/* For readability, we enforce usage of EAT_CHAR whenever a byte from the
 * stream should be consumed without the caller looking at it (that's why
 * GET_CHAR is marked `nodiscard').
 */
void sjp::Parser::eat_char(void)
{
    char c = get_char();
    (void)c;
}

void sjp::Parser::match_char(char e)
{
    char c = get_char();
    if (c != e) {
        cursor.correct_for_reporting(c);
        if (c == EOF)  logger->error("expected `%c', got EOF at %s",
                                     e, cursor.to_string().c_str());
        if (c == '\n') logger->error("expected `%c', got NL at %s",
                                     e, cursor.to_string().c_str());
        logger->error("expected `%c', got `%c' at %s",
                      e, c, cursor.to_string().c_str());
    }
}

void sjp::Parser::match_string(const char* s)
{
    char c;
    const char* p;
    for (p = s; *p && ((c = get_char()) == *p); p++)
        ;

    if (*p != '\0') {
        char* copy = strdup(s);
        size_t offset = p - s;
        copy[offset] = c;
        copy[offset+1] = '\0';
        logger->error("got invalid `%s', maybe misspelling of `%s' at %s",
                      copy, s, cursor.to_string().c_str());
    }
}

/* Peeking is implemented in the most straight-forward way: we get N chars from
 * the input stream, put them onto UNGET_QUEUE and return the n'th char to the
 * caller. Thus, the next call to GET_CHAR will return the correct ordering of
 * characters. An edge case is the end of the stream. We might end up with N
 * EOF symbols on UNGET_QUEUE.
 */
[[nodiscard]] char sjp::Parser::peek_char(size_t n)
{
    char c;
    std::vector<char> local_queue;

    while (n--) {
        c = get_char();
        local_queue.push_back(c);
    }

    for (char c: local_queue) unget_char(c);
    return c;
}

void sjp::Parser::unget_char(char c)
{
    unget_queue.push(c);
    if (c == '\n') {
        cursor.char_no = cursor.prev_line_len;
        cursor.line_no--;
    } else if (c == EOF && cursor.char_no == 0) {
        // We decremented the line number in GET_CHAR, so we reverse that.
        cursor.line_no++;
    } else {
        cursor.char_no--;
    }
}

// Advance the stream to the next non-whitespace character.
void sjp::Parser::ws(void)
{
    auto is_whitespace = [](char c) -> bool
    { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; };

    char c;
    while ((c = peek_char()) != EOF && is_whitespace(c))
        eat_char();
}

void sjp::JsonObject::add_value(const std::string& name, sjp::JsonValue* val,
                                const io::Logger& log)
{
    if (values.find(name) != values.end()) {
        log.warn("ignoring duplicate key `%s' at %ld:%ld",
                 name.c_str(), line_no, char_no);
    } else {
        names_in_order.push_back(name);
        values[name] = val;
    }
}

void sjp::JsonObject::print(FILE* stream, size_t d)
{
    fprintf(stream, "{\n");
    for (size_t i = 0; i < names_in_order.size(); i++) {
        const std::string& name = names_in_order[i];
        JsonValue* value = values[name];
        fprintf(stream, "%s\"%s\": ", padding(d+1).c_str(), name.c_str());
        value->print(stream, d+1);
        if (i < names_in_order.size()-1) fprintf(stream, ",\n");
        else                             fprintf(stream, "\n");
    }
    fprintf(stream, "%s}", padding(d).c_str());
}

void sjp::JsonArray::print(FILE* stream, size_t d)
{
    fprintf(stream, "[\n");
    for (size_t i = 0; i < values.size(); i++) {
        fprintf(stream, "%s", padding(d+1).c_str());
        values[i]->print(stream, d+1);
        if (i < values.size()-1) fprintf(stream, ",\n");
        else                     fprintf(stream, "\n");
    }
    fprintf(stream, "%s]", padding(d).c_str());
}

sjp::JsonValue& sjp::JsonObject::operator[](size_t i)
{
    if (names_in_order.size() <= i)
        return default_json_none;
    const std::string& name = names_in_order[i];
    return *(values[name]);
}

sjp::JsonValue& sjp::JsonObject::operator[](const std::string& n)
{
    if (values.find(n) == values.end())
        return default_json_none;
    return *(values[n]);
}

sjp::JsonValue& sjp::JsonArray::operator[](size_t i)
{
    if (values.size() <= i)
        return default_json_none;
    return *(values[i]);
}

// @NOTE: Could it make sense to access arrays via strings?
sjp::JsonValue& sjp::JsonArray::operator[](const std::string&)
{
    return default_json_none;
}
