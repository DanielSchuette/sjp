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
    json.print(stderr);

    std::vector<double> v;
    sjp::JsonValue& array = json["data"]["deeply"]["nested"];
    assert(array.get_type() == sjp::Type::Array);

    for (size_t i = 0; i < array.size(); i++) {
        sjp::JsonNumber& n = static_cast<sjp::JsonNumber&>(array[i]);
        v.push_back(n.value);
    }

    double s = 0.0;
    std::for_each(v.begin(), v.end(), [&s](const double&d) { s += d; });
    fprintf(stderr, "%g\n", s);

    fclose(stream);

    return 0;
}
