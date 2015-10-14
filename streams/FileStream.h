#ifndef DEF_L_FileStream
#define DEF_L_FileStream

#include "../system/File.h"
#include "Stream.h"

namespace L {
  class FileStream : public Stream {
    public:
      inline FileStream() : Stream(tmpfile()) {}
      inline FileStream(const char* filepath, const char* mode) : Stream(fopen(filepath,mode)) {}
      inline FileStream(const File& file, const char* mode) : Stream(fopen(file.path(),mode)) {}
      inline ~FileStream() {fclose(_fd);}
      inline void rewind() {::rewind(_fd);}
      inline int tell() const {return ::ftell(_fd);}
  };
  static FileStream tmpfile;
}

#endif

