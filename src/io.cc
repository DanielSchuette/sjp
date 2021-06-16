/* IO::SJPLOGGER implements the IO:LOGGER interface and can be used with an
 * SJP::PARSER. See ``io.hh'' for a "NULL object" that implements the interface
 * without doing anything.
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
#include <ctime>
#include <unistd.h>

#include "io.hh"

const char* TIME_FORMAT = "[%F %H:%M:%S]";

static void pr_time(FILE* stream)
{
    time_t t = time(nullptr);
    struct tm* tm = localtime(&t);

    char buf[BUFSIZ] {};
    if (!tm)                                              goto early_error;
    if (strftime(buf, sizeof(buf), TIME_FORMAT, tm) == 0) goto early_error;

    fprintf(stream, "%s", buf);
    return;

early_error:
    fprintf(stream, "logger: internal error in `pr_time()'\n");
    exit(1);
}

void io::SjpLogger::log(const char* fmt, ...) const
{
    if (!out_stream) return;

    va_list ap;
    va_start(ap, fmt);
    char* msg {};
    int n = vsnprintf(msg, 0, fmt, ap);
    va_end(ap);
    if (n < 0) goto early_error;

    msg = (char*)malloc(n+1); // man 3 vsnprintf
    if (!msg) goto early_error;

    va_start(ap, fmt);
    n = vsnprintf(msg, n+1, fmt, ap);
    va_end(ap);
    if (n < 0) goto early_error;

    pr_time(out_stream);
    ansi::enable_color(ansi::AnsiColor::FG_BLUE, out_stream);
    fprintf(out_stream, " %s: log:", prog_name);
    ansi::reset_color(out_stream);
    fprintf(out_stream, " %s\n", msg);

    free(msg);
    return;

early_error:
    free(msg);
    fprintf(out_stream, "logger: internal error in `error()'\n");
    exit(1);
}

void io::SjpLogger::warn(const char* fmt, ...) const
{
    if (!out_stream) return;

    va_list ap;
    va_start(ap, fmt);
    char* msg = nullptr;
    int n = vsnprintf(msg, 0, fmt, ap);
    va_end(ap);
    if (n < 0) goto early_error;

    msg = (char*)malloc(n+1); // man 3 vsnprintf
    if (!msg) goto early_error;

    va_start(ap, fmt);
    n = vsnprintf(msg, n+1, fmt, ap);
    va_end(ap);
    if (n < 0) goto early_error;

    pr_time(out_stream);
    ansi::enable_color(ansi::AnsiColor::FG_YELLOW, out_stream);
    fprintf(out_stream, " %s: warning:", prog_name);
    ansi::reset_color(out_stream);
    fprintf(out_stream, " %s\n", msg);

    free(msg);
    return;

early_error:
    free(msg);
    fprintf(out_stream, "logger: internal error in `error()'\n");
    exit(1);
}

[[noreturn]] void io::SjpLogger::error(const char* fmt, ...) const
{
    if (!out_stream) exit(1);

    va_list ap;
    va_start(ap, fmt);

    char* msg = nullptr;
    int n = vsnprintf(msg, 0, fmt, ap);
    va_end(ap);
    if (n < 0) goto early_error;
    msg = (char*)malloc(n+1); // man 3 vsnprintf
    if (!msg) goto early_error;

    va_start(ap, fmt);
    n = vsnprintf(msg, n+1, fmt, ap);
    va_end(ap);
    if (n < 0) goto early_error;

    pr_time(out_stream);
    ansi::enable_color(ansi::AnsiColor::FG_RED, out_stream);
    fprintf(out_stream, " %s: error:", prog_name);
    ansi::reset_color(out_stream);
    fprintf(out_stream, " %s\n", msg);

    free(msg);
    exit(1);

early_error:
    free(msg);
    fprintf(out_stream, "logger: internal error in `error()'\n");
    exit(1);
}


const char* ansi::color_to_str(AnsiColor color)
{
    using _ = AnsiColor;
    // using enum AnsiColor; // clang-tidy for some reason cannot handle this

    switch (color) {
    case _::RESET:     return "\x1b[0m";
    case _::FG_BLACK:  return "\x1b[30m";
    case _::BG_BLACK:  return "\x1b[40m";
    case _::FG_RED:    return "\x1b[31m";
    case _::BG_RED:    return "\x1b[41m";
    case _::FG_GREEN:  return "\x1b[32m";
    case _::BG_GREEN:  return "\x1b[42m";
    case _::FG_YELLOW: return "\x1b[33m";
    case _::BG_YELLOW: return "\x1b[43m";
    case _::FG_BLUE:   return "\x1b[34m";
    case _::BG_BLUE:   return "\x1b[44m";
    case _::FG_GREY:   return "\x1b[90m";
    case _::BG_GREY:   return "\x1b[100m";
    case _::FG_WHITE:  return "\x1b[37m";
    case _::BG_WHITE:  [[fallthrough]]; default: return "\x1b[47m";
    }
}

void ansi::enable_color(AnsiColor color, FILE* stream)
{
    if (!isatty(fileno(stream))) return;

    const char* color_str = color_to_str(color);
    fprintf(stream, "%s", color_str);
}

void ansi::reset_color(FILE* stream)
{
    if (!isatty(fileno(stream))) return;

    const char* reset_str = color_to_str(AnsiColor::RESET);
    fprintf(stream, "%s", reset_str);
}
