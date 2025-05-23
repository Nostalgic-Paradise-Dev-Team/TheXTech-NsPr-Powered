/*
 * A small crossplatform set of file manipulation functions.
 * All input/output strings are UTF-8 encoded, even on Windows!
 *
 * Copyright (c) 2017-2025 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies
 * or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "files.h"
#include "Logger/logger.h"
#include "Archives/archives.h"
#include <stdio.h>
#include <sdl_proxy/sdl_stdinc.h>
#include <SDL2/SDL_rwops.h>

#ifdef _WIN32
#include <windows.h>
#include <shlwapi.h>

static std::wstring Str2WStr(const std::string &path)
{
    std::wstring wpath;
    wpath.resize(path.size());
    int newlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), static_cast<int>(path.length()), &wpath[0], static_cast<int>(path.length()));
    wpath.resize(newlen);
    return wpath;
}
#else
#include <unistd.h>
#include <fcntl.h>         // open
#include <string.h>
#include <sys/stat.h>      // fstat
#include <sys/types.h>     // fstat
#include <cstdio>          // BUFSIZ
#endif

#if defined(__CYGWIN__) || defined(__DJGPP__) || defined(__MINGW32__)
#   define IS_PATH_SEPARATOR(c) (((c) == '/') || ((c) == '\\'))
#else
#   define IS_PATH_SEPARATOR(c) ((c) == '/')
#endif

static char fi_path_dot[] = ".";
static char fi_path_root[] = "/";

static char *fi_basename(char *s)
{
    char *rv;

    if(!s || !*s)
        return fi_path_dot;

    rv = s + strlen(s) - 1;

    do
    {
        if(IS_PATH_SEPARATOR(*rv))
            return rv + 1;
        --rv;
    }
    while(rv >= s);

    return s;
}

static char *fi_dirname(char *path)
{
    char *p;

    if(path == NULL || *path == '\0')
        return fi_path_dot;

    p = path + strlen(path) - 1;
    while(IS_PATH_SEPARATOR(*p))
    {
        if(p == path)
            return path;
        *p-- = '\0';
    }

    while(p >= path && !IS_PATH_SEPARATOR(*p))
        p--;

    if(p < path)
        return fi_path_dot;

    if(p == path)
        return fi_path_root;

    *p = '\0';
    return path;
}

Files::Data::Data(Files::Data&& o)
{
    *this = std::move(o);
}

Files::Data::~Data()
{
    if(m_free_me)
        free(const_cast<unsigned char*>(m_data));
}

const Files::Data& Files::Data::operator=(Files::Data&& o)
{
    if(m_free_me)
        free(const_cast<unsigned char*>(m_data));

    m_data = o.m_data;
    m_length = o.m_length;
    m_free_me = o.m_free_me;

    o.m_data = nullptr;
    o.m_length = -1;
    o.m_free_me = false;

    return *this;
}

void Files::Data::init_from_mem(const unsigned char* data, size_t size)
{
    if(m_free_me)
        free(const_cast<unsigned char*>(data));

    m_free_me = false;
    m_data = data;
    m_length = static_cast<long long int>(size);
}

void* Files::Data::disown()
{
    if(!m_free_me)
        return nullptr;

    void* ret = const_cast<uint8_t*>(m_data);

    m_data = nullptr;
    m_length = -1;
    m_free_me = false;

    return ret;
}

FILE *Files::utf8_fopen(const char *filePath, const char *modes)
{
#ifndef _WIN32
    return ::fopen(filePath, modes);
#else
    wchar_t wfile[MAX_PATH + 1];
    wchar_t wmode[21];
    int wfile_len = (int)strlen(filePath);
    int wmode_len = (int)strlen(modes);
    wfile_len = MultiByteToWideChar(CP_UTF8, 0, filePath, wfile_len, wfile, MAX_PATH);
    wmode_len = MultiByteToWideChar(CP_UTF8, 0, modes, wmode_len, wmode, 20);
    wfile[wfile_len] = L'\0';
    wmode[wmode_len] = L'\0';
    return ::_wfopen(wfile, wmode);
#endif
}

SDL_RWops *Files::open_file(const char *filePath, const char *modes)
{
    if(Archives::is_prefix(filePath[0]))
    {
        if(modes[0] != 'r' || (modes[1] != 'b' && modes[1] != '\0'))
            return nullptr;

        return Archives::open_file(filePath);
    }

    return SDL_RWFromFile(filePath, modes);
}

Files::Data Files::load_file(const char *filePath)
{
    Files::Data ret;

    SDL_RWops *in = Files::open_file(filePath, "rb");
    if(!in)
        return ret;

    off_t size = SDL_RWsize(in);

    // allocate extra byte for null terminator, but don't count it towards size of contained item
    unsigned char* target = (unsigned char*)malloc(size + 1);
    off_t to_read = size;

    if(!target)
    {
        SDL_RWclose(in);
        return ret;
    }

    ret.m_free_me = true;
    ret.m_data = target;
    ret.m_length = size;

    while(to_read)
    {
        size_t bytes_read = SDL_RWread(in, target, 1, to_read);
        if(!bytes_read)
        {
            pLogCritical("I/O error when reading [%s]", filePath);
            SDL_RWclose(in);

            // resets the return value and frees the malloc'd buffer
            ret = Files::Data();

            return ret;
        }

        to_read -= bytes_read;
        target += bytes_read;
    }

    // set null terminator
    *target = 0;

    SDL_RWclose(in);

    return ret;
}

void Files::flush_file(SDL_RWops *f)
{
#ifdef THEXTECH_NO_SDL_BUILD
    if(f->type == SDL_RWOPS_STDFILE)
        ::fflush((FILE*)f->hidden.unknown.data1);
#elif defined(HAVE_STDIO_H)
    if(f->type == SDL_RWOPS_STDFILE)
        ::fflush(f->hidden.stdio.fp);
#endif
}

int Files::skipBom(SDL_RWops* file, const char** charset)
{
    char buf[4];
    auto pos = SDL_RWtell(file);

    // Check for a BOM marker
    if(SDL_RWread(file, buf, 1, 4) == 4)
    {
        if(::memcmp(buf, "\xEF\xBB\xBF", 3) == 0) // UTF-8 is only supported
        {
            if(charset)
                *charset = "[UTF-8 BOM]";
            SDL_RWseek(file, pos + 3, RW_SEEK_SET);
            return CHARSET_UTF8;
        }
        // Unsupported charsets
        else if(::memcmp(buf, "\xFE\xFF", 2) == 0)
        {
            if(charset)
                *charset = "[UTF16-BE BOM]";
            SDL_RWseek(file, pos + 2, RW_SEEK_SET);
            return CHARSET_UTF16BE;
        }
        else if(::memcmp(buf, "\xFF\xFE", 2) == 0)
        {
            if(charset)
                *charset = "[UTF16-LE BOM]";
            SDL_RWseek(file, pos + 2, RW_SEEK_SET);
            return CHARSET_UTF16LE;
        }
        else if(::memcmp(buf, "\x00\x00\xFE\xFF", 4) == 0)
        {
            if(charset)
                *charset = "[UTF32-BE BOM]";
            SDL_RWseek(file, pos + 4, RW_SEEK_SET);
            return CHARSET_UTF32BE;
        }
        else if(::memcmp(buf, "\x00\x00\xFF\xFE", 4) == 0)
        {
            if(charset)
                *charset = "[UTF32-LE BOM]";
            SDL_RWseek(file, pos + 4, RW_SEEK_SET);
            return CHARSET_UTF32LE;
        }
    }

    if(charset)
        *charset = "[NO BOM]";

    // No BOM detected, seek to begining of the file
    SDL_RWseek(file, pos, RW_SEEK_SET);

    return CHARSET_UTF8;
}

bool Files::fileExists(const std::string &path)
{
#if defined(_WIN32)
    if(!Archives::has_prefix(path))
    {
        std::wstring wpath = Str2WStr(path);
        return PathFileExistsW(wpath.c_str()) == TRUE;
    }
#endif

    SDL_RWops *ops = Files::open_file(path, "rb");
    if(ops)
    {
        SDL_RWclose(ops);
        return true;
    }

    return false;
}

bool Files::deleteFile(const std::string &path)
{
#ifdef _WIN32
    std::wstring wpath = Str2WStr(path);
    return (DeleteFileW(wpath.c_str()) == TRUE);
#else
    return ::unlink(path.c_str()) == 0;
#endif
}

bool Files::copyFile(const std::string &to, const std::string &from, bool override)
{
    if(!override && fileExists(to))
        return false;// Don't override exist target if not requested

    bool ret = true;

#ifdef _WIN32

    std::wstring wfrom  = Str2WStr(from);
    std::wstring wto    = Str2WStr(to);
    ret = (bool)CopyFileW(wfrom.c_str(), wto.c_str(), !override);

#else

    char    buf[BUFSIZ];
    ssize_t size;
    ssize_t sizeOut;

    int source  = open(from.c_str(), O_RDONLY, 0);
    if(source == -1)
        return false;

    int dest    = open(to.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0640);
    if(dest == -1)
    {
        close(source);
        return false;
    }

    while((size = read(source, buf, BUFSIZ)) > 0)
    {
        sizeOut = write(dest, buf, static_cast<size_t>(size));
        if(sizeOut != size)
        {
            ret = false;
            break;
        }
    }

    close(source);
    close(dest);
#endif

    return ret;
}

bool Files::moveFile(const std::string& to, const std::string& from, bool override)
{
    bool ret = copyFile(to, from, override);
    if(ret)
        ret &= deleteFile(from);
    return ret;
}


std::string Files::dirname(std::string path)
{
    char *p = strdup(path.c_str());
    char *d = ::fi_dirname(p);
    path = d;
    free(p);
    return path;
}

std::string Files::basename(std::string path)
{
    char *p = strdup(path.c_str());
    char *d = ::fi_basename(p);
    path = d;
    free(p);
    return path;
}

std::string Files::basenameNoSuffix(std::string path)
{
    char *p = strdup(path.c_str());
    char *d = ::fi_basename(p);
    path = d;
    free(p);
    std::string::size_type dot = path.find_last_of('.');
    if(dot != std::string::npos)
        path.resize(dot);
    return path;
}


std::string Files::changeSuffix(std::string path, const std::string &suffix)
{
    size_t pos = path.find_last_of('.');// Find dot
    if((path.size() < suffix.size()) || (pos == std::string::npos))
        path.append(suffix);
    else
        path.replace(pos, suffix.size(), suffix);
    return path;
}

bool Files::hasSuffix(const std::string &path, const std::string &suffix)
{
    if(suffix.size() > path.size())
        return false;

    return (SDL_strncasecmp(path.c_str() + path.size() - suffix.size(), suffix.c_str(), suffix.size()) == 0);
}


bool Files::isAbsolute(const std::string& path)
{
    if(Archives::has_prefix(path))
        return true;

    bool firstCharIsSlash = (path.size() > 0) ? path[0] == '/' : false;
#ifdef _WIN32
    bool containsWinChars = (path.size() > 2) ? (path[1] == ':') && ((path[2] == '\\') || (path[2] == '/')) : false;
    if(firstCharIsSlash || containsWinChars)
    {
        return true;
    }
    return false;
#else
    return firstCharIsSlash;
#endif
}

void Files::getGifMask(std::string& mask, const std::string& front)
{
    mask = front;
    //Make mask filename
    size_t dotPos = mask.find_last_of('.');
    if(dotPos == std::string::npos)
        mask.push_back('m');
    else
        mask.insert(mask.begin() + dotPos, 'm');
}
