/* IO::LOGGER and IO::SJPLOGGER classes as well as a few utility functions we
 * use exclusive with IO::SJPLOGGER. It is quite easy to get rid of the
 * declarations in this file, simply copy IO::LOGGER over to ``sjp.hh'' and
 * provide an implementation.
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
#ifndef _IO_HH_
#define _IO_HH_

#include <cstdarg>

#include "common.hh"

static const char* strip_dir(const char*);

namespace io {
    class Logger;
    class SjpLogger;
    class NullLogger;
}

namespace ansi {
    enum class AnsiColor {
        RESET,
        FG_WHITE, BG_WHITE,
        FG_RED, BG_RED,
        FG_GREEN, BG_GREEN,
        FG_BLUE, BG_BLUE,
        FG_YELLOW, BG_YELLOW,
        FG_GREY, BG_GREY,
        FG_BLACK, BG_BLACK
    };
    const char* color_to_str(AnsiColor);
    void enable_color(AnsiColor, FILE*);
    void reset_color(FILE*);
}

// The abstract base class all concrete loggers must be based on.
class io::Logger {
public:
    virtual ~Logger(void) {}

    virtual void log(const char*, ...) const = 0;
    virtual void warn(const char*, ...) const = 0;
    [[noreturn]] virtual void error(const char*, ...) const = 0;
};

class io::SjpLogger : public io::Logger {
    const char* prog_name;
    FILE* out_stream; // we don't own this stream, so we don't free it

public:
    SjpLogger(const char* name, FILE* os)
        : prog_name(strip_dir(name)), out_stream(os) {}
    SjpLogger(void) : SjpLogger("unknown", stderr) {}
    virtual ~SjpLogger(void) {}

    // @TODO: Implement these.
    SjpLogger(const SjpLogger&) = delete;
    SjpLogger(SjpLogger&&)      = delete;
    SjpLogger& operator=(const SjpLogger&) = delete;
    SjpLogger& operator=(SjpLogger&&)      = delete;

    virtual void log(const char*, ...) const override;
    virtual void warn(const char*, ...) const override;
    [[noreturn]] virtual void error(const char*, ...) const override;
};

/* A minimal interface implementation that can be passed if no logging should
 * be done.
 */
class io::NullLogger : public io::Logger {
public:
    NullLogger(void) {}
    virtual ~NullLogger(void) {}

    virtual void log(const char*, ...) const override {}
    virtual void warn(const char*, ...) const override {}
    [[noreturn]] virtual void error(const char*, ...) const override
    { exit(1); }
};

/* Strip directory names from a file path, e.g. `./my/build/dir/prog' becomes
 * just `prog'. We don't do allocation in here, just return a pointer to the
 * offset within PATH that effectively strips the correct prefix.
 */
static const char* strip_dir(const char* path)
{
    ssize_t i = static_cast<ssize_t>(strlen(path))-1;
    for (; i >= 0; i--) if (path[i] == '/') break;
    return path+i+1;
}

#endif /* _IO_HH_ */
