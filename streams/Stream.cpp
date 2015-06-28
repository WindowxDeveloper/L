#include "Stream.h"

using namespace L;

const char* Stream::line() {
  static char buffer[4096];
  char* w(buffer);
  nospace();
  while((*w++ = get())!='\n' && !eof()) {} // End of line or file
  while(*(w-1)=='\n' || *(w-1)=='\r') w--; // Don't take \n into account
  *w = '\0'; // Null-end string
  return buffer;
}
