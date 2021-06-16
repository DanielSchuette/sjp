/* SJP is a minimal JSON parser. Below, you find a short usage example. The
 * code per se is fairly self-documenting, take a look at ``sjp.hh'' and
 * ``io.hh'' for the interfaces that we use in here.
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
#include "common.hh"
#include "io.hh"
#include "sjp.hh"

const char* infile = "data/test.json";

int main(int /*argc*/, char** argv)
{
    FILE* stream = fopen(infile, "r");
    assert(stream);

    auto logger = io::SjpLogger(*argv, stderr);
    logger.log("reading from `%s'", infile);

    auto parser = sjp::Parser(stream, &logger);
    sjp::Json json = parser.parse();
    json.print(stderr); // pretty-prints the parsed JSON to a FILE*

    /* Now, we can read data from the SJP::JSON object.
     * JSONOBJECTs are accessed via OPERATOR[] and string keys.
     */
    sjp::JsonValue& array = json["data"]["deeply"]["nested"];
    assert(array.get_type() == sjp::Type::Array);

    std::vector<double> v;
    for (size_t i = 0; i < array.size(); i++) {
        /* JSONARRAYs can be accessed via OPERATOR[] and integer keys.
         * We get a JSONVALUE&, which we must cast to the actual type before
         * the VALUE member (that every primitive type has) can be accessed.
         * As seen above, we can always check JSONVALUE.GET_TYPE() to
         * dynamically validate what kind of data we've got. The code is not
         * very readable, though:
         *
         * sjp::JsonValue& item = array[i];
         * if (item.get_type() == sjp::Type::Number) {
         *     sjp::JsonNumber& n = static_cast<sjp::JsonNumber&>(item);
         *     v.push_back(n.value);
         * } else {
         *     logger.warn("ignoring non-number item of type `%s'",
         *                 item.type_to_string().c_str());
         * }
         */

        /* The (probably better) alternative is to use polymorphic data
         * accessors that return STD::OPTIONAL-wrapped values. Those can then
         * be checked for actual content using the familiar C++ STL functions:
         */
        sjp::JsonValue& item = array[i];
        std::optional<double> opt_num = item.get_number();
        if (opt_num)
            v.push_back(*opt_num);
        else
            logger.warn("ignoring non-number item of type `%s'",
                        item.type_to_string().c_str());
    }

    double s = 0.0;
    std::for_each(v.begin(), v.end(), [&s](const double&d) { s += d; });
    logger.log("sum over all number items in the array: %g", s);

    fclose(stream);

    return 0;
}
