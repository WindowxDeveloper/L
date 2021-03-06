#include "LSLexer.h"

#include <cstring>
#include <L/src/dev/debug.h>

using namespace L;

char LSLexer::get() {
  L_ASSERT(!end_of_text());
  const char c(*_read);
  _read++;
  _read_size--;
  if(c=='\n')
    _line++;
  return c;
}
void LSLexer::read(const char* text, size_t size, bool last_read) {
  L_ASSERT(!_read || end_of_text());
  L_ASSERT(!_last_read);
  _read = text;
  _read_size = size;
  _last_read = last_read;
}
bool LSLexer::next_token() {
  if(_state==Comment) {
    while(!end_of_text() && peek()!='\n')
      get();
    _state = end_of_text() ? Comment : Done;
    if(_state==Comment)
      return false;
  }
  if(_state==Done) {
    _write = _buffer;
    _literal = false;
    _state = Started;
  }

  while(true) {
    if(end_of_text()) {
      if(_last_read && _write>_buffer) {
        *_write = '\0';
        _state = Done;
        return true;
      } else return false;
    }
    if(_write==_buffer && !_literal) { // Before anything relevant
      char c(get());
      if(c==';') { // Comment
        while(!end_of_text() && peek()!='\n')
          get();
        _state = end_of_text() ? Comment : Done;
      } else if(c=='"') { // Start of literal
        _literal = true;
      } else if(special_char(c) || c=='.') { // One char token
        _buffer[0] = c;
        _buffer[1] = '\0';
        _state = Done;
        return true;
      } else if(!space_char(c)) { // Start of token
        *_write++ = c;
      }
    } else if(_literal) { // Literal string
      char c(get());
      if(_write>_buffer && *(_write-1)=='\\') { // Escaped character
        switch(c) {
          case '\\': *(_write-1) = '\\'; break;
          case 't': *(_write-1) = '\t'; break;
          case 'n': *(_write-1) = '\n'; break;
          case 'r': *(_write-1) = '\r'; break;
          case '"': *(_write-1) = '"'; break;
          default: *_write++ = c; break;
        }
      } else if(c=='"') { // End of literal string
        *_write = '\0';
        _state = Done;
        return true;
      } else { // Simple character in string
        *_write++ = c;
      }
    } else { // Regular token
      const char c(peek());
      const bool dot_inside_non_number(c=='.' && !strchr("-0123456789", _buffer[0]));
      if(special_char(c) || space_char(c) || dot_inside_non_number) { // End of token
        *_write = '\0';
        _state = Done;
        return true;
      } else {
        *_write++ = get();
      }
    }
  }
}
bool LSLexer::accept_token(const char* str) {
  if(is_token(str)) {
    next_token();
    return true;
  } else return false;
}
