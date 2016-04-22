#include "Variable.h"

#include "../String.h"

using namespace L;

Variable::Variable(const char* s) : _td(Type<String>::description()) {
  if(local())
    new(&_data) String(s);
  else _p = new String(s);
}
Variable::Variable(const Variable& other) : _td(other._td) {
  if(local()) // Value is to be contained locally
    _td->cpyto(value(),other.value());
  else _p = _td->cpy(other._p);
}
Variable::Variable(Variable&& other) : _td(other._td) {
  memcpy(_data,other._data,sizeof(_data));
  other._td = Type<int>::description();
}
Variable& Variable::operator=(const Variable& other) {
  if(this != &other) {
    if(_td==other._td) // Same type
      _td->assign(value(),other.value()); // Simple assignment
    else{ // Different type
      this->~Variable();
      new(this) Variable(other);
    }
  }
  return *this;
}
Variable& Variable::operator=(Variable&& other){
  if(this != &other) {
    this->~Variable();
    memcpy(this,&other,sizeof(Variable));
    other._td = Type<int>::description();
  }
  return *this;
}
Variable::~Variable() {
  if(local()) // Value is contained locally
    _td->dtr(&_data);
  else _td->del(_p); // Value has been dynamically allocated
}

Variable& Variable::operator[](const Variable& key) {
  if(!is<Map<String,Variable> >()) *this = Map<Variable,Variable>();
  return as<Map<Variable,Variable> >()[key];
}
const Variable& Variable::operator[](const Variable& key) const {
  return as<Map<Variable,Variable> >()[key];
}
Variable& Variable::operator[](size_t i) {
  if(!is<Array<Variable> >()) *this = Array<Variable>();
  if(as<Array<Variable> >().size()<=i) as<Array<Variable> >().size(i+1);
  return as<Array<Variable> >()[i];
}

Stream& L::operator<<(L::Stream& s,const Variable& v) {
  v.type()->out(s,v.value());
  return s;
}
