/* The SJP namespace contains the parser and the data wrappers around the
 * the data types JSON defines (objects, arrays, strings, etc.). JSONVALUE
 * serves as the abstract base class to use them polymorphically. See
 * ``main.cc'' for a usage example.
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
#ifndef _JSON_HH_
#define _JSON_HH_

#include <unordered_map>
#include <string>
#include <queue>

#include "common.hh"
#include "io.hh"

/* There are only 2 classes that make up the API: PARSER and JSON. The user
 * provides an input stream pointer and a logger and the parser will then
 * return a JSON object that can be queried for data.
 */
namespace sjp {
    class Json;

    class Parser;

    class JsonValue;
    class JsonObject;
    class JsonArray;
    class JsonString;
    class JsonNumber;
    class JsonTrue;
    class JsonFalse;
    class JsonNull;
    class JsonNone;

    enum class Type { Object, Array, String, Number, True, False, Null, None };

    static const char* type_to_str(Type);
}

class sjp::JsonValue {
protected:
    size_t line_no, char_no;

public:
    JsonValue(size_t l, size_t c) : line_no(l), char_no(c) {}
    virtual ~JsonValue(void) {};

    virtual Type        get_type(void) = 0;
    virtual JsonValue&  operator[](size_t) = 0;
    virtual JsonValue&  operator[](const std::string&) = 0;

    virtual size_t      size(void)           { return 1; }
    virtual std::string type_to_string(void) { return type_to_str(get_type()); }
    virtual void        print(FILE* stream, size_t = 0)
    { fprintf(stream, "%s", type_to_str(get_type())); }
};

class sjp::JsonNone : public JsonValue {
public:
    friend class sjp::Parser;

    JsonNone(size_t l, size_t c) : JsonValue(l, c) {}
    ~JsonNone(void) {}

    virtual Type get_type(void) override { return Type::None; }
    virtual JsonValue& operator[](size_t) override { return *this; }
    virtual JsonValue& operator[](const std::string&) override
    { return *this; }
};

// This is the default value that's referenced whenever the user tries to
// access a non-existant field on a JSONVALUE.
static sjp::JsonNone default_json_none(0, 0);

class sjp::JsonObject : public JsonValue {
    /* To be able to retrieve elements in O(1) time _and_ remember the
     * insertion order, we need additional space.
     */
    std::unordered_map<std::string, JsonValue*> values         = {};
    std::vector<std::string>                    names_in_order = {};

    void add_value(const std::string&, JsonValue*, const io::Logger&);

public:
    friend class sjp::Parser;

    JsonObject(size_t l, size_t c) : JsonValue(l, c) {}
    virtual ~JsonObject(void)
    { for (const std::string& name: names_in_order) delete values[name]; }

    virtual Type   get_type(void) override { return Type::Object; }
    virtual size_t size(void)     override { return values.size(); }

    virtual JsonValue& operator[](size_t) override;
    virtual JsonValue& operator[](const std::string&) override;
    virtual void       print(FILE* stream, size_t d = 0) override;
};

class sjp::JsonArray : public JsonValue {
    std::vector<JsonValue*> values = {};

    void add_value(JsonValue* v) { values.push_back(v); }

public:
    friend class sjp::Parser;

    JsonArray(size_t l, size_t c) : JsonValue(l, c) {}
    virtual ~JsonArray(void) { for (JsonValue* n: values) delete n; }

    virtual Type   get_type(void) override { return Type::Array; }
    virtual size_t size(void)     override { return values.size(); }

    virtual JsonValue& operator[](size_t) override;
    virtual JsonValue& operator[](const std::string&) override;
    virtual void       print(FILE* stream, size_t d = 0) override;
};

class sjp::JsonString : public JsonValue {
    void        add_value(const std::string& s) { value = s; }
    std::string get_string_copy(void)           { return value; }

public:
    friend class sjp::Parser;

    /* The user can access our value directly, we avoid trivial getters. This
     * is the case for all "basic" JSON types, i.e. strings, numbers, bools
     * and null (represented by NULLPTR);
     */
    std::string value = "";

    JsonString(size_t l, size_t c) : JsonValue(l, c) {}
    virtual ~JsonString(void) {}

    virtual Type get_type(void) override { return Type::String; }

    /* We put "default" implementations on all basic types because it doesn't
     * seem to make sense to use OPERATOR[] on them. If the user wants their
     * values, he needs to use the appropriate accessor.
     */
    virtual JsonValue& operator[](size_t) override
    { return default_json_none; }

    virtual JsonValue& operator[](const std::string&) override
    { return default_json_none; }

    virtual void print(FILE* stream, size_t) override
    { fprintf(stream, "\"%s\"", value.c_str()); }
};

class sjp::JsonNumber : public JsonValue {
    void add_value(double d) { value = d; }

public:
    friend class sjp::Parser;

    double value = 0.0;

    JsonNumber(size_t l, size_t c) : JsonValue(l, c) {}
    virtual ~JsonNumber(void) {}

    virtual Type get_type(void) override { return Type::Number; }

    virtual JsonValue& operator[](size_t) override
    { return default_json_none; }

    virtual JsonValue& operator[](const std::string&) override
    { return default_json_none; }

    virtual void print(FILE* stream, size_t) override
    { fprintf(stream, "%g", value); }
};

class sjp::JsonTrue : public JsonValue {
public:
    friend class sjp::Parser;

    const bool value = true;

    JsonTrue(size_t l, size_t c) : JsonValue(l, c) {}
    virtual ~JsonTrue(void) {}

    virtual Type get_type(void) override { return Type::True; }

    virtual JsonValue& operator[](size_t) override
    { return default_json_none; }

    virtual JsonValue& operator[](const std::string&) override
    { return default_json_none; }
};

class sjp::JsonFalse : public JsonValue {
public:
    friend class sjp::Parser;

    const bool value = false;

    JsonFalse(size_t l, size_t c) : JsonValue(l, c) {}
    virtual ~JsonFalse(void) {}

    virtual Type get_type(void) override { return Type::False; }

    virtual JsonValue& operator[](size_t) override
    { return default_json_none; }

    virtual JsonValue& operator[](const std::string&) override
    { return default_json_none; }
};

class sjp::JsonNull : public JsonValue {
public:
    friend class sjp::Parser;

    // @NOTE: JSONNULL is a primitive type, so for consistency we want a value
    // on it. Since (probably) any value will do, we have a constant byte here.
    const uint8_t value = 0;

    JsonNull(size_t l, size_t c) : JsonValue(l, c) {}
    virtual ~JsonNull(void) {}

    virtual Type get_type(void) override { return Type::Null; }

    virtual JsonValue& operator[](size_t) override
    { return default_json_none; }

    virtual JsonValue& operator[](const std::string&) override
    { return default_json_none; }
};

class sjp::Json {
private:
    sjp::JsonValue* root; // we own this

public:
    Json(sjp::JsonValue* r) : root(r) {}
    ~Json(void) { delete root; }

    // @TODO: Should we implement a deep copy of a JSON object?
    Json(const Json&) = delete;
    Json(Json&&)      = delete;
    Json& operator=(const Json&) = delete;
    Json& operator=(Json&&)      = delete;

    JsonValue& operator[](size_t i) { return (*root)[i]; }
    JsonValue& operator[](const std::string& n) { return (*root)[n]; }

    void print(FILE* stream) { root->print(stream); fprintf(stream, "\n"); }
};

class sjp::Parser {
    // @NOTE: We don't own these pointers and don't free them.
    FILE* in_stream = nullptr;
    const io::Logger* logger = nullptr;
    std::queue<char> unget_queue = {};

    /* These point at the position of the last char read from IN_STREAM.
     * @NOTE: At some point, we might want to better sync line numbers and
     * stream access by hiding the cursor behind getters/setters. Also, lexing
     * first could be easier - we then just store line numbers with tokens.
     */
    struct cursor {
        size_t char_no = 0; // 0 means we just increased the line number
        size_t line_no = 1;
        size_t prev_line_len = 0;

        void correct_for_reporting(char c)
        { if (c == '\n') { line_no--; char_no = prev_line_len; } }

        std::string to_string(void)
        { return std::to_string(line_no)+":"+std::to_string(char_no); };

        void increment_line(void)
        { line_no++; prev_line_len = char_no; char_no = 0; }
    } cursor = {};

    // @NOTE: Users cannot construct a parser without stream/logger.
    Parser(void) {}

    [[nodiscard]] char get_char(void);
    [[nodiscard]] char peek_char(size_t = 1);
    void eat_char(void);
    void match_char(char);
    void match_string(const char*);
    void unget_char(char);
    void ws(void);

    // @TODO: We don't implement anything but ASCII streams right now. This
    // obviously doesn't comply with the JSON spec which allows UTF8-encoded
    // characters in its string literals.
    char get_unicode_from_hex(void);

    JsonValue* json(void);
    JsonValue* element(void);
    JsonValue* value(void);
    JsonValue* object(void);
    JsonValue* array(void);
    JsonValue* string(void);
    JsonValue* number(void);
    JsonValue* true_(void);
    JsonValue* false_(void);
    JsonValue* null(void);

public:
    Parser(FILE*, const io::Logger*);
    ~Parser(void) { /* @NOTE: STREAM is _not_ freed. */ }

    /* @NOTE: I cannot imaging why someone might use these but it why not
     * provide them. Note, though: The user needs to adjust the input stream,
     * we just read from it while parsing.
     */
    Parser(const Parser& other) noexcept;
    Parser(Parser&&) noexcept;
    Parser& operator=(Parser other) noexcept;
    friend void swap(Parser& fst, Parser& snd) noexcept
    {
        using std::swap;

        swap(fst.in_stream, snd.in_stream);
        swap(fst.logger, snd.logger);
        swap(fst.unget_queue, snd.unget_queue);
        swap(fst.cursor, snd.cursor);
    }

    Json parse(void);
};

static const char* sjp::type_to_str(Type type)
{
    switch (type) {
    case Type::Object: return "object";
    case Type::Array:  return "array";
    case Type::Number: return "number";
    case Type::String: return "string";
    case Type::True:   return "true";
    case Type::False:  return "false";
    case Type::Null:   return "null";
    case Type::None:   return "none";
    default: assert(!"incomplete switch in `sjp::type_to_str()'");
    }
}

#endif /* _JSON_HH_ */
